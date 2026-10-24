#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/shm.h>

int shmctl(int id,int command,struct shmid_ds *data) {
    static int (*next)(int,int,struct shmid_ds*);static unsigned count;
    if(!next)next=dlsym(RTLD_NEXT,"shmctl");
    int result=next(id,command,data);
    const char *owner=getenv("MAVK_X11_SYSV_OWNER");
    if(!result&&command==IPC_STAT&&data&&owner){
        unsigned uid=(unsigned)strtoul(owner,NULL,10);
        if(count++<8)fprintf(stderr,"MAVK_X11_SHM_OWNER id=%d old=%u new=%u\n",id,(unsigned)data->shm_perm.uid,uid);
        data->shm_perm.uid=data->shm_perm.cuid=uid;
        data->shm_perm.gid=data->shm_perm.cgid=uid;
    }
    return result;
}
