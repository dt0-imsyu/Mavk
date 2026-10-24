// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <vulkan/vulkan.h>
#include <stdexcept>
#include <string>

inline const void* shaderStageNativeChain(const VkPipelineShaderStageCreateInfo& stage){
    if(!stage.pNext)return nullptr;
    const auto next=static_cast<const VkBaseInStructure*>(stage.pNext);
    if(!stage.module&&next->sType==VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO&&!next->pNext)return nullptr;
    if(stage.module&&next->sType==VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO&&!next->pNext)
        return stage.pNext;
    throw std::invalid_argument("extended shader stage unsupported sType="+std::to_string(next->sType));
}

inline VkPipelineShaderStageCreateInfo shaderStageWithModule(const VkPipelineShaderStageCreateInfo& stage,VkShaderModule module){
    auto native=stage;
    native.pNext=shaderStageNativeChain(stage);
    native.module=module;
    return native;
}
