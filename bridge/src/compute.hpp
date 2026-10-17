#pragma once
#include <mavk/core.hpp>
#include <memory>
#include <vector>
#include <cstring>
#include <stdexcept>
namespace mavk::bridge {
inline std::vector<uint8_t> dispatchStorage(DeviceContext& c,VkQueue queue,const std::vector<uint32_t>& code,const std::vector<uint8_t>& input,const std::vector<uint8_t>& push,uint32_t groups){
    auto check=[](VkResult r){if(r!=VK_SUCCESS)throw std::runtime_error("Compute Vulkan failure");};
    struct Work {
        DeviceContext& c;VkDescriptorSetLayout descriptors{};VkPipelineLayout layout{};VkDescriptorPool pool{};
        std::unique_ptr<Buffer> buffer;std::unique_ptr<Shader> shader;std::unique_ptr<Pipeline> pipeline;std::unique_ptr<CommandBatch> commands;std::unique_ptr<Fence> fence;
        explicit Work(DeviceContext& context):c(context){}
        ~Work(){vkDeviceWaitIdle(c.vkDevice());commands.reset();fence.reset();pipeline.reset();shader.reset();if(pool)vkDestroyDescriptorPool(c.vkDevice(),pool,nullptr);if(layout)vkDestroyPipelineLayout(c.vkDevice(),layout,nullptr);if(descriptors)vkDestroyDescriptorSetLayout(c.vkDevice(),descriptors,nullptr);}
    } w(c);
    auto device=c.vkDevice();w.buffer=std::make_unique<Buffer>(c,input.size(),VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    std::memcpy(w.buffer->map(),input.data(),input.size());w.buffer->flush();
    VkDescriptorSetLayoutBinding binding{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr};VkDescriptorSetLayoutCreateInfo d{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};d.bindingCount=1;d.pBindings=&binding;check(vkCreateDescriptorSetLayout(device,&d,nullptr,&w.descriptors));
    VkPushConstantRange range{VK_SHADER_STAGE_COMPUTE_BIT,0,uint32_t(push.size())};VkPipelineLayoutCreateInfo l{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};l.setLayoutCount=1;l.pSetLayouts=&w.descriptors;l.pushConstantRangeCount=push.empty()?0:1;l.pPushConstantRanges=&range;check(vkCreatePipelineLayout(device,&l,nullptr,&w.layout));
    w.shader=std::make_unique<Shader>(c,code.data(),code.size()*4);VkComputePipelineCreateInfo p{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};p.layout=w.layout;p.stage={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};p.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;p.stage.module=w.shader->handle();p.stage.pName="main";w.pipeline=std::make_unique<Pipeline>(c,p);
    VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1};VkDescriptorPoolCreateInfo pc{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pc.maxSets=1;pc.poolSizeCount=1;pc.pPoolSizes=&size;check(vkCreateDescriptorPool(device,&pc,nullptr,&w.pool));VkDescriptorSetAllocateInfo a{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};a.descriptorPool=w.pool;a.descriptorSetCount=1;a.pSetLayouts=&w.descriptors;VkDescriptorSet set{};check(vkAllocateDescriptorSets(device,&a,&set));
    VkDescriptorBufferInfo bi{w.buffer->handle(),0,input.size()};VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=set;write.descriptorCount=1;write.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;write.pBufferInfo=&bi;vkUpdateDescriptorSets(device,1,&write,0,nullptr);
    w.commands=std::make_unique<CommandBatch>(c);w.fence=std::make_unique<Fence>(c);w.commands->begin();auto command=w.commands->handle();vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_COMPUTE,w.pipeline->handle());vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_COMPUTE,w.layout,0,1,&set,0,nullptr);if(!push.empty())vkCmdPushConstants(command,w.layout,VK_SHADER_STAGE_COMPUTE_BIT,0,uint32_t(push.size()),push.data());vkCmdDispatch(command,groups,1,1);w.commands->hostReadBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_WRITE_BIT);w.commands->end();VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;check(c.submit(queue,submit,w.fence->handle()));check(w.fence->wait(5'000'000'000ull));w.buffer->invalidate();std::vector<uint8_t> output(input.size());std::memcpy(output.data(),w.buffer->map(),output.size());return output;
}
}
