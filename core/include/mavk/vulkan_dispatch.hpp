// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <vulkan/vulkan.h>
#include <stdexcept>
#include <string>

// A layer owns DOWNSTREAM handles,not application-side loader handles. Never
// feed them back into global physical-device trampolines or recursively enter
// the layer for Core-owned fallback/metadata resources.
#define MAVK_INSTANCE_DISPATCH(X) \
 X(vkGetPhysicalDeviceMemoryProperties) X(vkGetPhysicalDeviceProperties) X(vkGetPhysicalDeviceFormatProperties) X(vkGetPhysicalDeviceImageFormatProperties)
#define MAVK_DEVICE_DISPATCH(X) \
 X(vkCreateBuffer) X(vkDestroyBuffer) X(vkGetBufferMemoryRequirements) \
 X(vkAllocateMemory) X(vkFreeMemory) X(vkBindBufferMemory) X(vkMapMemory) X(vkUnmapMemory) \
 X(vkFlushMappedMemoryRanges) X(vkInvalidateMappedMemoryRanges) \
 X(vkCreateImage) X(vkDestroyImage) X(vkGetImageMemoryRequirements) X(vkBindImageMemory) \
 X(vkCreateImageView) X(vkDestroyImageView) X(vkCreateSampler) X(vkDestroySampler) X(vkCreateBufferView) X(vkDestroyBufferView) \
 X(vkCreateShaderModule) X(vkDestroyShaderModule) X(vkCreateComputePipelines) X(vkCreateGraphicsPipelines) X(vkDestroyPipeline) \
 X(vkCreateFence) X(vkDestroyFence) X(vkResetFences) X(vkWaitForFences) \
 X(vkCreateCommandPool) X(vkDestroyCommandPool) X(vkAllocateCommandBuffers) X(vkBeginCommandBuffer) X(vkEndCommandBuffer) X(vkResetCommandBuffer) \
 X(vkQueueSubmit) X(vkDeviceWaitIdle) X(vkCmdPipelineBarrier) X(vkCmdCopyBufferToImage) X(vkCmdClearColorImage) \
 X(vkCreateSemaphore) X(vkDestroySemaphore) \
 X(vkCreateDescriptorSetLayout) X(vkDestroyDescriptorSetLayout) X(vkCreatePipelineLayout) X(vkDestroyPipelineLayout) \
 X(vkCreateDescriptorPool) X(vkDestroyDescriptorPool) X(vkAllocateDescriptorSets) X(vkUpdateDescriptorSets)

namespace mavk {
struct VulkanDispatch {
#define MAVK_DECLARE(name) PFN_##name name{};
    MAVK_INSTANCE_DISPATCH(MAVK_DECLARE)
    MAVK_DEVICE_DISPATCH(MAVK_DECLARE)
#undef MAVK_DECLARE
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr{};
    static VulkanDispatch loader(){
        VulkanDispatch out;
#define MAVK_GLOBAL(name) out.name=&::name;
        MAVK_INSTANCE_DISPATCH(MAVK_GLOBAL)
        MAVK_DEVICE_DISPATCH(MAVK_GLOBAL)
#undef MAVK_GLOBAL
        out.vkGetDeviceProcAddr=&::vkGetDeviceProcAddr;return out;
    }
    static VulkanDispatch next(VkInstance instance,PFN_vkGetInstanceProcAddr gipa,VkDevice device,PFN_vkGetDeviceProcAddr gdpa){
        if(!instance||!device||!gipa||!gdpa)throw std::invalid_argument("Mavk downstream Vulkan dispatch");
        VulkanDispatch out;
#define MAVK_INSTANCE(name) out.name=reinterpret_cast<PFN_##name>(gipa(instance,#name));if(!out.name)throw std::runtime_error("Mavk missing downstream " #name);
        MAVK_INSTANCE_DISPATCH(MAVK_INSTANCE)
#undef MAVK_INSTANCE
#define MAVK_DEVICE(name) out.name=reinterpret_cast<PFN_##name>(gdpa(device,#name));if(!out.name)throw std::runtime_error("Mavk missing downstream " #name);
        MAVK_DEVICE_DISPATCH(MAVK_DEVICE)
#undef MAVK_DEVICE
        out.vkGetDeviceProcAddr=gdpa;return out;
    }
};
}
#undef MAVK_INSTANCE_DISPATCH
#undef MAVK_DEVICE_DISPATCH
