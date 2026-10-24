#pragma once
#include "ahb-blit.h"

static uint32_t bgra_pattern_rgba(unsigned x,unsigned y,unsigned frame) {
    uint32_t v=x*0x9e3779b9u ^ y*0x85ebca6bu ^ frame*0xc2b2ae35u;
    v^=v>>16;v*=0x7feb352du;v^=v>>15;
    return (v&0x00ffffffu)|0xff000000u;
}

static int native_check_bgra_blit(int copy_negative,int pattern) {
    const uint32_t width=1920,height=1200;
    uint32_t family;
    native_async_mode=1;
    if(native_init_gpu(&family)||pool_dispatch())return 3;
    VkFormatProperties src_caps,dst_caps;
    vkGetPhysicalDeviceFormatProperties(physical,VK_FORMAT_B8G8R8A8_UNORM,&src_caps);
    vkGetPhysicalDeviceFormatProperties(physical,VK_FORMAT_R8G8B8A8_UNORM,&dst_caps);
    if(!(src_caps.optimalTilingFeatures&VK_FORMAT_FEATURE_BLIT_SRC_BIT)||
       !(dst_caps.optimalTilingFeatures&VK_FORMAT_FEATURE_BLIT_DST_BIT))return fail("BGRA blit native capabilities",-1);
    AHardwareBuffer_Desc request={.width=width,.height=height,.layers=1,
        .format=AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
        .usage=AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE|AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT|AHARDWAREBUFFER_USAGE_COMPOSER_OVERLAY};
    AHardwareBuffer *ahb=NULL;
    if(AHardwareBuffer_allocate(&request,&ahb)||!ahb)return fail("BGRA oracle AHB allocation",-1);
    AHardwareBuffer_Desc actual;AHardwareBuffer_describe(ahb,&actual);
    AhbPoolSlot slot={0};
    if(pool_import(&slot,ahb,&actual,family,1))return 3;
    VkBuffer upload=VK_NULL_HANDLE;VkDeviceMemory upload_memory=VK_NULL_HANDLE;uint32_t *upload_pixels=NULL;
    if(pattern) {
        VkBufferCreateInfo buffer={.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,.size=(VkDeviceSize)width*height*4,
            .usage=VK_BUFFER_USAGE_TRANSFER_SRC_BIT};
        OK(vkCreateBuffer(device,&buffer,NULL,&upload));
        VkMemoryRequirements req;vkGetBufferMemoryRequirements(device,upload,&req);
        uint32_t memory=memory_type(req.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        if(memory==UINT32_MAX)return fail("BGRA pattern upload memory",-1);
        VkMemoryAllocateInfo allocation={.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,.allocationSize=req.size,.memoryTypeIndex=memory};
        OK(vkAllocateMemory(device,&allocation,NULL,&upload_memory));OK(vkBindBufferMemory(device,upload,upload_memory,0));
        OK(vkMapMemory(device,upload_memory,0,VK_WHOLE_SIZE,0,(void**)&upload_pixels));
    }
    VkImage source=VK_NULL_HANDLE;VkDeviceMemory source_memory=VK_NULL_HANDLE;
    VkImageCreateInfo ci={.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,.imageType=VK_IMAGE_TYPE_2D,
        .format=VK_FORMAT_B8G8R8A8_UNORM,.extent={width,height,1},.mipLevels=1,.arrayLayers=1,
        .samples=VK_SAMPLE_COUNT_1_BIT,.tiling=VK_IMAGE_TILING_OPTIMAL,
        .usage=VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT};
    OK(vkCreateImage(device,&ci,NULL,&source));
    VkMemoryRequirements requirements;vkGetImageMemoryRequirements(device,source,&requirements);
    uint32_t type=memory_type(requirements.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if(type==UINT32_MAX)return fail("BGRA oracle device memory",-1);
    VkMemoryAllocateInfo ai={.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,.allocationSize=requirements.size,.memoryTypeIndex=type};
    OK(vkAllocateMemory(device,&ai,NULL,&source_memory));OK(vkBindImageMemory(device,source,source_memory,0));
    PFN_vkCmdBlitImage blit=(PFN_vkCmdBlitImage)vkGetDeviceProcAddr(device,"vkCmdBlitImage");
    if(!blit)return fail("BGRA oracle blit dispatch",-1);
    VkQueue queue;vkGetDeviceQueue(device,family,0,&queue);
    uint32_t queue_count=0;vkGetPhysicalDeviceQueueFamilyProperties(physical,&queue_count,NULL);
    VkQueueFamilyProperties queue_properties[64];
    if(queue_count>64||family>=queue_count)return fail("BGRA oracle timestamp queue",-1);
    vkGetPhysicalDeviceQueueFamilyProperties(physical,&queue_count,queue_properties);
    VkPhysicalDeviceProperties gpu_properties;vkGetPhysicalDeviceProperties(physical,&gpu_properties);
    VkQueryPool timestamps=VK_NULL_HANDLE;
    uint32_t timestamp_bits=queue_properties[family].timestampValidBits;
    if(timestamp_bits) {
        VkQueryPoolCreateInfo queries={.sType=VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,.queryType=VK_QUERY_TYPE_TIMESTAMP,.queryCount=2};
        OK(vkCreateQueryPool(device,&queries,NULL,&timestamps));
    }
    double blit_total_ms=0,blit_max_ms=0;unsigned timed_frames=0;
    const uint32_t red[]={0,17,91,153,224,255};
    int result=0;uint64_t begin_ns=pool_now();
    for(unsigned frame=0;frame<sizeof(red)/sizeof(red[0]);frame++) {
        if(pattern) {
            for(unsigned y=0;y<height;y++)for(unsigned x=0;x<width;x++) {
                uint32_t rgba=bgra_pattern_rgba(x,y,frame);
                upload_pixels[(size_t)y*width+x]=(rgba&0xff00ff00u)|((rgba&255u)<<16)|((rgba>>16)&255u);
            }
            VkMappedMemoryRange written={.sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,.memory=upload_memory,.size=VK_WHOLE_SIZE};
            OK(vkFlushMappedMemoryRanges(device,1,&written));
        }
        OK(pool_api.reset_pool(device,slot.pool,0));OK(pool_api.reset_fences(device,1,&slot.fence));
        VkCommandBufferBeginInfo begin={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
        OK(vkBeginCommandBuffer(slot.cmd,&begin));
        if(timestamps)vkCmdResetQueryPool(slot.cmd,timestamps,0,2);
        VkImageMemoryBarrier prepare[2]={0};
        for(unsigned i=0;i<2;i++) {
            prepare[i].sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            prepare[i].srcAccessMask=frame?VK_ACCESS_TRANSFER_READ_BIT:0;
            prepare[i].dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
            prepare[i].oldLayout=frame?VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:VK_IMAGE_LAYOUT_UNDEFINED;
            prepare[i].newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            prepare[i].srcQueueFamilyIndex=prepare[i].dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
            prepare[i].subresourceRange=(VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        }
        prepare[0].image=source;prepare[1].image=slot.image;
        vkCmdPipelineBarrier(slot.cmd,frame?VK_PIPELINE_STAGE_TRANSFER_BIT:VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,NULL,0,NULL,2,prepare);
        VkClearColorValue color={{red[frame]/255.0f,34.0f/255.0f,51.0f/255.0f,1}};
        if(pattern) {
            VkBufferMemoryBarrier uploaded={.sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,.srcAccessMask=VK_ACCESS_HOST_WRITE_BIT,
                .dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=upload,.size=VK_WHOLE_SIZE};
            vkCmdPipelineBarrier(slot.cmd,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,NULL,1,&uploaded,0,NULL);
            VkBufferImageCopy pixels={.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1},.imageExtent={width,height,1}};
            vkCmdCopyBufferToImage(slot.cmd,upload,source,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&pixels);
        } else pool_api.clear(slot.cmd,source,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&color,1,&prepare[0].subresourceRange);
        VkImageMemoryBarrier ready=prepare[0];ready.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;ready.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
        ready.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;ready.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        vkCmdPipelineBarrier(slot.cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,NULL,0,NULL,1,&ready);
        if(timestamps)vkCmdWriteTimestamp(slot.cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,timestamps,0);
        if(copy_negative) {
            VkImageCopy raw={.srcSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1},
                .dstSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1},.extent={width,height,1}};
            vkCmdCopyImage(slot.cmd,source,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,slot.image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&raw);
        } else mavk_record_bgra_rgba_blit(blit,slot.cmd,source,slot.image,(VkExtent2D){width,height});
        if(timestamps)vkCmdWriteTimestamp(slot.cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,timestamps,1);
        ready.image=slot.image;
        vkCmdPipelineBarrier(slot.cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,NULL,0,NULL,1,&ready);
        VkBufferImageCopy copy={.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1},.imageExtent={width,height,1}};
        vkCmdCopyImageToBuffer(slot.cmd,slot.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,slot.read_buffer,1,&copy);
        VkBufferMemoryBarrier host={.sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask=VK_ACCESS_HOST_READ_BIT,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=slot.read_buffer,.size=VK_WHOLE_SIZE};
        vkCmdPipelineBarrier(slot.cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,NULL,1,&host,0,NULL);
        OK(vkEndCommandBuffer(slot.cmd));
        VkSubmitInfo submit={.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,.commandBufferCount=1,.pCommandBuffers=&slot.cmd};
        OK(vkQueueSubmit(queue,1,&submit,slot.fence));slot.pending=1;slot.last_red=red[frame];slot.submits++;
        if(pattern)slot.consumer=0;
        result=pool_retire(&slot);
        if(pattern&&!result) {
            VkMappedMemoryRange read={.sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,.memory=slot.read_memory,.size=VK_WHOLE_SIZE};
            OK(vkInvalidateMappedMemoryRanges(device,1,&read));
            const uint32_t *rgba=(const uint32_t*)slot.mapped;
            for(unsigned y=0;y<height&&!result;y++)for(unsigned x=0;x<width;x++) {
                uint32_t expected=bgra_pattern_rgba(x,y,frame);
                if(rgba[(size_t)y*width+x]!=expected) {
                    printf("BGRA_PATTERN_FAIL frame=%u x=%u y=%u got=%08x expected=%08x\n",frame,x,y,rgba[(size_t)y*width+x],expected);
                    result=3;break;
                }
            }
            if(!result)pool_pixels+=(uint64_t)width*height;
        }
        if(timestamps) {
            uint64_t values[2];
            OK(vkGetQueryPoolResults(device,timestamps,0,2,sizeof(values),values,sizeof(uint64_t),VK_QUERY_RESULT_64_BIT|VK_QUERY_RESULT_WAIT_BIT));
            uint64_t delta=values[1]-values[0];
            if(timestamp_bits<64)delta&=(1ull<<timestamp_bits)-1;
            double ms=delta*(double)gpu_properties.limits.timestampPeriod/1000000.0;
            blit_total_ms+=ms;if(ms>blit_max_ms)blit_max_ms=ms;timed_frames++;
        }
        printf("BGRA_AHB_PIXEL_%s frame=%u red=%u pixels=%u pattern=%d\n",result?"FAIL":"PASS",frame,red[frame],width*height,pattern);
        if(result)break;
    }
    printf("BGRA_AHB_ORACLE result=%d copyNegative=%d pattern=%d checkedPixels=%llu elapsedMs=%.3f cpuReadbackForOracle=1 visibleFpsUnmeasured=1\n",
        result,copy_negative,pattern,(unsigned long long)pool_pixels,(pool_now()-begin_ns)/1000000.0);
    printf("BGRA_AHB_GPU_TIMING samples=%u meanMs=%.3f maxMs=%.3f isolatedBlit=1 warmup=0 notFrameLatency=1\n",
        timed_frames,timed_frames?blit_total_ms/timed_frames:0,blit_max_ms);
    OK(vkDeviceWaitIdle(device));
    if(timestamps)vkDestroyQueryPool(device,timestamps,NULL);
    pool_destroy(&slot);vkDestroyImage(device,source,NULL);vkFreeMemory(device,source_memory,NULL);
    if(upload_pixels)vkUnmapMemory(device,upload_memory);
    if(upload)vkDestroyBuffer(device,upload,NULL);
    if(upload_memory)vkFreeMemory(device,upload_memory,NULL);
    AHardwareBuffer_release(ahb);vkDestroyDevice(device,NULL);vkDestroyInstance(instance,NULL);
    device=VK_NULL_HANDLE;instance=VK_NULL_HANDLE;
    return result;
}
