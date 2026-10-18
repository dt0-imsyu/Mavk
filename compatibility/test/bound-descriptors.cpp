#include <mavk/compat/vulkan_descriptor_updates.hpp>
#include <iostream>
using namespace mavk::compat;
unsigned checks=0,negatives=0;
void check(bool yes){checks++;if(!yes)throw std::runtime_error("bound descriptor assertion");}
template<class F>void reject(F f){try{f();}catch(const std::exception&){negatives++;return;}throw std::runtime_error("invalid bound descriptors accepted");}
template<class T>T handle(uintptr_t n){if constexpr(std::is_pointer_v<T>)return reinterpret_cast<T>(n);else return T(n);}
int main(){try{
    using A=GuardMetadataAbi;auto owner=std::make_shared<int>(1);VulkanDescriptorResources resources;auto a=handle<VkBuffer>(1),b=handle<VkBuffer>(2);
    resources.registerBuffer(a,1024,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,owner);resources.registerBuffer(b,2048,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,owner);
    VulkanDescriptorUpdates updates(resources,{},true);auto one=handle<VkDescriptorSet>(10),two=handle<VkDescriptorSet>(11),blank=handle<VkDescriptorSet>(12);
    VkDescriptorSetLayoutBinding binding{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr};VkDescriptorSetLayoutCreateInfo layout{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};layout.bindingCount=1;layout.pBindings=&binding;updates.registerSet(one,layout);updates.registerSet(two,layout);updates.registerSet(blank,layout);
    VkDescriptorBufferInfo info[]={{a,16,32},{b,8,64}};VkWriteDescriptorSet writes[2]{};for(size_t i=0;i<2;i++){auto& w=writes[i];w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w.dstSet=i?two:one;w.dstBinding=0;w.descriptorCount=1;w.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;w.pBufferInfo=&info[i];}updates.update(2,writes,0,nullptr);
    for(auto abi:{A::IndexAndInit,A::BufferRanges}){auto first=updates.snapshotBindings({one,VK_NULL_HANDLE,two},abi);check(first->bindings.size()==2);check(first->at(0,0,0).offset==16);check(first->at(2,0,0).range==64);check(updates.snapshotBindings({one,VK_NULL_HANDLE,two},abi)==first);}
    auto first=updates.snapshotBindings({one,two},A::IndexAndInit);auto reversed=updates.snapshotBindings({two,one},A::IndexAndInit);check(reversed!=first&&reversed->at(0,0,0).offset==8);check(first->at(0,0,0).offset==16);
    auto duplicated=updates.snapshotBindings({one,one},A::IndexAndInit);check(duplicated->at(1,0,0).owner==duplicated->at(0,0,0).owner);
    VkDescriptorBufferInfo nullInfo{VK_NULL_HANDLE,0,VK_WHOLE_SIZE};writes[1].pBufferInfo=&nullInfo;updates.update(1,&writes[1],0,nullptr);auto changed=updates.snapshotBindings({one,two},A::IndexAndInit);check(changed!=first);check(changed->at(1,0,0).status==DescriptorStatus::Null);check(first->at(1,0,0).status==DescriptorStatus::Bound);check(changed==updates.snapshotBindings({one,two},A::IndexAndInit));
    reject([&]{updates.snapshotBindings({},A::IndexAndInit);});reject([&]{updates.snapshotBindings(std::vector<VkDescriptorSet>(8,one),A::IndexAndInit);});reject([&]{updates.snapshotBindings({VK_NULL_HANDLE},A::IndexAndInit);});reject([&]{updates.snapshotBindings({blank,one},A::IndexAndInit);});reject([&]{updates.snapshotBindings({handle<VkDescriptorSet>(999)},A::IndexAndInit);});
    resources.unregisterBuffer(a);reject([&]{updates.snapshotBindings({one,two},A::IndexAndInit);});check(first->at(0,0,0).owner!=nullptr);
    bool deleted=false;std::weak_ptr<int> weak;
    {VulkanDescriptorResources r;VulkanDescriptorUpdates u(r);auto object=std::shared_ptr<int>(new int(7),[&](int* p){deleted=true;delete p;});weak=object;r.registerBuffer(a,1024,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,object);object.reset();u.registerSet(one,layout);auto w=writes[0];u.update(1,&w,0,nullptr);auto snap=u.snapshotBindings({one},A::IndexAndInit);r.unregisterBuffer(a);u.unregisterSet(one);check(!weak.expired());snap.reset();check(weak.expired()&&deleted);}
    std::cout<<"BOUND_DESCRIPTORS_PASS checks="<<checks<<" negatives="<<negatives<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
