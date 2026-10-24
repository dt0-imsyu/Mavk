#pragma once

#include "wsi/swapchain_image.hpp"
#include "ahb_image.h"
#include <dlfcn.h>
#include <cstdio>
#include "wsi-ahb-sync.hpp"
#include "wsi-ahb-conversion.hpp"
#include "wsi/image_backing_memory_device.hpp"

namespace wsi {

struct ahb_description {
   uint32_t width, height, layers, format;
   uint64_t usage;
   uint32_t stride, reserved0;
   uint64_t reserved1;
};
static_assert(sizeof(ahb_description) == 40);

struct ahb_native_api {
   int (*allocate)(const ahb_description *, AHardwareBuffer **){};
   void (*describe)(const AHardwareBuffer *, ahb_description *){};
   void (*release)(AHardwareBuffer *){};
   int (*send_handle)(const AHardwareBuffer *, int){};

   static const ahb_native_api &get()
   {
      static const ahb_native_api api = [] {
         ahb_native_api result;
         void *common = dlopen("libhybris-common.so.1", RTLD_NOW | RTLD_LOCAL);
         if (!common) return result;
         auto open = reinterpret_cast<void *(*)(const char *, int)>(dlsym(common, "hybris_dlopen"));
         auto symbol = reinterpret_cast<void *(*)(void *, const char *)>(dlsym(common, "hybris_dlsym"));
         if (!open || !symbol) return result;
         void *window = open("libnativewindow.so", RTLD_NOW);
         if (!window) return result;
         result.allocate = reinterpret_cast<decltype(result.allocate)>(symbol(window, "AHardwareBuffer_allocate"));
         result.describe = reinterpret_cast<decltype(result.describe)>(symbol(window, "AHardwareBuffer_describe"));
         result.release = reinterpret_cast<decltype(result.release)>(symbol(window, "AHardwareBuffer_release"));
         result.send_handle = reinterpret_cast<decltype(result.send_handle)>(symbol(window, "AHardwareBuffer_sendHandleToUnixSocket"));
         return result;
      }();
      return api;
   }
};

class image_backing_memory_ahb final : public image_backing_memory {
public:
   image_backing_memory_ahb(layer::device_private_data &data, VkExtent2D extent, VkImageUsageFlags usage, bool sync,
                           VkFormat format=VK_FORMAT_R8G8B8A8_UNORM)
      : m_data(data), m_extent(extent), m_usage(usage), m_format(format),
        m_source_memory(data,util::allocator{data.get_allocator(),VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}),
        m_sync_enabled(sync), m_sync(data) {}

   ~image_backing_memory_ahb() override
   {
      // swapchain_image destroys its VkImage before releasing backing memory.
      if (m_image.image && (m_source_image || !m_handed_off)) m_api.destroy_image(m_data.device,m_image.image,nullptr);
      if (m_source_image && !m_handed_off) m_api.destroy_image(m_data.device,m_source_image,nullptr);
      if (m_image.memory) m_api.free_memory(m_data.device, m_image.memory, nullptr);
      if (m_buffer) ahb_native_api::get().release(m_buffer);
   }

   VkResult create_bound_image(VkImage *image) override
   {
      if (m_buffer || m_image.image || !image) return VK_ERROR_INITIALIZATION_FAILED;
      const auto &native = ahb_native_api::get();
      if (!native.allocate || !native.describe || !native.release || !native.send_handle)
      {
         std::fprintf(stderr, "MAVK_WSI_AHB_FAIL stage=native-functions\n");
         return VK_ERROR_INITIALIZATION_FAILED;
      }
      auto gdpa = m_data.disp.get_fn<PFN_vkGetDeviceProcAddr>("vkGetDeviceProcAddr");
      if (!gdpa) { std::fprintf(stderr, "MAVK_WSI_AHB_FAIL stage=gdpa\n"); return VK_ERROR_INITIALIZATION_FAILED; }
#define AHB_DEVICE(field, name) m_api.field = reinterpret_cast<PFN_vk##name>((*gdpa)(m_data.device, "vk" #name))
      AHB_DEVICE(properties, GetAndroidHardwareBufferPropertiesANDROID);
      AHB_DEVICE(create_image, CreateImage);
      AHB_DEVICE(destroy_image, DestroyImage);
      AHB_DEVICE(allocate_memory, AllocateMemory);
      AHB_DEVICE(free_memory, FreeMemory);
      AHB_DEVICE(bind_memory, BindImageMemory);
#undef AHB_DEVICE
      m_api.image_properties = m_data.instance_data.disp.get_fn<PFN_vkGetPhysicalDeviceImageFormatProperties2>(
         "vkGetPhysicalDeviceImageFormatProperties2KHR").value_or(nullptr);
      m_api.memory_properties = m_data.instance_data.disp.get_fn<PFN_vkGetPhysicalDeviceMemoryProperties>(
         "vkGetPhysicalDeviceMemoryProperties").value_or(nullptr);
      m_api.memory_properties2 = m_data.instance_data.disp.get_fn<PFN_vkGetPhysicalDeviceMemoryProperties2>(
         "vkGetPhysicalDeviceMemoryProperties2KHR").value_or(nullptr);
      ahb_description request{m_extent.width, m_extent.height, 1, 1, 2816, 0, 0, 0};
      if (native.allocate(&request, &m_buffer) || !m_buffer) return VK_ERROR_OUT_OF_DEVICE_MEMORY;
      native.describe(m_buffer, &m_description);
      std::fprintf(stderr, "MAVK_WSI_AHB_ALLOC req=%ux%u got=%ux%u fmt=%u layers=%u usage=%llx stride=%u\n",
         m_extent.width, m_extent.height, m_description.width, m_description.height,
         m_description.format, m_description.layers, (unsigned long long)m_description.usage, m_description.stride);
      if (m_description.width != m_extent.width || m_description.height != m_extent.height)
         return VK_ERROR_FORMAT_NOT_SUPPORTED;
      // BGRA consumers access the private source image, not the RGBA display buffer.
      const VkImageUsageFlags display_usage = m_format==VK_FORMAT_B8G8R8A8_UNORM
         ? static_cast<VkImageUsageFlags>(VK_IMAGE_USAGE_TRANSFER_DST_BIT) : m_usage;
      std::fprintf(stderr,"MAVK_AHB_IMPORT_USAGE source=%x display=%x format=%u\n",m_usage,display_usage,(unsigned)m_format);
      VkResult result = mavk_ahb_image_import_rgba8(&m_api, m_data.physical_device, m_data.device,
         m_buffer, m_description.width, m_description.height, m_description.layers,
         m_description.format, m_description.usage,
         display_usage, &m_image);
      std::fprintf(stderr, "MAVK_WSI_AHB_IMPORT result=%d extent=%ux%u ahbProperties=%u imageCaps=%u memoryCaps=%u\n",
         result, m_description.width, m_description.height, m_api.properties != nullptr,
         m_api.image_properties != nullptr, m_api.memory_properties != nullptr || m_api.memory_properties2 != nullptr);
      if (result==VK_SUCCESS && m_format==VK_FORMAT_B8G8R8A8_UNORM)
      {
         if (!m_sync_enabled || !mavk_ahb_bgra_enabled() ||
             !mavk_ahb_bgra_supported(m_data.instance_data,m_data.physical_device)) return VK_ERROR_FEATURE_NOT_PRESENT;
         VkAndroidHardwareBufferFormatPropertiesANDROID properties{};
         properties.sType=VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_FORMAT_PROPERTIES_ANDROID;
         VkAndroidHardwareBufferPropertiesANDROID buffer_properties{};
         buffer_properties.sType=VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_PROPERTIES_ANDROID;
         buffer_properties.pNext=&properties;
         TRY(m_api.properties(m_data.device,m_buffer,&buffer_properties));
         if (!(properties.formatFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) return VK_ERROR_FORMAT_NOT_SUPPORTED;
         VkImageCreateInfo create{};
         create.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO; create.imageType=VK_IMAGE_TYPE_2D;
         create.format=m_format; create.extent={m_extent.width,m_extent.height,1};
         create.mipLevels=create.arrayLayers=1; create.samples=VK_SAMPLE_COUNT_1_BIT;
         create.tiling=VK_IMAGE_TILING_OPTIMAL; create.usage=m_usage|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
         create.sharingMode=VK_SHARING_MODE_EXCLUSIVE; create.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
         TRY(m_api.create_image(m_data.device,&create,nullptr,&m_source_image));
         TRY(m_source_memory.allocate_and_bind(m_source_image));
         std::fprintf(stderr,"MAVK_WSI_BGRA_GPU_BLIT source=44 output=37 gpuCopiesPerPresent=1 cpuReadbacks=0\n");
      }
      if (result == VK_SUCCESS && m_sync_enabled)
      {
         result=m_sync.init(m_image.image,m_source_image,m_extent);
      }
      if (result == VK_SUCCESS) { *image = m_source_image ? m_source_image : m_image.image; m_handed_off=true; }
      return result;
   }

   VkResult bind(const VkBindImageMemoryInfo *) override { return VK_ERROR_FEATURE_NOT_PRESENT; }
   uint64_t get_modifier() const override { return UINT64_MAX; }
   AHardwareBuffer *buffer() const { return m_buffer; }
   const ahb_description &description() const { return m_description; }
   bool native_sync() const override { return m_sync_enabled; }
   VkResult native_acquire(VkSemaphore semaphore, VkFence fence, uint64_t timeout) override
   { return m_sync.acquire(semaphore, fence, timeout); }
   VkResult native_present(VkQueue queue, const queue_submit_semaphores &sync, const void *next) override
   { return m_sync.present(queue, sync, next); }
   bool return_dependency(int fd) { return m_sync.return_dependency(fd); }
   int take_present_fd() { return m_sync.take_present_fd(); }

private:
   layer::device_private_data &m_data;
   VkExtent2D m_extent;
   VkImageUsageFlags m_usage;
   VkFormat m_format;
   image_backing_memory_device m_source_memory;
   VkImage m_source_image{};
   bool m_handed_off=false;
   MavkAhbImageApi m_api{};
   MavkAhbImage m_image{};
   AHardwareBuffer *m_buffer{};
   ahb_description m_description{};
   bool m_sync_enabled;
   mavk_ahb_sync m_sync;
};

class ahb_image_backing_creator final : public image_backing_memory_creator {
public:
   ahb_image_backing_creator(layer::device_private_data &data, VkExtent2D extent, VkImageUsageFlags usage, bool sync,
                            VkFormat format=VK_FORMAT_R8G8B8A8_UNORM)
      : m_data(data), m_extent(extent), m_usage(usage), m_sync(sync), m_format(format) {}
   util::unique_ptr<image_backing_memory> create_image_backing_memory(util::allocator &allocator) override
   {
      return allocator.make_unique<image_backing_memory_ahb>(m_data, m_extent, m_usage, m_sync, m_format);
   }
private:
   layer::device_private_data &m_data;
   VkExtent2D m_extent;
   VkImageUsageFlags m_usage;
   bool m_sync;
   VkFormat m_format;
};

}
