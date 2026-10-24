static int ahb_image_check(struct AHardwareBuffer *ahb,
                           const HardwareBufferDesc *desc,
                           const VkAndroidHardwareBufferPropertiesANDROID *properties,
                           VkFormat format, uint32_t family, int read_only, int expected_red,VkSemaphore acquire_wait) {
    const VkImageUsageFlags usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    PFN_vkGetPhysicalDeviceImageFormatProperties2 image_properties=(PFN_vkGetPhysicalDeviceImageFormatProperties2)get(instance,"vkGetPhysicalDeviceImageFormatProperties2");
    PFN_vkCmdClearColorImage clear_image=(PFN_vkCmdClearColorImage)get(instance,"vkCmdClearColorImage");
    if(!image_properties||!clear_image||format!=VK_FORMAT_R8G8B8A8_UNORM)return fail("AHB image entry/format",-1);
    VkPhysicalDeviceExternalImageFormatInfo external_query={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO,.handleType=VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID};
    VkPhysicalDeviceImageFormatInfo2 query={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,.pNext=&external_query,.format=format,.type=VK_IMAGE_TYPE_2D,.tiling=VK_IMAGE_TILING_OPTIMAL,.usage=usage};
    VkExternalImageFormatProperties external_properties={.sType=VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES};
    VkImageFormatProperties2 image_caps={.sType=VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2,.pNext=&external_properties};
    OK(image_properties(physical,&query,&image_caps));
    if(!(external_properties.externalMemoryProperties.externalMemoryFeatures&VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT)||
       desc->width>image_caps.imageFormatProperties.maxExtent.width||desc->height>image_caps.imageFormatProperties.maxExtent.height)
        return fail("AHB image import/extent unsupported",-1);
    VkExternalMemoryImageCreateInfo external={.sType=VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,.handleTypes=VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID};
    VkImageCreateInfo create={.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,.pNext=&external,.imageType=VK_IMAGE_TYPE_2D,.format=format,.extent={desc->width,desc->height,1},.mipLevels=1,.arrayLayers=1,.samples=VK_SAMPLE_COUNT_1_BIT,.tiling=VK_IMAGE_TILING_OPTIMAL,.usage=usage};
    VkImage image;OK(vkCreateImage(device,&create,NULL,&image));
    uint32_t image_type=memory_type(properties->memoryTypeBits,0);
    if(image_type==UINT32_MAX)return fail("AHB compatible memory type",-1);
    VkMemoryDedicatedAllocateInfo dedicated={.sType=VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,.image=image};
    VkImportAndroidHardwareBufferInfoANDROID import={.sType=VK_STRUCTURE_TYPE_IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID,.pNext=&dedicated,.buffer=ahb};
    VkMemoryAllocateInfo allocation={.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,.pNext=&import,.allocationSize=properties->allocationSize,.memoryTypeIndex=image_type};
    VkDeviceMemory image_memory;OK(vkAllocateMemory(device,&allocation,NULL,&image_memory));
    OK(vkBindImageMemory(device,image,image_memory,0));
    const VkDeviceSize bytes=(VkDeviceSize)desc->width*desc->height*4;
    VkBufferCreateInfo buffer_create={.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,.size=bytes,.usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT};
    VkBuffer buffer;OK(vkCreateBuffer(device,&buffer_create,NULL,&buffer));
    VkMemoryRequirements req;vkGetBufferMemoryRequirements(device,buffer,&req);
    uint32_t buffer_type=memory_type(req.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    if(buffer_type==UINT32_MAX)return fail("AHB oracle host memory",-1);
    allocation.pNext=NULL;allocation.allocationSize=req.size;allocation.memoryTypeIndex=buffer_type;
    VkDeviceMemory buffer_memory;OK(vkAllocateMemory(device,&allocation,NULL,&buffer_memory));
    OK(vkBindBufferMemory(device,buffer,buffer_memory,0));
    uint8_t *mapped;OK(vkMapMemory(device,buffer_memory,0,VK_WHOLE_SIZE,0,(void**)&mapped));
    VkQueue queue;vkGetDeviceQueue(device,family,0,&queue);
    VkCommandPoolCreateInfo pool_create={.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,.queueFamilyIndex=family};
    VkCommandPool pool;OK(vkCreateCommandPool(device,&pool_create,NULL,&pool));
    const uint32_t frames=read_only?1:2;
    int oracle_ok=1;
    for(uint32_t frame=0;frame<frames;frame++) {
        VkCommandBufferAllocateInfo cmd_allocate={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,.commandPool=pool,.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,.commandBufferCount=1};
        VkCommandBuffer cmd;OK(vkAllocateCommandBuffers(device,&cmd_allocate,&cmd));
        VkCommandBufferBeginInfo begin={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
        OK(vkBeginCommandBuffer(cmd,&begin));
        VkImageMemoryBarrier acquire={.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout=read_only?VK_IMAGE_LAYOUT_GENERAL:(frame?VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:VK_IMAGE_LAYOUT_UNDEFINED),.newLayout=read_only?VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex=frame?VK_QUEUE_FAMILY_IGNORED:VK_QUEUE_FAMILY_FOREIGN_EXT,
            .dstQueueFamilyIndex=frame?VK_QUEUE_FAMILY_IGNORED:family,.image=image,.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1}};
        if(frame)acquire.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
        if(read_only)acquire.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(cmd,frame?VK_PIPELINE_STAGE_TRANSFER_BIT:VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,NULL,0,NULL,1,&acquire);
        VkClearColorValue color={{(frame?91.0f:17.0f)/255.0f,34.0f/255.0f,51.0f/255.0f,1.0f}};
        if(!read_only)clear_image(cmd,image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&color,1,&acquire.subresourceRange);
        VkImageMemoryBarrier read=acquire;read.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;read.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
        read.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;read.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        read.srcQueueFamilyIndex=read.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        if(!read_only)vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,NULL,0,NULL,1,&read);
        VkBufferImageCopy copy={.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1},.imageExtent={desc->width,desc->height,1}};
        vkCmdCopyImageToBuffer(cmd,image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,buffer,1,&copy);
        if(frame+1==frames){
            VkImageMemoryBarrier release=read;release.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;release.dstAccessMask=0;
            release.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;release.newLayout=VK_IMAGE_LAYOUT_GENERAL;
            release.srcQueueFamilyIndex=family;release.dstQueueFamilyIndex=VK_QUEUE_FAMILY_FOREIGN_EXT;
            vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,0,0,NULL,0,NULL,1,&release);
        }
        VkBufferMemoryBarrier host={.sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT,.dstAccessMask=VK_ACCESS_HOST_READ_BIT,
            .srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=buffer,.size=VK_WHOLE_SIZE};
        vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,NULL,1,&host,0,NULL);
        OK(vkEndCommandBuffer(cmd));
        VkFenceCreateInfo fence_create={.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};VkFence fence;OK(vkCreateFence(device,&fence_create,NULL,&fence));
        VkPipelineStageFlags wait_stage=VK_PIPELINE_STAGE_TRANSFER_BIT;
        VkSubmitInfo submit={.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,.commandBufferCount=1,.pCommandBuffers=&cmd,
            .waitSemaphoreCount=acquire_wait?1u:0u,.pWaitSemaphores=acquire_wait?&acquire_wait:NULL,.pWaitDstStageMask=acquire_wait?&wait_stage:NULL};
        OK(vkQueueSubmit(queue,1,&submit,fence));
        OK(vkWaitForFences(device,1,&fence,VK_TRUE,5000000000ull));
        VkMappedMemoryRange range={.sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,.memory=buffer_memory,.size=VK_WHOLE_SIZE};
        OK(vkInvalidateMappedMemoryRanges(device,1,&range));
        const int expected[]={read_only?expected_red:(frame?91:17),34,51,255};
        for(VkDeviceSize p=0;p<bytes;p++)if(abs((int)mapped[p]-expected[p%4])>1){oracle_ok=0;break;}
        vkDestroyFence(device,fence,NULL);
        printf("AHB_GPU_FRAME_%s frame=%u pixels=%u firstRed=%u readOnly=%u expectedRed=%u\n",oracle_ok?"PASS":"FAIL",frame,desc->width*desc->height,mapped[0],read_only!=0,expected[0]);
    }
    OK(vkDeviceWaitIdle(device));vkDestroyCommandPool(device,pool,NULL);vkUnmapMemory(device,buffer_memory);
    vkDestroyBuffer(device,buffer,NULL);vkFreeMemory(device,buffer_memory,NULL);vkDestroyImage(device,image,NULL);vkFreeMemory(device,image_memory,NULL);
    if(!oracle_ok)return fail("AHB GPU pixel oracle",-1);
    printf("AHB_IMAGE_IMPORT_GPU_PASS frames=%u width=%u height=%u importBytes=%llu oracleBytes=%llu readOnly=%u noDisplayClaim=1\n",frames,desc->width,desc->height,(unsigned long long)properties->allocationSize,(unsigned long long)bytes,read_only!=0);
    return 0;
}
