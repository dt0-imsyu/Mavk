#include <poll.h>
#include <unistd.h>

typedef struct {
    VkImage image;
    VkDeviceMemory memory;
    VkSemaphore signal;
    VkCommandPool pool;
    int fd;
} AhbAsyncProducer;
static AhbAsyncProducer async_producer={.fd=-2};

static void ahb_async_cleanup(void){
    vkDeviceWaitIdle(device);
    if(async_producer.fd>=0)close(async_producer.fd);
    if(async_producer.pool)vkDestroyCommandPool(device,async_producer.pool,NULL);
    if(async_producer.signal){PFN_vkDestroySemaphore destroy=(PFN_vkDestroySemaphore)get(instance,"vkDestroySemaphore");destroy(device,async_producer.signal,NULL);}
    if(async_producer.image)vkDestroyImage(device,async_producer.image,NULL);
    if(async_producer.memory)vkFreeMemory(device,async_producer.memory,NULL);
    async_producer=(AhbAsyncProducer){.fd=-2};
}

static int ahb_async_submit(struct AHardwareBuffer *buffer,const HardwareBufferDesc *desc,
                             const VkAndroidHardwareBufferPropertiesANDROID *props,VkFormat format,uint32_t family){
    PFN_vkGetPhysicalDeviceExternalSemaphoreProperties capabilities=(PFN_vkGetPhysicalDeviceExternalSemaphoreProperties)get(instance,"vkGetPhysicalDeviceExternalSemaphoreProperties");
    PFN_vkCreateSemaphore create_sem=(PFN_vkCreateSemaphore)get(instance,"vkCreateSemaphore");
    PFN_vkGetSemaphoreFdKHR export_fd=(PFN_vkGetSemaphoreFdKHR)get(instance,"vkGetSemaphoreFdKHR");
    PFN_vkCmdClearColorImage clear_image=(PFN_vkCmdClearColorImage)get(instance,"vkCmdClearColorImage");
    if(!capabilities||!create_sem||!export_fd||!clear_image)return fail("SYNC_FD entrypoints",-1);
    VkPhysicalDeviceExternalSemaphoreInfo query={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO,.handleType=VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT};
    VkExternalSemaphoreProperties support={.sType=VK_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_PROPERTIES};capabilities(physical,&query,&support);
    printf("AHB_SYNCFD_EXPORT_CAP features=%x compatible=%x\n",support.externalSemaphoreFeatures,support.compatibleHandleTypes);
    if(!(support.externalSemaphoreFeatures&VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT))return fail("SYNC_FD not exportable",-1);
    const VkImageUsageFlags usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    PFN_vkGetPhysicalDeviceImageFormatProperties2 image_props=(PFN_vkGetPhysicalDeviceImageFormatProperties2)get(instance,"vkGetPhysicalDeviceImageFormatProperties2");
    if(!image_props)return fail("async image properties dispatch",-1);
    VkPhysicalDeviceExternalImageFormatInfo external_query={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO,.handleType=VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID};
    VkPhysicalDeviceImageFormatInfo2 iq={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,.pNext=&external_query,.format=format,.type=VK_IMAGE_TYPE_2D,.tiling=VK_IMAGE_TILING_OPTIMAL,.usage=usage};
    VkExternalImageFormatProperties ep={.sType=VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES};
    VkImageFormatProperties2 ip={.sType=VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2,.pNext=&ep};OK(image_props(physical,&iq,&ip));
    if(!(ep.externalMemoryProperties.externalMemoryFeatures&VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT))return fail("async AHB import unsupported",-1);
    VkExternalMemoryImageCreateInfo external={.sType=VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,.handleTypes=VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID};
    VkImageCreateInfo ci={.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,.pNext=&external,.imageType=VK_IMAGE_TYPE_2D,.format=format,.extent={desc->width,desc->height,1},.mipLevels=1,.arrayLayers=1,.samples=VK_SAMPLE_COUNT_1_BIT,.tiling=VK_IMAGE_TILING_OPTIMAL,.usage=usage};
    OK(vkCreateImage(device,&ci,NULL,&async_producer.image));
    VkMemoryDedicatedAllocateInfo dedicated={.sType=VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,.image=async_producer.image};
    VkImportAndroidHardwareBufferInfoANDROID import={.sType=VK_STRUCTURE_TYPE_IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID,.pNext=&dedicated,.buffer=buffer};
    uint32_t memory=memory_type(props->memoryTypeBits,0);if(memory==UINT32_MAX)return fail("async AHB memory type",-1);
    VkMemoryAllocateInfo ai={.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,.pNext=&import,.allocationSize=props->allocationSize,.memoryTypeIndex=memory};
    OK(vkAllocateMemory(device,&ai,NULL,&async_producer.memory));OK(vkBindImageMemory(device,async_producer.image,async_producer.memory,0));
    VkExportSemaphoreCreateInfo export_info={.sType=VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO,.handleTypes=VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT};
    VkSemaphoreCreateInfo si={.sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,.pNext=&export_info};OK(create_sem(device,&si,NULL,&async_producer.signal));
    VkCommandPoolCreateInfo pci={.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,.queueFamilyIndex=family};OK(vkCreateCommandPool(device,&pci,NULL,&async_producer.pool));
    VkCommandBufferAllocateInfo ca={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,.commandPool=async_producer.pool,.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,.commandBufferCount=1};
    VkCommandBuffer cmd;OK(vkAllocateCommandBuffers(device,&ca,&cmd));VkCommandBufferBeginInfo begin={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};OK(vkBeginCommandBuffer(cmd,&begin));
    VkImageMemoryBarrier acquire={.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT,.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED,.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_FOREIGN_EXT,.dstQueueFamilyIndex=family,.image=async_producer.image,.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1}};
    vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,NULL,0,NULL,1,&acquire);
    VkClearColorValue color={{91.0f/255.0f,34.0f/255.0f,51.0f/255.0f,1.0f}};clear_image(cmd,async_producer.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&color,1,&acquire.subresourceRange);
    VkImageMemoryBarrier release=acquire;release.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;release.dstAccessMask=0;release.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;release.newLayout=VK_IMAGE_LAYOUT_GENERAL;release.srcQueueFamilyIndex=family;release.dstQueueFamilyIndex=VK_QUEUE_FAMILY_FOREIGN_EXT;
    vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,0,0,NULL,0,NULL,1,&release);OK(vkEndCommandBuffer(cmd));
    VkQueue queue;vkGetDeviceQueue(device,family,0,&queue);
    VkSubmitInfo submit={.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,.commandBufferCount=1,.pCommandBuffers=&cmd,.signalSemaphoreCount=1,.pSignalSemaphores=&async_producer.signal};
    OK(vkQueueSubmit(queue,1,&submit,VK_NULL_HANDLE));
    VkSemaphoreGetFdInfoKHR fi={.sType=VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR,.semaphore=async_producer.signal,.handleType=VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT};
    OK(export_fd(device,&fi,&async_producer.fd));
    struct pollfd poll_fd={.fd=async_producer.fd,.events=POLLIN};int ready=async_producer.fd<0?1:poll(&poll_fd,1,0);
    printf("AHB_ASYNC_SUBMITTED fdPresent=%u initiallyReady=%d producerCpuGpuWaitsBeforePublish=0 sourceReadbacks=0\n",async_producer.fd>=0,ready);
    return 0;
}
