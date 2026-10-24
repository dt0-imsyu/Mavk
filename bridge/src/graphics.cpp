#include "graphics.hpp"
#include <stdexcept>
#include <cstring>
#include <iterator>
#include <chrono>

namespace mavk::bridge {
namespace {
using CpuClock=std::chrono::steady_clock;
double elapsedCpu(CpuClock::time_point start){return std::chrono::duration<double,std::milli>(CpuClock::now()-start).count();}
const uint32_t vertex[] =
#include "triangle.vert.inc"
;
const uint32_t fragment[] =
#include "triangle.frag.inc"
;
void check(VkResult r){if(r!=VK_SUCCESS)throw std::runtime_error("Bridge graphics VkResult="+std::to_string(r));}
void validateClip(const VkRect2D* clip,uint32_t w,uint32_t h){
    if(clip&&(clip->offset.x<0||clip->offset.y<0||!clip->extent.width||!clip->extent.height||
        uint64_t(clip->offset.x)+clip->extent.width>w||uint64_t(clip->offset.y)+clip->extent.height>h))
        throw std::invalid_argument("Offscreen clip outside target");
}
struct ColorTarget {
    DeviceContext& context;std::unique_ptr<Image> image;std::unique_ptr<Buffer> readback;VkImageView view{};
    ColorTarget(DeviceContext& c,uint32_t w,uint32_t h):context(c){
        VkImageCreateInfo i{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};i.imageType=VK_IMAGE_TYPE_2D;i.format=VK_FORMAT_R8G8B8A8_UNORM;i.extent={w,h,1};i.mipLevels=1;i.arrayLayers=1;i.samples=VK_SAMPLE_COUNT_1_BIT;i.tiling=VK_IMAGE_TILING_OPTIMAL;i.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        image=std::make_unique<Image>(c,i);readback=std::make_unique<Buffer>(c,uint64_t(w)*h*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        VkImageViewCreateInfo v{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};v.image=image->handle();v.viewType=VK_IMAGE_VIEW_TYPE_2D;v.format=i.format;v.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};check(vkCreateImageView(c.vkDevice(),&v,nullptr,&view));
    }
    ~ColorTarget(){if(view)vkDestroyImageView(context.vkDevice(),view,nullptr);}
};
struct Graphics {
    DeviceContext& c;
    std::shared_ptr<ColorTarget> color;
    VkImageView view{},depthView{};VkRenderPass pass{};VkFramebuffer frame{};VkPipelineLayout layout{};
    std::unique_ptr<Image> depth;std::unique_ptr<Buffer> vertices,indices,uniform;
    std::unique_ptr<SampledImage> texture;
    VkDescriptorSetLayout descriptorLayout{};VkDescriptorPool descriptorPool{};VkDescriptorSet descriptor{};
    std::unique_ptr<Shader> vs,fs,gs,tcs,tes;std::unique_ptr<Pipeline> pipeline;
    std::unique_ptr<CommandBatch> commands;std::unique_ptr<Fence> fence;
    explicit Graphics(DeviceContext& context):c(context){}
    ~Graphics(){
        // Completion before retirement also on failed fence/submit paths.
        vkDeviceWaitIdle(c.vkDevice());commands.reset();fence.reset();pipeline.reset();
        if(frame)vkDestroyFramebuffer(c.vkDevice(),frame,nullptr);
        if(pass)vkDestroyRenderPass(c.vkDevice(),pass,nullptr);
        if(depthView)vkDestroyImageView(c.vkDevice(),depthView,nullptr);
        if(layout)vkDestroyPipelineLayout(c.vkDevice(),layout,nullptr);
        if(descriptorPool)vkDestroyDescriptorPool(c.vkDevice(),descriptorPool,nullptr);
        if(descriptorLayout)vkDestroyDescriptorSetLayout(c.vkDevice(),descriptorLayout,nullptr);
    }
    void init(uint32_t w,uint32_t h,const uint32_t* vert,size_t vertBytes,const uint32_t* frag,size_t fragBytes,const VertexGeometry* geometry,uint32_t pushBytes,VkShaderStageFlags pushStages,const SceneState* scene,const std::vector<uint8_t>* uniforms,const SampledPixels* sampled,SampledImage* uploaded,OffscreenTiming* timing,const RasterOptions* raster,std::shared_ptr<ColorTarget> sharedColor={}){
        VkDevice d=c.vkDevice();
        if(raster&&((raster->sourceOver&&raster->uiOverlayAlpha)||raster->sampledBinding>1||(uniforms&&raster->sampledBinding==0)))throw std::invalid_argument("Conflicting native raster/descriptor options");
        uint32_t sampledBinding=raster?raster->sampledBinding:1;
        if(raster&&(raster->sourceOver||raster->uiOverlayAlpha)){VkFormatProperties p{};vkGetPhysicalDeviceFormatProperties(c.physicalDevice(),VK_FORMAT_R8G8B8A8_UNORM,&p);if(!(p.optimalTilingFeatures&VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT))throw std::runtime_error("Native RGBA blend unavailable");}
        if(scene){
            if(!geometry||!geometry->indexCount||geometry->indices.size()!=uint64_t(geometry->indexCount)*(geometry->indexType==VK_INDEX_TYPE_UINT16?2:4))throw std::invalid_argument("Native indexed scene geometry");
            for(const auto& draw:scene->draws){
                if(draw.push.size()<pushBytes||draw.firstIndex>geometry->indexCount||draw.indexCount>geometry->indexCount-draw.firstIndex)throw std::invalid_argument("Native scene draw bounds");
                if(draw.clipped)validateClip(&draw.clip,w,h);
                for(uint32_t index=draw.firstIndex;index<uint64_t(draw.firstIndex)+draw.indexCount;index++){
                    uint32_t value=0;if(geometry->indexType==VK_INDEX_TYPE_UINT16){uint16_t v;std::memcpy(&v,geometry->indices.data()+index*2,2);value=v;}else if(geometry->indexType==VK_INDEX_TYPE_UINT32)std::memcpy(&value,geometry->indices.data()+index*4,4);else throw std::invalid_argument("Native index type");
                    int64_t vertex=int64_t(value)+draw.vertexOffset;if(vertex<0||vertex>=geometry->count)throw std::invalid_argument("Native base vertex bounds");
                }
            }
        }
        VkImageCreateInfo i{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};i.imageType=VK_IMAGE_TYPE_2D;i.format=VK_FORMAT_R8G8B8A8_UNORM;i.extent={w,h,1};i.mipLevels=1;i.arrayLayers=1;i.samples=VK_SAMPLE_COUNT_1_BIT;i.tiling=VK_IMAGE_TILING_OPTIMAL;i.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        bool loadColor=bool(sharedColor);color=sharedColor?std::move(sharedColor):std::make_shared<ColorTarget>(c,w,h);view=color->view;
        VkImageViewCreateInfo v{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};v.image=color->image->handle();v.viewType=VK_IMAGE_VIEW_TYPE_2D;v.format=i.format;v.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        bool useDepth=scene&&scene->depthFormat!=VK_FORMAT_UNDEFINED;
        VkImageView views[2]={view,VK_NULL_HANDLE};
        if(useDepth){
            VkImageCreateInfo depthInfo=i;depthInfo.format=scene->depthFormat;depthInfo.usage=VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            depth=std::make_unique<Image>(c,depthInfo);VkImageViewCreateInfo dv=v;dv.image=depth->handle();dv.format=depthInfo.format;dv.subresourceRange.aspectMask=VK_IMAGE_ASPECT_DEPTH_BIT;
            check(vkCreateImageView(d,&dv,nullptr,&depthView));views[1]=depthView;
        }
        VkAttachmentDescription attachments[2]{};auto& a=attachments[0];a.format=i.format;a.samples=VK_SAMPLE_COUNT_1_BIT;a.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;a.storeOp=VK_ATTACHMENT_STORE_OP_STORE;a.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;a.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;a.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;a.finalLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        if(loadColor){a.loadOp=VK_ATTACHMENT_LOAD_OP_LOAD;a.initialLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;}
        VkAttachmentReference color{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;sub.colorAttachmentCount=1;sub.pColorAttachments=&color;
        VkAttachmentReference depthReference{1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        if(useDepth){auto& z=attachments[1];z.format=scene->depthFormat;z.samples=VK_SAMPLE_COUNT_1_BIT;z.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;z.storeOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;z.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;z.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;z.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;z.finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;sub.pDepthStencilAttachment=&depthReference;}
        VkSubpassDependency dep{};dep.srcSubpass=VK_SUBPASS_EXTERNAL;dep.dstSubpass=0;dep.srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;dep.dstStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;dep.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        if(loadColor){dep.srcAccessMask|=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;dep.dstAccessMask|=VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;}
        if(useDepth){dep.srcStageMask|=VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;dep.dstStageMask|=VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;dep.dstAccessMask|=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;}
        VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};rp.attachmentCount=useDepth?2:1;rp.pAttachments=attachments;rp.subpassCount=1;rp.pSubpasses=&sub;rp.dependencyCount=1;rp.pDependencies=&dep;check(vkCreateRenderPass(d,&rp,nullptr,&pass));
        VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};fb.renderPass=pass;fb.attachmentCount=useDepth?2:1;fb.pAttachments=views;fb.width=w;fb.height=h;fb.layers=1;check(vkCreateFramebuffer(d,&fb,nullptr,&frame));
        if(sampled)texture=std::make_unique<SampledImage>(c,*sampled);
        if(uniforms){
            uniform=std::make_unique<Buffer>(c,uniforms->size(),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            std::memcpy(uniform->map(),uniforms->data(),uniforms->size());uniform->flush();
        }
        if(uniforms||sampled||uploaded){
            std::vector<VkDescriptorSetLayoutBinding> bindings;
            std::vector<VkDescriptorPoolSize> sizes;
            if(uniforms){bindings.push_back({0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,raster?raster->uniformStages:VkShaderStageFlags(VK_SHADER_STAGE_VERTEX_BIT),nullptr});sizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1});}
            if(sampled||uploaded){bindings.push_back({sampledBinding,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr});sizes.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1});}
            VkDescriptorSetLayoutCreateInfo dl{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};dl.bindingCount=uint32_t(bindings.size());dl.pBindings=bindings.data();check(vkCreateDescriptorSetLayout(d,&dl,nullptr,&descriptorLayout));
            VkDescriptorPoolCreateInfo dp{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};dp.maxSets=1;dp.poolSizeCount=uint32_t(sizes.size());dp.pPoolSizes=sizes.data();check(vkCreateDescriptorPool(d,&dp,nullptr,&descriptorPool));
            VkDescriptorSetAllocateInfo da{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};da.descriptorPool=descriptorPool;da.descriptorSetCount=1;da.pSetLayouts=&descriptorLayout;check(vkAllocateDescriptorSets(d,&da,&descriptor));
            VkDescriptorBufferInfo buffer{};VkDescriptorImageInfo imageInfo{};
            std::vector<VkWriteDescriptorSet> writes;
            if(uniforms){buffer={uniform->handle(),0,uniforms->size()};VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=descriptor;write.dstBinding=0;write.descriptorCount=1;write.descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;write.pBufferInfo=&buffer;writes.push_back(write);}
            if(sampled||uploaded){imageInfo=(uploaded?uploaded:texture.get())->descriptor();VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=descriptor;write.dstBinding=sampledBinding;write.descriptorCount=1;write.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;write.pImageInfo=&imageInfo;writes.push_back(write);}
            vkUpdateDescriptorSets(d,uint32_t(writes.size()),writes.data(),0,nullptr);
        }
        VkPushConstantRange push{pushStages,0,pushBytes};VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};pl.pushConstantRangeCount=pushBytes?1:0;pl.pPushConstantRanges=pushBytes?&push:nullptr;pl.setLayoutCount=(uniforms||sampled||uploaded)?1:0;pl.pSetLayouts=(uniforms||sampled||uploaded)?&descriptorLayout:nullptr;check(vkCreatePipelineLayout(d,&pl,nullptr,&layout));
        vs=std::make_unique<Shader>(c,vert,vertBytes);fs=std::make_unique<Shader>(c,frag,fragBytes);
        VkPipelineShaderStageCreateInfo stages[5]{};for(auto& s:stages){s.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;s.pName="main";}stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;stages[0].module=vs->handle();stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT;stages[1].module=fs->handle();uint32_t stageCount=2;
        if(raster&&(!raster->tessellationControlShader.empty()||!raster->tessellationEvaluationShader.empty())){
            VkPhysicalDeviceProperties properties{};vkGetPhysicalDeviceProperties(c.physicalDevice(),&properties);
            if(raster->tessellationControlShader.size()<5||raster->tessellationControlShader.size()>262144||raster->tessellationEvaluationShader.size()<5||raster->tessellationEvaluationShader.size()>262144||!raster->patchControlPoints||raster->patchControlPoints>properties.limits.maxTessellationPatchSize)throw std::invalid_argument("bounded native tessellation shader pair required");
            tcs=std::make_unique<Shader>(c,raster->tessellationControlShader.data(),raster->tessellationControlShader.size()*4);tes=std::make_unique<Shader>(c,raster->tessellationEvaluationShader.data(),raster->tessellationEvaluationShader.size()*4);
            stages[stageCount].stage=VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;stages[stageCount++].module=tcs->handle();stages[stageCount].stage=VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;stages[stageCount++].module=tes->handle();
        }
        if(raster&&!raster->geometryShader.empty()){
            if(raster->geometryShader.size()<5||raster->geometryShader.size()>262144)throw std::invalid_argument("bounded native geometry shader required");
            gs=std::make_unique<Shader>(c,raster->geometryShader.data(),raster->geometryShader.size()*4);
            stages[stageCount].stage=VK_SHADER_STAGE_GEOMETRY_BIT;stages[stageCount++].module=gs->handle();
        }
        VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        VkVertexInputBindingDescription binding{};
        if(geometry){
            binding={0,geometry->stride,VK_VERTEX_INPUT_RATE_VERTEX};vi.vertexBindingDescriptionCount=1;vi.pVertexBindingDescriptions=&binding;
            vi.vertexAttributeDescriptionCount=uint32_t(geometry->attributes.size());vi.pVertexAttributeDescriptions=geometry->attributes.data();
            vertices=std::make_unique<Buffer>(c,geometry->bytes.size(),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            std::memcpy(vertices->map(),geometry->bytes.data(),geometry->bytes.size());vertices->flush();
            if(geometry->indexCount){
                indices=std::make_unique<Buffer>(c,geometry->indices.size(),VK_BUFFER_USAGE_INDEX_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                std::memcpy(indices->map(),geometry->indices.data(),geometry->indices.size());indices->flush();
            }
        }
        VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};ia.topology=tes?VK_PRIMITIVE_TOPOLOGY_PATCH_LIST:VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineTessellationStateCreateInfo tessellation{VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO};tessellation.patchControlPoints=raster?raster->patchControlPoints:3;
        VkViewport vp{0,0,float(w),float(h),0,1};VkRect2D scissor{{0,0},{w,h}};VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};viewport.viewportCount=1;viewport.pViewports=&vp;viewport.scissorCount=1;viewport.pScissors=&scissor;
        VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};rs.polygonMode=VK_POLYGON_MODE_FILL;rs.cullMode=scene?scene->cull:VkCullModeFlags(VK_CULL_MODE_NONE);rs.frontFace=scene?scene->front:VK_FRONT_FACE_COUNTER_CLOCKWISE;rs.lineWidth=1;
        VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        VkPipelineColorBlendAttachmentState blend{};blend.colorWriteMask=15;VkPipelineColorBlendStateCreateInfo cb{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};cb.attachmentCount=1;cb.pAttachments=&blend;
        if(raster&&raster->sourceOver){blend.blendEnable=VK_TRUE;blend.srcColorBlendFactor=VK_BLEND_FACTOR_SRC_ALPHA;blend.dstColorBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;blend.colorBlendOp=VK_BLEND_OP_ADD;blend.srcAlphaBlendFactor=VK_BLEND_FACTOR_ONE;blend.dstAlphaBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;blend.alphaBlendOp=VK_BLEND_OP_ADD;}
        if(raster&&raster->uiOverlayAlpha){blend.blendEnable=VK_TRUE;blend.srcColorBlendFactor=VK_BLEND_FACTOR_SRC_ALPHA;blend.dstColorBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;blend.colorBlendOp=VK_BLEND_OP_ADD;blend.srcAlphaBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;blend.dstAlphaBlendFactor=VK_BLEND_FACTOR_ZERO;blend.alphaBlendOp=VK_BLEND_OP_ADD;}
        VkDynamicState dynamicStates[]={VK_DYNAMIC_STATE_SCISSOR,VK_DYNAMIC_STATE_VIEWPORT};VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dynamic.dynamicStateCount=2;dynamic.pDynamicStates=dynamicStates;
        VkPipelineDepthStencilStateCreateInfo depthState{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};depthState.depthTestEnable=useDepth;depthState.depthWriteEnable=useDepth;depthState.depthCompareOp=VK_COMPARE_OP_LESS_OR_EQUAL;
        VkGraphicsPipelineCreateInfo gp{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};gp.stageCount=stageCount;gp.pStages=stages;gp.pVertexInputState=&vi;gp.pInputAssemblyState=&ia;gp.pTessellationState=tes?&tessellation:nullptr;gp.pViewportState=&viewport;gp.pRasterizationState=&rs;gp.pMultisampleState=&ms;gp.pColorBlendState=&cb;gp.pDepthStencilState=useDepth?&depthState:nullptr;gp.pDynamicState=&dynamic;gp.layout=layout;gp.renderPass=pass;
        auto pipelineStart=timing?CpuClock::now():CpuClock::time_point{};pipeline=std::make_unique<Pipeline>(c,gp);if(timing)timing->pipelineCreateMs=elapsedCpu(pipelineStart);
        commands=std::make_unique<CommandBatch>(c);fence=std::make_unique<Fence>(c);
    }
};
}
namespace {
void recordPass(Graphics& g,VkCommandBuffer cmd,uint32_t w,uint32_t h,const void* state,const float* background,const VertexGeometry* geometry,uint32_t pushBytes,VkShaderStageFlags pushStages,const SceneState* scene,bool descriptors,bool uploadTexture,const VkRect2D* clip){
    if(g.texture&&uploadTexture)g.texture->recordInitialUpload(cmd,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    VkClearValue clear[2]{};std::memcpy(clear[0].color.float32,background,16);clear[1].depthStencil={scene?scene->clearDepth:1.0f,0};VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};begin.renderPass=g.pass;begin.framebuffer=g.frame;begin.renderArea.extent={w,h};begin.clearValueCount=g.depth?2:1;begin.pClearValues=clear;
    vkCmdBeginRenderPass(cmd,&begin,VK_SUBPASS_CONTENTS_INLINE);vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,g.pipeline->handle());VkRect2D scissor=clip?*clip:VkRect2D{{0,0},{w,h}};vkCmdSetScissor(cmd,0,1,&scissor);if(!scene&&pushBytes)vkCmdPushConstants(cmd,g.layout,pushStages,0,pushBytes,state);
    VkViewport viewport{0,0,float(w),float(h),0,1};vkCmdSetViewport(cmd,0,1,&viewport);
    if(descriptors)vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,g.layout,0,1,&g.descriptor,0,nullptr);
    if(geometry){VkBuffer buffer=g.vertices->handle();VkDeviceSize offset=0;vkCmdBindVertexBuffers(cmd,0,1,&buffer,&offset);}
    if(geometry&&geometry->indexCount){vkCmdBindIndexBuffer(cmd,g.indices->handle(),0,geometry->indexType);
        if(scene){for(const auto& draw:scene->draws){const VkRect2D& drawClip=draw.clipped?draw.clip:scissor;vkCmdSetScissor(cmd,0,1,&drawClip);if(pushBytes)vkCmdPushConstants(cmd,g.layout,pushStages,0,pushBytes,draw.push.data());vkCmdDrawIndexed(cmd,draw.indexCount,1,draw.firstIndex,draw.vertexOffset,0);}}
        else vkCmdDrawIndexed(cmd,geometry->indexCount,1,0,0,0);
    }
    else vkCmdDraw(cmd,geometry?geometry->count:3,1,0,0);
    vkCmdEndRenderPass(cmd);
}
std::vector<uint8_t> finishFrame(Graphics& g,DeviceContext& c,VkQueue queue,uint32_t w,uint32_t h,OffscreenTiming* timing,CpuClock::time_point start){
    VkCommandBuffer cmd=g.commands->handle();auto& target=*g.color;
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};b.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;b.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;b.oldLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;b.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;b.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.image=target.image->handle();b.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&b);
    VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={w,h,1};vkCmdCopyImageToBuffer(cmd,target.image->handle(),VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,target.readback->handle(),1,&copy);g.commands->hostReadBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_TRANSFER_WRITE_BIT);g.commands->end();
    if(timing){timing->recordMs=elapsedCpu(start);start=CpuClock::now();}
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&cmd;check(c.submit(queue,submit,g.fence->handle()));check(g.fence->wait(5'000'000'000ull));
    if(timing){timing->submitWaitMs=elapsedCpu(start);start=CpuClock::now();}
    auto* pixels=target.readback->map();target.readback->invalidate();std::vector<uint8_t> out(size_t(w)*h*4);std::memcpy(out.data(),pixels,out.size());if(timing)timing->readbackMs=elapsedCpu(start);return out;
}
std::vector<uint8_t> renderOffscreen(Graphics& g,DeviceContext& c,VkQueue queue,uint32_t w,uint32_t h,const void* state,const float* background,const VertexGeometry* geometry,uint32_t pushBytes,VkShaderStageFlags pushStages,const SceneState* scene,bool descriptors,bool uploadTexture,OffscreenTiming* timing,const VkRect2D* clip){
    auto start=timing?CpuClock::now():CpuClock::time_point{};
    g.fence->reset();g.commands->begin();
    recordPass(g,g.commands->handle(),w,h,state,background,geometry,pushBytes,pushStages,scene,descriptors,uploadTexture,clip);
    return finishFrame(g,c,queue,w,h,timing,start);
}
}
std::vector<uint8_t> drawOffscreen(DeviceContext& c,VkQueue queue,uint32_t w,uint32_t h,const std::vector<uint32_t>& vert,const std::vector<uint32_t>& frag,const void* state,const float* background,const VertexGeometry* geometry,uint32_t pushBytes,VkShaderStageFlags pushStages,const SceneState* scene,const std::vector<uint8_t>* uniform,const SampledPixels* sampled,SampledImage* uploaded,OffscreenTiming* timing,const RasterOptions* raster,const VkRect2D* clip){
    if(sampled&&uploaded)throw std::invalid_argument("Only one sampled image source");
    validateClip(clip,w,h);
    auto start=timing?CpuClock::now():CpuClock::time_point{};if(timing)*timing={};
    Graphics g(c);g.init(w,h,vert.data(),vert.size()*4,frag.data(),frag.size()*4,geometry,pushBytes,pushStages,scene,uniform,sampled,uploaded,timing,raster);
    if(timing)timing->initializeMs=elapsedCpu(start);
    return renderOffscreen(g,c,queue,w,h,state,background,geometry,pushBytes,pushStages,scene,uniform||sampled||uploaded,true,timing,clip);
}
struct OffscreenSession::Impl {
    DeviceContext& context;VkQueue queue;uint32_t w,h,pushBytes;VkShaderStageFlags pushStages;
    VertexGeometry geometry;SceneState scene;bool hasGeometry,hasScene,descriptors,first=true,failed=false;
    std::shared_ptr<Impl> previous;uint32_t layerCount=1;Graphics graphics;
    Impl(DeviceContext& c,VkQueue q,uint32_t width,uint32_t height,const std::vector<uint32_t>& vs,const std::vector<uint32_t>& fs,const VertexGeometry* geo,uint32_t bytes,VkShaderStageFlags stages,const SceneState* sc,const std::vector<uint8_t>* uniform,const SampledPixels* sampled,SampledImage* uploaded,const RasterOptions* raster,std::shared_ptr<Impl> prev)
        :context(c),queue(q),w(width),h(height),pushBytes(bytes),pushStages(stages),hasGeometry(geo!=nullptr),hasScene(sc!=nullptr),descriptors(uniform||sampled||uploaded),previous(std::move(prev)),graphics(c){
        if(sampled&&uploaded)throw std::invalid_argument("Only one sampled image source");
        if(previous){if(&previous->context!=&c||previous->queue!=q||previous->w!=w||previous->h!=h||previous->failed||previous->layerCount>=16)throw std::invalid_argument("Incompatible previous layer");layerCount=previous->layerCount+1;}
        if(geo)geometry=*geo;if(sc)scene=*sc;
        graphics.init(w,h,vs.data(),vs.size()*4,fs.data(),fs.size()*4,hasGeometry?&geometry:nullptr,pushBytes,pushStages,hasScene?&scene:nullptr,uniform,sampled,uploaded,nullptr,raster,previous?previous->graphics.color:nullptr);
    }
};
OffscreenSession::OffscreenSession(DeviceContext& c,VkQueue q,uint32_t w,uint32_t h,const std::vector<uint32_t>& vs,const std::vector<uint32_t>& fs,const VertexGeometry* geo,uint32_t bytes,VkShaderStageFlags stages,const SceneState* scene,const std::vector<uint8_t>* uniform,const SampledPixels* sampled,SampledImage* uploaded,const RasterOptions* raster,OffscreenSession* previousLayer)
    :impl_(std::make_shared<Impl>(c,q,w,h,vs,fs,geo,bytes,stages,scene,uniform,sampled,uploaded,raster,previousLayer?previousLayer->impl_:nullptr)){}
OffscreenSession::~OffscreenSession()=default;
std::vector<uint8_t> OffscreenSession::render(const void* state,const float* background,const std::vector<uint8_t>* uniform,OffscreenTiming* timing,const VkRect2D* clip){
    auto& p=*impl_;if(p.failed)throw std::logic_error("Failed offscreen session cannot be reused");
    if(p.previous)throw std::invalid_argument("Use renderLayers for a layered session");
    if(!background||(!p.hasScene&&p.pushBytes&&!state))throw std::invalid_argument("Offscreen render input");
    validateClip(clip,p.w,p.h);
    if(uniform){if(!p.graphics.uniform||uniform->size()!=p.graphics.uniform->size())throw std::invalid_argument("Offscreen uniform size is immutable");std::memcpy(p.graphics.uniform->map(),uniform->data(),uniform->size());p.graphics.uniform->flush();}
    if(timing)*timing={};
    try{auto out=renderOffscreen(p.graphics,p.context,p.queue,p.w,p.h,state,background,p.hasGeometry?&p.geometry:nullptr,p.pushBytes,p.pushStages,p.hasScene?&p.scene:nullptr,p.descriptors,p.first,timing,clip);p.first=false;return out;}
    catch(...){p.failed=true;throw;}
}
std::vector<uint8_t> OffscreenSession::renderLayers(const std::vector<LayerFrame>& inputs,const float* background){
    if(!background||inputs.size()!=impl_->layerCount)throw std::invalid_argument("Layer frame count/clear");
    std::vector<Impl*> layers;for(auto* p=impl_.get();p;p=p->previous.get())layers.push_back(p);std::reverse(layers.begin(),layers.end());
    for(size_t i=0;i<layers.size();i++){
        auto& p=*layers[i];const auto& input=inputs[i];
        if(p.failed)throw std::logic_error("Failed layer cannot be reused");
        if(!p.hasScene&&p.pushBytes&&!input.push)throw std::invalid_argument("Layer push missing");
        validateClip(input.clip,p.w,p.h);
        if(input.uniform&&(!p.graphics.uniform||input.uniform->size()!=p.graphics.uniform->size()))throw std::invalid_argument("Layer UBO size is immutable");
    }
    // Validate all layers before mutating any UBO or recording work.
    for(size_t i=0;i<layers.size();i++)if(inputs[i].uniform){auto& u=*layers[i]->graphics.uniform;std::memcpy(u.map(),inputs[i].uniform->data(),inputs[i].uniform->size());u.flush();}
    try{
        auto& root=*layers.front();root.graphics.fence->reset();root.graphics.commands->begin();
        for(size_t i=0;i<layers.size();i++){auto& p=*layers[i];recordPass(p.graphics,root.graphics.commands->handle(),p.w,p.h,inputs[i].push,background,p.hasGeometry?&p.geometry:nullptr,p.pushBytes,p.pushStages,p.hasScene?&p.scene:nullptr,p.descriptors,p.first,inputs[i].clip);}
        auto pixels=finishFrame(root.graphics,root.context,root.queue,root.w,root.h,nullptr,{});
        for(auto* p:layers)p->first=false;return pixels;
    }catch(...){for(auto* p:layers)p->failed=true;throw;}
}
std::vector<uint8_t> drawTriangle(DeviceContext& c,VkQueue queue,uint32_t w,uint32_t h){
    const float state[4]={0,0,.25f,0},clear[4]={0,0,0,1};
    return drawOffscreen(c,queue,w,h,{std::begin(vertex),std::end(vertex)},
                         {std::begin(fragment),std::end(fragment)},state,clear);
}
}
