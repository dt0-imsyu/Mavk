#include <mavk/compat/vulkan_descriptor_updates.hpp>
#include <iostream>
using namespace mavk::compat;
unsigned checks=0,negatives=0;
void check(bool yes){checks++;if(!yes)throw std::runtime_error("Vulkan descriptor assertion");}
template<class F>void reject(F f){try{f();}catch(const std::exception&){negatives++;return;}throw std::runtime_error("invalid Vulkan descriptor accepted");}
template<class T>T handle(uintptr_t value){if constexpr(std::is_pointer_v<T>)return reinterpret_cast<T>(value);else return T(value);}
int main(){try{
    using A=GuardMetadataAbi;auto owner=std::make_shared<int>(1);VulkanDescriptorResources resources;
    auto a=handle<VkBuffer>(1),b=handle<VkBuffer>(2);auto view=handle<VkImageView>(3);auto sampler=handle<VkSampler>(4);
    resources.registerBuffer(a,1024,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,owner);resources.registerBuffer(b,2048,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,owner);
    resources.registerImageView(view,VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_STORAGE_BIT,owner);resources.registerSampler(sampler,owner);
    VulkanDescriptorUpdates updates(resources,{16,8},true);auto one=handle<VkDescriptorSet>(10),two=handle<VkDescriptorSet>(11),blank=handle<VkDescriptorSet>(12);
    VkDescriptorSetLayoutBinding bindings[]={{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,2,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{2,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,2,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}};
    VkDescriptorSetLayoutCreateInfo layout{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};layout.bindingCount=3;layout.pBindings=bindings;updates.registerSet(one,layout);updates.registerSet(two,layout);updates.registerSet(blank,layout);
    VkDescriptorBufferInfo info[]={{a,0,32},{b,8,64},{a,16,32},{b,24,64},{a,32,VK_WHOLE_SIZE}};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=one;write.dstBinding=0;write.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;write.descriptorCount=5;write.pBufferInfo=info;
    VkCopyDescriptorSet copy{VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET};copy.srcSet=one;copy.srcBinding=0;copy.dstSet=two;copy.dstBinding=0;copy.descriptorCount=5;
    updates.update(1,&write,1,&copy);auto first=updates.snapshot(one,A::IndexAndInit),second=updates.snapshot(two,A::IndexAndInit);
    check(first->at(0,0,1).offset==8);check(second->at(0,1,0).offset==16);check(second->at(0,2,1).range==992);
    updates.update(1,&write,0,nullptr);check(first==updates.snapshot(one,A::IndexAndInit));
    VkCopyDescriptorSet chain[]={copy,copy};chain[0].descriptorCount=1;chain[1].srcSet=two;chain[1].dstSet=two;chain[1].srcArrayElement=0;chain[1].dstArrayElement=1;chain[1].descriptorCount=1;updates.update(0,nullptr,2,chain);check(updates.snapshot(two,A::IndexAndInit)->at(0,0,1).offset==0);
    auto stable=updates.snapshot(one,A::IndexAndInit);auto bad=write;bad.descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;reject([&]{updates.update(1,&bad,0,nullptr);});
    VkDescriptorBufferInfo invalid{handle<VkBuffer>(999),0,32};VkWriteDescriptorSet batch[]={write,write};batch[0].descriptorCount=1;batch[0].pBufferInfo=&info[1];batch[1].descriptorCount=1;batch[1].pBufferInfo=&invalid;reject([&]{updates.update(2,batch,0,nullptr);});check(stable==updates.snapshot(one,A::IndexAndInit));
    auto overlap=copy;overlap.dstSet=one;overlap.dstArrayElement=1;overlap.descriptorCount=2;reject([&]{updates.update(0,nullptr,1,&overlap);});
    auto undefined=copy;undefined.srcSet=blank;undefined.descriptorCount=1;updates.update(0,nullptr,1,&undefined);reject([&]{updates.snapshot(two,A::IndexAndInit);});updates.update(0,nullptr,1,&copy);check(updates.snapshot(two,A::IndexAndInit)->at(0,0,0).status==DescriptorStatus::Bound);
    VkDescriptorBufferInfo nullInfo{VK_NULL_HANDLE,0,VK_WHOLE_SIZE};auto nullWrite=write;nullWrite.descriptorCount=1;nullWrite.pBufferInfo=&nullInfo;updates.update(1,&nullWrite,0,nullptr);check(updates.snapshot(one,A::IndexAndInit)->at(0,0,0).status==DescriptorStatus::Null);
    nullInfo.range=32;reject([&]{updates.update(1,&nullWrite,0,nullptr);});nullInfo.range=VK_WHOLE_SIZE;nullInfo.offset=8;reject([&]{updates.update(1,&nullWrite,0,nullptr);});nullInfo.offset=0;
    VulkanDescriptorUpdates closed(resources);closed.registerSet(one,layout);reject([&]{closed.update(1,&nullWrite,0,nullptr);});
    auto imageSet=handle<VkDescriptorSet>(20);VkDescriptorSetLayoutBinding imageBinding{0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr};layout.bindingCount=1;layout.pBindings=&imageBinding;updates.registerSet(imageSet,layout);
    VkDescriptorImageInfo imageInfo{sampler,view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};VkWriteDescriptorSet imageWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};imageWrite.dstSet=imageSet;imageWrite.dstBinding=0;imageWrite.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;imageWrite.descriptorCount=1;imageWrite.pImageInfo=&imageInfo;
    updates.update(1,&imageWrite,0,nullptr);auto imageSnapshot=updates.snapshot(imageSet,A::IndexAndInit);updates.update(1,&imageWrite,0,nullptr);check(imageSnapshot==updates.snapshot(imageSet,A::IndexAndInit));
    auto native=VulkanDescriptorResources::resolve(imageSnapshot->bindings[0],0,imageSnapshot->at(0,0,0));check(native.image.imageView==view&&native.image.sampler==sampler);
    imageInfo.imageView=VK_NULL_HANDLE;updates.update(1,&imageWrite,0,nullptr);check(updates.snapshot(imageSet,A::IndexAndInit)->at(0,0,0).status==DescriptorStatus::Null);
    imageInfo.sampler=VK_NULL_HANDLE;reject([&]{updates.update(1,&imageWrite,0,nullptr);});imageInfo.sampler=sampler;imageInfo.imageView=view;
    reject([&]{resources.registerBuffer(a,16,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,owner);});reject([&]{updates.registerSet(one,layout);});
    bad=write;bad.sType=VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;reject([&]{updates.update(1,&bad,0,nullptr);});bad=write;bad.pNext=&layout;reject([&]{updates.update(1,&bad,0,nullptr);});reject([&]{updates.update(1,nullptr,0,nullptr);});
    auto stageSet=handle<VkDescriptorSet>(30);VkDescriptorSetLayoutBinding mismatch[]={{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT,nullptr}};layout.bindingCount=2;layout.pBindings=mismatch;updates.registerSet(stageSet,layout);bad=write;bad.dstSet=stageSet;bad.descriptorCount=2;reject([&]{updates.update(1,&bad,0,nullptr);});
    auto zeroSet=handle<VkDescriptorSet>(31);VkDescriptorSetLayoutBinding zero[]={{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{1,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0,0,nullptr},{2,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}};layout.bindingCount=3;layout.pBindings=zero;updates.registerSet(zeroSet,layout);bad=write;bad.dstSet=zeroSet;bad.descriptorCount=2;updates.update(1,&bad,0,nullptr);check(updates.snapshot(zeroSet,A::IndexAndInit)->at(0,2,0).offset==8);
    auto nullSnapshot=updates.snapshot(imageSet,A::IndexAndInit);check(nullSnapshot->at(0,0,0).owner!=nullptr);resources.unregisterSampler(sampler);reject([&]{updates.snapshot(imageSet,A::IndexAndInit);});
    resources.unregisterBuffer(a);updates.update(0,nullptr,1,&copy);reject([&]{updates.snapshot(two,A::IndexAndInit);});check(first->at(0,0,0).owner!=nullptr);
    bool deleted=false;std::weak_ptr<int> weak;
    {VulkanDescriptorResources r;VulkanDescriptorUpdates u(r);auto object=std::shared_ptr<int>(new int(7),[&](int* p){deleted=true;delete p;});weak=object;r.registerBuffer(a,16,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,object);object.reset();VkDescriptorSetLayoutBinding l{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr};layout.bindingCount=1;layout.pBindings=&l;u.registerSet(one,layout);VkDescriptorBufferInfo i{a,0,16};auto w=write;w.descriptorCount=1;w.pBufferInfo=&i;u.update(1,&w,0,nullptr);auto snap=u.snapshot(one,A::IndexAndInit);r.unregisterBuffer(a);u.unregisterSet(one);check(!deleted&&!weak.expired());snap.reset();check(deleted&&weak.expired());}
    std::cout<<"VULKAN_DESCRIPTOR_UPDATES_PASS checks="<<checks<<" negatives="<<negatives<<"\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
