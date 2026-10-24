#include <mavk/compat/vulkan_descriptor_updates.hpp>
#include <iostream>
using namespace mavk::compat;
unsigned checks=0,negatives=0;
void check(bool yes){++checks;if(!yes)throw std::runtime_error("selected binding assertion");}
template<class F>void reject(F f){try{f();}catch(const std::exception&){++negatives;return;}throw std::runtime_error("selected binding rejection missing");}
template<class T>T handle(uintptr_t x){return reinterpret_cast<T>(x);}
int main(){try{
    VulkanDescriptorResources r;auto owner=std::make_shared<int>(1);auto buffer=handle<VkBuffer>(1),dead=handle<VkBuffer>(2);
    r.registerBuffer(buffer,4096,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,owner);r.registerBuffer(dead,4096,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,owner);
    VulkanDescriptorUpdates u(r,{},true);auto set=handle<VkDescriptorSet>(10);
    VkDescriptorSetLayoutBinding bs[]={{1,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,2,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{3,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,2,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{8,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}};
    VkDescriptorSetLayoutCreateInfo l{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};l.bindingCount=3;l.pBindings=bs;u.registerSet(set,l);
    VkDescriptorBufferInfo info[]={{buffer,0,16},{buffer,16,16}};VkWriteDescriptorSet w{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};w.dstSet=set;w.dstBinding=3;w.descriptorCount=1;w.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;w.pBufferInfo=info;u.update(1,&w,0,nullptr);
    reject([&]{u.snapshotUsedBindings({set},GuardMetadataAbi::IndexAndInit,{{0,3}});});w.descriptorCount=2;u.update(1,&w,0,nullptr);
    const std::vector<std::pair<uint32_t,uint32_t>> used{{0,3}};auto snapshot=[&]{return u.snapshotUsedBindings({set},GuardMetadataAbi::IndexAndInit,used);};
    auto a=snapshot();check(a->bindings.size()==1&&a->values[0].size()==2);check(snapshot()==a);reject([&]{u.snapshot(set,GuardMetadataAbi::IndexAndInit);});
    u.update(1,&w,0,nullptr);check(snapshot()==a);
    auto range=u.snapshotUsedBindings({VK_NULL_HANDLE,set},GuardMetadataAbi::BufferRanges,{{1,3}});check(range->bindings.size()==1&&range->bindings[0].set==1&&range->at(1,3,1).range==16);check(u.snapshotUsedBindings({VK_NULL_HANDLE,set},GuardMetadataAbi::BufferRanges,{{1,3}})==range);
    reject([&]{u.snapshotUsedBindings({set},static_cast<GuardMetadataAbi>(99),used);});
    reject([&]{u.snapshotUsedBindings({set},GuardMetadataAbi::IndexAndInit,{{0,1}});});
    reject([&]{u.snapshotUsedBindings({set},GuardMetadataAbi::IndexAndInit,{{0,3},{0,3}});});
    reject([&]{u.snapshotUsedBindings({set},GuardMetadataAbi::IndexAndInit,{{0,8},{0,3}});});
    reject([&]{u.snapshotUsedBindings({set},GuardMetadataAbi::IndexAndInit,{{1,3}});});
    reject([&]{u.snapshotUsedBindings({set},GuardMetadataAbi::IndexAndInit,{});});
    reject([&]{u.snapshotUsedBindings({set},GuardMetadataAbi::IndexAndInit,{{0,99}});});
    w.dstBinding=8;w.descriptorCount=1;VkDescriptorBufferInfo deadInfo{dead,0,16};w.pBufferInfo=&deadInfo;u.update(1,&w,0,nullptr);r.unregisterBuffer(dead);auto b=snapshot();check(b->bindings.size()==1);check(b->at(0,3,0).owner==a->at(0,3,0).owner);
    reject([&]{u.snapshotUsedBindings({set},GuardMetadataAbi::IndexAndInit,{{0,8}});});
    w.dstBinding=3;w.descriptorCount=2;w.pBufferInfo=info;info[0].offset=32;u.update(1,&w,0,nullptr);auto c=snapshot();check(c!=b&&c->at(0,3,0).offset==32);check(a->at(0,3,0).offset==0);
    r.unregisterBuffer(buffer);reject([&]{snapshot();});
    u.unregisterSet(set);reject([&]{snapshot();});u.registerSet(set,l);reject([&]{snapshot();});
    descriptor_detail::Layouts native;native.sets.resize(2);native.bindings={{0,3,2,DescriptorKind::StorageBuffer}};native.dynamicBindings={{0,1},{0,3},{0,8}};
    native.configureAllocation({{0,8,1,DescriptorKind::StorageBuffer},{0,3,2,DescriptorKind::StorageBuffer},{0,1,2,DescriptorKind::UniformBuffer}});
    check(native.allocationBindings.size()==3&&native.dynamicSlots.size()==5);check(native.dynamicSlots[0].resource==SIZE_MAX&&native.dynamicSlots[1].resource==SIZE_MAX);check(native.dynamicSlots[2].resource==0&&native.dynamicSlots[3].resource==1&&native.dynamicSlots[4].resource==SIZE_MAX);
    reject([&]{native.configureAllocation({{0,3,1,DescriptorKind::StorageBuffer}});});
    reject([&]{native.configureAllocation({{0,3,2,DescriptorKind::UniformBuffer}});});
    reject([&]{native.configureAllocation({{1,3,2,DescriptorKind::StorageBuffer}});});
    std::cout<<"SELECTED_BINDINGS_HOST_PASS checks="<<checks<<" negatives="<<negatives<<"\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
