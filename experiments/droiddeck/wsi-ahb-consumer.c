#define _GNU_SOURCE
#define VK_USE_PLATFORM_WAYLAND_KHR
#define main mavk_existing_probe_main
#include "hybris_probe.c"
#undef main
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include "wsi-ahb-render.h"

static struct wl_compositor *compositor;
static struct xdg_wm_base *wm;
static int configured,window_error;
static void wm_ping(void *data,struct xdg_wm_base *base,uint32_t serial){(void)data;xdg_wm_base_pong(base,serial);}
static const struct xdg_wm_base_listener wm_listener={wm_ping};
static void surface_configure(void *data,struct xdg_surface *surface,uint32_t serial){(void)data;xdg_surface_ack_configure(surface,serial);configured=1;}
static const struct xdg_surface_listener surface_listener={surface_configure};
static void top_size(void *data,struct xdg_toplevel *top,int32_t w,int32_t h,struct wl_array *states){
    (void)data;(void)top;(void)states;printf("WSI_WINDOW_CONFIGURE width=%d height=%d\n",w,h);
    if((w>0&&w!=1920)||(h>0&&h!=1200))window_error=1;
}
static void top_close(void *data,struct xdg_toplevel *top){(void)data;(void)top;window_error=1;}
static const struct xdg_toplevel_listener top_listener={.configure=top_size,.close=top_close};
static void registry_global(void *data, struct wl_registry *registry, uint32_t name,
                            const char *interface, uint32_t version) {
    (void)data;
    if (!strcmp(interface,"wl_compositor"))
        compositor=wl_registry_bind(registry,name,&wl_compositor_interface,version<4?version:4);
    else if(!strcmp(interface,"xdg_wm_base")){
        wm=wl_registry_bind(registry,name,&xdg_wm_base_interface,1);xdg_wm_base_add_listener(wm,&wm_listener,NULL);
    }
}
static void registry_remove(void *data, struct wl_registry *registry,uint32_t name) {
    (void)data;(void)registry;(void)name;
}
int main(void) {
    setvbuf(stdout,NULL,_IOLBF,0);
    const char *path=getenv("MAVK_AHB_WAYLAND_SOCKET");
    const char prefix[]="/data/user/0/com.droiddeck.launcher.mavk/files/";
    if(!path||strncmp(path,prefix,sizeof(prefix)-1)||strstr(path,".."))return 2;
    struct wl_display *display=wl_display_connect(path);
    if(!display)return fail("Wayland connect",-1);
    struct wl_registry *registry=wl_display_get_registry(display);
    const struct wl_registry_listener listener={registry_global,registry_remove};
    wl_registry_add_listener(registry,&listener,NULL);
    if(wl_display_roundtrip(display)<0||!compositor)return fail("Wayland registry",-1);
    struct wl_surface *window=wl_compositor_create_surface(compositor);
    const char *draw_flag=getenv("MAVK_AHB_WSI_DRAW");int draw=draw_flag&&!strcmp(draw_flag,"1");
    struct xdg_surface *xdg=NULL;struct xdg_toplevel *top=NULL;
    if(draw){
        if(!wm)return fail("xdg shell",-1);
        xdg=xdg_wm_base_get_xdg_surface(wm,window);xdg_surface_add_listener(xdg,&surface_listener,NULL);
        top=xdg_surface_get_toplevel(xdg);xdg_toplevel_add_listener(top,&top_listener,NULL);
        xdg_toplevel_set_title(top,"Mavk standard Vulkan swapchain");xdg_toplevel_set_app_id(top,"org.mavk.standard-wsi");
        xdg_toplevel_set_fullscreen(top,NULL);wl_surface_commit(window);
        if(wl_display_roundtrip(display)<0||wl_display_roundtrip(display)<0||!configured||window_error)return fail("fullscreen configure",-1);
    }
    void *loader=dlopen("libvulkan.so.1",RTLD_NOW|RTLD_LOCAL);
    get=loader?(PFN_vkGetInstanceProcAddr)dlsym(loader,"vkGetInstanceProcAddr"):NULL;
    if(!get)return fail("Vulkan loader",-1);
    vkCreateInstance=(PFN_vkCreateInstance)get(NULL,"vkCreateInstance");
    const char *extensions[]={VK_KHR_SURFACE_EXTENSION_NAME,VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME};
    VkApplicationInfo app={.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,.pApplicationName="Mavk standard WSI allocation consumer",.apiVersion=VK_API_VERSION_1_1};
    VkInstanceCreateInfo ci={.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,.pApplicationInfo=&app,.enabledExtensionCount=2,.ppEnabledExtensionNames=extensions};
    OK(vkCreateInstance(&ci,NULL,&instance));
    const char *expected_wsi=getenv("MAVK_EXPECT_WSI_PATH"),*expected_layer=getenv("MAVK_EXPECT_LAYER_PATH");
    FILE *maps=fopen("/proc/self/maps","r");char mapline[4096];int saw_wsi=0,saw_layer=0;
    if(maps){while(fgets(mapline,sizeof(mapline),maps)){
        if(expected_wsi&&strstr(mapline,expected_wsi))saw_wsi=1;
        if(expected_layer&&strstr(mapline,expected_layer))saw_layer=1;
    }fclose(maps);}
    if(!saw_wsi||!saw_layer)return fail("expected WSI/common mappings",-1);
    printf("WSI_AHB_MAPPED wsi=%s common=%s\n",expected_wsi,expected_layer);
#define LOAD_WSI(n) vk##n=(PFN_vk##n)get(instance,"vk" #n);if(!vk##n)return fail("resolve " #n,-1);
    FUNCTIONS(LOAD_WSI)
#undef LOAD_WSI
#define WSI(type,name) PFN_vk##type name=(PFN_vk##type)get(instance,"vk" #type);if(!name)return fail("resolve " #type,-1)
    WSI(CreateWaylandSurfaceKHR,create_surface);
    WSI(DestroySurfaceKHR,destroy_surface);
    WSI(GetPhysicalDeviceSurfaceFormatsKHR,formats);
    WSI(GetPhysicalDeviceSurfaceSupportKHR,support);
    WSI(CreateSwapchainKHR,create_swapchain);
    WSI(GetSwapchainImagesKHR,swapchain_images);
    WSI(DestroySwapchainKHR,destroy_swapchain);
    WSI(AcquireNextImageKHR,acquire_image);
    WSI(QueuePresentKHR,present);
#undef WSI
    uint32_t count=0;OK(vkEnumeratePhysicalDevices(instance,&count,NULL));
    if(!count||count>16)return fail("physical device count",-1);
    VkPhysicalDevice devices[16];OK(vkEnumeratePhysicalDevices(instance,&count,devices));
    for(uint32_t i=0;i<count;i++){
        VkPhysicalDeviceProperties props;vkGetPhysicalDeviceProperties(devices[i],&props);
        if(props.vendorID==0x13b5&&props.deviceType!=VK_PHYSICAL_DEVICE_TYPE_CPU){physical=devices[i];printf("WSI_AHB_DEVICE %s\n",props.deviceName);break;}
    }
    if(!physical)return fail("Mali hardware",-1);
    VkWaylandSurfaceCreateInfoKHR surface_info={.sType=VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,.display=display,.surface=window};
    VkSurfaceKHR surface;OK(create_surface(instance,&surface_info,NULL,&surface));
    count=0;OK(formats(physical,surface,&count,NULL));
    if(!count||count>64)return fail("surface format count",-1);
    VkSurfaceFormatKHR offered[64];OK(formats(physical,surface,&count,offered));
    const char *bgra=getenv("MAVK_AHB_WSI_BGRA");
    VkFormat selected_format=bgra&&!strcmp(bgra,"1")?VK_FORMAT_B8G8R8A8_UNORM:VK_FORMAT_R8G8B8A8_UNORM;
    int rgba=0;for(uint32_t i=0;i<count;i++)if(offered[i].format==selected_format)rgba=1;
    if(!rgba)return fail("requested surface format",-1);
    printf("WSI_STANDARD_FORMAT selected=%u\n",(unsigned)selected_format);
    count=0;vkGetPhysicalDeviceQueueFamilyProperties(physical,&count,NULL);
    if(!count||count>32)return fail("queue family count",-1);
    VkQueueFamilyProperties families[32];vkGetPhysicalDeviceQueueFamilyProperties(physical,&count,families);
    uint32_t family=UINT32_MAX;
    for(uint32_t i=0;i<count;i++){
        VkBool32 can_present=VK_FALSE;OK(support(physical,i,surface,&can_present));
        if(can_present&&(families[i].queueFlags&VK_QUEUE_GRAPHICS_BIT)){family=i;break;}
    }
    if(family==UINT32_MAX)return fail("surface graphics queue",-1);
    float priority=1;
    VkDeviceQueueCreateInfo qi={.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,.queueFamilyIndex=family,.queueCount=1,.pQueuePriorities=&priority};
    const char *device_extensions[]={VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo di={.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,.queueCreateInfoCount=1,.pQueueCreateInfos=&qi,.enabledExtensionCount=1,.ppEnabledExtensionNames=device_extensions};
    OK(vkCreateDevice(physical,&di,NULL,&device));
    VkSwapchainCreateInfoKHR si={.sType=VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,.surface=surface,.minImageCount=3,
        .imageFormat=selected_format,.imageColorSpace=VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent={1920,1200},.imageArrayLayers=1,.imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode=VK_SHARING_MODE_EXCLUSIVE,.preTransform=VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,.presentMode=VK_PRESENT_MODE_FIFO_KHR,.clipped=VK_TRUE};
    VkSwapchainKHR swapchain;OK(create_swapchain(device,&si,NULL,&swapchain));
    count=0;OK(swapchain_images(device,swapchain,&count,NULL));
    if(count<3||count>8)return fail("swapchain image count",-1);
    VkImage images[8];OK(swapchain_images(device,swapchain,&count,images));
    if(!draw){
    VkFence acquired;
    VkFenceCreateInfo acquire_info={.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    OK(vkCreateFence(device,&acquire_info,NULL,&acquired));
    uint32_t image_index=UINT32_MAX;
    OK(acquire_image(device,swapchain,1000000000ull,VK_NULL_HANDLE,acquired,&image_index));
    if(image_index>=count)return fail("acquired image index",-1);
    OK(vkWaitForFences(device,1,&acquired,VK_TRUE,5000000000ull));
    vkDestroyFence(device,acquired,NULL);
    }
    for(uint32_t i=0;i<count;i++){
        VkImageView view;
        VkImageViewCreateInfo vi={.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,.image=images[i],.viewType=VK_IMAGE_VIEW_TYPE_2D,
            .format=selected_format,.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1}};
        OK(vkCreateImageView(device,&vi,NULL,&view));vkDestroyImageView(device,view,NULL);
    }
    if(draw&&wsi_render(display,swapchain,images,count,family,acquire_image,present,&window_error,selected_format))return 1;
    OK(vkDeviceWaitIdle(device));
    destroy_swapchain(device,swapchain,NULL);vkDestroyDevice(device,NULL);
    destroy_surface(instance,surface,NULL);vkDestroyInstance(instance,NULL);
    if(top)xdg_toplevel_destroy(top);if(xdg)xdg_surface_destroy(xdg);if(wm)xdg_wm_base_destroy(wm);
    wl_surface_destroy(window);wl_compositor_destroy(compositor);wl_registry_destroy(registry);wl_display_disconnect(display);
    printf("WSI_AHB_CONSUMER_PASS standardApi=1 images=%u extent=1920x1200 imageViews=1 firstAcquireFence=%u presentTested=%u pixelTested=0\n",count,draw?0u:1u,draw?1u:0u);
    return 0;
}
