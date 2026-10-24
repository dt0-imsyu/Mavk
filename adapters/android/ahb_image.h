#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>

typedef struct {
    PFN_vkGetAndroidHardwareBufferPropertiesANDROID properties;
    PFN_vkGetPhysicalDeviceImageFormatProperties2 image_properties;
    PFN_vkGetPhysicalDeviceMemoryProperties memory_properties;
    PFN_vkCreateImage create_image;
    PFN_vkDestroyImage destroy_image;
    PFN_vkAllocateMemory allocate_memory;
    PFN_vkFreeMemory free_memory;
    PFN_vkBindImageMemory bind_memory;
    PFN_vkGetPhysicalDeviceMemoryProperties2 memory_properties2;
} MavkAhbImageApi;

typedef struct {
    VkImage image;
    VkDeviceMemory memory;
} MavkAhbImage;

#ifdef __cplusplus
extern "C" {
#endif

void mavk_ahb_image_destroy(const MavkAhbImageApi *api, VkDevice device, MavkAhbImage *image);
VkResult mavk_ahb_image_import_rgba8(
    const MavkAhbImageApi *api, VkPhysicalDevice physical, VkDevice device,
    struct AHardwareBuffer *buffer, uint32_t width, uint32_t height,
    uint32_t layers, uint32_t ndk_format, uint64_t ndk_usage,
    VkImageUsageFlags usage, MavkAhbImage *out);

#ifdef __cplusplus
}
#endif
