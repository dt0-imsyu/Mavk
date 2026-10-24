#include "ahb_image.h"

/* Caller must retire GPU/display uses before destruction. Import retains the AHB. */
void mavk_ahb_image_destroy(const MavkAhbImageApi *api, VkDevice device,
                                         MavkAhbImage *image) {
    if (image->image) api->destroy_image(device, image->image, NULL);
    if (image->memory) api->free_memory(device, image->memory, NULL);
    image->image = VK_NULL_HANDLE;
    image->memory = VK_NULL_HANDLE;
}

/* Description must come from AHardwareBuffer_describe, not caller guesses. */
VkResult mavk_ahb_image_import_rgba8(
    const MavkAhbImageApi *api, VkPhysicalDevice physical, VkDevice device,
    struct AHardwareBuffer *buffer, uint32_t width, uint32_t height,
    uint32_t layers, uint32_t ndk_format, uint64_t ndk_usage,
    VkImageUsageFlags usage, MavkAhbImage *out) {
    if (!out || out->image || out->memory || !api || !api->properties ||
        !api->image_properties || (!api->memory_properties && !api->memory_properties2) || !api->create_image ||
        !api->destroy_image || !api->allocate_memory || !api->free_memory ||
        !api->bind_memory || !buffer || !width || !height || layers != 1 ||
        ndk_format != 1 || !usage)
        return VK_ERROR_INITIALIZATION_FAILED;

    const VkImageUsageFlags supported = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if ((usage & ~supported) || (ndk_usage & ((1ull << 14) | (1ull << 26))) ||
        ((usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) && !(ndk_usage & (1ull << 9))) ||
        ((usage & VK_IMAGE_USAGE_SAMPLED_BIT) && !(ndk_usage & (1ull << 8))) ||
        !(ndk_usage & ((1ull << 8) | (1ull << 9))))
        return VK_ERROR_FORMAT_NOT_SUPPORTED;

    VkPhysicalDeviceExternalImageFormatInfo external_query = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO,
        .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID};
    VkPhysicalDeviceImageFormatInfo2 query = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
        .pNext = &external_query, .format = VK_FORMAT_R8G8B8A8_UNORM,
        .type = VK_IMAGE_TYPE_2D, .tiling = VK_IMAGE_TILING_OPTIMAL, .usage = usage};
    VkExternalImageFormatProperties external_caps = {
        .sType = VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES};
    VkImageFormatProperties2 caps = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2, .pNext = &external_caps};
    VkResult result = api->image_properties(physical, &query, &caps);
    if (result != VK_SUCCESS) return result;
    if (!(external_caps.externalMemoryProperties.externalMemoryFeatures &
          VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT) ||
        width > caps.imageFormatProperties.maxExtent.width ||
        height > caps.imageFormatProperties.maxExtent.height ||
        !(caps.imageFormatProperties.sampleCounts & VK_SAMPLE_COUNT_1_BIT))
        return VK_ERROR_FORMAT_NOT_SUPPORTED;

    VkAndroidHardwareBufferFormatPropertiesANDROID format = {
        .sType = VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_FORMAT_PROPERTIES_ANDROID};
    VkAndroidHardwareBufferPropertiesANDROID properties = {
        .sType = VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_PROPERTIES_ANDROID, .pNext = &format};
    result = api->properties(device, buffer, &properties);
    if (result != VK_SUCCESS) return result;
    if (format.format != VK_FORMAT_R8G8B8A8_UNORM || !properties.allocationSize)
        return VK_ERROR_FORMAT_NOT_SUPPORTED;
    VkFormatFeatureFlags required = 0;
    if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) required |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
    if (usage & VK_IMAGE_USAGE_SAMPLED_BIT) required |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
    if (usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) required |= VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
    if (usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) required |= VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
    if ((format.formatFeatures & required) != required)
        return VK_ERROR_FORMAT_NOT_SUPPORTED;

    VkPhysicalDeviceMemoryProperties memories = {0};
    if (api->memory_properties) api->memory_properties(physical, &memories);
    else {
        VkPhysicalDeviceMemoryProperties2 properties2 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2};
        api->memory_properties2(physical, &properties2);
        memories = properties2.memoryProperties;
    }
    uint32_t memory_type = 0;
    while (memory_type < memories.memoryTypeCount && memory_type < 32 &&
           !(properties.memoryTypeBits & (1u << memory_type))) ++memory_type;
    if (memory_type >= memories.memoryTypeCount || memory_type >= 32)
        return VK_ERROR_INVALID_EXTERNAL_HANDLE;

    VkExternalMemoryImageCreateInfo external = {
        .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
        .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID};
    VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, .pNext = &external,
        .imageType = VK_IMAGE_TYPE_2D, .format = format.format,
        .extent = {width, height, 1}, .mipLevels = 1, .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT, .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage, .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};
    MavkAhbImage created = {0};
    result = api->create_image(device, &image_info, NULL, &created.image);
    if (result != VK_SUCCESS) return result;
    VkMemoryDedicatedAllocateInfo dedicated = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO, .image = created.image};
    VkImportAndroidHardwareBufferInfoANDROID import = {
        .sType = VK_STRUCTURE_TYPE_IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID,
        .pNext = &dedicated, .buffer = buffer};
    VkMemoryAllocateInfo allocation = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .pNext = &import,
        .allocationSize = properties.allocationSize, .memoryTypeIndex = memory_type};
    result = api->allocate_memory(device, &allocation, NULL, &created.memory);
    if (result == VK_SUCCESS)
        result = api->bind_memory(device, created.image, created.memory, 0);
    if (result != VK_SUCCESS) {
        mavk_ahb_image_destroy(api, device, &created);
        return result;
    }
    *out = created;
    return VK_SUCCESS;
}
