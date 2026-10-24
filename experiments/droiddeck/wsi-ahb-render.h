#pragma once
#include "triangle-renderer.h"
#include <time.h>

static int wsi_render(struct wl_display *display,VkSwapchainKHR swapchain,
                      const VkImage *images,uint32_t count,uint32_t family,
                      PFN_vkAcquireNextImageKHR acquire,PFN_vkQueuePresentKHR present,int *window_error,VkFormat format) {
    unsigned frames=180;
    const char *requested=getenv("MAVK_AHB_WSI_FRAMES");
    if(requested){char *end=NULL;unsigned long value=strtoul(requested,&end,10);if(!end||*end||value<6||value>3600)return fail("frame count",-1);frames=(unsigned)value;}
    if(mavk_triangle_setup(images,count,1920,1200,format))return 1;
    PFN_vkResetFences reset_fences=(PFN_vkResetFences)get(instance,"vkResetFences");
    PFN_vkResetCommandBuffer reset_command=(PFN_vkResetCommandBuffer)get(instance,"vkResetCommandBuffer");
    if(!reset_fences||!reset_command)return fail("frame reset dispatch",-1);
    VkQueue queue;vkGetDeviceQueue(device,family,0,&queue);
    VkCommandPool pool;VkCommandPoolCreateInfo pi={.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,.queueFamilyIndex=family};
    OK(vkCreateCommandPool(device,&pi,NULL,&pool));
    VkCommandBuffer command;VkCommandBufferAllocateInfo ai={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool=pool,.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,.commandBufferCount=1};
    OK(vkAllocateCommandBuffers(device,&ai,&command));
    VkSemaphore available,ready[8]={0};
    PFN_vkCreateSemaphore create_semaphore=(PFN_vkCreateSemaphore)get(instance,"vkCreateSemaphore");
    PFN_vkDestroySemaphore destroy_semaphore=(PFN_vkDestroySemaphore)get(instance,"vkDestroySemaphore");
    if(!create_semaphore||!destroy_semaphore)return fail("frame semaphore dispatch",-1);
    VkSemaphoreCreateInfo semaphore_info={.sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    OK(create_semaphore(device,&semaphore_info,NULL,&available));
    for(uint32_t i=0;i<count;i++)OK(create_semaphore(device,&semaphore_info,NULL,&ready[i]));
    VkFence fence;VkFenceCreateInfo fi={.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    OK(vkCreateFence(device,&fi,NULL,&fence));
    int used[8]={0};struct timespec start,end;clock_gettime(CLOCK_MONOTONIC,&start);
    printf("WSI_STANDARD_DRAW_BEGIN frames=%u extent=1920x1200\n",frames);
    for(unsigned frame=0;frame<frames;frame++){
        if(*window_error)return fail("window closed or resized",-1);
        if(frame)OK(vkWaitForFences(device,1,&fence,VK_TRUE,5000000000ull));
        OK(reset_fences(device,1,&fence));
        uint32_t index=UINT32_MAX;
        OK(acquire(device,swapchain,5000000000ull,available,VK_NULL_HANDLE,&index));
        if(index>=count)return fail("frame image index",-1);
        OK(reset_command(command,0));
        VkCommandBufferBeginInfo begin={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
        OK(vkBeginCommandBuffer(command,&begin));
        VkImageMemoryBarrier barrier={.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout=used[index]?VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,
            .image=images[index],.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1}};
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0,0,NULL,0,NULL,1,&barrier);
        mavk_triangle_record(command,index,frame);
        barrier.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;barrier.dstAccessMask=0;
        barrier.oldLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0,0,NULL,0,NULL,1,&barrier);
        OK(vkEndCommandBuffer(command));
        VkPipelineStageFlags stage=VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        VkSubmitInfo submit={.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,.waitSemaphoreCount=1,.pWaitSemaphores=&available,
            .pWaitDstStageMask=&stage,.commandBufferCount=1,.pCommandBuffers=&command,.signalSemaphoreCount=1,.pSignalSemaphores=&ready[index]};
        OK(vkQueueSubmit(queue,1,&submit,fence));
        VkPresentInfoKHR info={.sType=VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,.waitSemaphoreCount=1,.pWaitSemaphores=&ready[index],
            .swapchainCount=1,.pSwapchains=&swapchain,.pImageIndices=&index};
        OK(present(queue,&info));used[index]=1;
        if(wl_display_dispatch_pending(display)<0)return fail("application Wayland events",-1);
    }
    OK(vkDeviceWaitIdle(device));clock_gettime(CLOCK_MONOTONIC,&end);
    double elapsed=(end.tv_sec-start.tv_sec)*1000.0+(end.tv_nsec-start.tv_nsec)/1000000.0;
    printf("WSI_STANDARD_DRAW_SUBMITTED frames=%u elapsedMs=%.3f sourceReadbacks=0 visibleFpsUnmeasured=1\n",frames,elapsed);
    vkDestroyFence(device,fence,NULL);vkDestroyCommandPool(device,pool,NULL);
    destroy_semaphore(device,available,NULL);
    for(uint32_t i=0;i<count;i++)destroy_semaphore(device,ready[i],NULL);
    mavk_triangle_destroy();
    return 0;
}
