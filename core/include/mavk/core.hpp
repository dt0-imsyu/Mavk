#pragma once
#include <vulkan/vulkan.h>
#include <mavk/vulkan_dispatch.hpp>
#include <cstdint>
#include <cstddef>

// Native resource/submission layer, not a Vulkan ICD or remote command protocol.
// The caller owns VkDevice/queue and must keep them alive and externally
// synchronize queue use. Complete GPU work before retiring referenced resources.
namespace mavk {
struct Counters {
    uint64_t buffers=0,images=0,shaders=0,pipelines=0,fences=0,commandBatches=0;
    uint64_t samplers=0,imageViews=0,timelineSemaphores=0;
    uint64_t liveMemoryBytes=0,totalAllocations=0,totalSubmissions=0,totalPipelineCreates=0;
    bool empty() const {return !(buffers||images||shaders||pipelines||fences||commandBatches||samplers||imageViews||timelineSemaphores||liveMemoryBytes);}
};
class DeviceContext {
public:
    // Optional capability must actually have been enabled at vkCreateDevice.
    DeviceContext(VkPhysicalDevice physical,VkDevice device,uint32_t queueFamily,bool timelineEnabled=false);
    DeviceContext(VkPhysicalDevice physical,VkDevice device,uint32_t queueFamily,const VulkanDispatch&,bool timelineEnabled=false);
    DeviceContext(const DeviceContext&)=delete;
    DeviceContext& operator=(const DeviceContext&)=delete;
    VkDevice vkDevice() const {return device_;}
    uint32_t queueFamily() const {return family_;}
    VkPhysicalDevice physicalDevice() const {return physical_;}
    const VulkanDispatch& dispatch() const {return dispatch_;}
    const VkPhysicalDeviceProperties& properties() const {return properties_;}
    uint32_t memoryType(uint32_t compatible,VkMemoryPropertyFlags required,VkMemoryPropertyFlags preferred=0) const;
    VkMemoryPropertyFlags memoryFlags(uint32_t type) const;
    const Counters& counters() const {return counters_;}
    VkResult submit(VkQueue queue,const VkSubmitInfo& submit,VkFence fence);
private:
    friend class Buffer;friend class Image;friend class Shader;friend class Pipeline;friend class Fence;friend class CommandBatch;
    friend class SampledImage;
    friend class TimelineSemaphore;
    VkDevice device_;uint32_t family_;VkPhysicalDeviceMemoryProperties memory_{};Counters counters_{};VkPhysicalDevice physical_{};bool timelineEnabled_{};
    VulkanDispatch dispatch_;VkPhysicalDeviceProperties properties_{};
};
class Buffer {
public:
    Buffer(DeviceContext&,VkDeviceSize bytes,VkBufferUsageFlags,VkMemoryPropertyFlags required,VkMemoryPropertyFlags preferred=0);
    ~Buffer();Buffer(const Buffer&)=delete;Buffer& operator=(const Buffer&)=delete;
    VkBuffer handle() const {return buffer_;}
    VkDeviceSize size() const {return size_;}
    VkDeviceSize allocationBytes() const {return allocation_;}
    uint32_t memoryType() const {return type_;}
    void* map();void flush();void invalidate();
private:
    void clear() noexcept;
    DeviceContext& context_;VkBuffer buffer_{};VkDeviceMemory memory_{};void* mapped_{};
    VkDeviceSize size_{},allocation_{};uint32_t type_{};
};
class Image {
public:
    Image(DeviceContext&,const VkImageCreateInfo&,VkMemoryPropertyFlags required=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    ~Image();Image(const Image&)=delete;Image& operator=(const Image&)=delete;
    VkImage handle() const {return image_;}VkDeviceSize allocationBytes() const {return allocation_;}
private:
    void clear() noexcept;
    DeviceContext& context_;VkImage image_{};VkDeviceMemory memory_{};VkDeviceSize allocation_{};
};
class Shader {
public:
    Shader(DeviceContext&,const uint32_t* code,size_t bytes);~Shader();
    Shader(const Shader&)=delete;Shader& operator=(const Shader&)=delete;
    VkShaderModule handle() const {return shader_;}
private:DeviceContext& context_;VkShaderModule shader_{};
};
class Pipeline {
public:
    Pipeline(DeviceContext&,const VkComputePipelineCreateInfo&);
    Pipeline(DeviceContext&,const VkGraphicsPipelineCreateInfo&);
    ~Pipeline();Pipeline(const Pipeline&)=delete;Pipeline& operator=(const Pipeline&)=delete;
    VkPipeline handle() const {return pipeline_;}
private:DeviceContext& context_;VkPipeline pipeline_{};
};
class Fence {
public:
    explicit Fence(DeviceContext&);~Fence();Fence(const Fence&)=delete;Fence& operator=(const Fence&)=delete;
    VkFence handle() const {return fence_;}void reset();VkResult wait(uint64_t timeoutNs) const;
private:DeviceContext& context_;VkFence fence_{};
};
class CommandBatch {
public:
    explicit CommandBatch(DeviceContext&);~CommandBatch();CommandBatch(const CommandBatch&)=delete;CommandBatch& operator=(const CommandBatch&)=delete;
    VkCommandBuffer handle() const {return command_;}
    void begin();void end();
    void hostReadBarrier(VkPipelineStageFlags writerStage,VkAccessFlags writerAccess);
private:DeviceContext& context_;VkCommandPool pool_{};VkCommandBuffer command_{};
};
// Vulkan1.2 native timeline only, not fence/busy-loop emulation. Caller owns
// submission ordering and retires all GPU users before destroying this object.
class TimelineSemaphore {
public:
    explicit TimelineSemaphore(DeviceContext&,uint64_t initialValue=0);
    ~TimelineSemaphore();TimelineSemaphore(const TimelineSemaphore&)=delete;
    TimelineSemaphore& operator=(const TimelineSemaphore&)=delete;
    VkSemaphore handle() const {return semaphore_;}
    uint64_t counter() const;
    VkResult wait(uint64_t value,uint64_t timeoutNs) const;
    void signal(uint64_t value);
private:
    DeviceContext& context_;VkSemaphore semaphore_{};
    PFN_vkGetSemaphoreCounterValue counter_{};PFN_vkWaitSemaphores wait_{};PFN_vkSignalSemaphore signal_{};
};
}
