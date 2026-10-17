#include <mavk/bridge.hpp>
#include <map>
#include <stdexcept>
#include <string>
#include <cstring>
#include "graphics.hpp"
#include "vertex_geometry.hpp"
#include "shader-filter.hpp"
#include "compute-shape.hpp"
#include "compute.hpp"
#include "layers.hpp"
#include <cmath>

namespace mavk::bridge {
namespace {
#ifdef MAVK_DEVELOPMENT_BUNDLE
const uint8_t developmentBundle[]=
#include "development-bundle.inc"
;
static_assert(sizeof(developmentBundle)<=4*MaxPayload,"Development archive bound");
#endif
uint32_t u32(const std::vector<uint8_t>& b,size_t p){return uint32_t(b[p])|(uint32_t(b[p+1])<<8)|(uint32_t(b[p+2])<<16)|(uint32_t(b[p+3])<<24);}
uint64_t u64(const std::vector<uint8_t>& b,size_t p){return u32(b,p)|(uint64_t(u32(b,p+4))<<32);}
void put32(std::vector<uint8_t>& b,uint32_t v){for(uint32_t i=0;i<4;i++)b.push_back(uint8_t(v>>(i*8)));}
void put64(std::vector<uint8_t>& b,uint64_t v){put32(b,uint32_t(v));put32(b,uint32_t(v>>32));}
std::vector<uint8_t> response(uint32_t op,uint64_t id,Status status,const std::vector<uint8_t>& payload={}){
    std::vector<uint8_t> b;b.reserve(HeaderBytes+payload.size());put32(b,Magic);put32(b,1|(HeaderBytes<<16));put32(b,op);put32(b,1);put64(b,id);put32(b,uint32_t(payload.size()));put32(b,uint32_t(status));b.insert(b.end(),payload.begin(),payload.end());return b;
}
void ok(VkResult r){if(r!=VK_SUCCESS)throw std::runtime_error("Bridge Vulkan operation failed: "+std::to_string(r));}
struct Rejected{Status status;};
// Structural bound only, not a SPIR-V semantic validator or security sandbox.
// This prototype endpoint accepts trusted test guests, never public network data.
std::vector<uint32_t> shaderWords(const std::vector<uint8_t>& b,size_t start,uint32_t bytes,bool vertexUniform=false,bool sampledFragment=false,uint32_t sampledBinding=1){
    if(bytes<20||bytes>65536||bytes%4)throw Rejected{Status::BadPayload};
    std::vector<uint32_t> code(bytes/4);for(size_t i=0;i<code.size();i++)code[i]=u32(b,start+i*4);
    if(code[0]!=0x07230203||code[1]>0x00010300||!code[3]||code[3]>65536||code[4])throw Rejected{Status::Unsupported};
    if(!shaderInstructionsAllowed(code,vertexUniform,sampledFragment,sampledBinding))throw Rejected{Status::Unsupported};
    return code;
}
}
struct Session::Impl {
    DeviceContext& c;VkQueue queue;VkDescriptorSetLayout descriptorLayout{};VkPipelineLayout layout{};VkDescriptorPool pool{};VkDescriptorSet set{};
    std::unique_ptr<Shader> shader;std::unique_ptr<Pipeline> pipeline;std::unique_ptr<CommandBatch> commands;std::unique_ptr<Fence> fence;
    struct Entry{std::unique_ptr<Buffer> buffer;bool ready=false;};
    std::map<uint64_t,Entry> buffers;uint64_t nextHandle=1,lastRequest=0,totalBytes=0;bool failed=false;
    // Completed CPU readbacks only; not persistent Vulkan images or zero-copy.
    std::map<uint64_t,std::vector<uint8_t>> snapshots;uint64_t snapshotBytes=0;
    struct TextureEntry {SampledPixels pixels;uint32_t written{};uint64_t bytes{};std::unique_ptr<SampledImage> gpu;};
    std::map<uint64_t,TextureEntry> textures;uint64_t textureBytes=0;
    // One bounded retained renderer, not an unbounded per-shader cache. Exact
    // immutable wire bytes key shader/geometry/state; only UBO data may vary.
    std::vector<uint8_t> graphicsKey;
    std::unique_ptr<OffscreenSession> graphics;
    Impl(DeviceContext& context,VkQueue q):c(context),queue(q){}
    ~Impl(){
        vkDeviceWaitIdle(c.vkDevice());graphics.reset();textures.clear();buffers.clear();commands.reset();fence.reset();pipeline.reset();shader.reset();
        if(pool)vkDestroyDescriptorPool(c.vkDevice(),pool,nullptr);if(layout)vkDestroyPipelineLayout(c.vkDevice(),layout,nullptr);if(descriptorLayout)vkDestroyDescriptorSetLayout(c.vkDevice(),descriptorLayout,nullptr);
    }
    void init(const uint32_t* code,size_t bytes){
        shader=std::make_unique<Shader>(c,code,bytes);
        VkDescriptorSetLayoutBinding binding{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr};VkDescriptorSetLayoutCreateInfo d{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};d.bindingCount=1;d.pBindings=&binding;ok(vkCreateDescriptorSetLayout(c.vkDevice(),&d,nullptr,&descriptorLayout));
        VkPushConstantRange push{VK_SHADER_STAGE_COMPUTE_BIT,0,4};VkPipelineLayoutCreateInfo l{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};l.setLayoutCount=1;l.pSetLayouts=&descriptorLayout;l.pushConstantRangeCount=1;l.pPushConstantRanges=&push;ok(vkCreatePipelineLayout(c.vkDevice(),&l,nullptr,&layout));
        VkComputePipelineCreateInfo p{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};p.layout=layout;p.stage={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};p.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;p.stage.module=shader->handle();p.stage.pName="main";pipeline=std::make_unique<Pipeline>(c,p);
        VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1};VkDescriptorPoolCreateInfo dp{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};dp.maxSets=1;dp.poolSizeCount=1;dp.pPoolSizes=&size;ok(vkCreateDescriptorPool(c.vkDevice(),&dp,nullptr,&pool));VkDescriptorSetAllocateInfo da{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};da.descriptorPool=pool;da.descriptorSetCount=1;da.pSetLayouts=&descriptorLayout;ok(vkAllocateDescriptorSets(c.vkDevice(),&da,&set));
        commands=std::make_unique<CommandBatch>(c);fence=std::make_unique<Fence>(c);
    }
    Entry& entry(uint64_t id){auto i=buffers.find(id);if(i==buffers.end())throw Rejected{Status::NotFound};return i->second;}
    std::vector<uint8_t> execute(uint32_t op,const std::vector<uint8_t>& b){
        auto require=[&](size_t bytes){if(b.size()!=HeaderBytes+bytes)throw Rejected{Status::BadPayload};};std::vector<uint8_t> out;
        if(op==uint32_t(Op::Hello)){require(0);put32(out,1);put32(out,MaxPayload);put32(out,8);put32(out,8*MaxPayload);return out;}
        if(op==uint32_t(Op::CreateSampled)){
            require(52);size_t at=HeaderBytes;SampledPixels p;p.width=u32(b,at);p.height=u32(b,at+4);p.levels=u32(b,at+8);
            auto& s=p.sampler;s.mag=VkFilter(u32(b,at+12));s.min=VkFilter(u32(b,at+16));s.mip=VkSamplerMipmapMode(u32(b,at+20));s.u=VkSamplerAddressMode(u32(b,at+24));s.v=VkSamplerAddressMode(u32(b,at+28));s.w=VkSamplerAddressMode(u32(b,at+32));
            auto number=[&](size_t offset){uint32_t bits=u32(b,at+offset);float f;std::memcpy(&f,&bits,4);return f;};s.bias=number(36);s.minLod=number(40);s.maxLod=number(44);
            VkPhysicalDeviceProperties props{};vkGetPhysicalDeviceProperties(c.physicalDevice(),&props);
            if(!samplerStateValid(s,props.limits.maxSamplerLodBias))throw Rejected{Status::BadPayload};
            uint64_t bytes{};try{auto layout=rgbaMipLayout(p.width,p.height,p.levels);bytes=layout.back().offset+layout.back().bytes;}catch(const std::invalid_argument&){throw Rejected{Status::BadPayload};}
            if(bytes!=u32(b,at+48))throw Rejected{Status::BadPayload};
            if(textures.size()>=4||textureBytes+bytes>16*MaxPayload)throw Rejected{Status::Limit};
            p.rgba.resize(size_t(bytes));TextureEntry e;e.pixels=std::move(p);e.bytes=bytes;uint64_t id=nextHandle++;textures.emplace(id,std::move(e));textureBytes+=bytes;put64(out,id);return out;
        }
        if(op==uint32_t(Op::WriteSampled)||op==uint32_t(Op::CommitSampled)||op==uint32_t(Op::DestroySampled)){
            if(b.size()<HeaderBytes+8)throw Rejected{Status::BadPayload};auto found=textures.find(u64(b,HeaderBytes));if(found==textures.end())throw Rejected{Status::NotFound};auto& t=found->second;
            if(op==uint32_t(Op::DestroySampled)){require(8);graphics.reset();graphicsKey.clear();textureBytes-=t.bytes;textures.erase(found);return out;}
            if(op==uint32_t(Op::CommitSampled)){
                require(8);if(t.gpu||t.written!=t.bytes)throw Rejected{Status::NotReady};
                auto gpu=std::make_unique<SampledImage>(c,t.pixels);gpu->upload(queue,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);t.gpu=std::move(gpu);std::vector<uint8_t>().swap(t.pixels.rgba);return out;
            }
            if(b.size()<HeaderBytes+16)throw Rejected{Status::BadPayload};uint32_t offset=u32(b,HeaderBytes+8),bytes=u32(b,HeaderBytes+12);
            if(t.gpu||!bytes||offset!=t.written||offset>t.bytes||bytes>t.bytes-offset||b.size()-HeaderBytes-16!=bytes)throw Rejected{Status::BadPayload};
            std::memcpy(t.pixels.rgba.data()+offset,b.data()+HeaderBytes+16,bytes);t.written+=bytes;return out;
        }
        if(op==18){
            if(b.size()<HeaderBytes+24)throw Rejected{Status::BadPayload};size_t at=HeaderBytes;
            uint32_t codeBytes=u32(b,at),dataBytes=u32(b,at+4),pushBytes=u32(b,at+8),groups=u32(b,at+12);
            if(!groups||groups>256||u32(b,at+16)!=1||u32(b,at+20)!=1||!dataBytes||dataBytes>524288||dataBytes%4||pushBytes>64||pushBytes%4||codeBytes>65536||uint64_t(24)+codeBytes+dataBytes+pushBytes!=b.size()-HeaderBytes)throw Rejected{Status::BadPayload};
            auto code=shaderWords(b,at+24,codeBytes,true);uint32_t local=0;if(!computeShape(code,local))throw Rejected{Status::Unsupported};
            at+=24+codeBytes;std::vector<uint8_t> push(b.begin()+at,b.begin()+at+pushBytes);at+=pushBytes;
            return dispatchStorage(c,queue,code,{b.begin()+at,b.end()},push,groups);
        }
        if(op==9){
#ifdef MAVK_DEVELOPMENT_BUNDLE
            // One fixed private test artifact, never a file/path access service.
            constexpr uint32_t size=sizeof(developmentBundle);
            if(b.size()==HeaderBytes){
                if(size>MaxPayload)throw Rejected{Status::Limit};
                return {std::begin(developmentBundle),std::end(developmentBundle)};
            }
            require(8);uint32_t offset=u32(b,HeaderBytes),bytes=u32(b,HeaderBytes+4);
            if(!offset&&!bytes){put32(out,size);return out;}
            if(!bytes||bytes>MaxPayload||offset>size||bytes>size-offset)throw Rejected{Status::BadPayload};
            return {developmentBundle+offset,developmentBundle+offset+bytes};
#else
            require(0);
            throw Rejected{Status::Unsupported};
#endif
        }
        if(op==uint32_t(Op::DrawTriangle)){
            require(8);uint32_t w=u32(b,HeaderBytes),h=u32(b,HeaderBytes+4);
            if(w<16||h<16||w>512||h>512)throw Rejected{Status::Limit};
            return drawTriangle(c,queue,w,h);
        }
        if(op==uint32_t(Op::DeviceIdentity)){
            require(0);VkPhysicalDeviceProperties p{};vkGetPhysicalDeviceProperties(c.physicalDevice(),&p);
            put32(out,p.apiVersion);put32(out,p.driverVersion);put32(out,p.vendorID);put32(out,p.deviceID);put32(out,uint32_t(p.deviceType));
            out.insert(out.end(),p.deviceName,p.deviceName+VK_MAX_PHYSICAL_DEVICE_NAME_SIZE);return out;
        }
        if(op==uint32_t(Op::ReadSnapshot)){
            require(16);auto found=snapshots.find(u64(b,HeaderBytes));if(found==snapshots.end())throw Rejected{Status::NotFound};
            uint32_t offset=u32(b,HeaderBytes+8),bytes=u32(b,HeaderBytes+12);const auto& pixels=found->second;
            if(!bytes||bytes>MaxPayload||offset>pixels.size()||bytes>pixels.size()-offset)throw Rejected{Status::BadPayload};
            return {pixels.begin()+offset,pixels.begin()+offset+bytes};
        }
        if(op==uint32_t(Op::DestroySnapshot)){
            require(8);auto found=snapshots.find(u64(b,HeaderBytes));if(found==snapshots.end())throw Rejected{Status::NotFound};
            snapshotBytes-=found->second.size();snapshots.erase(found);return out;
        }
        if(op==uint32_t(Op::RenderLayersSnapshot)){
            std::vector<LayerRequest> layers;if(!decodeLayers(b,HeaderBytes,layers))throw Rejected{Status::BadPayload};
            uint64_t bytes=uint64_t(layers[0].scene.width)*layers[0].scene.height*4;
            if(snapshots.size()>=4||snapshotBytes+bytes>16*MaxPayload)throw Rejected{Status::Limit};
            struct Prepared{std::vector<uint32_t> vertex,fragment;SampledImage* texture=nullptr;};std::vector<Prepared> prepared;prepared.reserve(layers.size());
            // Complete CPU/schema/descriptor admission before any packet-owned
            // pipeline/attachment creation or queue work.
            for(const auto& layer:layers){
                Prepared p;if(layer.texture){auto found=textures.find(layer.texture);if(found==textures.end())throw Rejected{Status::NotFound};p.texture=found->second.gpu.get();if(!p.texture)throw Rejected{Status::NotReady};}
                const auto& s=layer.scene;
                if(s.state.depthFormat!=VK_FORMAT_UNDEFINED){VkFormatProperties properties{};vkGetPhysicalDeviceFormatProperties(c.physicalDevice(),s.state.depthFormat,&properties);if(!(properties.optimalTilingFeatures&VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))throw Rejected{Status::Unsupported};}
                p.vertex=shaderWords(layer.sceneBytes,s.vertexStart,s.vertexBytes,!layer.uniform.empty());
                p.fragment=shaderWords(layer.sceneBytes,s.fragmentStart,s.fragmentBytes,false,p.texture!=nullptr,layer.raster.sampledBinding);prepared.push_back(std::move(p));
            }
            std::vector<std::unique_ptr<OffscreenSession>> renderers;renderers.reserve(layers.size());
            for(size_t i=0;i<layers.size();i++){
                auto& layer=layers[i];auto& p=prepared[i];auto& s=layer.scene;
                renderers.push_back(std::make_unique<OffscreenSession>(c,queue,s.width,s.height,p.vertex,p.fragment,&s.geometry,s.state.pushBytes,s.state.pushStages,&s.state,layer.uniform.empty()?nullptr:&layer.uniform,nullptr,p.texture,&layer.raster,i?renderers.back().get():nullptr));
            }
            std::vector<LayerFrame> inputs(layers.size());auto pixels=renderers.back()->renderLayers(inputs,layers[0].scene.clear);
            uint64_t id=nextHandle++;snapshots.emplace(id,std::move(pixels));snapshotBytes+=bytes;put64(out,id);put32(out,uint32_t(bytes));put32(out,layers[0].scene.width*4);return out;
        }
        if(op==uint32_t(Op::DrawScene)||op==uint32_t(Op::RenderSnapshot)||op==uint32_t(Op::RenderUniformSnapshot)||op==uint32_t(Op::RenderSampledSnapshot)){
            bool sampledMode=op==uint32_t(Op::RenderSampledSnapshot),uniformMode=op==uint32_t(Op::RenderUniformSnapshot)||sampledMode,retain=op!=uint32_t(Op::DrawScene);
            size_t start=HeaderBytes;std::vector<uint8_t> uniform;
            SampledImage* texture=nullptr;uint64_t textureId=0;
            if(sampledMode){if(b.size()<start+8)throw Rejected{Status::BadPayload};textureId=u64(b,start);auto found=textures.find(textureId);start+=8;if(found==textures.end())throw Rejected{Status::NotFound};texture=found->second.gpu.get();if(!texture)throw Rejected{Status::NotReady};}
            if(uniformMode){
                if(b.size()<start+4)throw Rejected{Status::BadPayload};uint32_t bytes=u32(b,start);start+=4;
                if(!bytes||bytes>4096||bytes%(sampledMode?4:16)||bytes>b.size()-start)throw Rejected{Status::BadPayload};
                uniform.assign(b.begin()+start,b.begin()+start+bytes);start+=bytes;
            }
            SceneRequest request;if(!decodeScene(b,start,request,retain?1024:512,uniformMode))throw Rejected{Status::BadPayload};
            uint64_t bytes=uint64_t(request.width)*request.height*4;
            if(retain&&(snapshots.size()>=4||snapshotBytes+bytes>16*MaxPayload))throw Rejected{Status::Limit};
            if(request.state.depthFormat!=VK_FORMAT_UNDEFINED){
                VkFormatProperties properties{};vkGetPhysicalDeviceFormatProperties(c.physicalDevice(),request.state.depthFormat,&properties);
                if(!(properties.optimalTilingFeatures&VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))throw Rejected{Status::Unsupported};
            }
            auto vert=shaderWords(b,request.vertexStart,request.vertexBytes,uniformMode),frag=shaderWords(b,request.fragmentStart,request.fragmentBytes,false,sampledMode);
            std::vector<uint8_t> pixels;
            if(uniformMode){
                std::vector<uint8_t> key;key.reserve(16+b.size()-start);
                put32(key,op);put64(key,textureId);put32(key,uint32_t(uniform.size()));
                key.insert(key.end(),b.begin()+start,b.end());
                if(!graphics||key!=graphicsKey){
                    // Retire before replacement: at most one live graphics
                    // cache; never let its descriptor outlive the texture.
                    graphics.reset();graphicsKey.clear();
                    graphics=std::make_unique<OffscreenSession>(c,queue,request.width,request.height,vert,frag,&request.geometry,request.state.pushBytes,request.state.pushStages,&request.state,&uniform,nullptr,texture);
                    graphicsKey=std::move(key);
                }
                pixels=graphics->render(nullptr,request.clear,&uniform);
            }else pixels=drawOffscreen(c,queue,request.width,request.height,vert,frag,nullptr,request.clear,&request.geometry,request.state.pushBytes,request.state.pushStages,&request.state,nullptr,nullptr,texture);
            if(!retain)return pixels;
            uint64_t id=nextHandle++;snapshots.emplace(id,std::move(pixels));snapshotBytes+=bytes;
            put64(out,id);put32(out,uint32_t(bytes));put32(out,request.width*4);return out;
        }
        if(op==uint32_t(Op::DrawIndexedPush)){
            // Byte-preserving push constants: integers and NaNs are legal shader data.
            // Only clear colour is interpreted as finite float32 on the host.
            if(b.size()<HeaderBytes+40)throw Rejected{Status::BadPayload};
            uint32_t w=u32(b,HeaderBytes),h=u32(b,HeaderBytes+4),vs=u32(b,HeaderBytes+8),fs=u32(b,HeaderBytes+12);
            uint32_t stages=u32(b,HeaderBytes+16),pushBytes=u32(b,HeaderBytes+20);
            if(w<16||h<16||w>512||h>512)throw Rejected{Status::Limit};
            if(!stages||(stages&~uint32_t(VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT))||!pushBytes||pushBytes>64||pushBytes%4||vs>65536||fs>65536)throw Rejected{Status::BadPayload};
            size_t shaderStart=HeaderBytes+40+pushBytes,end=shaderStart+size_t(vs)+fs;
            if(end>b.size())throw Rejected{Status::BadPayload};
            VertexGeometry geometry;if(!decodeIndexedGeometry(b,end,geometry))throw Rejected{Status::BadPayload};
            float clear[4];for(uint32_t i=0;i<4;i++){uint32_t bits=u32(b,HeaderBytes+24+i*4);std::memcpy(&clear[i],&bits,4);if(!std::isfinite(clear[i]))throw Rejected{Status::BadPayload};}
            auto vert=shaderWords(b,shaderStart,vs),frag=shaderWords(b,shaderStart+vs,fs);
            return drawOffscreen(c,queue,w,h,vert,frag,b.data()+HeaderBytes+40,clear,&geometry,pushBytes,stages);
        }
        if(op==uint32_t(Op::DrawOffscreen)||op==uint32_t(Op::DrawVertices)||op==uint32_t(Op::DrawIndexed)){
            if(b.size()<HeaderBytes+48)throw Rejected{Status::BadPayload};
            uint32_t w=u32(b,HeaderBytes),h=u32(b,HeaderBytes+4),vs=u32(b,HeaderBytes+8),fs=u32(b,HeaderBytes+12);
            if(w<16||h<16||w>512||h>512)throw Rejected{Status::Limit};
            size_t end=HeaderBytes+48+size_t(vs)+fs;
            if(vs>65536||fs>65536||end>b.size())throw Rejected{Status::BadPayload};
            VertexGeometry geometry;
            bool vertexDraw=op!=uint32_t(Op::DrawOffscreen);
            bool geometryValid=op==uint32_t(Op::DrawIndexed)?decodeIndexedGeometry(b,end,geometry):
                vertexDraw?decodeVertexGeometry(b,end,geometry):end==b.size();
            if(!geometryValid)throw Rejected{Status::BadPayload};
            float state[4],clear[4];for(uint32_t i=0;i<8;i++){uint32_t bits=u32(b,HeaderBytes+16+i*4);float f;std::memcpy(&f,&bits,4);if(!std::isfinite(f))throw Rejected{Status::BadPayload};(i<4?state[i]:clear[i-4])=f;}
            auto vert=shaderWords(b,HeaderBytes+48,vs),frag=shaderWords(b,HeaderBytes+48+vs,fs);
            return drawOffscreen(c,queue,w,h,vert,frag,state,clear,vertexDraw?&geometry:nullptr);
        }
        if(op==uint32_t(Op::CreateBuffer)){
            require(4);uint32_t bytes=u32(b,HeaderBytes);
            // Trusted verification shader has local_size_x=256 and no bounds check.
            if(!bytes||bytes%1024||bytes>MaxPayload||buffers.size()>=8||totalBytes+bytes>8*MaxPayload)throw Rejected{Status::Limit};
            auto buffer=std::make_unique<Buffer>(c,bytes,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            uint64_t id=nextHandle++;buffers.emplace(id,Entry{std::move(buffer),false});totalBytes+=bytes;put64(out,id);return out;
        }
        if(op==uint32_t(Op::DestroyBuffer)){require(8);uint64_t id=u64(b,HeaderBytes);auto& e=entry(id);totalBytes-=e.buffer->size();buffers.erase(id);return out;}
        if(op==uint32_t(Op::DispatchVerify)){
            require(12);auto& e=entry(u64(b,HeaderBytes));uint32_t seed=u32(b,HeaderBytes+8);e.ready=false;
            VkDescriptorBufferInfo bi{e.buffer->handle(),0,e.buffer->size()};VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=set;write.descriptorCount=1;write.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;write.pBufferInfo=&bi;vkUpdateDescriptorSets(c.vkDevice(),1,&write,0,nullptr);
            fence->reset();commands->begin();VkCommandBuffer command=commands->handle();vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline->handle());vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_COMPUTE,layout,0,1,&set,0,nullptr);vkCmdPushConstants(command,layout,VK_SHADER_STAGE_COMPUTE_BIT,0,4,&seed);vkCmdDispatch(command,uint32_t(e.buffer->size()/1024),1,1);commands->hostReadBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_WRITE_BIT);commands->end();VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;ok(c.submit(queue,submit,fence->handle()));ok(fence->wait(5'000'000'000ull));e.ready=true;return out;
        }
        if(op==uint32_t(Op::ReadBuffer)){
            require(16);auto& e=entry(u64(b,HeaderBytes));uint32_t offset=u32(b,HeaderBytes+8),bytes=u32(b,HeaderBytes+12);
            if(!e.ready)throw Rejected{Status::NotReady};if(offset>e.buffer->size()||bytes>e.buffer->size()-offset||bytes>MaxPayload)throw Rejected{Status::BadPayload};
            auto* data=static_cast<const uint8_t*>(e.buffer->map());e.buffer->invalidate();out.resize(bytes);if(bytes)std::memcpy(out.data(),data+offset,bytes);return out;
        }
        throw Rejected{Status::Unsupported};
    }
};
Session::Session(DeviceContext& c,VkQueue queue,const uint32_t* code,size_t bytes):impl_(std::make_unique<Impl>(c,queue)){impl_->init(code,bytes);}
Session::~Session()=default;
std::vector<uint8_t> Session::exchange(const std::vector<uint8_t>& b){
    if(b.size()<HeaderBytes)return response(0,0,Status::BadHeader);uint32_t op=u32(b,8);uint64_t id=u64(b,16);
    if(u32(b,0)!=Magic||u32(b,4)!=(1|(HeaderBytes<<16))||u32(b,12)||u32(b,28)||u32(b,24)>MaxPayload||b.size()-HeaderBytes!=u32(b,24))return response(op,id,Status::BadHeader);
    if(!id||id<=impl_->lastRequest)return response(op,id,Status::BadSequence);impl_->lastRequest=id;
    if(impl_->failed)return response(op,id,Status::GpuFailure);
    try{return response(op,id,Status::Ok,impl_->execute(op,b));}
    catch(const Rejected& e){return response(op,id,e.status);}
    catch(const std::exception&){impl_->failed=true;return response(op,id,Status::GpuFailure);}
}
}
