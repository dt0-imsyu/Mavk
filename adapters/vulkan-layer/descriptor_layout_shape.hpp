// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <vulkan/vulkan.h>

inline bool ordinaryDescriptorLayout(const VkDescriptorSetLayoutCreateInfo& info){
    if(info.flags||info.bindingCount>1024)return false;
    if(!info.pNext)return true;
    const auto flags=static_cast<const VkDescriptorSetLayoutBindingFlagsCreateInfo*>(info.pNext);
    if(flags->sType!=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO||flags->pNext)return false;
    if(!flags->bindingCount)return true;
    if(flags->bindingCount!=info.bindingCount||!flags->pBindingFlags)return false;
    for(uint32_t i=0;i<flags->bindingCount;i++)if(flags->pBindingFlags[i])return false;
    return true;
}

inline bool deferredDescriptorLayout(const VkDescriptorSetLayoutCreateInfo& info){
    if(info.flags!=VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT||info.bindingCount>1024||!info.pNext||(info.bindingCount&&!info.pBindings))return false;
    const auto flags=static_cast<const VkDescriptorSetLayoutBindingFlagsCreateInfo*>(info.pNext);
    if(flags->sType!=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO||flags->pNext||flags->bindingCount!=info.bindingCount||(flags->bindingCount&&!flags->pBindingFlags))return false;
    constexpr VkDescriptorBindingFlags allowed=VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT|VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT|VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT|VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
    for(uint32_t i=0;i<info.bindingCount;i++)if((flags->pBindingFlags[i]&~allowed)||info.pBindings[i].pImmutableSamplers)return false;
    return true;
}
