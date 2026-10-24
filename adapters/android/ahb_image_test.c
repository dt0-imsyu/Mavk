#include "ahb_image.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int failure, calls, destroyed, freed;
static VKAPI_ATTR VkResult VKAPI_CALL image_properties(VkPhysicalDevice physical,
    const VkPhysicalDeviceImageFormatInfo2 *info, VkImageFormatProperties2 *out) {
    (void)physical; ++calls;
    assert(info->format == VK_FORMAT_R8G8B8A8_UNORM);
    if (failure == 1) return VK_ERROR_FORMAT_NOT_SUPPORTED;
    out->imageFormatProperties.maxExtent = (VkExtent3D){4096,4096,1};
    out->imageFormatProperties.sampleCounts = VK_SAMPLE_COUNT_1_BIT;
    ((VkExternalImageFormatProperties *)out->pNext)->externalMemoryProperties.externalMemoryFeatures =
        VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT;
    return VK_SUCCESS;
}
static VKAPI_ATTR VkResult VKAPI_CALL properties(VkDevice device,
    const struct AHardwareBuffer *buffer, VkAndroidHardwareBufferPropertiesANDROID *out) {
    (void)device; (void)buffer; ++calls;
    out->allocationSize = 8192; out->memoryTypeBits = failure == 3 ? 0 : 2;
    ((VkAndroidHardwareBufferFormatPropertiesANDROID *)out->pNext)->format =
        failure == 2 ? VK_FORMAT_UNDEFINED : VK_FORMAT_R8G8B8A8_UNORM;
    ((VkAndroidHardwareBufferFormatPropertiesANDROID *)out->pNext)->formatFeatures =
        failure == 7 ? 0 : VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
    return VK_SUCCESS;
}
static VKAPI_ATTR void VKAPI_CALL memory_properties(VkPhysicalDevice physical,
    VkPhysicalDeviceMemoryProperties *out) {
    (void)physical; memset(out,0,sizeof(*out)); out->memoryTypeCount=2; ++calls;
}
static VKAPI_ATTR void VKAPI_CALL memory_properties2(VkPhysicalDevice physical,
    VkPhysicalDeviceMemoryProperties2 *out) {
    memory_properties(physical, &out->memoryProperties);
}
static VKAPI_ATTR VkResult VKAPI_CALL create_image(VkDevice device,
    const VkImageCreateInfo *info, const VkAllocationCallbacks *allocator, VkImage *out) {
    (void)device; (void)allocator; ++calls;
    assert(info->extent.width==1920 && info->extent.height==1200);
    if (failure==4) return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    *out=(VkImage)(uintptr_t)11; return VK_SUCCESS;
}
static VKAPI_ATTR void VKAPI_CALL destroy_image(VkDevice device, VkImage image,
    const VkAllocationCallbacks *allocator) {
    (void)device; (void)allocator; assert(image==(VkImage)(uintptr_t)11); ++destroyed;
}
static VKAPI_ATTR VkResult VKAPI_CALL allocate_memory(VkDevice device,
    const VkMemoryAllocateInfo *info, const VkAllocationCallbacks *allocator, VkDeviceMemory *out) {
    (void)device; (void)allocator; ++calls;
    const VkImportAndroidHardwareBufferInfoANDROID *import=info->pNext;
    const VkMemoryDedicatedAllocateInfo *dedicated=import->pNext;
    assert(dedicated->image==(VkImage)(uintptr_t)11 && info->memoryTypeIndex==1);
    if (failure==5) return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    *out=(VkDeviceMemory)(uintptr_t)22; return VK_SUCCESS;
}
static VKAPI_ATTR void VKAPI_CALL free_memory(VkDevice device, VkDeviceMemory memory,
    const VkAllocationCallbacks *allocator) {
    (void)device; (void)allocator; assert(memory==(VkDeviceMemory)(uintptr_t)22); ++freed;
}
static VKAPI_ATTR VkResult VKAPI_CALL bind_memory(VkDevice device, VkImage image,
    VkDeviceMemory memory, VkDeviceSize offset) {
    (void)device; (void)image; (void)memory; assert(offset==0); ++calls;
    return failure==6 ? VK_ERROR_DEVICE_LOST : VK_SUCCESS;
}
int main(void) {
    MavkAhbImageApi api={properties,image_properties,memory_properties,create_image,
        destroy_image,allocate_memory,free_memory,bind_memory,NULL};
    struct AHardwareBuffer *buffer=(struct AHardwareBuffer *)(uintptr_t)1;
    VkImageUsageFlags usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
    MavkAhbImage out={0};
    assert(mavk_ahb_image_import_rgba8(&api,VK_NULL_HANDLE,VK_NULL_HANDLE,buffer,
        1920,1200,1,1,256,usage,&out)==VK_ERROR_FORMAT_NOT_SUPPORTED && calls==0);
    for(failure=0;failure<=7;++failure) {
        calls=destroyed=freed=0;
        VkResult result=mavk_ahb_image_import_rgba8(&api,VK_NULL_HANDLE,VK_NULL_HANDLE,
            buffer,1920,1200,1,1,768,usage,&out);
        if(!failure) {
            assert(result==VK_SUCCESS && out.image && out.memory);
            int before=calls;
            assert(mavk_ahb_image_import_rgba8(&api,VK_NULL_HANDLE,VK_NULL_HANDLE,
                buffer,1920,1200,1,1,768,usage,&out)==VK_ERROR_INITIALIZATION_FAILED);
            assert(calls==before);
            mavk_ahb_image_destroy(&api,VK_NULL_HANDLE,&out);
            mavk_ahb_image_destroy(&api,VK_NULL_HANDLE,&out);
            assert(destroyed==1 && freed==1);
        } else {
            assert(result!=VK_SUCCESS && !out.image && !out.memory);
            assert(destroyed==(failure==5 || failure==6) && freed==(failure==6));
        }
    }
    failure=0;api.memory_properties=NULL;api.memory_properties2=memory_properties2;
    assert(mavk_ahb_image_import_rgba8(&api,VK_NULL_HANDLE,VK_NULL_HANDLE,
        buffer,1920,1200,1,1,768,usage,&out)==VK_SUCCESS);
    mavk_ahb_image_destroy(&api,VK_NULL_HANDLE,&out);
    puts("AHB_IMAGE_LIFETIME_PASS failureStages=7 success=1 invalidUsage=1 occupiedOutput=1 MOCK_VULKAN_ONLY=1");
    return 0;
}
