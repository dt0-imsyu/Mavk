#include <mavk/compat/vulkan_descriptor_updates.hpp>
#include <iostream>
using namespace mavk::compat;
unsigned checks=0,rejections=0;
void check(bool pass){++checks;if(!pass)throw std::runtime_error("dynamic descriptor assertion");}
template<class F>void reject(F run){try{run();}catch(const std::invalid_argument&){++rejections;return;}throw std::runtime_error("invalid dynamic offset accepted");}
int main(){try{
    check(descriptor_api::kind(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)==DescriptorKind::UniformBuffer);
    check(descriptor_api::kind(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)==DescriptorKind::StorageBuffer);
    check(descriptor_api::dynamic(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC));check(descriptor_api::dynamic(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC));check(!descriptor_api::dynamic(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER));
    descriptor_detail::Layouts schema;schema.dynamicBindings={{0,3},{1,9}};
    check(schema.nativeType({0,3,2,DescriptorKind::UniformBuffer})==VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
    check(schema.nativeType({1,9,2,DescriptorKind::StorageBuffer})==VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC);
    check(schema.nativeType({0,9,1,DescriptorKind::StorageBuffer})==VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    VkPhysicalDeviceLimits limits{};limits.minUniformBufferOffsetAlignment=256;limits.minStorageBufferOffsetAlignment=16;
    NativeDescriptorResource buffer;buffer.buffer.buffer=reinterpret_cast<VkBuffer>(uintptr_t(1));buffer.buffer.offset=256;buffer.buffer.range=256;buffer.bufferBytes=1024;
    auto validate=[&](DescriptorKind kind,DescriptorStatus status,uint32_t offset){descriptor_detail::validateDynamicBufferOffset(buffer,kind,status,offset,limits);};
    validate(DescriptorKind::UniformBuffer,DescriptorStatus::Bound,0);++checks;
    validate(DescriptorKind::UniformBuffer,DescriptorStatus::Bound,512);++checks;
    validate(DescriptorKind::StorageBuffer,DescriptorStatus::Bound,16);++checks;
    reject([&]{validate(DescriptorKind::UniformBuffer,DescriptorStatus::Bound,16);});
    reject([&]{validate(DescriptorKind::StorageBuffer,DescriptorStatus::Bound,1);});
    reject([&]{validate(DescriptorKind::UniformBuffer,DescriptorStatus::Bound,768);});
    reject([&]{validate(DescriptorKind::StorageBuffer,DescriptorStatus::Bound,UINT32_MAX-15);});
    reject([&]{validate(DescriptorKind::SampledImage,DescriptorStatus::Bound,0);});
    reject([&]{validate(DescriptorKind::UniformBuffer,DescriptorStatus::Unwritten,0);});
    validate(DescriptorKind::UniformBuffer,DescriptorStatus::Null,0);++checks;
    reject([&]{validate(DescriptorKind::UniformBuffer,DescriptorStatus::Null,256);});
    buffer.buffer.range=768;validate(DescriptorKind::UniformBuffer,DescriptorStatus::Bound,0);++checks;
    reject([&]{validate(DescriptorKind::UniformBuffer,DescriptorStatus::Bound,256);});
    buffer.buffer.range=0;reject([&]{validate(DescriptorKind::UniformBuffer,DescriptorStatus::Bound,0);});
    buffer.buffer.offset=UINT64_MAX;reject([&]{validate(DescriptorKind::UniformBuffer,DescriptorStatus::Bound,0);});
    buffer.buffer.offset=0;buffer.buffer.buffer=VK_NULL_HANDLE;reject([&]{validate(DescriptorKind::UniformBuffer,DescriptorStatus::Bound,0);});
    {
        auto owner=std::make_shared<int>(1);VulkanDescriptorResources resources;
        auto handle=reinterpret_cast<VkBuffer>(uintptr_t(1));resources.registerBuffer(handle,1024,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT|VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,owner);
        VulkanDescriptorUpdates updates(resources,{16,16},true);
        auto set=reinterpret_cast<VkDescriptorSet>(uintptr_t(2)),copy=reinterpret_cast<VkDescriptorSet>(uintptr_t(3)),plain=reinterpret_cast<VkDescriptorSet>(uintptr_t(4));
        VkDescriptorSetLayoutBinding bindings[]={{7,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,2,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{3,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}};
        VkDescriptorSetLayoutCreateInfo layout{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};layout.bindingCount=2;layout.pBindings=bindings;updates.registerSet(set,layout);updates.registerSet(copy,layout);
        VkDescriptorBufferInfo infos[]={{handle,16,64},{handle,32,64},{handle,64,VK_WHOLE_SIZE}};VkWriteDescriptorSet writes[2]{};
        for(unsigned i=0;i<2;i++){writes[i].sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;writes[i].dstSet=set;writes[i].dstBinding=i?3:7;writes[i].descriptorType=i?VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;writes[i].descriptorCount=i?1:2;writes[i].pBufferInfo=infos+(i?2:0);}
        updates.update(2,writes,0,nullptr);auto original=updates.snapshot(set,GuardMetadataAbi::IndexAndInit);
        check(original->at(0,7,1).offset==32&&original->at(0,7,1).range==64);check(original->at(0,3,0).range==960);
        check(updates.snapshot(set,GuardMetadataAbi::IndexAndInit)==original);
        VkDescriptorUpdateTemplateEntry entry{7,0,2,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,0,sizeof(VkDescriptorBufferInfo)};
        VkDescriptorUpdateTemplateCreateInfo ci{VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO};ci.templateType=VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET;ci.descriptorSetLayout=reinterpret_cast<VkDescriptorSetLayout>(uintptr_t(5));ci.descriptorUpdateEntryCount=1;ci.pDescriptorUpdateEntries=&entry;
        auto compiled=updates.compileTemplate(set,ci);infos[0].offset=48;updates.updateWithTemplate(set,compiled,infos,2*sizeof(VkDescriptorBufferInfo));
        check(updates.snapshot(set,GuardMetadataAbi::IndexAndInit)->at(0,7,0).offset==48);check(original->at(0,7,0).offset==16);
        VkCopyDescriptorSet copies[2]{};for(unsigned i=0;i<2;i++){auto& c=copies[i];c.sType=VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;c.srcSet=set;c.dstSet=copy;c.srcBinding=c.dstBinding=i?3:7;c.descriptorCount=i?1:2;}updates.update(0,nullptr,2,copies);
        check(updates.snapshot(copy,GuardMetadataAbi::IndexAndInit)->at(0,7,0).offset==48);
        VkDescriptorSetLayoutBinding staticBinding{7,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,2,VK_SHADER_STAGE_COMPUTE_BIT,nullptr};layout.bindingCount=1;layout.pBindings=&staticBinding;updates.registerSet(plain,layout);copies[0].dstSet=plain;reject([&]{updates.update(0,nullptr,1,copies);});
        infos[0]={VK_NULL_HANDLE,0,VK_WHOLE_SIZE};writes[0].descriptorCount=1;updates.update(1,writes,0,nullptr);check(updates.snapshot(set,GuardMetadataAbi::IndexAndInit)->at(0,7,0).status==DescriptorStatus::Null);
        resources.unregisterBuffer(handle);reject([&]{updates.snapshot(copy,GuardMetadataAbi::IndexAndInit);});check(original->at(0,7,1).owner!=nullptr);
    }
    std::cout<<"DYNAMIC_BUFFER_CONTRACT_PASS checks="<<checks<<" rejections="<<rejections<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
