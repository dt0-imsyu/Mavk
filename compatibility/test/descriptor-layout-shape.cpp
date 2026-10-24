// SPDX-License-Identifier: Apache-2.0
#include "../../adapters/vulkan-layer/descriptor_layout_shape.hpp"
#include <cstdio>
#include <initializer_list>

static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL line=%d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    VkDescriptorSetLayoutCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    CHECK(ordinaryDescriptorLayout(info));
    info.bindingCount=2;CHECK(ordinaryDescriptorLayout(info));
    VkDescriptorSetLayoutBindingFlagsCreateInfo chain{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO};
    info.pNext=&chain;CHECK(ordinaryDescriptorLayout(info));
    VkDescriptorBindingFlags flags[2]{};chain.bindingCount=2;chain.pBindingFlags=flags;
    CHECK(ordinaryDescriptorLayout(info));
    for(auto bit:{VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT}){
        flags[1]=bit;CHECK(!ordinaryDescriptorLayout(info));
    }
    flags[1]=0;chain.bindingCount=1;CHECK(!ordinaryDescriptorLayout(info));
    chain.bindingCount=2;chain.pBindingFlags=nullptr;CHECK(!ordinaryDescriptorLayout(info));
    chain.pBindingFlags=flags;chain.pNext=&info;CHECK(!ordinaryDescriptorLayout(info));chain.pNext=nullptr;
    info.flags=VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;CHECK(!ordinaryDescriptorLayout(info));
    info.flags=VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;CHECK(!ordinaryDescriptorLayout(info));info.flags=0;
    VkBaseInStructure unknown{VK_STRUCTURE_TYPE_APPLICATION_INFO,nullptr};info.pNext=&unknown;CHECK(!ordinaryDescriptorLayout(info));
    info.pNext=nullptr;info.bindingCount=1025;CHECK(!ordinaryDescriptorLayout(info));
    VkDescriptorSetLayoutBinding bindings[2]{};bindings[0].descriptorType=bindings[1].descriptorType=VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    info.bindingCount=2;info.pBindings=bindings;info.flags=VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;info.pNext=&chain;
    chain.bindingCount=2;chain.pBindingFlags=flags;CHECK(deferredDescriptorLayout(info));
    flags[0]=VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT|VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;CHECK(deferredDescriptorLayout(info));
    VkSampler immutable=VK_NULL_HANDLE;bindings[0].pImmutableSamplers=&immutable;CHECK(!deferredDescriptorLayout(info));bindings[0].pImmutableSamplers=nullptr;
    flags[0]=~VkDescriptorBindingFlags(0);CHECK(!deferredDescriptorLayout(info));flags[0]=0;
    chain.pNext=&unknown;CHECK(!deferredDescriptorLayout(info));chain.pNext=nullptr;
    info.flags=VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;CHECK(!deferredDescriptorLayout(info));
    std::printf("DESCRIPTOR_LAYOUT_SHAPE_PASS checks=%u\n",checks);
}
