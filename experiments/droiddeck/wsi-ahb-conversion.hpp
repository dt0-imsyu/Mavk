#pragma once
#include <cstdlib>
#include <cstring>

inline bool mavk_ahb_bgra_enabled()
{
   const char *value=std::getenv("MAVK_WSI_AHB_BGRA_BLIT");
   return value && !std::strcmp(value,"1");
}

inline bool mavk_ahb_bgra_supported(layer::instance_private_data &instance, VkPhysicalDevice physical)
{
   auto query=instance.disp.get_fn<PFN_vkGetPhysicalDeviceFormatProperties2>("vkGetPhysicalDeviceFormatProperties2KHR");
   if (!query) query=instance.disp.get_fn<PFN_vkGetPhysicalDeviceFormatProperties2>("vkGetPhysicalDeviceFormatProperties2");
   if (!query) return false;
   VkFormatProperties2 source{}, destination{};
   source.sType=destination.sType=VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
   (*query)(physical,VK_FORMAT_B8G8R8A8_UNORM,&source);
   (*query)(physical,VK_FORMAT_R8G8B8A8_UNORM,&destination);
   return (source.formatProperties.optimalTilingFeatures & (VK_FORMAT_FEATURE_BLIT_SRC_BIT|VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)) ==
      (VK_FORMAT_FEATURE_BLIT_SRC_BIT|VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) &&
      (destination.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);
}
