#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/shm.h>

#ifdef MAVK_TRACE_WSI
typedef struct { unsigned sequence; } mavk_xcb_cookie;
mavk_xcb_cookie xcb_shm_put_image_checked(void *connection,uint32_t drawable,uint32_t gc,
    uint16_t total_width,uint16_t total_height,uint16_t src_x,uint16_t src_y,
    uint16_t src_width,uint16_t src_height,int16_t dst_x,int16_t dst_y,
    uint8_t depth,uint8_t format,uint8_t send_event,uint32_t segment,uint32_t offset) {
    static mavk_xcb_cookie (*next)(void*,uint32_t,uint32_t,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,int16_t,int16_t,uint8_t,uint8_t,uint8_t,uint32_t,uint32_t);
    static unsigned count;
    if(!next){void *library=dlopen("libxcb-shm.so.0",RTLD_NOW|RTLD_LOCAL);next=library?dlsym(library,"xcb_shm_put_image_checked"):NULL;}
    if(!next){fprintf(stderr,"MAVK_WSI_PUT missing native symbol\n");abort();}
    mavk_xcb_cookie cookie=next(connection,drawable,gc,total_width,total_height,src_x,src_y,src_width,src_height,dst_x,dst_y,depth,format,send_event,segment,offset);
    if(count++<4){
        void *library=dlopen("libxcb.so.1",RTLD_NOW|RTLD_LOCAL);
        void *(*check)(void*,mavk_xcb_cookie)=library?dlsym(library,"xcb_request_check"):NULL;
        unsigned char *error=check?check(connection,cookie):NULL;
        fprintf(stderr,"MAVK_WSI_PUT drawable=%x size=%ux%u segment=%u depth=%u error=%u major=%u\n",drawable,total_width,total_height,segment,depth,error?error[1]:0,error?error[10]:0);
        free(error);
    }
    return cookie;
}

int shmget(key_t key,size_t bytes,int flags) {
    static int (*next)(key_t,size_t,int);static unsigned count;
    if(!next)next=dlsym(RTLD_NEXT,"shmget");
    int result=next(key,bytes,flags),error=errno;
    if(count++<12)fprintf(stderr,"MAVK_WSI_SHMGET bytes=%zu id=%d errno=%d dir=%s\n",bytes,result,error,getenv("BL_SYSV_DIR"));
    errno=error;return result;
}
void *shmat(int id,const void *address,int flags) {
    static void *(*next)(int,const void*,int);static unsigned count;
    if(!next)next=dlsym(RTLD_NEXT,"shmat");
    void *result=next(id,address,flags);int error=errno;
    if(count++<12)fprintf(stderr,"MAVK_WSI_SHMAT id=%d address=%p errno=%d\n",id,result,error);
    errno=error;return result;
}
#endif

static int admitted_image_root(const char *path) {
    const char *root=getenv("MAVK_WINE_GUEST_ROOT");
    if(root&&root[0]=='/'&&strlen(root)>8&&!strncmp(path,root,strlen(root))&&path[strlen(root)]=='/')path+=strlen(root);
    const char *roots=getenv("MAVK_WINE_PE_ROOTS");
    if(!roots||strlen(roots)>8192)return 0;
    while(*roots){
        const char *end=strchr(roots,':');size_t count=end?(size_t)(end-roots):strlen(roots);
        if(count>8&&roots[0]=='/'&&!strncmp(path,roots,count)&&(path[count]=='/'||!path[count]))return 1;
        if(!end)break;roots=end+1;
    }
    return 0;
}

static void *pe_map(void *address,size_t bytes,int protection,int flags,int fd,off_t offset,
                    void *(*next)(void *,size_t,int,int,int,off_t)) {
    const char *enabled=getenv("MAVK_WINE_PE_READ_FALLBACK");
    if(!enabled||strcmp(enabled,"1")||fd<0||offset<0||!bytes||bytes>64*1024*1024||
       protection!=(PROT_READ|PROT_WRITE)||(flags&(MAP_SHARED|MAP_ANONYMOUS))||!(flags&MAP_PRIVATE))
        return next(address,bytes,protection,flags,fd,offset);
    char link[64],path[1024];snprintf(link,sizeof(link),"/proc/self/fd/%d",fd);
    ssize_t n=readlink(link,path,sizeof(path)-1);if(n<=0)return next(address,bytes,protection,flags,fd,offset);path[n]=0;
    const char *prefix="/data/user/0/com.droiddeck.launcher.mavk/files/";
    const char *wine="/root/.local/share/Steam/steamapps/common/Proton Experimental (ARM64)/files/lib/wine/";
    const char *windows="/root/.mavk-wine-poc/drive_c/windows/";
    int host=!strncmp(path,prefix,strlen(prefix))&&(strstr(path,"/files/lib/wine/")||strstr(path,"/mavk-window/pe/")||strstr(path,"/root/.mavk-wine-poc/drive_c/windows/"));
    if(!host&&strncmp(path,wine,strlen(wine))&&strncmp(path,windows,strlen(windows))&&!admitted_image_root(path))
        return next(address,bytes,protection,flags,fd,offset);
    unsigned char magic[2];struct stat status;
    if(pread(fd,magic,2,0)!=2||magic[0]!='M'||magic[1]!='Z'||fstat(fd,&status)||offset>=status.st_size)
        return next(address,bytes,protection,flags,fd,offset);
    size_t count=(size_t)(status.st_size-offset);if(count>bytes)count=bytes;
    void *mapped=next(address,bytes,protection,flags|MAP_ANONYMOUS,-1,0);if(mapped==MAP_FAILED)return mapped;
    size_t done=0;while(done<count){ssize_t got=pread(fd,(char*)mapped+done,count-done,offset+(off_t)done);
        if(got<0&&errno==EINTR)continue;
        if(got<=0){int error=got<0?errno:EIO;munmap(mapped,bytes);errno=error;return MAP_FAILED;}done+=(size_t)got;}
    static unsigned logged;
    unsigned record=__atomic_fetch_add(&logged,1,__ATOMIC_RELAXED);
    if(record<16)fprintf(stderr,"MAVK_WINE_PE_READ bytes=%zu file=%s\n",count,path);
    else if(record==16)fprintf(stderr,"MAVK_WINE_PE_READ further mapping messages suppressed\n");
    return mapped;
}

void *mmap(void *address,size_t bytes,int protection,int flags,int fd,off_t offset) {
    static void *(*next)(void *,size_t,int,int,int,off_t);
    if(!next)next=dlsym(RTLD_NEXT,"mmap");return pe_map(address,bytes,protection,flags,fd,offset,next);
}
void *mmap64(void *address,size_t bytes,int protection,int flags,int fd,off64_t offset) {
    static void *(*next)(void *,size_t,int,int,int,off_t);
    if(!next)next=dlsym(RTLD_NEXT,"mmap64");return pe_map(address,bytes,protection,flags,fd,offset,next);
}

int mprotect(void *address,size_t bytes,int protection) {
    static int (*next)(void *,size_t,int);
    static unsigned failures;
    if(!next)next=dlsym(RTLD_NEXT,"mprotect");
    int result=next(address,bytes,protection),error=errno;
    if(result&&failures++<16){
        fprintf(stderr,"MAVK_WINE_MPROTECT address=%p bytes=%zu prot=%x errno=%d\n",address,bytes,protection,error);
        FILE *maps=fopen("/proc/self/maps","r");
        if(maps){char line[1024];while(fgets(line,sizeof(line),maps)){unsigned long start,end;
            if(sscanf(line,"%lx-%lx",&start,&end)==2&&(uintptr_t)address>=start&&(uintptr_t)address<end){fprintf(stderr,"MAVK_WINE_MAPPING %s",line);break;}}
            fclose(maps);}
    }
    errno=error;return result;
}
