#define VK_USE_PLATFORM_ANDROID_KHR
#include <vulkan/vulkan.h>
#include <stdlib.h>

typedef AHardwareBuffer_Desc HardwareBufferDesc;
static VkInstance instance;
static VkPhysicalDevice physical;
static VkDevice device;
static PFN_vkGetInstanceProcAddr get;
static int native_expected_red=91;
static int native_async_mode=0,native_incoming_fd=-2;
static int fail(const char *stage,VkResult result){printf("NO_GO nativeGpuStage=%s VkResult=%d\n",stage,result);return 3;}
#define OK(call) do{VkResult result=(call);if(result!=VK_SUCCESS)return fail(#call,result);}while(0)
static uint32_t memory_type(uint32_t bits,uint32_t required){
    VkPhysicalDeviceMemoryProperties p;vkGetPhysicalDeviceMemoryProperties(physical,&p);
    for(uint32_t i=0;i<p.memoryTypeCount;i++)if((bits&(1u<<i))&&(p.memoryTypes[i].propertyFlags&required)==required)return i;
    return UINT32_MAX;
}
#include "ahb-image-check.h"

static int native_init_gpu(uint32_t *selected_family){
    VkApplicationInfo app={.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,.pApplicationName="Mavk native AHB consumer",.apiVersion=VK_API_VERSION_1_1};
    VkInstanceCreateInfo ci={.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,.pApplicationInfo=&app};
    OK(vkCreateInstance(&ci,NULL,&instance));get=vkGetInstanceProcAddr;
    uint32_t count=0;OK(vkEnumeratePhysicalDevices(instance,&count,NULL));if(!count||count>16)return 3;
    VkPhysicalDevice devices[16];OK(vkEnumeratePhysicalDevices(instance,&count,devices));
    for(uint32_t i=0;i<count;i++){
        VkPhysicalDeviceProperties props;vkGetPhysicalDeviceProperties(devices[i],&props);
        if(props.vendorID==0x13b5&&props.deviceType!=VK_PHYSICAL_DEVICE_TYPE_CPU){physical=devices[i];printf("AHB_NATIVE_GPU device=%s\n",props.deviceName);break;}
    }
    if(!physical)return 3;
    count=0;OK(vkEnumerateDeviceExtensionProperties(physical,NULL,&count,NULL));if(count>1024)return 3;
    VkExtensionProperties extensions[1024];OK(vkEnumerateDeviceExtensionProperties(physical,NULL,&count,extensions));
    int ahb=0,foreign=0,fd_extension=0;
    for(uint32_t i=0;i<count;i++){
        ahb|=!strcmp(extensions[i].extensionName,VK_ANDROID_EXTERNAL_MEMORY_ANDROID_HARDWARE_BUFFER_EXTENSION_NAME);
        foreign|=!strcmp(extensions[i].extensionName,VK_EXT_QUEUE_FAMILY_FOREIGN_EXTENSION_NAME);
        fd_extension|=!strcmp(extensions[i].extensionName,VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME);
    }
    if(!ahb||!foreign)return fail("required native extensions",-1);
    vkGetPhysicalDeviceQueueFamilyProperties(physical,&count,NULL);if(!count||count>64)return 3;
    VkQueueFamilyProperties families[64];vkGetPhysicalDeviceQueueFamilyProperties(physical,&count,families);
    uint32_t family=UINT32_MAX;for(uint32_t i=0;i<count;i++)if(families[i].queueFlags&VK_QUEUE_GRAPHICS_BIT){family=i;break;}
    if(family==UINT32_MAX)return 3;
    float priority=1;VkDeviceQueueCreateInfo q={.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,.queueFamilyIndex=family,.queueCount=1,.pQueuePriorities=&priority};
    if(native_async_mode){
        if(!fd_extension)return fail("native external semaphore FD extension",-1);
        VkPhysicalDeviceExternalSemaphoreInfo query={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO,.handleType=VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT};
        VkExternalSemaphoreProperties support={.sType=VK_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_PROPERTIES};vkGetPhysicalDeviceExternalSemaphoreProperties(physical,&query,&support);
        printf("AHB_SYNCFD_IMPORT_CAP features=%x compatible=%x\n",support.externalSemaphoreFeatures,support.compatibleHandleTypes);
        if(!(support.externalSemaphoreFeatures&VK_EXTERNAL_SEMAPHORE_FEATURE_IMPORTABLE_BIT))return fail("native SYNC_FD not importable",-1);
    }
    const char *enabled[]={VK_ANDROID_EXTERNAL_MEMORY_ANDROID_HARDWARE_BUFFER_EXTENSION_NAME,VK_EXT_QUEUE_FAMILY_FOREIGN_EXTENSION_NAME,VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME};
    VkDeviceCreateInfo di={.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,.queueCreateInfoCount=1,.pQueueCreateInfos=&q,.enabledExtensionCount=native_async_mode?3u:2u,.ppEnabledExtensionNames=enabled};
    OK(vkCreateDevice(physical,&di,NULL,&device));
    *selected_family=family;
    return 0;
}

static int native_verify_ahb(AHardwareBuffer *buffer,const AHardwareBuffer_Desc *desc){
    if(!desc->width||!desc->height||desc->width>2560||desc->height>1600||desc->layers!=1)return 3;
    uint32_t family;int init=native_init_gpu(&family);if(init)return init;
    VkSemaphore acquire_wait=VK_NULL_HANDLE;
    if(native_async_mode){
        if(native_incoming_fd < -1)return fail("native fence packet missing",-1);
        VkSemaphoreCreateInfo si={.sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};OK(vkCreateSemaphore(device,&si,NULL,&acquire_wait));
        PFN_vkImportSemaphoreFdKHR import_fd=(PFN_vkImportSemaphoreFdKHR)vkGetDeviceProcAddr(device,"vkImportSemaphoreFdKHR");if(!import_fd)return fail("native fence import dispatch",-1);
        VkImportSemaphoreFdInfoKHR ii={.sType=VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_FD_INFO_KHR,.semaphore=acquire_wait,.flags=VK_SEMAPHORE_IMPORT_TEMPORARY_BIT,.handleType=VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT,.fd=native_incoming_fd};
        OK(import_fd(device,&ii));native_incoming_fd=-2;
        printf("AHB_SYNCFD_IMPORTED temporary=1 waitStage=TRANSFER\n");
    }
    PFN_vkGetAndroidHardwareBufferPropertiesANDROID properties=(PFN_vkGetAndroidHardwareBufferPropertiesANDROID)vkGetDeviceProcAddr(device,"vkGetAndroidHardwareBufferPropertiesANDROID");
    if(!properties)return 3;
    VkAndroidHardwareBufferFormatPropertiesANDROID format={.sType=VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_FORMAT_PROPERTIES_ANDROID};
    VkAndroidHardwareBufferPropertiesANDROID props={.sType=VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_PROPERTIES_ANDROID,.pNext=&format};
    OK(properties(device,buffer,&props));
    int result=ahb_image_check(buffer,desc,&props,format.format,family,1,native_expected_red,acquire_wait);
    if(acquire_wait)vkDestroySemaphore(device,acquire_wait,NULL);
    vkDestroyDevice(device,NULL);vkDestroyInstance(instance,NULL);device=VK_NULL_HANDLE;instance=VK_NULL_HANDLE;
    if(!result)printf("AHB_NATIVE_GPU_CONTENT_PASS pixels=%u inputRed=91 wroteSource=0\n",desc->width*desc->height);
    return result;
}
