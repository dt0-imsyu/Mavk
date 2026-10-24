#pragma once
#include <time.h>
#include <unistd.h>
#include <poll.h>
#include "../../adapters/android/ahb_image.h"

/* Private interop probe, not a WSI implementation or an advertised Vulkan feature. */
typedef struct {
    VkImage image;
    VkDeviceMemory memory,read_memory;
    VkBuffer read_buffer;
    unsigned char *mapped;
    VkCommandPool pool;
    VkCommandBuffer cmd;
    VkSemaphore incoming,outgoing;
    VkFence fence;
    uint32_t width,height,last_red,submits;
    int consumer,pending;
} AhbPoolSlot;
#ifdef MAVK_AHB_WAYLAND
static void (*pool_draw_hook)(VkCommandBuffer,AhbPoolSlot*,unsigned);
#endif
typedef struct {
    PFN_vkCreateSemaphore create_sem;
    PFN_vkDestroySemaphore destroy_sem;
    PFN_vkImportSemaphoreFdKHR import_fd;
    PFN_vkGetSemaphoreFdKHR export_fd;
    PFN_vkResetCommandPool reset_pool;
    PFN_vkResetFences reset_fences;
    PFN_vkGetFenceStatus fence_status;
    PFN_vkCmdClearColorImage clear;
} AhbPoolDispatch;
static AhbPoolDispatch pool_api;
static uint64_t pool_wait_ns,pool_pixels,pool_wait_calls,pool_oracle_ns;
static uint64_t pool_import_pending,pool_export_pending;
static uint64_t pool_now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return(uint64_t)t.tv_sec*1000000000ull+t.tv_nsec;}
static int pool_dispatch(void){
#define POOL_LOAD(field,name) pool_api.field=(PFN_vk##name)get(instance,"vk" #name);if(!pool_api.field)return fail("pool dispatch " #name,-1)
    POOL_LOAD(create_sem,CreateSemaphore);POOL_LOAD(destroy_sem,DestroySemaphore);
    POOL_LOAD(import_fd,ImportSemaphoreFdKHR);POOL_LOAD(export_fd,GetSemaphoreFdKHR);
    POOL_LOAD(reset_pool,ResetCommandPool);POOL_LOAD(reset_fences,ResetFences);
    POOL_LOAD(fence_status,GetFenceStatus);POOL_LOAD(clear,CmdClearColorImage);
#undef POOL_LOAD
    VkPhysicalDeviceExternalSemaphoreInfo query={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO,.handleType=VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT};
    VkExternalSemaphoreProperties support={.sType=VK_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_PROPERTIES};
    PFN_vkGetPhysicalDeviceExternalSemaphoreProperties capabilities=(PFN_vkGetPhysicalDeviceExternalSemaphoreProperties)get(instance,"vkGetPhysicalDeviceExternalSemaphoreProperties");
    if(!capabilities)return fail("pool capabilities dispatch",-1);
    capabilities(physical,&query,&support);
    if((support.externalSemaphoreFeatures&3)!=3)return fail("pool requires SYNC_FD import and export",-1);
    return 0;
}
static int pool_retire(AhbPoolSlot *s){
    if(!s->pending)return 0;
    VkResult status=pool_api.fence_status(device,s->fence);
    if(status==VK_NOT_READY){uint64_t start=pool_now();pool_wait_calls++;OK(vkWaitForFences(device,1,&s->fence,VK_TRUE,5000000000ull));pool_wait_ns+=pool_now()-start;}
    else if(status!=VK_SUCCESS)return fail("pool fence status",status);
    if(s->consumer){
        VkMappedMemoryRange range={.sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,.memory=s->read_memory,.size=VK_WHOLE_SIZE};
        OK(vkInvalidateMappedMemoryRanges(device,1,&range));
        uint64_t start=pool_now();
        uint32_t expected=s->last_red|(34u<<8)|(51u<<16)|(255u<<24);
        size_t pixels=(size_t)s->width*s->height;
        const uint32_t *rgba=(const uint32_t*)s->mapped;
        for(size_t i=0;i<pixels;i++)if(rgba[i]!=expected){
            printf("POOL_PIXEL_FAIL submit=%u pixel=%zu got=%08x expected=%08x\n",s->submits,i,rgba[i],expected);return 3;
        }
        pool_oracle_ns+=pool_now()-start;
        pool_pixels+=(uint64_t)s->width*s->height;
    }
    s->pending=0;
    return 0;
}
static int pool_import(AhbPoolSlot *s,struct AHardwareBuffer *ahb,const HardwareBufferDesc *desc,uint32_t family,int consumer){
    *s=(AhbPoolSlot){.width=desc->width,.height=desc->height,.consumer=consumer};
    if(!s->width||!s->height||s->width>1920||s->height>1200||desc->format!=1||desc->layers!=1)return fail("pool descriptor bounds",-1);
    MavkAhbImageApi api={
        .properties=(PFN_vkGetAndroidHardwareBufferPropertiesANDROID)get(instance,"vkGetAndroidHardwareBufferPropertiesANDROID"),
        .image_properties=(PFN_vkGetPhysicalDeviceImageFormatProperties2)get(instance,"vkGetPhysicalDeviceImageFormatProperties2"),
        .memory_properties=vkGetPhysicalDeviceMemoryProperties,.create_image=vkCreateImage,
        .destroy_image=vkDestroyImage,.allocate_memory=vkAllocateMemory,.free_memory=vkFreeMemory,.bind_memory=vkBindImageMemory};
    MavkAhbImage imported={0};
    OK(mavk_ahb_image_import_rgba8(&api,physical,device,ahb,desc->width,desc->height,
        desc->layers,desc->format,desc->usage,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|
        VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,&imported));
    s->image=imported.image;s->memory=imported.memory;
    uint32_t memory;
    VkMemoryAllocateInfo ai={.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    if(consumer){
        VkBufferCreateInfo bc={.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,.size=(VkDeviceSize)s->width*s->height*4,.usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT};OK(vkCreateBuffer(device,&bc,NULL,&s->read_buffer));
        VkMemoryRequirements req;vkGetBufferMemoryRequirements(device,s->read_buffer,&req);
        memory=memory_type(req.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);if(memory==UINT32_MAX)return 3;
        ai.pNext=NULL;ai.allocationSize=req.size;ai.memoryTypeIndex=memory;
        OK(vkAllocateMemory(device,&ai,NULL,&s->read_memory));OK(vkBindBufferMemory(device,s->read_buffer,s->read_memory,0));OK(vkMapMemory(device,s->read_memory,0,VK_WHOLE_SIZE,0,(void**)&s->mapped));
    }
    VkSemaphoreCreateInfo si={.sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};OK(pool_api.create_sem(device,&si,NULL,&s->incoming));
    VkExportSemaphoreCreateInfo export={.sType=VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO,.handleTypes=VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT};si.pNext=&export;
    OK(pool_api.create_sem(device,&si,NULL,&s->outgoing));
    VkFenceCreateInfo fc={.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};OK(vkCreateFence(device,&fc,NULL,&s->fence));
    VkCommandPoolCreateInfo pc={.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,.queueFamilyIndex=family};OK(vkCreateCommandPool(device,&pc,NULL,&s->pool));
    VkCommandBufferAllocateInfo ca={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,.commandPool=s->pool,.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,.commandBufferCount=1};OK(vkAllocateCommandBuffers(device,&ca,&s->cmd));
    return 0;
}
static int pool_submit(AhbPoolSlot *s,uint32_t family,unsigned red,int *incoming_fd,int *outgoing_fd){
    if(pool_retire(s))return 3;
    int has_wait=*incoming_fd>=-1;
    if(has_wait){
        if(*incoming_fd>=0){struct pollfd p={.fd=*incoming_fd,.events=POLLIN};if(poll(&p,1,0)==0)pool_import_pending++;}
        VkImportSemaphoreFdInfoKHR ii={.sType=VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_FD_INFO_KHR,.semaphore=s->incoming,.flags=VK_SEMAPHORE_IMPORT_TEMPORARY_BIT,.handleType=VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT,.fd=*incoming_fd};
        OK(pool_api.import_fd(device,&ii));*incoming_fd=-2;
    }else if(s->consumer||s->submits)return fail("pool missing reuse fence",-1);
    OK(pool_api.reset_pool(device,s->pool,0));OK(pool_api.reset_fences(device,1,&s->fence));
    VkCommandBufferBeginInfo begin={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};OK(vkBeginCommandBuffer(s->cmd,&begin));
    int draw=0;
#ifdef MAVK_AHB_WAYLAND
    draw=!s->consumer&&pool_draw_hook!=NULL;
#endif
    VkPipelineStageFlags work_stage=draw?VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT:VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkImageLayout layout=draw?VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:(s->consumer?VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkImageMemoryBarrier acquire={.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,.dstAccessMask=s->consumer?VK_ACCESS_TRANSFER_READ_BIT:VK_ACCESS_TRANSFER_WRITE_BIT,.oldLayout=(!s->consumer&&!s->submits)?VK_IMAGE_LAYOUT_UNDEFINED:VK_IMAGE_LAYOUT_GENERAL,.newLayout=layout,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_FOREIGN_EXT,.dstQueueFamilyIndex=family,.image=s->image,.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1}};
    if(draw)acquire.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    vkCmdPipelineBarrier(s->cmd,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,work_stage,0,0,NULL,0,NULL,1,&acquire);
    if(s->consumer){
        VkBufferImageCopy copy={.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1},.imageExtent={s->width,s->height,1}};vkCmdCopyImageToBuffer(s->cmd,s->image,layout,s->read_buffer,1,&copy);
        VkBufferMemoryBarrier host={.sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT,.dstAccessMask=VK_ACCESS_HOST_READ_BIT,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=s->read_buffer,.size=VK_WHOLE_SIZE};
        vkCmdPipelineBarrier(s->cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,NULL,1,&host,0,NULL);
    }
#ifdef MAVK_AHB_WAYLAND
    else if(draw)pool_draw_hook(s->cmd,s,red);
#endif
    else{
        VkClearColorValue color={{red/255.0f,34.0f/255.0f,51.0f/255.0f,1.0f}};pool_api.clear(s->cmd,s->image,layout,&color,1,&acquire.subresourceRange);
    }
    VkImageMemoryBarrier release=acquire;release.srcAccessMask=acquire.dstAccessMask;release.dstAccessMask=0;release.oldLayout=layout;release.newLayout=VK_IMAGE_LAYOUT_GENERAL;release.srcQueueFamilyIndex=family;release.dstQueueFamilyIndex=VK_QUEUE_FAMILY_FOREIGN_EXT;
    vkCmdPipelineBarrier(s->cmd,work_stage,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,0,0,NULL,0,NULL,1,&release);OK(vkEndCommandBuffer(s->cmd));
    VkPipelineStageFlags stage=work_stage;
    VkSubmitInfo submit={.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,.waitSemaphoreCount=has_wait?1u:0u,.pWaitSemaphores=has_wait?&s->incoming:NULL,.pWaitDstStageMask=has_wait?&stage:NULL,.commandBufferCount=1,.pCommandBuffers=&s->cmd,.signalSemaphoreCount=1,.pSignalSemaphores=&s->outgoing};
    VkQueue queue;vkGetDeviceQueue(device,family,0,&queue);OK(vkQueueSubmit(queue,1,&submit,s->fence));s->pending=1;s->submits++;s->last_red=red;
    VkSemaphoreGetFdInfoKHR fi={.sType=VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR,.semaphore=s->outgoing,.handleType=VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT};OK(pool_api.export_fd(device,&fi,outgoing_fd));
    if(*outgoing_fd>=0){struct pollfd p={.fd=*outgoing_fd,.events=POLLIN};if(poll(&p,1,0)==0)pool_export_pending++;}
    return 0;
}
static void pool_destroy(AhbPoolSlot *s){
    if(s->pool)vkDestroyCommandPool(device,s->pool,NULL);
    if(s->fence)vkDestroyFence(device,s->fence,NULL);
    if(s->incoming)pool_api.destroy_sem(device,s->incoming,NULL);
    if(s->outgoing)pool_api.destroy_sem(device,s->outgoing,NULL);
    if(s->mapped)vkUnmapMemory(device,s->read_memory);
    if(s->read_buffer)vkDestroyBuffer(device,s->read_buffer,NULL);
    if(s->read_memory)vkFreeMemory(device,s->read_memory,NULL);
    if(s->image)vkDestroyImage(device,s->image,NULL);
    if(s->memory)vkFreeMemory(device,s->memory,NULL);
    *s=(AhbPoolSlot){0};
}
