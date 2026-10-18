#include <mavk/compat/vulkan_descriptor_updates.hpp>
#include <iostream>
#include <limits>
using namespace mavk::compat;
unsigned checks=0,negatives=0;
void check(bool yes){checks++;if(!yes)throw std::runtime_error("descriptor template assertion");}
template<class F>void reject(F f){try{f();}catch(const std::invalid_argument&){negatives++;return;}throw std::runtime_error("invalid descriptor template accepted");}
template<class T>T handle(uintptr_t n){if constexpr(std::is_pointer_v<T>)return reinterpret_cast<T>(n);else return T(n);}
int main(){try{
    using A=GuardMetadataAbi;auto owner=std::make_shared<int>(1);VulkanDescriptorResources resources;
    auto a=handle<VkBuffer>(1),b=handle<VkBuffer>(2);auto view=handle<VkImageView>(3);auto sampler=handle<VkSampler>(4);
    resources.registerBuffer(a,1024,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,owner);resources.registerBuffer(b,2048,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,owner);
    resources.registerImageView(view,VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_STORAGE_BIT,owner);resources.registerSampler(sampler,owner);
    VulkanDescriptorUpdates updates(resources,{},true);auto one=handle<VkDescriptorSet>(10),two=handle<VkDescriptorSet>(11),other=handle<VkDescriptorSet>(12);
    VkDescriptorSetLayoutBinding bindings[]={{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,2,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{2,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{3,VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}};
    VkDescriptorSetLayoutCreateInfo layout{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};layout.bindingCount=4;layout.pBindings=bindings;updates.registerSet(one,layout);updates.registerSet(two,layout);
    bindings[0].stageFlags=VK_SHADER_STAGE_VERTEX_BIT;updates.registerSet(other,layout);bindings[0].stageFlags=VK_SHADER_STAGE_COMPUTE_BIT;
    VkDescriptorUpdateTemplateEntry entries[]={{0,0,3,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,32},{2,0,1,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,101,0},{3,0,1,VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,137,0}};
    VkDescriptorUpdateTemplateCreateInfo create{VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO};create.descriptorUpdateEntryCount=3;create.pDescriptorUpdateEntries=entries;create.templateType=VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET;create.descriptorSetLayout=handle<VkDescriptorSetLayout>(99);
    auto compiled=updates.compileTemplate(one,create);check(compiled.requiredBytes()==137+sizeof(VkDescriptorImageInfo));
    std::vector<unsigned char> data(compiled.requiredBytes());VkDescriptorBufferInfo infos[]={{a,0,32},{b,8,64},{a,16,VK_WHOLE_SIZE}};
    for(size_t i=0;i<3;i++)std::memcpy(data.data()+1+32*i,&infos[i],sizeof(infos[i]));
    VkDescriptorImageInfo sampled{sampler,view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},storage{VK_NULL_HANDLE,view,VK_IMAGE_LAYOUT_GENERAL};std::memcpy(data.data()+101,&sampled,sizeof(sampled));std::memcpy(data.data()+137,&storage,sizeof(storage));
    entries[0].offset=99999; // Compiled template does not retain caller entries.
    updates.updateWithTemplate(one,compiled,data.data(),data.size());auto first=updates.snapshot(one,A::IndexAndInit);
    check(first->at(0,0,1).offset==8);check(first->at(0,1,0).range==1008);check(first->at(0,2,0).status==DescriptorStatus::Bound);
    check(VulkanDescriptorResources::resolve(first->bindings[3],0,first->at(0,3,0)).image.imageLayout==VK_IMAGE_LAYOUT_GENERAL);
    updates.updateWithTemplate(one,compiled,data.data(),data.size());check(first==updates.snapshot(one,A::IndexAndInit));
    updates.updateWithTemplate(two,compiled,data.data(),data.size());check(updates.snapshot(two,A::IndexAndInit)->at(0,0,1).offset==8);
    reject([&]{updates.updateWithTemplate(one,compiled,data.data(),data.size()-1);});reject([&]{updates.updateWithTemplate(one,compiled,nullptr,data.size());});
    reject([&]{updates.updateWithTemplate(other,compiled,data.data(),data.size());});reject([&]{updates.updateWithTemplate(one,VulkanDescriptorUpdates::UpdateTemplate{},data.data(),data.size());});
    auto badData=data;auto bad=infos[1];bad.buffer=handle<VkBuffer>(999);std::memcpy(badData.data()+33,&bad,sizeof(bad));reject([&]{updates.updateWithTemplate(one,compiled,badData.data(),badData.size());});check(first==updates.snapshot(one,A::IndexAndInit));
    auto nullData=data;VkDescriptorBufferInfo nullBuffer{VK_NULL_HANDLE,0,VK_WHOLE_SIZE};std::memcpy(nullData.data()+1,&nullBuffer,sizeof(nullBuffer));sampled.imageView=VK_NULL_HANDLE;std::memcpy(nullData.data()+101,&sampled,sizeof(sampled));updates.updateWithTemplate(two,compiled,nullData.data(),nullData.size());auto nulls=updates.snapshot(two,A::IndexAndInit);check(nulls->at(0,0,0).status==DescriptorStatus::Null);check(nulls->at(0,2,0).status==DescriptorStatus::Null&&nulls->at(0,2,0).owner!=nullptr);
    entries[0].offset=1;auto invalid=create;invalid.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;reject([&]{updates.compileTemplate(one,invalid);});invalid=create;invalid.pNext=&layout;reject([&]{updates.compileTemplate(one,invalid);});invalid=create;invalid.flags=1;reject([&]{updates.compileTemplate(one,invalid);});invalid=create;invalid.templateType=VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_PUSH_DESCRIPTORS_KHR;reject([&]{updates.compileTemplate(one,invalid);});invalid=create;invalid.descriptorSetLayout=VK_NULL_HANDLE;reject([&]{updates.compileTemplate(one,invalid);});invalid=create;invalid.descriptorUpdateEntryCount=0;reject([&]{updates.compileTemplate(one,invalid);});invalid=create;invalid.pDescriptorUpdateEntries=nullptr;reject([&]{updates.compileTemplate(one,invalid);});
    auto old=entries[0];entries[0].stride=std::numeric_limits<size_t>::max();reject([&]{updates.compileTemplate(one,create);});entries[0]=old;entries[0].offset=std::numeric_limits<size_t>::max();reject([&]{updates.compileTemplate(one,create);});entries[0]=old;entries[0].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;reject([&]{updates.compileTemplate(one,create);});entries[0]=old;entries[0].descriptorCount=0;reject([&]{updates.compileTemplate(one,create);});entries[0]=old;
    // Zero stride legally repeats one source object across consecutive slots.
    entries[0].stride=0;auto repeated=updates.compileTemplate(one,create);updates.updateWithTemplate(two,repeated,data.data(),data.size());auto repeat=updates.snapshot(two,A::IndexAndInit);check(repeat->at(0,0,1).offset==0&&repeat->at(0,1,0).range==32);
    updates.unregisterSet(one);updates.updateWithTemplate(two,compiled,data.data(),data.size());check(updates.snapshot(two,A::IndexAndInit)->at(0,1,0).offset==16);reject([&]{updates.compileTemplate(one,create);});
    resources.unregisterSampler(sampler);reject([&]{updates.updateWithTemplate(two,compiled,data.data(),data.size());});check(first->at(0,2,0).owner!=nullptr);
    std::cout<<"DESCRIPTOR_TEMPLATES_PASS checks="<<checks<<" negatives="<<negatives<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
