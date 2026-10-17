// CPU API recording + packet oracle, constructed internal pipelines.
// Does not call loader, shader compiler, transport or GPU; factory gate separate.
#define MAVK_EXPERIMENTAL_UI_LAYERS
#include "../windows/icd.cpp"
#include "../../bridge/src/layers.hpp"
#include <cassert>
#include <cstdio>
int main(){
    Device d{};Memory colorMem{&d,0,std::vector<uint8_t>(4096)},depthMem{&d,0,std::vector<uint8_t>(4096)};
    Image image{32,32,&colorMem},depth{32,32,&depthMem,VK_FORMAT_D32_SFLOAT};Frame frame{&image,&depth};
    Memory vma{&d,0,std::vector<uint8_t>(64)},vmb{&d,0,std::vector<uint8_t>(128)},ima{&d,0,std::vector<uint8_t>(6)},imb{&d,0,std::vector<uint8_t>(12)},ubm{&d,0,std::vector<uint8_t>(16)};
    for(size_t i=0;i<vmb.bytes.size();i++)vmb.bytes[i]=uint8_t(i);
    const uint16_t ia[]={0,1,2},ib[]={0,1,2,1,2,3};std::memcpy(ima.bytes.data(),ia,sizeof(ia));std::memcpy(imb.bytes.data(),ib,sizeof(ib));
    Buffer va{64,&vma,0,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT},vb{128,&vmb,0,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT},ixa{6,&ima,0,VK_BUFFER_USAGE_INDEX_BUFFER_BIT},ixb{12,&imb,0,VK_BUFFER_USAGE_INDEX_BUFFER_BIT},ub{16,&ubm,0,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT};
    DescriptorPool dp{};dp.device=&d;DescriptorSet set{&dp};set.buffer=&ub;set.range=16;
    Shader shader;const uint32_t words[]={0x07230203,0x00010000,0,8,0};shader.code.resize(20);std::memcpy(shader.code.data(),words,20);
    Pipeline base{},ui{};base.stride=8;base.depth=true;base.pushLayout.uniform=true;base.attributes={{0,0,VK_FORMAT_R32G32_SFLOAT,0}};
    ui.stride=12;ui.blendMode=2;ui.pushLayout={16,VK_SHADER_STAGE_VERTEX_BIT};ui.attributes={{0,0,VK_FORMAT_R32G32_SFLOAT,0},{1,0,VK_FORMAT_R8G8B8A8_UNORM,8}};
    for(auto* p:{&base,&ui}){p->vs=std::make_unique<Shader>(shader);p->fs=std::make_unique<Shader>(shader);}
    Command c{};c.device=&d;auto h=handle<VkCommandBuffer>(&c);VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};assert(vkBeginCommandBuffer(h,&begin)==VK_SUCCESS);c.frame=&frame;c.inPass=true;c.clear[3]=1;
    VkViewport vp{0,0,32,32,0,1};vkCmdSetViewport(h,0,1,&vp);VkRect2D clip{{0,0},{32,32}};vkCmdSetScissor(h,0,1,&clip);
    auto bind=[&](Pipeline& p,Buffer& v,Buffer& ix,VkDeviceSize offset){vkCmdBindPipeline(h,VK_PIPELINE_BIND_POINT_GRAPHICS,handle<VkPipeline>(&p));auto vh=handle<VkBuffer>(&v);vkCmdBindVertexBuffers(h,0,1,&vh,&offset);vkCmdBindIndexBuffer(h,handle<VkBuffer>(&ix),0,VK_INDEX_TYPE_UINT16);};
    bind(base,va,ixa,0);auto ds=handle<VkDescriptorSet>(&set);vkCmdBindDescriptorSets(h,VK_PIPELINE_BIND_POINT_GRAPHICS,handle<VkPipelineLayout>(&base.pushLayout),0,1,&ds,0,nullptr);vkCmdDrawIndexed(h,3,1,0,0,0);
    bind(ui,vb,ixb,12);uint32_t push[]={1,2,3,4};vkCmdPushConstants(h,handle<VkPipelineLayout>(&ui.pushLayout),VK_SHADER_STAGE_VERTEX_BIT,0,16,push);clip={{5,7},{19,13}};vkCmdSetScissor(h,0,1,&clip);vkCmdDrawIndexed(h,3,1,0,2,0);
    push[0]=123;vkCmdPushConstants(h,handle<VkPipelineLayout>(&ui.pushLayout),VK_SHADER_STAGE_VERTEX_BIT,0,16,push);clip={{8,9},{1,2}};vkCmdSetScissor(h,0,1,&clip);vkCmdDrawIndexed(h,3,1,3,-1,0);
    assert(!c.bad&&c.drawSnapshots.size()==3);ubm.bytes[0]=117;vmb.bytes[12]=73;
    UiPacket packet;assert(buildUiPacket(&c,&d,packet)&&packet.textures.empty());std::vector<mavk::bridge::LayerRequest> layers;assert(mavk::bridge::decodeLayers(packet.bytes,0,layers));
    assert(layers.size()==2&&layers[0].scene.state.depthFormat==VK_FORMAT_D32_SFLOAT&&layers[0].uniform.size()==16&&layers[0].uniform[0]==117);
    const auto& uiScene=layers[1].scene;assert(layers[1].raster.uiOverlayAlpha&&uiScene.state.depthFormat==VK_FORMAT_UNDEFINED&&uiScene.geometry.count==5&&uiScene.geometry.bytes[0]==73&&uiScene.state.draws.size()==2);
    const auto& a=uiScene.state.draws[0];const auto& b=uiScene.state.draws[1];assert(a.firstIndex==0&&b.firstIndex==3&&a.vertexOffset==2&&b.vertexOffset==-1&&a.clip.offset.x==5&&b.clip.offset.x==8&&get32(a.push.data())==1&&get32(b.push.data())==123);
    assert(uiScene.geometry.indices==imb.bytes);unsigned rejected=0;
    auto fail=[&](){UiPacket unchanged;unchanged.bytes={77};assert(!buildUiPacket(&c,&d,unchanged)&&unchanged.bytes==std::vector<uint8_t>{77});rejected++;};
    ui.depth=true;fail();ui.depth=false;c.drawSnapshots[1].baseVertex=-1;fail();c.drawSnapshots[1].baseVertex=2;
    c.drawSnapshots[1].firstIndex=UINT32_MAX;fail();c.drawSnapshots[1].firstIndex=0;
    c.drawSnapshots[1].vertexBufferOffset=UINT64_MAX;fail();c.drawSnapshots[1].vertexBufferOffset=12;
    c.drawSnapshots[1].clip.extent.width=UINT32_MAX;fail();c.drawSnapshots[1].clip.extent.width=19;
    ubm.bytes.resize(8);fail();ubm.bytes.resize(16);assert(buildUiPacket(&c,&d,packet));
    std::printf("ICD_UI_PACKET_CPU_PASS groups=2 draw_snapshots=3 rejects=%u late_ubo_vertex=PASS signed_base_clip_push=PASS NOT_GPU_OR_GUEST_PASS\n",rejected);
}
