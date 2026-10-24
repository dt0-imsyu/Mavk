#pragma once
#include <string.h>

static inline void mavk_record_bgra_rgba_blit(PFN_vkCmdBlitImage blit,VkCommandBuffer command,
                                            VkImage source,VkImage destination,VkExtent2D extent) {
    VkImageBlit region;
    memset(&region,0,sizeof(region));
    region.srcSubresource.aspectMask=region.dstSubresource.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
    region.srcSubresource.layerCount=region.dstSubresource.layerCount=1;
    region.srcOffsets[1].x=region.dstOffsets[1].x=(int32_t)extent.width;
    region.srcOffsets[1].y=region.dstOffsets[1].y=(int32_t)extent.height;
    region.srcOffsets[1].z=region.dstOffsets[1].z=1;
    blit(command,source,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,destination,
         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&region,VK_FILTER_NEAREST);
}
