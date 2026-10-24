#define _GNU_SOURCE
#include <android/hardware_buffer.h>
#include <errno.h>
#include <poll.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>
#include "ahb-ipc.h"
#include "ahb-native-gpu-check.h"
#include "ahb-fence-wire.h"
#include "ahb-pool-gpu.h"
#define MAVK_AHB_NATIVE_POOL
#include "ahb-pool-wire.h"
#include "ahb-bgra-check.h"

static int query_export_formats(void) {
    uint32_t family;
    int status=native_init_gpu(&family);if(status)return status;
    const VkFormat formats[]={VK_FORMAT_R8G8B8A8_UNORM,VK_FORMAT_B8G8R8A8_UNORM};
    const VkImageUsageFlags usages[]={VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT};
    for(unsigned i=0;i<2;i++)for(unsigned usage=0;usage<2;usage++){
        VkPhysicalDeviceExternalImageFormatInfo external={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO,
            .handleType=VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID};
        VkPhysicalDeviceImageFormatInfo2 query={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,.pNext=&external,
            .format=formats[i],.type=VK_IMAGE_TYPE_2D,.tiling=VK_IMAGE_TILING_OPTIMAL,
            .usage=usages[usage]};
        VkExternalImageFormatProperties memory={.sType=VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES};
        VkImageFormatProperties2 result={.sType=VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2,.pNext=&memory};
        VkResult code=vkGetPhysicalDeviceImageFormatProperties2(physical,&query,&result);
        printf("AHB_FORMAT_QUERY format=%u usage=0x%x result=%d externalFeatures=0x%x max=%ux%u allocation=NOT_TESTED\n",
            (unsigned)formats[i],(unsigned)usages[usage],code,memory.externalMemoryProperties.externalMemoryFeatures,
            result.imageFormatProperties.maxExtent.width,result.imageFormatProperties.maxExtent.height);
    }
    vkDestroyDevice(device,NULL);vkDestroyInstance(instance,NULL);
    device=VK_NULL_HANDLE;instance=VK_NULL_HANDLE;
    return 0;
}

int main(int argc,char **argv) {
    setvbuf(stdout,NULL,_IOLBF,0);
    if(argc==2&&!strcmp(argv[1],"--format-support"))return query_export_formats();
    if(argc==2&&!strcmp(argv[1],"--bgra-blit-check"))return native_check_bgra_blit(0,0);
    if(argc==2&&!strcmp(argv[1],"--bgra-copy-negative"))return native_check_bgra_blit(1,0);
    if(argc==2&&!strcmp(argv[1],"--bgra-pattern-check"))return native_check_bgra_blit(0,1);
    if(argc==2&&!strcmp(argv[1],"--bgra-pattern-negative"))return native_check_bgra_blit(1,1);
    const char prefix[]="/data/user/0/com.droiddeck.launcher.mavk/files/mavk-ahb/";
    if((argc!=2&&argc!=3)||strncmp(argv[1],prefix,sizeof(prefix)-1)||strstr(argv[1],"..")||strlen(argv[1])>=sizeof(((struct sockaddr_un*)0)->sun_path))return 2;
    int pool_mode=0;
    if(argc==3){if(!strcmp(argv[2],"negative-control"))native_expected_red=42;else if(!strcmp(argv[2],"async-fence"))native_async_mode=1;else if(!strcmp(argv[2],"pool"))pool_mode=1;else if(!strcmp(argv[2],"pool-negative"))pool_mode=2;else return 2;}
    struct sockaddr_un address={.sun_family=AF_UNIX};strcpy(address.sun_path,argv[1]);
    umask(077);
    int server=socket(AF_UNIX,SOCK_STREAM,0);if(server<0)return 2;
    if(bind(server,(struct sockaddr*)&address,sizeof(address))||listen(server,1)){perror("AHB_BIND");close(server);return 2;}
    printf("AHB_NATIVE_LISTEN pid=%d uid=%d\n",getpid(),getuid());
    struct pollfd ready={.fd=server,.events=POLLIN};int client=-1,result=3;
    if(poll(&ready,1,30000)<=0)goto finish;
    client=accept(server,NULL,NULL);if(client<0)goto finish;
    struct timeval timeout={.tv_sec=5};setsockopt(client,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));setsockopt(client,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
    struct ucred peer;socklen_t length=sizeof(peer);
    if(getsockopt(client,SOL_SOCKET,SO_PEERCRED,&peer,&length)||peer.uid!=getuid())goto finish;
    if(pool_mode){result=native_pool_session(client,pool_mode==2);goto finish;}
    MavkAhbHello hello={.magic=MAVK_AHB_HELLO_MAGIC,.version=2,.async_fence=(uint32_t)native_async_mode};
    if(ahb_wire_write(client,&hello,sizeof(hello)))goto finish;
    AHardwareBuffer *buffer=NULL;
    int received=AHardwareBuffer_recvHandleFromUnixSocket(client,&buffer);
    if(received||!buffer){printf("NO_GO stage=native-AHB-recv result=%d errno=%d\n",received,errno);goto finish;}
    AHardwareBuffer_Desc desc={0};AHardwareBuffer_describe(buffer,&desc);
    MavkAhbAck ack={.width=desc.width,.height=desc.height,.layers=desc.layers,.format=desc.format,.stride=desc.stride};
    ack.status=AHardwareBuffer_getId(buffer,&ack.id);
    if(!ack.status&&native_async_mode&&ahb_wire_recv_fence(client,ack.id,&native_incoming_fd))ack.status=3;
    if(!ack.status)ack.status=native_verify_ahb(buffer,&desc);
    size_t sent=0;while(sent<sizeof(ack)){ssize_t n=send(client,(char*)&ack+sent,sizeof(ack)-sent,MSG_NOSIGNAL);if(n<=0)break;sent+=(size_t)n;}
    printf("AHB_NATIVE_RECEIVE id=%llu width=%u height=%u format=%u stride=%u status=%d peerPid=%d ackBytes=%zu\n",(unsigned long long)ack.id,ack.width,ack.height,ack.format,ack.stride,ack.status,peer.pid,sent);
    AHardwareBuffer_release(buffer);
    result=ack.status==0&&sent==sizeof(ack)?0:3;
finish:
    if(native_incoming_fd>=0)close(native_incoming_fd);
    if(client>=0)close(client);close(server);unlink(argv[1]);
    return result;
}
