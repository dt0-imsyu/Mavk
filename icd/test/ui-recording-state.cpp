// API recording on constructed internal objects; factories/encoder pending.
// No loader/transport/GPU/guest success claim.
#define MAVK_EXPERIMENTAL_UI_LAYERS
#include "../windows/icd.cpp"
#include <cassert>
#include <cstdio>
int main(){
    Device device{};device.queue.device=&device;Pool pool{};pool.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    Image image{64,48};Frame frame{&image};Command c{};c.device=&device;c.pool=&pool;
    auto command=handle<VkCommandBuffer>(&c);VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    assert(vkBeginCommandBuffer(command,&begin)==VK_SUCCESS);c.frame=&frame;c.inPass=true;
    Pipeline first{},second{};first.stride=second.stride=20;first.pushLayout={64,VK_SHADER_STAGE_VERTEX_BIT,true};second.pushLayout={16,VK_SHADER_STAGE_VERTEX_BIT,true};
    DescriptorPool descriptors{};descriptors.device=&device;DescriptorSet setA{&descriptors},setB{&descriptors};
    Buffer va{128,nullptr,0,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT},vb{256,nullptr,0,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT};
    Buffer ia{64,nullptr,0,VK_BUFFER_USAGE_INDEX_BUFFER_BIT},ib{128,nullptr,0,VK_BUFFER_USAGE_INDEX_BUFFER_BIT};
    const VkViewport viewport{0,0,64,48,0,1};vkCmdSetViewport(command,0,1,&viewport);
    uint32_t values[16];for(unsigned i=0;i<16;i++)values[i]=0x7fc00000u+i;
    auto record=[&](Pipeline& p,DescriptorSet& set,Buffer& v,Buffer& index,VkDeviceSize vo,VkDeviceSize io,uint32_t firstIndex,int32_t base,VkRect2D clip){
        vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,handle<VkPipeline>(&p));
        auto ds=handle<VkDescriptorSet>(&set);vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,handle<VkPipelineLayout>(&p.pushLayout),0,1,&ds,0,nullptr);
        auto vertex=handle<VkBuffer>(&v);vkCmdBindVertexBuffers(command,0,1,&vertex,&vo);vkCmdBindIndexBuffer(command,handle<VkBuffer>(&index),io,VK_INDEX_TYPE_UINT16);
        vkCmdSetScissor(command,0,1,&clip);vkCmdPushConstants(command,handle<VkPipelineLayout>(&p.pushLayout),p.pushLayout.stages,0,p.pushLayout.bytes,values);
        vkCmdDrawIndexed(command,6,1,firstIndex,base,0);
    };
    record(first,setA,va,ia,4,2,3,0,{{0,0},{64,48}});
    values[0]=0x12345678;record(second,setB,vb,ib,12,6,9,17,{{5,7},{19,13}});
    values[0]=0x89abcdef;record(second,setA,vb,ib,20,8,15,-3,{{8,9},{1,2}});
    assert(!c.bad&&c.drawSnapshots.size()==3&&c.indexedDraws.empty());
    const auto& a=c.drawSnapshots[0];const auto& b=c.drawSnapshots[1];const auto& last=c.drawSnapshots[2];
    assert(a.pipeline==&first&&a.descriptor==&setA&&a.vertices==&va&&a.indices==&ia&&a.vertexBufferOffset==4&&a.indexBufferOffset==2&&a.firstIndex==3&&a.baseVertex==0);
    assert(b.pipeline==&second&&b.descriptor==&setB&&b.vertices==&vb&&b.indices==&ib&&b.vertexBufferOffset==12&&b.indexBufferOffset==6&&b.firstIndex==9&&b.baseVertex==17&&b.clip.offset.x==5&&b.clip.extent.height==13);
    assert(a.push.size()==64&&b.push.size()==16&&last.baseVertex==-3&&last.descriptor==&setA);
    assert(get32(a.push.data())==0x7fc00000&&get32(b.push.data())==0x12345678&&get32(last.push.data())==0x89abcdef);
    // Mutating current state or mapped data does not rewrite old bindings;
    // actual buffer bytes remain referenced for submit-time resolution.
    Memory late{};late.bytes.resize(128);va.memory=&late;late.bytes[4]=73;
    assert(a.vertices->memory->bytes[4]==73);std::memset(c.push,0,sizeof(c.push));assert(get32(a.push.data())==0x7fc00000);
    vkCmdEndRenderPass(command);assert(vkEndCommandBuffer(command)==VK_SUCCESS);
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;
    assert(vkQueueSubmit(handle<VkQueue>(&device.queue),1,&submit,VK_NULL_HANDLE)==Unsupported); // incomplete fixture: never fake a last-draw PASS
    assert(vkResetCommandBuffer(command,0)==VK_SUCCESS&&c.drawSnapshots.empty());c.frame=&frame;c.inPass=true;c.recording=true;
    unsigned rejects=0;
    for(auto clip:std::vector<VkRect2D>{{{-1,0},{1,1}},{{0,0},{0,1}},{{63,0},{2,1}},{{1,0},{UINT32_MAX,1}}}){c.bad=false;vkCmdSetScissor(command,0,1,&clip);assert(c.bad);rejects++;}
    c.bad=false;c.scissor=c.viewport=true;c.currentClip={{0,0},{64,48}};c.pipeline=&second;c.descriptor=&setA;c.vertices=&vb;c.indices=&ib;c.pushBytes=16;c.pushStages=VK_SHADER_STAGE_VERTEX_BIT;
    while(c.drawSnapshots.size()<64)vkCmdDrawIndexed(command,6,1,0,0,0);
    vkCmdDrawIndexed(command,6,1,0,0,0);assert(c.bad&&c.drawSnapshots.size()==64);rejects++;
    assert(vkBeginCommandBuffer(command,&begin)==VK_SUCCESS&&c.drawSnapshots.empty()&&!c.directDraw&&!c.scissor);
    std::printf("ICD_UI_RECORDING_STATE_CPU_PASS snapshots=3 rejects=%u mapped_bytes_late=PASS NOT_GPU_OR_GUEST_PASS\n",rejects);
}
