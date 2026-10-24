#include <mavk/compat/vulkan_descriptor_updates.hpp>
#include <iostream>
using namespace mavk::compat;
unsigned checks=0,negatives=0;
void check(bool pass){++checks;if(!pass)throw std::runtime_error("empty descriptor assertion");}
template<class F>void reject(F f){try{f();}catch(const std::exception&){++negatives;return;}throw std::runtime_error("invalid empty descriptor accepted");}
int main(){try{
    DescriptorState empty({});auto first=empty.snapshot(GuardMetadataAbi::IndexAndInit);check(first->bindings.empty()&&first->values.empty());check(first->metadata==std::vector<uint32_t>{1});check(first==empty.snapshot(GuardMetadataAbi::IndexAndInit));check(empty.snapshot(GuardMetadataAbi::BufferRanges)->metadata==std::vector<uint32_t>{0});
    reject([&]{empty.setNull(0,0,0);});reject([&]{first->at(0,0,0);});reject([&]{DescriptorState bad({}, {0,1});});
    VulkanDescriptorResources resources;VulkanDescriptorUpdates updates(resources);auto set=reinterpret_cast<VkDescriptorSet>(uintptr_t(1));VkDescriptorSetLayoutCreateInfo ci{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};updates.registerSet(set,ci);check(updates.snapshot(set,GuardMetadataAbi::IndexAndInit)->values.empty());
    auto combined=updates.snapshotBindings({set,VK_NULL_HANDLE,set},GuardMetadataAbi::IndexAndInit);check(combined->bindings.empty()&&combined->metadata==first->metadata);check(combined==updates.snapshotBindings({set,VK_NULL_HANDLE,set},GuardMetadataAbi::IndexAndInit));updates.update(0,nullptr,0,nullptr);
    reject([&]{updates.snapshotBindings({VK_NULL_HANDLE,VK_NULL_HANDLE},GuardMetadataAbi::IndexAndInit);});
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=set;write.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;write.descriptorCount=1;VkDescriptorBufferInfo buffer{};write.pBufferInfo=&buffer;reject([&]{updates.update(1,&write,0,nullptr);});
    VkDescriptorSetLayoutBinding reserved{12,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,0,0,nullptr};ci.bindingCount=1;ci.pBindings=&reserved;auto second=reinterpret_cast<VkDescriptorSet>(uintptr_t(2));updates.registerSet(second,ci);check(updates.snapshot(second,GuardMetadataAbi::IndexAndInit)->values.empty());
    updates.unregisterSet(set);reject([&]{updates.snapshot(set,GuardMetadataAbi::IndexAndInit);});check(combined->values.empty());
    ci.bindingCount=0;ci.pBindings=nullptr;ci.flags=VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;reject([&]{updates.registerSet(set,ci);});
    std::cout<<"EMPTY_DESCRIPTORS_HOST_PASS checks="<<checks<<" negatives="<<negatives<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
