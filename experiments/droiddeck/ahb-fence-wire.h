#pragma once
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "ahb-ipc.h"

static inline int ahb_wire_read(int socket,void *data,size_t bytes){
    size_t done=0;while(done<bytes){ssize_t n=recv(socket,(char*)data+done,bytes-done,0);if(n<0&&errno==EINTR)continue;if(n<=0)return -1;done+=(size_t)n;}return 0;
}
static inline int ahb_wire_write(int socket,const void *data,size_t bytes){
    size_t done=0;while(done<bytes){ssize_t n=send(socket,(const char*)data+done,bytes-done,MSG_NOSIGNAL);if(n<0&&errno==EINTR)continue;if(n<=0)return -1;done+=(size_t)n;}return 0;
}
static inline int ahb_wire_send_fence(int socket,uint64_t id,int fd){
    MavkAhbFencePacket packet={.magic=MAVK_AHB_FENCE_MAGIC,.signaled=fd<0,.id=id};
    struct iovec io={.iov_base=&packet,.iov_len=sizeof(packet)};
    union{struct cmsghdr alignment;char bytes[CMSG_SPACE(sizeof(int))];}control={0};
    struct msghdr message={.msg_iov=&io,.msg_iovlen=1};
    if(fd>=0){message.msg_control=control.bytes;message.msg_controllen=sizeof(control.bytes);struct cmsghdr *c=CMSG_FIRSTHDR(&message);c->cmsg_level=SOL_SOCKET;c->cmsg_type=SCM_RIGHTS;c->cmsg_len=CMSG_LEN(sizeof(int));memcpy(CMSG_DATA(c),&fd,sizeof(fd));}
    ssize_t sent;do{sent=sendmsg(socket,&message,MSG_NOSIGNAL);}while(sent<0&&errno==EINTR);
    return sent==(ssize_t)sizeof(packet)?0:-1;
}
static inline int ahb_wire_recv_fence(int socket,uint64_t expected_id,int *fd){
    MavkAhbFencePacket packet={0};struct iovec io={.iov_base=&packet,.iov_len=sizeof(packet)};
    union{struct cmsghdr alignment;char bytes[CMSG_SPACE(2*sizeof(int))];}control={0};
    struct msghdr message={.msg_iov=&io,.msg_iovlen=1,.msg_control=control.bytes,.msg_controllen=sizeof(control.bytes)};
    ssize_t received;do{received=recvmsg(socket,&message,MSG_WAITALL|MSG_CMSG_CLOEXEC);}while(received<0&&errno==EINTR);
    if(received<0)return -1;
    int handles[2]={-1,-1};unsigned count=0;int invalid=0;
    for(struct cmsghdr *c=CMSG_FIRSTHDR(&message);c;c=CMSG_NXTHDR(&message,c)){
        if(c->cmsg_level!=SOL_SOCKET||c->cmsg_type!=SCM_RIGHTS||c->cmsg_len<CMSG_LEN(0)){invalid=1;continue;}
        size_t bytes=c->cmsg_len-CMSG_LEN(0);if(bytes%sizeof(int))invalid=1;
        for(size_t i=0;i<bytes/sizeof(int);i++){int value;memcpy(&value,(char*)CMSG_DATA(c)+i*sizeof(int),sizeof(value));if(count<2)handles[count++]=value;else{close(value);invalid=1;}}
    }
    invalid|=received!=(ssize_t)sizeof(packet)||(message.msg_flags&(MSG_CTRUNC|MSG_TRUNC))||packet.magic!=MAVK_AHB_FENCE_MAGIC||packet.id!=expected_id||packet.signaled>1;
    invalid|=count!=(packet.signaled?0u:1u);
    if(invalid){for(unsigned i=0;i<count;i++)close(handles[i]);return -1;}
    *fd=count?handles[0]:-1;return 0;
}
