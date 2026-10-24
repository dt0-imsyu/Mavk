#define _GNU_SOURCE
#define VK_USE_PLATFORM_ANDROID_KHR
#define main mavk_existing_probe_main
#include "hybris_probe.c"
#undef main

/* Public NDK ABI; do not mix Bionic system headers into a glibc translation unit. */
typedef struct {
    uint32_t width, height, layers, format;
    uint64_t usage;
    uint32_t stride, rfu0;
    uint64_t rfu1;
} HardwareBufferDesc;
_Static_assert(sizeof(HardwareBufferDesc) == 40, "NDK descriptor ABI");
static uint64_t ahb_now_ns(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return(uint64_t)t.tv_sec*1000000000ull+(uint64_t)t.tv_nsec;}
#include "ahb-image-check.h"
#include "ahb-async-producer.h"
#include "ahb-send-check.h"
#include "ahb-pool-gpu.h"
#include "ahb-pool-wire.h"
#ifdef MAVK_AHB_WAYLAND
#include "ahb-wayland-producer.h"
#endif

int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 0);
    void *loader = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
    if (!loader) { printf("NO_GO stage=loader error=%s\n", dlerror()); return 2; }
    get = (PFN_vkGetInstanceProcAddr)dlsym(loader, "vkGetInstanceProcAddr");
    if (!get) return fail("getInstanceProcAddr", -1);
    vkCreateInstance = (PFN_vkCreateInstance)get(NULL, "vkCreateInstance");
    VkApplicationInfo app = {.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName="Mavk AHB interop prerequisites", .apiVersion=VK_API_VERSION_1_1};
    VkInstanceCreateInfo ci = {.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pApplicationInfo=&app};
    OK(vkCreateInstance(&ci, NULL, &instance));
    const char *expected_layer=getenv("MAVK_EXPECT_LAYER_PATH");
    FILE *maps=fopen("/proc/self/maps","r");int mapped=0;char line[4096];
    if(maps){while(fgets(line,sizeof(line),maps))if(expected_layer&&strstr(line,expected_layer))mapped=1;fclose(maps);}
    if(!mapped)return fail("expected Mavk layer mapping",-1);
    printf("AHB_LAYER_MAPPED %s\n",expected_layer);
#define LOAD_AHB(n) vk##n=(PFN_vk##n)get(instance,"vk" #n); if (!vk##n) return fail("resolve " #n,-1);
    FUNCTIONS(LOAD_AHB)
    uint32_t count=0;
    OK(vkEnumeratePhysicalDevices(instance,&count,NULL));
    if (!count || count>16) return fail("physical count",-1);
    VkPhysicalDevice devices[16];
    OK(vkEnumeratePhysicalDevices(instance,&count,devices));
    for (uint32_t i=0;i<count;i++) {
        VkPhysicalDeviceProperties props; vkGetPhysicalDeviceProperties(devices[i],&props);
        if (props.vendorID==0x13b5 && props.deviceType!=VK_PHYSICAL_DEVICE_TYPE_CPU) {
            physical=devices[i]; printf("AHB_DEVICE name=%s api=%u\n",props.deviceName,props.apiVersion); break;
        }
    }
    if (!physical) return fail("Mali hardware required",-1);
    OK(vkEnumerateDeviceExtensionProperties(physical,NULL,&count,NULL));
    if (count>1024) return fail("extension bound",-1);
    VkExtensionProperties extensions[1024];
    OK(vkEnumerateDeviceExtensionProperties(physical,NULL,&count,extensions));
    int ahb=0,foreign=0,syncfd=0;
    for (uint32_t i=0;i<count;i++) {
        const char *name=extensions[i].extensionName;
        if (!strcmp(name,VK_ANDROID_EXTERNAL_MEMORY_ANDROID_HARDWARE_BUFFER_EXTENSION_NAME)) ahb=1;
        if (!strcmp(name,VK_EXT_QUEUE_FAMILY_FOREIGN_EXTENSION_NAME)) foreign=1;
        if (!strcmp(name,VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME)) syncfd=1;
        if (strstr(name,"external_memory") || strstr(name,"external_semaphore") || strstr(name,"foreign") || strstr(name,"drm_format"))
            printf("AHB_CAP %s\n",name);
    }
    void *common=dlopen("libhybris-common.so.1",RTLD_NOW|RTLD_LOCAL);
    void *(*open_android)(const char*,int)=common?dlsym(common,"hybris_dlopen"):NULL;
    void *(*symbol_android)(void*,const char*)=common?dlsym(common,"hybris_dlsym"):NULL;
    if (!open_android || !symbol_android) { printf("NO_GO stage=hybris-linker-api\n"); return 3; }
    void *window=open_android("libnativewindow.so",RTLD_NOW);
    if (!window) { printf("NO_GO stage=bionic-libnativewindow\n"); return 3; }
    int (*allocate)(const HardwareBufferDesc*,struct AHardwareBuffer**)=symbol_android(window,"AHardwareBuffer_allocate");
    void (*describe)(const struct AHardwareBuffer*,HardwareBufferDesc*)=symbol_android(window,"AHardwareBuffer_describe");
    void (*release)(struct AHardwareBuffer*)=symbol_android(window,"AHardwareBuffer_release");
    if (!allocate || !describe || !release) { printf("NO_GO stage=AHB-public-functions\n"); return 3; }
    HardwareBufferDesc request={.width=64,.height=64,.layers=1,.format=1,.usage=(1ull<<8)|(1ull<<9)}, actual={0};
    if(getenv("MAVK_AHB_FULL_SIZE")){request.width=1920;request.height=1200;}
    if(getenv("MAVK_AHB_WINDOW_SIZE")){request.width=1280;request.height=800;}
    if(getenv("MAVK_AHB_COMPOSER"))request.usage|=1ull<<11;
    struct AHardwareBuffer *buffer=NULL;
    uint64_t allocate_start=ahb_now_ns();
    int allocation=allocate(&request,&buffer);
    printf("AHB_ALLOC result=%d bufferPresent=%u allocationMs=%.3f\n",allocation,buffer!=NULL,(double)(ahb_now_ns()-allocate_start)/1000000.0);
    if (allocation || !buffer) return 3;
    describe(buffer,&actual);
    printf("AHB_DESC width=%u height=%u layers=%u format=%u stride=%u usage=%llu\n",actual.width,actual.height,actual.layers,actual.format,actual.stride,(unsigned long long)actual.usage);
    if (!ahb) { release(buffer);vkDestroyInstance(instance,NULL);printf("NO_GO stage=AHB-device-extension-not-exposed allocationWorks=1\n");return 3; }
    uint32_t familiesCount=0;vkGetPhysicalDeviceQueueFamilyProperties(physical,&familiesCount,NULL);
    if (!familiesCount || familiesCount>64) return 3;
    VkQueueFamilyProperties families[64];vkGetPhysicalDeviceQueueFamilyProperties(physical,&familiesCount,families);
    uint32_t family=UINT32_MAX;for(uint32_t i=0;i<familiesCount;i++)if(families[i].queueFlags&VK_QUEUE_GRAPHICS_BIT){family=i;break;}
    if(family==UINT32_MAX)return 3;
    float priority=1;VkDeviceQueueCreateInfo queue={.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,.queueFamilyIndex=family,.queueCount=1,.pQueuePriorities=&priority};
    if(!foreign){release(buffer);vkDestroyInstance(instance,NULL);printf("NO_GO stage=foreign-queue-family-extension\n");return 3;}
    const char *pool_frames=getenv("MAVK_AHB_POOL_FRAMES");
    const char *async_mode=getenv("MAVK_AHB_ASYNC_FENCE");int asynchronous=pool_frames||(async_mode&&!strcmp(async_mode,"1"));
#ifdef MAVK_AHB_WAYLAND
    asynchronous=1;
#endif
    if(asynchronous&&!syncfd)return fail("external semaphore FD extension",-1);
    const char *enabled[]={VK_ANDROID_EXTERNAL_MEMORY_ANDROID_HARDWARE_BUFFER_EXTENSION_NAME,VK_EXT_QUEUE_FAMILY_FOREIGN_EXTENSION_NAME,VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME};
    VkDeviceCreateInfo di={.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,.queueCreateInfoCount=1,.pQueueCreateInfos=&queue,.enabledExtensionCount=asynchronous?3:2,.ppEnabledExtensionNames=enabled};
    OK(vkCreateDevice(physical,&di,NULL,&device));
#ifdef MAVK_AHB_WAYLAND
    int wayland_result=ahb_wayland_run(window,symbol_android,buffer,&actual,family);
    vkDestroyDevice(device,NULL);release(buffer);vkDestroyInstance(instance,NULL);
    return wayland_result;
#endif
    if(pool_frames){
        char *end=NULL;unsigned long frames=strtoul(pool_frames,&end,10);
        int result=(!end||*end||frames<6||frames>240)?3:ahb_pool_producer(window,symbol_android,buffer,&actual,family,(unsigned)frames);
        vkDestroyDevice(device,NULL);release(buffer);vkDestroyInstance(instance,NULL);
        return result;
    }
    PFN_vkGetDeviceProcAddr gdpa=(PFN_vkGetDeviceProcAddr)get(instance,"vkGetDeviceProcAddr");
    PFN_vkGetAndroidHardwareBufferPropertiesANDROID properties=(PFN_vkGetAndroidHardwareBufferPropertiesANDROID)gdpa(device,"vkGetAndroidHardwareBufferPropertiesANDROID");
    if(!properties){printf("NO_GO stage=loader-AHB-device-dispatch\n");return 3;}
    VkAndroidHardwareBufferFormatPropertiesANDROID format={.sType=VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_FORMAT_PROPERTIES_ANDROID};
    VkAndroidHardwareBufferPropertiesANDROID props={.sType=VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_PROPERTIES_ANDROID,.pNext=&format};
    OK(properties(device,buffer,&props));
    printf("AHB_PROPERTIES_PASS bytes=%llu memoryTypes=%x format=%u features=%x externalFormat=%llu\n",(unsigned long long)props.allocationSize,props.memoryTypeBits,format.format,format.formatFeatures,(unsigned long long)format.externalFormat);
    int result=asynchronous?ahb_async_submit(buffer,&actual,&props,format.format,family):ahb_image_check(buffer,&actual,&props,format.format,family,0,91,VK_NULL_HANDLE);
    if(!result)result=ahb_send_check(window,symbol_android,buffer,&actual);
    if(asynchronous)ahb_async_cleanup();
    vkDestroyDevice(device,NULL);release(buffer);vkDestroyInstance(instance,NULL);
    return result;
}
