#define _GNU_SOURCE
#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static int (*android_address)(const void*,void*);
static void *(*bionic_tls)(void);
struct write_record { uintptr_t destination, caller; size_t size; unsigned kind; };
static __thread struct write_record writes[256];
static __thread unsigned write_index;
static void *(*real_copy)(void*,const void*,size_t);
static void *(*real_move)(void*,const void*,size_t);
static void *(*real_fill)(void*,int,size_t);

static void record_write(void *destination,size_t size,unsigned kind,void *caller) {
    uintptr_t stack=(uintptr_t)__builtin_frame_address(0),target=(uintptr_t)destination;
    if(target>=stack&&target-stack<65536) {
        writes[write_index++%256]=(struct write_record){target,(uintptr_t)caller,size,kind};
    }
}
static void *trace_copy(void *destination,const void *source,size_t size) {
    record_write(destination,size,1,__builtin_return_address(0));
    return real_copy(destination,source,size);
}
static void *trace_move(void *destination,const void *source,size_t size) {
    record_write(destination,size,2,__builtin_return_address(0));
    return real_move(destination,source,size);
}
static void *trace_fill(void *destination,int value,size_t size) {
    record_write(destination,size,3,__builtin_return_address(0));
    return real_fill(destination,value,size);
}

static void mapping(void *address) {
    FILE *maps=fopen("/proc/self/maps","r");if(!maps)return;
    char line[1024];while(fgets(line,sizeof(line),maps)){
        unsigned long low,high;
        if(sscanf(line,"%lx-%lx",&low,&high)==2&&(uintptr_t)address>=low&&(uintptr_t)address<high){
            dprintf(STDERR_FILENO,"MAVK_PC_MAPPING address=%p %s",address,line);break;
        }
    }fclose(maps);
}

static void describe(void *address) {
    Dl_info info={0};
    if(android_address&&android_address(address,&info))
        dprintf(STDERR_FILENO,"MAVK_ANDROID_PC module=%s offset=%#lx symbol=%s\n",
                info.dli_fname?info.dli_fname:"?",(unsigned long)((uintptr_t)address-(uintptr_t)info.dli_fbase),
                info.dli_sname?info.dli_sname:"?");
}

static void trace_abort(int signal_number) {
    void *frames[64];
    int count=backtrace(frames,64);
    dprintf(STDERR_FILENO,"MAVK_ABORT_STACK pid=%d frames=%d\n",getpid(),count);
    backtrace_symbols_fd(frames,count,STDERR_FILENO);
    signal(signal_number,SIG_DFL);
    raise(signal_number);
}

__attribute__((noreturn,used,visibility("hidden"))) void mavk_canary_detail(uintptr_t original_sp,uintptr_t bionic_tp,void *caller,uintptr_t compared_current,uintptr_t compared_saved) {
    void *frames[64];
    dprintf(STDERR_FILENO,"MAVK_CANARY_FAILURE caller=%p\n",caller);
    describe(caller);
    for(unsigned i=0;i<256;i++) {
        struct write_record *record=&writes[i];
        uintptr_t guard=original_sp+0x158;
        if(record->size&&guard>=record->destination&&guard-record->destination<record->size) {
            dprintf(STDERR_FILENO,"MAVK_STACK_WRITE kind=%u size=%zu guard_offset=%lu\n",record->kind,record->size,(unsigned long)(guard-record->destination));
            describe((void*)record->caller);
        }
    }
    dprintf(STDERR_FILENO,"MAVK_CANARY_REGISTERS saved_zero=%u current_zero=%u equal=%u\n",compared_saved==0,compared_current==0,compared_saved==compared_current);
    Dl_info fault={0};
    if(android_address&&android_address(caller,&fault)&&
       (uintptr_t)caller-(uintptr_t)fault.dli_fbase==0x9314f0){
        uintptr_t saved=*(uintptr_t*)(original_sp+0x158),current=*(uintptr_t*)(bionic_tp+0x28);
        dprintf(STDERR_FILENO,"MAVK_CANARY_VALUES saved_zero=%u current_zero=%u equal=%u\n",saved==0,current==0,saved==current);
        describe((void*)saved);
        uintptr_t native_tp=(uintptr_t)__builtin_thread_pointer();
        uintptr_t native_guard=*(uintptr_t*)(native_tp+0x28);
        uintptr_t mapped_tp=bionic_tls?(uintptr_t)bionic_tls():0;
        uint32_t mrs=*(uint32_t*)((uintptr_t)fault.dli_fbase+0x930f3c);
        void *return_pc=*(void**)(original_sp+0x168);
        dprintf(STDERR_FILENO,"MAVK_TLS_STATE same_bionic=%u same_native=%u saved_matches_native=%u mrs_word=%08x return_pc=%p\n",
                bionic_tp==mapped_tp,bionic_tp==native_tp,saved==native_guard,mrs,return_pc);
        Dl_info parent={0};if(dladdr(return_pc,&parent))dprintf(STDERR_FILENO,"MAVK_GLIBC_PC module=%s offset=%#lx\n",parent.dli_fname,(unsigned long)((uintptr_t)return_pc-(uintptr_t)parent.dli_fbase));
        describe(return_pc);
        mapping(return_pc);
        mapping((void*)saved);
    }
    uintptr_t frame=(uintptr_t)__builtin_frame_address(0),low=0,high=0;
    FILE *maps=fopen("/proc/self/maps","r");
    if(maps){char line[1024];while(fgets(line,sizeof(line),maps)){
        unsigned long a,b;char mode[5]={0};
        if(sscanf(line,"%lx-%lx %4s",&a,&b,mode)==3&&mode[0]=='r'&&frame>=a&&frame+16<=b){low=a;high=b;break;}
    }fclose(maps);}
    for(unsigned i=0;i<16&&low&&frame>=low&&frame+16<=high&&!(frame&15);i++){
        uintptr_t *record=(uintptr_t*)frame,next=record[0],pc=record[1];
        dprintf(STDERR_FILENO,"MAVK_FRAME index=%u pc=%#lx\n",i,(unsigned long)pc);
        describe((void*)pc);
        if(next<=frame)break;
        frame=next;
    }
    int count=backtrace(frames,64);
    backtrace_symbols_fd(frames,count,STDERR_FILENO);
    abort();
}

void mavk_canary_entry(void);
__asm__(".text\n.hidden mavk_canary_entry\n.type mavk_canary_entry,%function\nmavk_canary_entry:\n"
        "mov x0,sp\nmov x1,x22\nmov x2,x30\nmov x3,x8\nmov x4,x9\nb mavk_canary_detail\n");

static void *trace_hook(const char *name,const char *requester) {
    if(!strcmp(name,"memcpy"))return trace_copy;
    if(!strcmp(name,"memmove"))return trace_move;
    if(!strcmp(name,"memset"))return trace_fill;
    if(strcmp(name,"__stack_chk_fail"))return 0;
    dprintf(STDERR_FILENO,"MAVK_CANARY_BIND requester=%s\n",requester?requester:"unknown");
    return mavk_canary_entry;
}

__attribute__((constructor)) static void install_trace(void) {
    real_copy=dlsym(RTLD_NEXT,"memcpy");
    real_move=dlsym(RTLD_NEXT,"memmove");
    real_fill=dlsym(RTLD_NEXT,"memset");
    struct sigaction action={0};
    action.sa_handler=trace_abort;
    sigemptyset(&action.sa_mask);
    action.sa_flags=SA_RESETHAND;
    if(sigaction(SIGABRT,&action,0)==0)
        dprintf(STDERR_FILENO,"MAVK_ABORT_TRACE pid=%d installed\n",getpid());
    void *library=dlopen("libhybris-common.so.1",RTLD_NOW|RTLD_GLOBAL);
    android_address=library?dlsym(library,"android_dladdr"):0;
    bionic_tls=library?dlsym(library,"_hybris_hook___get_tls_hooks"):0;
    void (*set_hook)(void *(*)(const char*,const char*))=library?dlsym(library,"hybris_set_hook_callback"):0;
    if(set_hook)set_hook(trace_hook);
}
