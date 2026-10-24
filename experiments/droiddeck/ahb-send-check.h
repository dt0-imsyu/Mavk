#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>
#include "ahb-ipc.h"
#include "ahb-fence-wire.h"

static int ahb_send_check(void *window,void *(*symbol)(void*,const char*),
                          struct AHardwareBuffer *buffer,const HardwareBufferDesc *desc) {
    const char *path=getenv("MAVK_AHB_SOCKET");if(!path)return 0;
    const char prefix[]="/data/user/0/com.droiddeck.launcher.mavk/files/mavk-ahb/";
    if(strncmp(path,prefix,sizeof(prefix)-1)||strstr(path,"..")||strlen(path)>=sizeof(((struct sockaddr_un*)0)->sun_path))return 3;
    int (*send_handle)(const struct AHardwareBuffer*,int)=symbol(window,"AHardwareBuffer_sendHandleToUnixSocket");
    int (*get_id)(const struct AHardwareBuffer*,uint64_t*)=symbol(window,"AHardwareBuffer_getId");
    uint64_t id=0;if(!send_handle||!get_id||get_id(buffer,&id))return fail("AHB send/getId API",-1);
    int fd=socket(AF_UNIX,SOCK_STREAM,0);if(fd<0)return fail("AHB IPC socket",-1);
    struct timeval timeout={.tv_sec=5};setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
    struct sockaddr_un address={.sun_family=AF_UNIX};strcpy(address.sun_path,path);
    uint64_t ipc_start=ahb_now_ns();
    if(connect(fd,(struct sockaddr*)&address,sizeof(address))){close(fd);return fail("AHB IPC connect",-1);}
    struct ucred peer;socklen_t size=sizeof(peer);
    if(getsockopt(fd,SOL_SOCKET,SO_PEERCRED,&peer,&size)||peer.uid!=getuid()){close(fd);return fail("AHB IPC peer UID",-1);}
    MavkAhbHello hello={0};int asynchronous=async_producer.fd>=-1;
    if(ahb_wire_read(fd,&hello,sizeof(hello))||hello.magic!=MAVK_AHB_HELLO_MAGIC||hello.version!=2||hello.async_fence!=(uint32_t)asynchronous||hello.reserved){close(fd);return fail("AHB protocol/mode handshake",-1);}
    int sent=send_handle(buffer,fd);
    if(sent){close(fd);return fail("AHB send handle",sent);}
    if(asynchronous&&ahb_wire_send_fence(fd,id,async_producer.fd)){close(fd);return fail("AHB sync fence send",-1);}
    MavkAhbAck ack={0};size_t received=0;
    while(received<sizeof(ack)){ssize_t n=recv(fd,(char*)&ack+received,sizeof(ack)-received,0);if(n<=0)break;received+=(size_t)n;}
    close(fd);
    if(received!=sizeof(ack)||ack.status||ack.id!=id||ack.width!=desc->width||ack.height!=desc->height||ack.layers!=desc->layers||ack.format!=desc->format||ack.stride!=desc->stride)
        return fail("AHB cross-process identity/descriptor",-1);
    printf("AHB_GLIBC_TO_BIONIC_HANDLE_PASS id=%llu peerPid=%d width=%u height=%u ackBytes=%zu pixelPayloadBytes=0 handoffAckMs=%.3f noSurfaceClaim=1\n",(unsigned long long)id,peer.pid,ack.width,ack.height,received,(double)(ahb_now_ns()-ipc_start)/1000000.0);
    return 0;
}
