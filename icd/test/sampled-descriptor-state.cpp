// Actual opt-in ICD descriptor implementation, no GPU or guest PASS.
#define MAVK_EXPERIMENTAL_SAMPLED
#include "../windows/icd.cpp"
#include <cassert>
int main(){
    Device device{};auto d=handle<VkDevice>(&device);
    VkDescriptorSetLayoutBinding bindings[]={{1,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},{0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT,nullptr}};
    VkDescriptorSetLayoutCreateInfo li{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};li.bindingCount=2;li.pBindings=bindings;
    VkDescriptorSetLayout sl{};assert(vkCreateDescriptorSetLayout(d,&li,nullptr,&sl)==VK_SUCCESS);
    VkPipelineLayoutCreateInfo pi{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};pi.setLayoutCount=1;pi.pSetLayouts=&sl;VkPipelineLayout layout{};assert(vkCreatePipelineLayout(d,&pi,nullptr,&layout)==VK_SUCCESS&&as<Layout>(layout)->sampled);
    VkDescriptorPoolSize sizes[]={{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1},{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1}};VkDescriptorPoolCreateInfo pc{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pc.flags=VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;pc.maxSets=1;pc.poolSizeCount=2;pc.pPoolSizes=sizes;VkDescriptorPool pool{};assert(vkCreateDescriptorPool(d,&pc,nullptr,&pool)==VK_SUCCESS);
    VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};ai.descriptorPool=pool;ai.descriptorSetCount=1;ai.pSetLayouts=&sl;VkDescriptorSet set{};assert(vkAllocateDescriptorSets(d,&ai,&set)==VK_SUCCESS);
    Memory memory{&device,0,std::vector<uint8_t>(176)};Buffer buffer{176,&memory,0,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT};Image image{};image.sampled=true;image.owner=&device;View view{};view.image=&image;Sampler sampler{};sampler.owner=&device;
    VkDescriptorBufferInfo bi{handle<VkBuffer>(&buffer),16,148};VkDescriptorImageInfo ii{handle<VkSampler>(&sampler),handle<VkImageView>(&view),VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};VkWriteDescriptorSet writes[2]{};for(auto& w:writes){w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w.dstSet=set;w.descriptorCount=1;}writes[0].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;writes[0].pBufferInfo=&bi;writes[1].dstBinding=1;writes[1].descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;writes[1].pImageInfo=&ii;
    vkUpdateDescriptorSets(d,2,writes,0,nullptr);auto* state=as<DescriptorSet>(set);assert(!state->bad&&state->range==148&&state->imageView==&view&&state->sampler==&sampler);
    Command cmd{};cmd.device=&device;cmd.recording=true;vkCmdBindDescriptorSets(handle<VkCommandBuffer>(&cmd),VK_PIPELINE_BIND_POINT_GRAPHICS,layout,0,1,&set,0,nullptr);assert(!cmd.bad&&cmd.descriptor==state);
    memory.bytes[16]=73;std::vector<uint8_t> bytes;assert(uniformSnapshot(&cmd,bytes)&&bytes.size()==148&&bytes[0]==73);
    unsigned rejected=0;
    for(unsigned mode=0;mode<6;mode++){
        auto bad=writes[1];auto info=ii;bad.pImageInfo=&info;if(mode==0)info.imageLayout=VK_IMAGE_LAYOUT_GENERAL;if(mode==1)info.sampler=VK_NULL_HANDLE;if(mode==2)info.imageView=VK_NULL_HANDLE;if(mode==3)bad.dstBinding=2;if(mode==4)bad.descriptorType=VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;if(mode==5)bad.descriptorCount=2;
        vkUpdateDescriptorSets(d,1,&bad,0,nullptr);assert(state->bad);rejected++;vkUpdateDescriptorSets(d,2,writes,0,nullptr);assert(!state->bad);
    }
    for(VkDeviceSize range:{VkDeviceSize(0),VkDeviceSize(147),VkDeviceSize(164)}){bi.range=range;vkUpdateDescriptorSets(d,1,writes,0,nullptr);assert(state->bad);rejected++;}bi.range=148;vkUpdateDescriptorSets(d,2,writes,0,nullptr);assert(!state->bad);
    assert(vkFreeDescriptorSets(d,pool,1,&set)==VK_SUCCESS);vkDestroyDescriptorPool(d,pool,nullptr);vkDestroyPipelineLayout(d,layout,nullptr);vkDestroyDescriptorSetLayout(d,sl,nullptr);
    std::printf("ICD_SAMPLED_DESCRIPTOR_CPU_PASS rejects=%u range=148 late_writes=PASS NOT_GPU_OR_GUEST_PASS\n",rejected);
}
