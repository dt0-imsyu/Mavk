// Internal host-only ICD test. Artificial source bytes, no loader/GPU execution.
#include "../windows/icd.cpp"
#include <cassert>
int main(){
    Device device{};device.queue.device=&device;
    VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};info.imageType=VK_IMAGE_TYPE_2D;info.format=VK_FORMAT_R8G8B8A8_UNORM;
    info.extent={1024,1024,1};info.mipLevels=info.arrayLayers=1;info.samples=VK_SAMPLE_COUNT_1_BIT;info.tiling=VK_IMAGE_TILING_LINEAR;info.usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    VkImage destination{};assert(vkCreateImage(handle<VkDevice>(&device),&info,nullptr,&destination)==VK_SUCCESS);
    VkMemoryRequirements req{};vkGetImageMemoryRequirements({},destination,&req);assert(req.size==4194304&&req.memoryTypeBits==1);
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=req.size;allocation.memoryTypeIndex=0;
    VkDeviceMemory memory{};assert(vkAllocateMemory(handle<VkDevice>(&device),&allocation,nullptr,&memory)==VK_SUCCESS);
    assert(vkBindImageMemory({},destination,memory,0)==VK_SUCCESS);
    Memory pixels{&device,1,std::vector<uint8_t>(4194304)};
    for(size_t i=0;i<pixels.bytes.size();i++)pixels.bytes[i]=uint8_t((i*31)^(i>>13));
    Image source{1024,1024,&pixels,VK_FORMAT_R8G8B8A8_UNORM,false,true,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL};
    Command command{};command.device=&device;const auto h=handle<VkCommandBuffer>(&command);
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};assert(vkBeginCommandBuffer(h,&begin)==VK_SUCCESS);
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.image=destination;
    barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
    barrier.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    vkCmdPipelineBarrier(h,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
    VkImageCopy copy{};copy.srcSubresource=copy.dstSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.extent={1024,1024,1};
    vkCmdCopyImage(h,handle<VkImage>(&source),VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,destination,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
    barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_GENERAL;
    vkCmdPipelineBarrier(h,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
    assert(!command.bad&&command.imageOps.size()==3&&as<Image>(destination)->layout==VK_IMAGE_LAYOUT_UNDEFINED);
    assert(vkEndCommandBuffer(h)==VK_SUCCESS);VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&h;
    Fence fence{};assert(vkQueueSubmit(handle<VkQueue>(&device.queue),1,&submit,handle<VkFence>(&fence))==VK_SUCCESS&&fence.signaled);
    assert(as<Image>(destination)->initialized&&as<Image>(destination)->layout==VK_IMAGE_LAYOUT_GENERAL);
    VkImageSubresource sub{VK_IMAGE_ASPECT_COLOR_BIT,0,0};VkSubresourceLayout layout{};vkGetImageSubresourceLayout({},destination,&sub,&layout);
    assert(!layout.offset&&layout.rowPitch==4096&&layout.size==4194304);
    void* mapped{};assert(vkMapMemory({},memory,0,VK_WHOLE_SIZE,0,&mapped)==VK_SUCCESS&&!std::memcmp(mapped,pixels.bytes.data(),4194304));
    vkUnmapMemory({},memory);vkDestroyImage({},destination,nullptr);vkFreeMemory({},memory,nullptr);assert(device.allocated==0);
    std::printf("ICD_LINEAR_READBACK_CPU_PASS bytes=4194304 rowPitch=4096 (artificial bytes; not GPU/Windows PASS)\n");
}
