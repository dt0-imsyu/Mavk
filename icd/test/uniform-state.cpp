// Actual ICD descriptor/recording helpers, CPU only; no GPU/runtime PASS.
#include "../windows/icd.cpp"
#include <cassert>
int main(){
    Device device{};const auto dh=handle<VkDevice>(&device);
    VkDescriptorSetLayoutBinding binding{0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT,nullptr};VkDescriptorSetLayoutCreateInfo li{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};li.bindingCount=1;li.pBindings=&binding;
    VkDescriptorSetLayout layout{};assert(vkCreateDescriptorSetLayout(dh,&li,nullptr,&layout)==VK_SUCCESS);
    VkPipelineLayoutCreateInfo pi{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};pi.setLayoutCount=1;pi.pSetLayouts=&layout;VkPipelineLayout pipelineLayout{};assert(vkCreatePipelineLayout(dh,&pi,nullptr,&pipelineLayout)==VK_SUCCESS&&as<Layout>(pipelineLayout)->uniform&&!as<Layout>(pipelineLayout)->bytes);
    VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,2};VkDescriptorPoolCreateInfo pc{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pc.flags=VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;pc.maxSets=2;pc.poolSizeCount=1;pc.pPoolSizes=&size;VkDescriptorPool pool{};assert(vkCreateDescriptorPool(dh,&pc,nullptr,&pool)==VK_SUCCESS);
    VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};ai.descriptorPool=pool;ai.descriptorSetCount=1;ai.pSetLayouts=&layout;VkDescriptorSet set{},other{};assert(vkAllocateDescriptorSets(dh,&ai,&set)==VK_SUCCESS&&vkAllocateDescriptorSets(dh,&ai,&other)==VK_SUCCESS);VkDescriptorSet extra{};assert(vkAllocateDescriptorSets(dh,&ai,&extra)==VK_ERROR_OUT_OF_POOL_MEMORY&&!extra);
    Memory memory{&device,0,std::vector<uint8_t>(228)};Buffer buffer{224,&memory,4,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT};VkDescriptorBufferInfo bi{handle<VkBuffer>(&buffer),32,VK_WHOLE_SIZE};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=set;write.descriptorCount=1;write.descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;write.pBufferInfo=&bi;vkUpdateDescriptorSets(dh,1,&write,0,nullptr);
    Command command{};command.device=&device;command.recording=true;vkCmdBindDescriptorSets(handle<VkCommandBuffer>(&command),VK_PIPELINE_BIND_POINT_GRAPHICS,pipelineLayout,0,1,&set,0,nullptr);assert(!command.bad&&command.descriptor==as<DescriptorSet>(set));
    // CPU writes made AFTER command recording must be visible at submission.
    for(unsigned i=0;i<192;i++)memory.bytes[36+i]=uint8_t(i^0x5a);
    std::vector<uint8_t> snapshot;assert(uniformSnapshot(&command,snapshot)&&snapshot.size()==192);for(unsigned i=0;i<192;i++)assert(snapshot[i]==uint8_t(i^0x5a));
    unsigned rejected=1;
    for(uint64_t offset: {uint64_t(1),uint64_t(223),UINT64_MAX}){bi.offset=offset;vkUpdateDescriptorSets(dh,1,&write,0,nullptr);assert(as<DescriptorSet>(set)->bad&&!uniformSnapshot(&command,snapshot));rejected++;}
    bi.offset=32;for(uint64_t range:{uint64_t(0),uint64_t(17),uint64_t(208),UINT64_MAX-1}){bi.range=range;vkUpdateDescriptorSets(dh,1,&write,0,nullptr);assert(as<DescriptorSet>(set)->bad);rejected++;}
    bi.range=192;vkUpdateDescriptorSets(dh,1,&write,0,nullptr);assert(!as<DescriptorSet>(set)->bad);
    VkDescriptorSet duplicate[]={set,set};assert(vkFreeDescriptorSets(dh,pool,2,duplicate)==Unsupported&&as<DescriptorPool>(pool)->sets.size()==2);rejected++;
    assert(vkFreeDescriptorSets(dh,pool,1,&other)==VK_SUCCESS);assert(vkAllocateDescriptorSets(dh,&ai,&other)==VK_SUCCESS);
    command.drew=true;vkCmdBindDescriptorSets(handle<VkCommandBuffer>(&command),VK_PIPELINE_BIND_POINT_GRAPHICS,pipelineLayout,0,1,&other,0,nullptr);assert(command.bad);rejected++;
    assert(vkResetDescriptorPool(dh,pool,0)==VK_SUCCESS&&as<DescriptorPool>(pool)->sets.empty());
    vkDestroyDescriptorPool(dh,pool,nullptr);vkDestroyPipelineLayout(dh,pipelineLayout,nullptr);vkDestroyDescriptorSetLayout(dh,layout,nullptr);
    VkCommandPoolCreateInfo cp{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};cp.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VkCommandPool commands{};assert(vkCreateCommandPool(dh,&cp,nullptr,&commands)==VK_SUCCESS);
    VkCommandBufferAllocateInfo ca{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ca.commandPool=commands;ca.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ca.commandBufferCount=2;
    VkCommandBuffer pair[2]{};assert(vkAllocateCommandBuffers(dh,&ca,pair)==VK_SUCCESS&&pair[0]!=pair[1]);
    for(auto cmd:pair){auto* c=as<Command>(cmd);auto loader=c->dispatch;c->drew=true;c->ready=true;c->descriptor=reinterpret_cast<DescriptorSet*>(uintptr_t(1));c->copies.push_back({});assert(vkResetCommandBuffer(cmd,0)==VK_SUCCESS&&!c->ready&&!c->drew&&!c->descriptor&&c->copies.empty()&&c->device==&device&&c->pool==as<Pool>(commands));assert(!std::memcmp(&loader,&c->dispatch,sizeof(loader)));assert(vkGetDeviceProcAddr(dh,"vkResetCommandBuffer"));}
    assert(vkResetCommandBuffer(pair[0],2)==Unsupported);rejected++;
    vkFreeCommandBuffers(dh,commands,2,pair);assert(as<Pool>(commands)->commands.empty());vkDestroyCommandPool(dh,commands,nullptr);
    cp.flags=0;assert(vkCreateCommandPool(dh,&cp,nullptr,&commands)==VK_SUCCESS);ca.commandPool=commands;ca.commandBufferCount=1;assert(vkAllocateCommandBuffers(dh,&ca,pair)==VK_SUCCESS);assert(vkResetCommandBuffer(pair[0],0)==Unsupported);rejected++;vkDestroyCommandPool(dh,commands,nullptr);
    std::printf("ICD_UNIFORM_STATE_CPU_PASS rejects=%u range=192 late_mapped_writes=PASS two_command_buffers/reset=PASS (not GPU/guest PASS)\n",rejected);
}
