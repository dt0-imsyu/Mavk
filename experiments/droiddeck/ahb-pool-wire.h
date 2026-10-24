#pragma once
#include "ahb-fence-wire.h"
#define AHB_POOL_MAGIC 0x4d504f33u
#define AHB_POOL_SLOTS 3u
typedef struct {uint32_t magic,frames,width,height;} AhbPoolConfig;

#ifdef MAVK_AHB_NATIVE_POOL
static int native_pool_session(int socket,int negative){
    AhbPoolSlot slots[AHB_POOL_SLOTS]={0};AHardwareBuffer *buffers[AHB_POOL_SLOTS]={0};
    int result=3,incoming=-2,outgoing=-2;uint32_t family;
    MavkAhbHello hello={.magic=MAVK_AHB_HELLO_MAGIC,.version=3,.async_fence=1,.reserved=AHB_POOL_SLOTS};
    AhbPoolConfig config={0};
    if(ahb_wire_write(socket,&hello,sizeof(hello))||ahb_wire_read(socket,&config,sizeof(config)))goto finish;
    if(config.magic!=AHB_POOL_MAGIC||config.frames<6||config.frames>240||!config.width||!config.height||config.width>1920||config.height>1200)goto finish;
    native_async_mode=1;
    if(native_init_gpu(&family)||pool_dispatch())goto finish;
    for(unsigned i=0;i<AHB_POOL_SLOTS;i++){
        if(AHardwareBuffer_recvHandleFromUnixSocket(socket,&buffers[i])||!buffers[i])goto finish;
        AHardwareBuffer_Desc desc={0};AHardwareBuffer_describe(buffers[i],&desc);
        MavkAhbAck ack={.width=desc.width,.height=desc.height,.layers=desc.layers,.format=desc.format,.stride=desc.stride};
        if(AHardwareBuffer_getId(buffers[i],&ack.id)||desc.width!=config.width||desc.height!=config.height||pool_import(&slots[i],buffers[i],&desc,family,1))goto finish;
        if(ahb_wire_write(socket,&ack,sizeof(ack)))goto finish;
        printf("POOL_NATIVE_SLOT slot=%u id=%llu\n",i,(unsigned long long)ack.id);
    }
    uint64_t start=pool_now();
    for(unsigned seq=0;seq<config.frames;seq++){
        unsigned index=seq%AHB_POOL_SLOTS,red=17+seq%200;
        if(ahb_wire_recv_fence(socket,seq,&incoming))goto finish;
        if(negative&&seq==7)red++;
        if(pool_submit(&slots[index],family,red,&incoming,&outgoing))goto finish;
        if(ahb_wire_send_fence(socket,seq,outgoing))goto finish;
        if(outgoing>=0)close(outgoing);outgoing=-2;
    }
    for(unsigned i=0;i<AHB_POOL_SLOTS;i++)if(pool_retire(&slots[i]))goto finish;
    MavkAhbAck final={.id=config.frames,.width=config.width,.height=config.height};
    if(ahb_wire_write(socket,&final,sizeof(final)))goto finish;
    printf("POOL_NATIVE_PASS frames=%u slots=3 pixels=%llu elapsedMs=%.3f resourceRecycleWaits=%llu recycleWaitMs=%.3f cpuOracleMs=%.3f acquireAndReleaseSyncFd=1 surfaceClaim=0\n",config.frames,(unsigned long long)pool_pixels,(pool_now()-start)/1000000.0,(unsigned long long)pool_wait_calls,pool_wait_ns/1000000.0,pool_oracle_ns/1000000.0);
    printf("POOL_NATIVE_FENCES pendingAtImport=%llu pendingAtExport=%llu hostPollWaits=0\n",(unsigned long long)pool_import_pending,(unsigned long long)pool_export_pending);
    result=0;
finish:
    if(incoming>=0)close(incoming);if(outgoing>=0)close(outgoing);
    if(device){vkDeviceWaitIdle(device);for(unsigned i=0;i<AHB_POOL_SLOTS;i++)pool_destroy(&slots[i]);vkDestroyDevice(device,NULL);device=VK_NULL_HANDLE;}
    for(unsigned i=0;i<AHB_POOL_SLOTS;i++)if(buffers[i])AHardwareBuffer_release(buffers[i]);
    if(instance){vkDestroyInstance(instance,NULL);instance=VK_NULL_HANDLE;}
    return result;
}
#else
static int ahb_pool_producer(void *window,void *(*symbol)(void*,const char*),struct AHardwareBuffer *first,const HardwareBufferDesc *desc,uint32_t family,unsigned frames){
    AhbPoolSlot slots[AHB_POOL_SLOTS]={0};struct AHardwareBuffer *buffers[AHB_POOL_SLOTS]={first,NULL,NULL};
    int (*allocate)(const HardwareBufferDesc*,struct AHardwareBuffer**)=symbol(window,"AHardwareBuffer_allocate");
    void (*release)(struct AHardwareBuffer*)=symbol(window,"AHardwareBuffer_release");
    int (*send_handle)(const struct AHardwareBuffer*,int)=symbol(window,"AHardwareBuffer_sendHandleToUnixSocket");
    int (*get_id)(const struct AHardwareBuffer*,uint64_t*)=symbol(window,"AHardwareBuffer_getId");
    const char *path=getenv("MAVK_AHB_SOCKET");const char prefix[]="/data/user/0/com.droiddeck.launcher.mavk/files/mavk-ahb/";
    if(!path||strncmp(path,prefix,sizeof(prefix)-1)||strstr(path,"..")||strlen(path)>=sizeof(((struct sockaddr_un*)0)->sun_path)||frames<6||frames>240||!allocate||!release||!send_handle||!get_id)return 3;
    int socket_fd=-1,incoming=-2,outgoing=-2,result=3,returns[AHB_POOL_SLOTS]={-2,-2,-2};
    if(pool_dispatch())goto finish;
    socket_fd=socket(AF_UNIX,SOCK_STREAM,0);if(socket_fd<0)goto finish;
    struct timeval timeout={.tv_sec=5};setsockopt(socket_fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));setsockopt(socket_fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
    struct sockaddr_un address={.sun_family=AF_UNIX};strcpy(address.sun_path,path);
    if(connect(socket_fd,(struct sockaddr*)&address,sizeof(address)))goto finish;
    struct ucred peer;socklen_t bytes=sizeof(peer);if(getsockopt(socket_fd,SOL_SOCKET,SO_PEERCRED,&peer,&bytes)||peer.uid!=getuid())goto finish;
    MavkAhbHello hello={0};if(ahb_wire_read(socket_fd,&hello,sizeof(hello))||hello.magic!=MAVK_AHB_HELLO_MAGIC||hello.version!=3||hello.async_fence!=1||hello.reserved!=AHB_POOL_SLOTS)goto finish;
    AhbPoolConfig config={.magic=AHB_POOL_MAGIC,.frames=frames,.width=desc->width,.height=desc->height};if(ahb_wire_write(socket_fd,&config,sizeof(config)))goto finish;
    for(unsigned i=0;i<AHB_POOL_SLOTS;i++){
        HardwareBufferDesc request=*desc;request.stride=0;request.rfu0=0;request.rfu1=0;
        if(i&&allocate(&request,&buffers[i]))goto finish;
        if(pool_import(&slots[i],buffers[i],desc,family,0))goto finish;
        uint64_t id;if(get_id(buffers[i],&id)||send_handle(buffers[i],socket_fd))goto finish;
        MavkAhbAck ack;if(ahb_wire_read(socket_fd,&ack,sizeof(ack))||ack.status||ack.id!=id||ack.width!=desc->width||ack.height!=desc->height||ack.format!=1||ack.layers!=1||ack.stride!=desc->stride)goto finish;
        printf("POOL_GLIBC_SLOT slot=%u id=%llu\n",i,(unsigned long long)id);
    }
    uint64_t start=pool_now();
    for(unsigned seq=0;seq<frames;seq++){
        unsigned index=seq%AHB_POOL_SLOTS;incoming=returns[index];returns[index]=-2;
        if(pool_submit(&slots[index],family,17+seq%200,&incoming,&outgoing))goto finish;
        if(ahb_wire_send_fence(socket_fd,seq,outgoing))goto finish;
        if(outgoing>=0)close(outgoing);outgoing=-2;
        if(ahb_wire_recv_fence(socket_fd,seq,&returns[index]))goto finish;
    }
    MavkAhbAck final;if(ahb_wire_read(socket_fd,&final,sizeof(final))||final.status||final.id!=frames||final.width!=desc->width||final.height!=desc->height)goto finish;
    for(unsigned i=0;i<AHB_POOL_SLOTS;i++)if(pool_retire(&slots[i]))goto finish;
    printf("POOL_GLIBC_PASS frames=%u slots=3 elapsedMs=%.3f sourceReadbacks=0 pixelPayloadBytes=0 resourceRecycleWaits=%llu recycleWaitMs=%.3f noSurfaceClaim=1\n",frames,(pool_now()-start)/1000000.0,(unsigned long long)pool_wait_calls,pool_wait_ns/1000000.0);
    printf("POOL_GLIBC_FENCES pendingAtImport=%llu pendingAtExport=%llu hostPollWaits=0\n",(unsigned long long)pool_import_pending,(unsigned long long)pool_export_pending);
    result=0;
finish:
    if(incoming>=0)close(incoming);if(outgoing>=0)close(outgoing);
    vkDeviceWaitIdle(device);for(unsigned i=0;i<AHB_POOL_SLOTS;i++){if(returns[i]>=0)close(returns[i]);pool_destroy(&slots[i]);if(i&&buffers[i])release(buffers[i]);}
    if(socket_fd>=0)close(socket_fd);return result;
}
#endif
