#define MAVK_EXPERIMENTAL_SAMPLED
#include "../windows/icd.cpp"
#include <cassert>
#include <limits>
int main(){
    Device device{};device.queue.device=&device;auto d=handle<VkDevice>(&device);
    VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};ci.imageType=VK_IMAGE_TYPE_2D;ci.format=VK_FORMAT_R8G8B8A8_UNORM;ci.extent={1024,1024,1};ci.mipLevels=11;ci.arrayLayers=1;ci.samples=VK_SAMPLE_COUNT_1_BIT;ci.tiling=VK_IMAGE_TILING_OPTIMAL;ci.usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
    VkImage image{};assert(vkCreateImage(d,&ci,nullptr,&image)==VK_SUCCESS);VkMemoryRequirements req{};vkGetImageMemoryRequirements(d,image,&req);assert(req.size==5592404&&req.memoryTypeBits==2);
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=req.size;allocation.memoryTypeIndex=1;VkDeviceMemory imageMemory{};assert(vkAllocateMemory(d,&allocation,nullptr,&imageMemory)==VK_SUCCESS);assert(vkBindImageMemory(d,image,imageMemory,0)==VK_SUCCESS);
    VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};bi.size=req.size;bi.usage=VK_BUFFER_USAGE_TRANSFER_SRC_BIT;VkBuffer buffer{};assert(vkCreateBuffer(d,&bi,nullptr,&buffer)==VK_SUCCESS);allocation.memoryTypeIndex=0;VkDeviceMemory bufferMemory{};assert(vkAllocateMemory(d,&allocation,nullptr,&bufferMemory)==VK_SUCCESS);assert(vkBindBufferMemory(d,buffer,bufferMemory,0)==VK_SUCCESS);void* mapped{};assert(vkMapMemory(d,bufferMemory,0,VK_WHOLE_SIZE,0,&mapped)==VK_SUCCESS);std::memset(mapped,0x12,size_t(req.size));
    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};vi.image=image;vi.viewType=VK_IMAGE_VIEW_TYPE_2D;vi.format=ci.format;vi.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,11,0,1};VkImageView view{};assert(vkCreateImageView(d,&vi,nullptr,&view)==VK_SUCCESS);
    VkCommandPoolCreateInfo pi{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pi.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;VkCommandPool pool{};assert(vkCreateCommandPool(d,&pi,nullptr,&pool)==VK_SUCCESS);VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ai.commandPool=pool;ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ai.commandBufferCount=1;VkCommandBuffer command{};assert(vkAllocateCommandBuffers(d,&ai,&command)==VK_SUCCESS);
    auto mips=mavk::rgbaMipLayout(1024,1024,11);std::vector<VkBufferImageCopy> regions;
    for(unsigned j=0;j<mips.size();j++){VkBufferImageCopy r{};r.bufferOffset=mips[j].offset;r.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,j,0,1};r.imageExtent={mips[j].width,mips[j].height,1};regions.push_back(r);}
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};assert(vkBeginCommandBuffer(command,&begin)==VK_SUCCESS);
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.image=image;barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,11,0,1};barrier.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
    vkCmdCopyBufferToImage(command,buffer,image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,11,regions.data());barrier.oldLayout=barrier.newLayout;barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,nullptr,0,nullptr,1,&barrier);assert(vkEndCommandBuffer(command)==VK_SUCCESS);
    // Vulkan transfer input is read at submission, not copied during recording.
    std::memset(mapped,0x73,size_t(req.size));VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;assert(vkQueueSubmit(handle<VkQueue>(&device.queue),1,&submit,VK_NULL_HANDLE)==VK_SUCCESS);
    auto* result=as<Image>(image);assert(result->initialized&&result->layout==VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL&&result->uploadedMips==2047&&result->remoteTexture==0);assert(!std::memcmp(result->memory->bytes.data(),mapped,size_t(req.size)));
    unsigned rejected=0;
    for(unsigned mode=0;mode<6;mode++){
        auto invalid=regions[0];if(mode==0)invalid.imageSubresource.mipLevel=11;if(mode==1)invalid.bufferOffset=req.size;if(mode==2)invalid.imageExtent.width=1023;if(mode==3)invalid.bufferRowLength=1023;if(mode==4)invalid.imageSubresource.layerCount=2;if(mode==5)invalid.imageOffset.x=1;
        assert(vkBeginCommandBuffer(command,&begin)==VK_SUCCESS);vkCmdCopyBufferToImage(command,buffer,image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&invalid);assert(as<Command>(command)->bad&&vkEndCommandBuffer(command)==Unsupported);rejected++;
    }
    VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};si.magFilter=si.minFilter=VK_FILTER_LINEAR;si.mipmapMode=VK_SAMPLER_MIPMAP_MODE_LINEAR;si.addressModeU=si.addressModeV=si.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;si.maxLod=10;
    VkSampler sampler{};assert(vkCreateSampler(d,&si,nullptr,&sampler)==VK_SUCCESS);vkDestroySampler(d,sampler,nullptr);
    for(unsigned mode=0;mode<6;mode++){
        auto invalid=si;if(mode==0)invalid.anisotropyEnable=1;if(mode==1)invalid.compareEnable=1;if(mode==2)invalid.unnormalizedCoordinates=1;if(mode==3)invalid.mipLodBias=std::numeric_limits<float>::quiet_NaN();if(mode==4)invalid.minLod=11;if(mode==5)invalid.addressModeU=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        sampler=VK_NULL_HANDLE;assert(vkCreateSampler(d,&invalid,nullptr,&sampler)==Unsupported&&!sampler);rejected++;
    }
    vkDestroyImageView(d,view,nullptr);vkDestroyImage(d,image,nullptr);vkDestroyBuffer(d,buffer,nullptr);vkFreeMemory(d,imageMemory,nullptr);vkFreeMemory(d,bufferMemory,nullptr);vkDestroyCommandPool(d,pool,nullptr);assert(device.allocated==0);
    std::printf("ICD_SAMPLED_RESOURCE_CPU_PASS mips=11 bytes=5592404 late_writes=PASS rejects=%u NOT_GPU_OR_GUEST_PASS\n",rejected);
}
