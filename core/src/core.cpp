#include <mavk/core.hpp>
#include <mavk/sampled.hpp>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>

namespace mavk {
namespace {void check(VkResult r,const char* what){if(r!=VK_SUCCESS)throw std::runtime_error(std::string("Mavk ")+what+" VkResult="+std::to_string(r));}}
DeviceContext::DeviceContext(VkPhysicalDevice physical,VkDevice device,uint32_t family,bool timelineEnabled):device_(device),family_(family),physical_(physical),timelineEnabled_(timelineEnabled){
    if(!physical||!device)throw std::invalid_argument("Mavk requires actual Vulkan handles");
    vkGetPhysicalDeviceMemoryProperties(physical,&memory_);
}
uint32_t DeviceContext::memoryType(uint32_t compatible,VkMemoryPropertyFlags required,VkMemoryPropertyFlags preferred) const {
    uint32_t fallback=memory_.memoryTypeCount;
    for(uint32_t i=0;i<memory_.memoryTypeCount;i++)if((compatible&(1u<<i))&&(memory_.memoryTypes[i].propertyFlags&required)==required){
        if((memory_.memoryTypes[i].propertyFlags&preferred)==preferred)return i;
        if(fallback==memory_.memoryTypeCount)fallback=i;
    }
    if(fallback==memory_.memoryTypeCount)throw std::runtime_error("Mavk compatible memory unavailable");return fallback;
}
VkMemoryPropertyFlags DeviceContext::memoryFlags(uint32_t type) const {
    if(type>=memory_.memoryTypeCount)throw std::out_of_range("Mavk memory type");return memory_.memoryTypes[type].propertyFlags;
}
VkResult DeviceContext::submit(VkQueue queue,const VkSubmitInfo& submit,VkFence fence){
    VkResult r=vkQueueSubmit(queue,1,&submit,fence);if(r==VK_SUCCESS)counters_.totalSubmissions++;return r;
}
Buffer::Buffer(DeviceContext& c,VkDeviceSize bytes,VkBufferUsageFlags usage,VkMemoryPropertyFlags required,VkMemoryPropertyFlags preferred):context_(c),size_(bytes){
    if(!bytes)throw std::invalid_argument("Mavk zero-sized buffer");
    try{
        VkBufferCreateInfo ci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};ci.size=bytes;ci.usage=usage;
        check(vkCreateBuffer(c.device_,&ci,nullptr,&buffer_),"create buffer");c.counters_.buffers++;
        VkMemoryRequirements req;vkGetBufferMemoryRequirements(c.device_,buffer_,&req);type_=c.memoryType(req.memoryTypeBits,required,preferred);
        VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};ai.allocationSize=req.size;ai.memoryTypeIndex=type_;
        check(vkAllocateMemory(c.device_,&ai,nullptr,&memory_),"allocate buffer");allocation_=req.size;c.counters_.liveMemoryBytes+=allocation_;c.counters_.totalAllocations++;
        check(vkBindBufferMemory(c.device_,buffer_,memory_,0),"bind buffer");
    }catch(...){clear();throw;}
}
void Buffer::clear() noexcept {
    if(mapped_){vkUnmapMemory(context_.device_,memory_);mapped_=nullptr;}
    if(buffer_){vkDestroyBuffer(context_.device_,buffer_,nullptr);buffer_=VK_NULL_HANDLE;context_.counters_.buffers--;}
    if(memory_){vkFreeMemory(context_.device_,memory_,nullptr);memory_=VK_NULL_HANDLE;context_.counters_.liveMemoryBytes-=allocation_;}
}
Buffer::~Buffer(){clear();}
void* Buffer::map(){
    if(!(context_.memoryFlags(type_)&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))throw std::logic_error("Mavk non-host-visible map");
    if(!mapped_)check(vkMapMemory(context_.device_,memory_,0,VK_WHOLE_SIZE,0,&mapped_),"map buffer");return mapped_;
}
void Buffer::flush(){
    if(!mapped_)throw std::logic_error("Mavk flush requires mapped buffer");
    if(!(context_.memoryFlags(type_)&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){VkMappedMemoryRange r{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};r.memory=memory_;r.size=VK_WHOLE_SIZE;check(vkFlushMappedMemoryRanges(context_.device_,1,&r),"flush buffer");}
}
void Buffer::invalidate(){
    if(!mapped_)throw std::logic_error("Mavk invalidate requires mapped buffer");
    if(!(context_.memoryFlags(type_)&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){VkMappedMemoryRange r{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};r.memory=memory_;r.size=VK_WHOLE_SIZE;check(vkInvalidateMappedMemoryRanges(context_.device_,1,&r),"invalidate buffer");}
}
Image::Image(DeviceContext& c,const VkImageCreateInfo& ci,VkMemoryPropertyFlags required):context_(c){
    try{
        check(vkCreateImage(c.device_,&ci,nullptr,&image_),"create image");c.counters_.images++;
        VkMemoryRequirements req;vkGetImageMemoryRequirements(c.device_,image_,&req);
        VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};ai.allocationSize=req.size;ai.memoryTypeIndex=c.memoryType(req.memoryTypeBits,required);
        check(vkAllocateMemory(c.device_,&ai,nullptr,&memory_),"allocate image");allocation_=req.size;c.counters_.liveMemoryBytes+=allocation_;c.counters_.totalAllocations++;
        check(vkBindImageMemory(c.device_,image_,memory_,0),"bind image");
    }catch(...){clear();throw;}
}
void Image::clear() noexcept {
    if(image_){vkDestroyImage(context_.device_,image_,nullptr);image_=VK_NULL_HANDLE;context_.counters_.images--;}
    if(memory_){vkFreeMemory(context_.device_,memory_,nullptr);memory_=VK_NULL_HANDLE;context_.counters_.liveMemoryBytes-=allocation_;}
}
Image::~Image(){clear();}
SampledImage::SampledImage(DeviceContext& c,const SampledPixels& p):context_(c),mips_(sampledMipLayout(p.width,p.height,p.levels,p.format)){
    pixelBytes_=p.format==VK_FORMAT_R32G32B32A32_SFLOAT?16:4;
    const auto& s=p.sampler;const auto& last=mips_.back();
    if(p.rgba.size()!=last.offset+last.bytes)throw std::invalid_argument("Mavk mip byte count");
    VkPhysicalDeviceProperties props{};vkGetPhysicalDeviceProperties(c.physical_,&props);
    if(!samplerStateValid(s,props.limits.maxSamplerLodBias))
        throw std::invalid_argument("Mavk unsupported sampler state");
    VkFormatProperties format{};vkGetPhysicalDeviceFormatProperties(c.physical_,p.format,&format);
    VkFormatFeatureFlags required=VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT|VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
    if(s.mag==VK_FILTER_LINEAR||s.min==VK_FILTER_LINEAR||s.mip==VK_SAMPLER_MIPMAP_MODE_LINEAR)required|=VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
    if((format.optimalTilingFeatures&required)!=required)throw std::runtime_error("Mavk RGBA sampled format unsupported");
    try{
        VkImageCreateInfo i{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};i.imageType=VK_IMAGE_TYPE_2D;i.format=p.format;i.extent={p.width,p.height,1};i.mipLevels=p.levels;i.arrayLayers=1;i.samples=VK_SAMPLE_COUNT_1_BIT;i.tiling=VK_IMAGE_TILING_OPTIMAL;i.usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
        image_=std::make_unique<Image>(c,i);
        staging_=std::make_unique<Buffer>(c,p.rgba.size(),VK_BUFFER_USAGE_TRANSFER_SRC_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        std::memcpy(staging_->map(),p.rgba.data(),p.rgba.size());staging_->flush();
        VkImageViewCreateInfo v{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};v.image=image_->handle();v.viewType=VK_IMAGE_VIEW_TYPE_2D;v.format=i.format;v.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,p.levels,0,1};
        check(vkCreateImageView(c.device_,&v,nullptr,&view_),"sampled image view");c.counters_.imageViews++;
        VkSamplerCreateInfo ci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};ci.magFilter=s.mag;ci.minFilter=s.min;ci.mipmapMode=s.mip;ci.addressModeU=s.u;ci.addressModeV=s.v;ci.addressModeW=s.w;ci.mipLodBias=s.bias;ci.minLod=s.minLod;ci.maxLod=s.maxLod;ci.maxAnisotropy=1;
        check(vkCreateSampler(c.device_,&ci,nullptr,&sampler_),"sampler");c.counters_.samplers++;
    }catch(...){clear();throw;}
}
void SampledImage::clear() noexcept {
    if(sampler_){vkDestroySampler(context_.device_,sampler_,nullptr);sampler_=VK_NULL_HANDLE;context_.counters_.samplers--;}
    if(view_){vkDestroyImageView(context_.device_,view_,nullptr);view_=VK_NULL_HANDLE;context_.counters_.imageViews--;}
}
SampledImage::~SampledImage(){clear();}
void SampledImage::recordInitialUpload(VkCommandBuffer cmd,VkPipelineStageFlags consumers){
    constexpr VkPipelineStageFlags supported=VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    if(recorded_||!consumers||(consumers&~supported))throw std::logic_error("Mavk initial sampled upload state");
    std::vector<VkBufferImageCopy> regions;
    for(uint32_t level=0;level<mips_.size();level++){
        const auto& m=mips_[level];VkBufferImageCopy copy{};copy.bufferOffset=m.offset;copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,level,0,1};copy.imageExtent={m.width,m.height,1};regions.push_back(copy);
    }
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};b.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;b.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;b.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.image=image_->handle();b.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,uint32_t(mips_.size()),0,1};
    vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&b);
    vkCmdCopyBufferToImage(cmd,staging_->handle(),image_->handle(),VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,uint32_t(regions.size()),regions.data());
    b.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;b.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;b.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;b.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,consumers,0,0,nullptr,0,nullptr,1,&b);recorded_=true;
}
void SampledImage::upload(VkQueue queue,VkPipelineStageFlags consumers){
    CommandBatch command(context_);Fence fence(context_);
    try{
        command.begin();recordInitialUpload(command.handle(),consumers);command.end();
        VkCommandBuffer cmd=command.handle();VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&cmd;
        check(context_.submit(queue,submit,fence.handle()),"sampled upload submit");
        check(fence.wait(5'000'000'000ull),"sampled upload fence");
        staging_.reset();
    }catch(...){vkDeviceWaitIdle(context_.device_);throw;}
}
void SampledImage::update(VkQueue queue,uint32_t level,const VkRect2D& rect,const std::vector<uint8_t>& pixels,VkPipelineStageFlags consumers){
    constexpr VkPipelineStageFlags supported=VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    if(!recorded_||staging_)throw std::logic_error("Mavk sampled update requires completed initial upload");
    if(!queue||level>=mips_.size()||!consumers||(consumers&~supported)||rect.offset.x<0||rect.offset.y<0||!rect.extent.width||!rect.extent.height)throw std::invalid_argument("Mavk sampled update state/region");
    const auto& m=mips_[level];uint32_t x=uint32_t(rect.offset.x),y=uint32_t(rect.offset.y),w=rect.extent.width,h=rect.extent.height;
    if(x>=m.width||y>=m.height||w>m.width-x||h>m.height-y||VkDeviceSize(w)*h*pixelBytes_!=pixels.size())throw std::invalid_argument("Mavk sampled update extent/bytes");
    Buffer staging(context_,pixels.size(),VK_BUFFER_USAGE_TRANSFER_SRC_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    std::memcpy(staging.map(),pixels.data(),pixels.size());staging.flush();CommandBatch command(context_);Fence fence(context_);
    try{
        command.begin();auto cmd=command.handle();VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        b.srcAccessMask=VK_ACCESS_SHADER_READ_BIT;b.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;b.oldLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;b.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.image=image_->handle();b.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,level,1,0,1};
        vkCmdPipelineBarrier(cmd,consumers,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&b);
        VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,level,0,1};copy.imageOffset={rect.offset.x,rect.offset.y,0};copy.imageExtent={w,h,1};
        vkCmdCopyBufferToImage(cmd,staging.handle(),image_->handle(),VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
        b.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;b.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;b.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;b.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,consumers,0,0,nullptr,0,nullptr,1,&b);command.end();
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&cmd;
        check(context_.submit(queue,submit,fence.handle()),"sampled partial update submit");check(fence.wait(5'000'000'000ull),"sampled partial update fence");
    }catch(...){recorded_=false;vkDeviceWaitIdle(context_.device_);throw;}
}
Shader::Shader(DeviceContext& c,const uint32_t* code,size_t bytes):context_(c){
    if(!code||bytes<20||bytes%4||code[0]!=0x07230203)throw std::invalid_argument("Mavk malformed SPIR-V header");
    VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};ci.codeSize=bytes;ci.pCode=code;
    check(vkCreateShaderModule(c.device_,&ci,nullptr,&shader_),"shader");c.counters_.shaders++;
}
Shader::~Shader(){if(shader_){vkDestroyShaderModule(context_.device_,shader_,nullptr);context_.counters_.shaders--;}}
Pipeline::Pipeline(DeviceContext& c,const VkComputePipelineCreateInfo& ci):context_(c){
    VkResult r=vkCreateComputePipelines(c.device_,VK_NULL_HANDLE,1,&ci,nullptr,&pipeline_);
    if(r!=VK_SUCCESS){if(pipeline_)vkDestroyPipeline(c.device_,pipeline_,nullptr);check(r,"compute pipeline");}c.counters_.pipelines++;c.counters_.totalPipelineCreates++;
}
Pipeline::Pipeline(DeviceContext& c,const VkGraphicsPipelineCreateInfo& ci):context_(c){
    VkResult r=vkCreateGraphicsPipelines(c.device_,VK_NULL_HANDLE,1,&ci,nullptr,&pipeline_);
    if(r!=VK_SUCCESS){if(pipeline_)vkDestroyPipeline(c.device_,pipeline_,nullptr);check(r,"graphics pipeline");}c.counters_.pipelines++;c.counters_.totalPipelineCreates++;
}
Pipeline::~Pipeline(){if(pipeline_){vkDestroyPipeline(context_.device_,pipeline_,nullptr);context_.counters_.pipelines--;}}
Fence::Fence(DeviceContext& c):context_(c){VkFenceCreateInfo ci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};check(vkCreateFence(c.device_,&ci,nullptr,&fence_),"fence");c.counters_.fences++;}
Fence::~Fence(){if(fence_){vkDestroyFence(context_.device_,fence_,nullptr);context_.counters_.fences--;}}
void Fence::reset(){check(vkResetFences(context_.device_,1,&fence_),"reset fence");}
VkResult Fence::wait(uint64_t timeout) const {return vkWaitForFences(context_.device_,1,&fence_,VK_TRUE,timeout);}
CommandBatch::CommandBatch(DeviceContext& c):context_(c){
    VkCommandPoolCreateInfo ci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};ci.queueFamilyIndex=c.family_;ci.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    check(vkCreateCommandPool(c.device_,&ci,nullptr,&pool_),"command pool");
    VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ai.commandPool=pool_;ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ai.commandBufferCount=1;
    VkResult r=vkAllocateCommandBuffers(c.device_,&ai,&command_);if(r!=VK_SUCCESS){vkDestroyCommandPool(c.device_,pool_,nullptr);check(r,"command buffer");}c.counters_.commandBatches++;
}
CommandBatch::~CommandBatch(){if(pool_){vkDestroyCommandPool(context_.device_,pool_,nullptr);context_.counters_.commandBatches--;}}
void CommandBatch::begin(){check(vkResetCommandBuffer(command_,0),"reset command");VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};bi.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;check(vkBeginCommandBuffer(command_,&bi),"begin command");}
void CommandBatch::end(){check(vkEndCommandBuffer(command_),"end command");}
void CommandBatch::hostReadBarrier(VkPipelineStageFlags stage,VkAccessFlags access){VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};barrier.srcAccessMask=access;barrier.dstAccessMask=VK_ACCESS_HOST_READ_BIT;vkCmdPipelineBarrier(command_,stage,VK_PIPELINE_STAGE_HOST_BIT,0,1,&barrier,0,nullptr,0,nullptr);}
TimelineSemaphore::TimelineSemaphore(DeviceContext& c,uint64_t initial):context_(c){
    if(!c.timelineEnabled_)throw std::invalid_argument("Mavk timeline feature not enabled by device owner");
    counter_=reinterpret_cast<PFN_vkGetSemaphoreCounterValue>(vkGetDeviceProcAddr(c.device_,"vkGetSemaphoreCounterValue"));
    wait_=reinterpret_cast<PFN_vkWaitSemaphores>(vkGetDeviceProcAddr(c.device_,"vkWaitSemaphores"));
    signal_=reinterpret_cast<PFN_vkSignalSemaphore>(vkGetDeviceProcAddr(c.device_,"vkSignalSemaphore"));
    if(!counter_||!wait_||!signal_)throw std::runtime_error("Mavk native Vulkan1.2 timeline entrypoints unavailable");
    VkSemaphoreTypeCreateInfo type{VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO};type.semaphoreType=VK_SEMAPHORE_TYPE_TIMELINE;type.initialValue=initial;
    VkSemaphoreCreateInfo ci{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};ci.pNext=&type;
    check(vkCreateSemaphore(c.device_,&ci,nullptr,&semaphore_),"timeline semaphore");c.counters_.timelineSemaphores++;
}
TimelineSemaphore::~TimelineSemaphore(){if(semaphore_){vkDestroySemaphore(context_.device_,semaphore_,nullptr);context_.counters_.timelineSemaphores--;}}
uint64_t TimelineSemaphore::counter() const {uint64_t value{};check(counter_(context_.device_,semaphore_,&value),"timeline counter");return value;}
VkResult TimelineSemaphore::wait(uint64_t value,uint64_t timeout) const {
    VkSemaphoreWaitInfo info{VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO};info.semaphoreCount=1;info.pSemaphores=&semaphore_;info.pValues=&value;
    return wait_(context_.device_,&info,timeout);
}
void TimelineSemaphore::signal(uint64_t value){
    if(value<=counter())throw std::invalid_argument("Mavk timeline host signal must advance counter");
    VkSemaphoreSignalInfo info{VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO};info.semaphore=semaphore_;info.value=value;
    check(signal_(context_.device_,&info),"timeline signal");
}
}
