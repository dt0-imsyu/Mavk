// Host-only internal state unit test. No Vulkan loader/device/GPU success claim.
#include "../windows/icd.cpp"
#include <cassert>
#include <limits>

int main(){
    Layout matrix{64,VK_SHADER_STAGE_VERTEX_BIT};Command command{};command.recording=true;
    uint32_t data[16];for(uint32_t i=0;i<16;i++)data[i]=0x7fc00000u+i;
    const auto h=handle<VkCommandBuffer>(&command);
    vkCmdPushConstants(h,handle<VkPipelineLayout>(&matrix),matrix.stages,0,32,data);
    vkCmdPushConstants(h,handle<VkPipelineLayout>(&matrix),matrix.stages,32,32,data+8);
    assert(!command.bad&&command.pushBytes==64&&command.pushStages==matrix.stages);
    assert(!std::memcmp(command.push,data,64)); // Byte-preserving, including NaN bit patterns.
    unsigned rejected=0;
    auto reject=[&](VkShaderStageFlags stages,uint32_t offset,uint32_t size){
        command.bad=false;uint8_t before[64];std::memcpy(before,command.push,64);
        vkCmdPushConstants(h,handle<VkPipelineLayout>(&matrix),stages,offset,size,data);
        assert(command.bad&&!std::memcmp(before,command.push,64));rejected++;
    };
    reject(matrix.stages,0,0);reject(matrix.stages,0,3);reject(matrix.stages,2,4);
    reject(matrix.stages,64,4);reject(matrix.stages,60,8);
    reject(matrix.stages,std::numeric_limits<uint32_t>::max(),4);
    reject(VK_SHADER_STAGE_FRAGMENT_BIT,0,4);
    command.recording=false;reject(matrix.stages,0,4);command.recording=true;
    command.drew=true;reject(matrix.stages,0,4);command.drew=false;
    Layout other{16,matrix.stages};command.bad=false;
    vkCmdPushConstants(h,handle<VkPipelineLayout>(&other),other.stages,0,16,data);
    assert(command.bad&&!std::memcmp(command.push,data,64));rejected++;
    VkPipelineLayoutCreateInfo info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    VkPushConstantRange range{matrix.stages,0,64};info.pushConstantRangeCount=1;info.pPushConstantRanges=&range;
    VkPipelineLayout layout{};assert(vkCreatePipelineLayout({},&info,nullptr,&layout)==VK_SUCCESS);
    assert(as<Layout>(layout)->bytes==64);vkDestroyPipelineLayout({},layout,nullptr);
    range.size=68;assert(vkCreatePipelineLayout({},&info,nullptr,&layout)==Unsupported&&!layout);rejected++;
    VkDescriptorSetLayoutCreateInfo descriptor{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    VkDescriptorSetLayout ds{};assert(vkCreateDescriptorSetLayout({},&descriptor,nullptr,&ds)==VK_SUCCESS);
    vkDestroyDescriptorSetLayout({},ds,nullptr);descriptor.bindingCount=1;
    assert(vkCreateDescriptorSetLayout({},&descriptor,nullptr,&ds)==Unsupported&&!ds);rejected++;
    VkPipelineCacheCreateInfo cache{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};VkPipelineCache pc{};
    assert(vkCreatePipelineCache({},&cache,nullptr,&pc)==VK_SUCCESS);vkDestroyPipelineCache({},pc,nullptr);
    cache.initialDataSize=4;assert(vkCreatePipelineCache({},&cache,nullptr,&pc)==Unsupported&&!pc);rejected++;
    // Actual ICD recording code: each draw must retain its own push snapshot.
    Image color{64,64};Image depth{64,64,nullptr,VK_FORMAT_D32_SFLOAT};Frame frame{&color,&depth};
    Pipeline pipeline{};pipeline.stride=24;pipeline.pushLayout=matrix;pipeline.depth=true;
    Buffer vertexBuffer{},indexBuffer{};Command scene{};scene.recording=true;scene.inPass=true;
    scene.frame=&frame;scene.pipeline=&pipeline;scene.vertices=&vertexBuffer;scene.indices=&indexBuffer;scene.viewport=scene.scissor=true;
    float transform[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};const auto sh=handle<VkCommandBuffer>(&scene);
    vkCmdPushConstants(sh,handle<VkPipelineLayout>(&matrix),matrix.stages,0,64,transform);
    vkCmdDrawIndexed(sh,3,1,0,0,0);assert(!scene.bad&&scene.indexedDraws.size()==1);
    transform[0]=.5f;vkCmdPushConstants(sh,handle<VkPipelineLayout>(&matrix),matrix.stages,0,64,transform);
    vkCmdDrawIndexed(sh,3,1,3,0,0);assert(!scene.bad&&scene.indexedDraws.size()==2);
    float first=0,second=0;std::memcpy(&first,scene.indexedDraws[0].push.data(),4);std::memcpy(&second,scene.indexedDraws[1].push.data(),4);
    assert(first==1&&second==.5f&&scene.indexedDraws[0].firstIndex==0&&scene.indexedDraws[1].firstIndex==3);
    while(scene.indexedDraws.size()<16)vkCmdDrawIndexed(sh,3,1,0,0,0);
    vkCmdDrawIndexed(sh,3,1,0,0,0);assert(scene.bad&&scene.indexedDraws.size()==16);rejected++;
    std::printf("ICD_PUSH_SCENE_STATE_CPU_PASS rejected=%u (no loader/device/GPU executed)\n",rejected);
}
