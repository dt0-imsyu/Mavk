#pragma once
#include <wayland-client.h>
#include "banner-ahb-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"

typedef struct {
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct xdg_wm_base *wm;
    struct banner_ahb_v1 *factory;
    struct wl_surface *surface;
    struct xdg_surface *xdg;
    struct xdg_toplevel *top;
    struct wl_callback *frame;
    struct wl_buffer *buffers[3];
    AhbPoolSlot slots[3];
    int released[3],dependency[3],busy[3],returned[3];
    int configured,frame_ready,mode,synced,error;
    unsigned width,height,callbacks,fence_returns,immediate_returns;
} AhbWayland;
#include "ahb-wayland-draw.h"
static void ahb_wl_return(AhbWayland *c,struct wl_buffer *buffer,int fd){
    for(unsigned i=0;i<3;i++)if(c->buffers[i]==buffer){
        if(!c->busy[i]||c->returned[i]){if(fd>=0)close(fd);c->error=1;return;}
        c->dependency[i]=fd;c->returned[i]=1;
        if(fd>=0)c->fence_returns++;else c->immediate_returns++;
        return;
    }
    if(fd>=0)close(fd);c->error=1;
}
static void ahb_wl_mode(void *data,struct banner_ahb_v1 *factory,uint32_t enabled){(void)factory;((AhbWayland*)data)->mode=enabled?1:0;}
static void ahb_wl_fenced(void *data,struct banner_ahb_v1 *factory,struct wl_buffer *buffer,int32_t fd){(void)factory;ahb_wl_return(data,buffer,fd);}
static void ahb_wl_immediate(void *data,struct banner_ahb_v1 *factory,struct wl_buffer *buffer){(void)factory;ahb_wl_return(data,buffer,-1);}
static const struct banner_ahb_v1_listener ahb_wl_factory_listener={ahb_wl_mode,ahb_wl_fenced,ahb_wl_immediate};
static void ahb_wl_ping(void *data,struct xdg_wm_base *wm,uint32_t serial){(void)data;xdg_wm_base_pong(wm,serial);}
static const struct xdg_wm_base_listener ahb_wl_wm_listener={ahb_wl_ping};
static void ahb_wl_global(void *data,struct wl_registry *registry,uint32_t id,const char *name,uint32_t version){
    AhbWayland *c=data;
    if(!strcmp(name,"wl_compositor"))c->compositor=wl_registry_bind(registry,id,&wl_compositor_interface,version<4?version:4);
    else if(!strcmp(name,"xdg_wm_base")){c->wm=wl_registry_bind(registry,id,&xdg_wm_base_interface,1);xdg_wm_base_add_listener(c->wm,&ahb_wl_wm_listener,c);}
    else if(!strcmp(name,"banner_ahb_v1")&&version>=3){c->factory=wl_registry_bind(registry,id,&banner_ahb_v1_interface,3);banner_ahb_v1_add_listener(c->factory,&ahb_wl_factory_listener,c);}
}
static void ahb_wl_remove(void *data,struct wl_registry *registry,uint32_t id){(void)data;(void)registry;(void)id;}
static const struct wl_registry_listener ahb_wl_registry_listener={ahb_wl_global,ahb_wl_remove};
static void ahb_wl_configure(void *data,struct xdg_surface *surface,uint32_t serial){xdg_surface_ack_configure(surface,serial);((AhbWayland*)data)->configured=1;}
static const struct xdg_surface_listener ahb_wl_xdg_listener={ahb_wl_configure};
static void ahb_wl_size(void *data,struct xdg_toplevel *top,int32_t w,int32_t h,struct wl_array *states){
    (void)top;(void)states;AhbWayland *c=data;
    if((w>0&&(unsigned)w!=c->width)||(h>0&&(unsigned)h!=c->height)){printf("NO_GO Wayland configure=%dx%d expected=%ux%u\n",w,h,c->width,c->height);c->error=1;}
}
static void ahb_wl_close(void *data,struct xdg_toplevel *top){(void)top;((AhbWayland*)data)->error=1;}
static const struct xdg_toplevel_listener ahb_wl_top_listener={.configure=ahb_wl_size,.close=ahb_wl_close};
static void ahb_wl_buffer_release(void *data,struct wl_buffer *buffer){
    AhbWayland *c=data;for(unsigned i=0;i<3;i++)if(c->buffers[i]==buffer){c->released[i]=1;return;}c->error=1;
}
static const struct wl_buffer_listener ahb_wl_buffer_listener={ahb_wl_buffer_release};
static void ahb_wl_frame(void *data,struct wl_callback *callback,uint32_t stamp){
    (void)stamp;AhbWayland *c=data;wl_callback_destroy(callback);c->frame=NULL;c->frame_ready=1;c->callbacks++;
}
static const struct wl_callback_listener ahb_wl_frame_listener={ahb_wl_frame};
static void ahb_wl_sync(void *data,struct wl_callback *callback,uint32_t stamp){(void)stamp;wl_callback_destroy(callback);((AhbWayland*)data)->synced=1;}
static const struct wl_callback_listener ahb_wl_sync_listener={ahb_wl_sync};
static int ahb_wl_error(AhbWayland *c,const char *stage){
    const struct wl_interface *interface=NULL;uint32_t id=0;
    uint32_t code=wl_display_get_protocol_error(c->display,&interface,&id);
    printf("NO_GO Wayland stage=%s error=%d protocol=%s object=%u code=%u\n",stage,wl_display_get_error(c->display),interface?interface->name:"none",id,code);
    return 3;
}
static int ahb_wl_pump(AhbWayland *c){
    if(c->error)return 3;
    while(wl_display_prepare_read(c->display)!=0)if(wl_display_dispatch_pending(c->display)<0||c->error)return 3;
    int flushed=wl_display_flush(c->display);
    if(flushed<0&&errno!=EAGAIN){wl_display_cancel_read(c->display);return 3;}
    struct pollfd p={.fd=wl_display_get_fd(c->display),.events=POLLIN|(flushed<0?POLLOUT:0)};
    int result;do{result=poll(&p,1,5000);}while(result<0&&errno==EINTR);
    if(result<=0){wl_display_cancel_read(c->display);printf("NO_GO Wayland event wait=%d\n",result);return 3;}
    if(p.revents&POLLIN){if(wl_display_read_events(c->display)<0)return ahb_wl_error(c,"read");}else wl_display_cancel_read(c->display);
    if(wl_display_dispatch_pending(c->display)<0)return ahb_wl_error(c,"dispatch");
    if(p.revents&(POLLERR|POLLHUP|POLLNVAL))return ahb_wl_error(c,"poll-hup");
    return c->error?3:0;
}
static int ahb_wl_roundtrip(AhbWayland *c){
    c->synced=0;struct wl_callback *sync=wl_display_sync(c->display);if(!sync)return 3;
    wl_callback_add_listener(sync,&ahb_wl_sync_listener,c);
    while(!c->synced)if(ahb_wl_pump(c))return 3;
    return 0;
}
static int ahb_wayland_run(void *window,void *(*symbol)(void*,const char*),struct AHardwareBuffer *first,const HardwareBufferDesc *desc,uint32_t family){
    AhbWayland c={.width=desc->width,.height=desc->height,.mode=-1,.dependency={-2,-2,-2}};
    struct AHardwareBuffer *hardware[3]={first,NULL,NULL};int result=3,outgoing=-2;
    int (*allocate)(const HardwareBufferDesc*,struct AHardwareBuffer**)=symbol(window,"AHardwareBuffer_allocate");
    void (*release)(struct AHardwareBuffer*)=symbol(window,"AHardwareBuffer_release");
    int (*send_handle)(const struct AHardwareBuffer*,int)=symbol(window,"AHardwareBuffer_sendHandleToUnixSocket");
    const char *socket=getenv("MAVK_AHB_WAYLAND_SOCKET");const char prefix[]="/data/user/0/com.droiddeck.launcher.mavk/files/";
    if(!allocate||!release||!send_handle||!socket||strncmp(socket,prefix,sizeof(prefix)-1)||strstr(socket,".."))return 3;
    c.display=wl_display_connect(socket);if(!c.display)return fail("Wayland connect",-1);
    c.registry=wl_display_get_registry(c.display);wl_registry_add_listener(c.registry,&ahb_wl_registry_listener,&c);
    if(ahb_wl_roundtrip(&c)||!c.compositor||!c.wm||!c.factory||ahb_wl_roundtrip(&c)||c.mode!=1){printf("NO_GO Wayland compositor=%d xdg=%d explicitFactory=%d mode=%d\n",c.compositor!=NULL,c.wm!=NULL,c.factory!=NULL,c.mode);goto finish;}
    if(pool_dispatch())goto finish;
    for(unsigned i=0;i<3;i++){
        HardwareBufferDesc request=*desc;request.stride=0;request.rfu0=0;request.rfu1=0;
        if(i&&allocate(&request,&hardware[i]))goto finish;
        if(pool_import(&c.slots[i],hardware[i],desc,family,0))goto finish;
        int wire[2];if(socketpair(AF_UNIX,SOCK_STREAM|SOCK_CLOEXEC,0,wire))goto finish;
        struct timeval timeout={.tv_sec=2};
        if(setsockopt(wire[1],SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout))){close(wire[0]);close(wire[1]);goto finish;}
        if(send_handle(hardware[i],wire[0])){close(wire[0]);close(wire[1]);goto finish;}
        c.buffers[i]=banner_ahb_v1_create_buffer(c.factory,wire[1],desc->width,desc->height,desc->stride,3);
        close(wire[0]);close(wire[1]);if(!c.buffers[i])goto finish;
        wl_buffer_add_listener(c.buffers[i],&ahb_wl_buffer_listener,&c);
    }
    if(ahb_wl_draw_setup(&c))goto finish;
    c.surface=wl_compositor_create_surface(c.compositor);c.xdg=xdg_wm_base_get_xdg_surface(c.wm,c.surface);
    xdg_surface_add_listener(c.xdg,&ahb_wl_xdg_listener,&c);c.top=xdg_surface_get_toplevel(c.xdg);
    xdg_toplevel_add_listener(c.top,&ahb_wl_top_listener,&c);xdg_toplevel_set_title(c.top,"Mavk explicit AHB source");xdg_toplevel_set_app_id(c.top,"org.mavk.ahb-client");
    xdg_toplevel_set_fullscreen(c.top,NULL);wl_surface_commit(c.surface);
    while(!c.configured)if(ahb_wl_pump(&c))goto finish;
    uint64_t start=pool_now();c.frame_ready=1;
    const unsigned frames=360;
    for(unsigned seq=0;seq<frames;seq++){
        unsigned index=seq%3;
        while(!c.frame_ready||(c.busy[index]&&(!c.returned[index]||!c.released[index])))if(ahb_wl_pump(&c))goto finish;
        if(!c.mode){printf("NO_GO Wayland zero-copy mode changed\n");goto finish;}
        if(pool_submit(&c.slots[index],family,17+seq%200,&c.dependency[index],&outgoing))goto finish;
        c.busy[index]=1;c.returned[index]=0;c.released[index]=0;
        if(outgoing>=0){banner_ahb_v1_set_acquire_fence(c.factory,c.buffers[index],outgoing);close(outgoing);}else banner_ahb_v1_set_acquire_ready(c.factory,c.buffers[index]);outgoing=-2;
        wl_surface_attach(c.surface,c.buffers[index],0,0);wl_surface_damage(c.surface,0,0,(int32_t)c.width,(int32_t)c.height);
        c.frame=wl_surface_frame(c.surface);wl_callback_add_listener(c.frame,&ahb_wl_frame_listener,&c);c.frame_ready=0;
        wl_surface_commit(c.surface);wl_display_flush(c.display);
    }
    while(!c.frame_ready)if(ahb_wl_pump(&c))goto finish;
    printf("AHB_WAYLAND_SOURCE_SUBMITTED frames=%u callbacks=%u elapsedMs=%.3f fencedReturns=%u immediateReturns=%u sourceReadbacks=0 realShaderDraw=1 uniqueSurfaceFpsUnmeasured=1\n",frames,c.callbacks,(pool_now()-start)/1000000.0,c.fence_returns,c.immediate_returns);
    result=0;
finish:
    if(outgoing>=0)close(outgoing);
    for(unsigned i=0;i<3;i++)if(c.slots[i].pending){
        VkResult wait=vkWaitForFences(device,1,&c.slots[i].fence,VK_TRUE,5000000000ull);
        if(wait!=VK_SUCCESS){printf("NO_GO bounded source retirement=%d exiting isolated producer\n",wait);fflush(stdout);_Exit(3);}
    }
    vkDeviceWaitIdle(device);
    ahb_wl_draw_destroy();
    if(c.surface){wl_surface_attach(c.surface,NULL,0,0);wl_surface_commit(c.surface);wl_display_flush(c.display);}
    if(c.frame)wl_callback_destroy(c.frame);
    for(unsigned i=0;i<3;i++){if(c.buffers[i])wl_buffer_destroy(c.buffers[i]);if(c.dependency[i]>=0)close(c.dependency[i]);pool_destroy(&c.slots[i]);if(i&&hardware[i])release(hardware[i]);}
    if(c.top)xdg_toplevel_destroy(c.top);if(c.xdg)xdg_surface_destroy(c.xdg);if(c.surface)wl_surface_destroy(c.surface);
    if(c.factory)banner_ahb_v1_destroy(c.factory);if(c.wm)xdg_wm_base_destroy(c.wm);if(c.compositor)wl_compositor_destroy(c.compositor);if(c.registry)wl_registry_destroy(c.registry);
    wl_display_flush(c.display);wl_display_disconnect(c.display);
    return result;
}
