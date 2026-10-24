// Standard D3D11 consumer of the real pinned Sarek implementation,not Mavk API.
#include <d3d11.h>
#include <dlfcn.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <vector>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string>
#include "d3d11_video_blit_vert.h"
#include "d3d11_video_blit_frag.h"
#include "sarek-blit-draw.hpp"
#include "sarek-frame-vs.h"
#include "sarek-frame-ps.h"
#include "sarek-texture-ps.h"
#include "sarek-filtered-ps.h"

static bool d3d11DrawFrame(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11RenderTargetView* view,ID3D11Texture2D* target,ID3D11Texture2D* readback){
    ID3D11VertexShader* vs=nullptr;ID3D11PixelShader* ps=nullptr;ID3D11RasterizerState* raster=nullptr;
    ID3D11Texture2D* texture=nullptr;ID3D11ShaderResourceView* textureView=nullptr;ID3D11PixelShader* texturePS=nullptr;
    ID3D11PixelShader* filteredPS=nullptr;ID3D11SamplerState* filters[2]{};
    auto finish=[&](bool ok){context->ClearState();for(auto f:filters)if(f)f->Release();if(filteredPS)filteredPS->Release();if(texturePS)texturePS->Release();if(textureView)textureView->Release();if(texture)texture->Release();if(raster)raster->Release();if(ps)ps->Release();if(vs)vs->Release();return ok;};
    HRESULT hr=device->CreateVertexShader(mavkFrameVS,sizeof(mavkFrameVS),nullptr,&vs);
    if(FAILED(hr)){std::fprintf(stderr,"SAREK_VERTEX_SHADER_FAIL=0x%08x\n",unsigned(hr));return finish(false);}
    hr=device->CreatePixelShader(mavkFramePS,sizeof(mavkFramePS),nullptr,&ps);
    if(FAILED(hr)){std::fprintf(stderr,"SAREK_PIXEL_SHADER_FAIL=0x%08x\n",unsigned(hr));return finish(false);}
    D3D11_RASTERIZER_DESC rs{};rs.FillMode=D3D11_FILL_SOLID;rs.CullMode=D3D11_CULL_NONE;rs.DepthClipEnable=TRUE;
    hr=device->CreateRasterizerState(&rs,&raster);if(FAILED(hr))return finish(false);
    D3D11_VIEWPORT viewport{0,0,64,64,0,1};context->RSSetViewports(1,&viewport);context->RSSetState(raster);
    context->OMSetRenderTargets(1,&view,nullptr);context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vs,nullptr,0);context->PSSetShader(ps,nullptr,0);
    std::printf("SAREK_STAGE=D3D11Draw\n");std::fflush(stdout);context->Draw(3,0);context->CopyResource(readback,target);
    D3D11_MAPPED_SUBRESOURCE pixels{};hr=context->Map(readback,0,D3D11_MAP_READ,0,&pixels);
    if(FAILED(hr)){std::fprintf(stderr,"SAREK_DRAW_MAP_FAIL=0x%08x\n",unsigned(hr));return finish(false);}
    const unsigned expected[]={64,191,128,255};unsigned verified=0;bool ok=true;
    for(unsigned y=0;y<64&&ok;y++){auto row=static_cast<const unsigned char*>(pixels.pData)+y*pixels.RowPitch;for(unsigned x=0;x<64&&ok;x++)for(unsigned c=0;c<4;c++){
        if(std::abs(int(row[4*x+c])-int(expected[c]))>1){std::fprintf(stderr,"SAREK_DRAW_PIXEL_FAIL=%u,%u,%u:%u\n",x,y,c,unsigned(row[4*x+c]));ok=false;break;}verified++;
    }}
    context->Unmap(readback,0);if(ok)std::printf("SAREK_D3D11_DRAW_READBACK_PASS components=%u\n",verified);std::fflush(stdout);if(!ok)return finish(false);
    unsigned char texels[4*4*4];
    for(unsigned y=0;y<4;y++)for(unsigned x=0;x<4;x++){auto p=texels+4*(y*4+x);p[0]=20+x*50;p[1]=30+y*50;p[2]=40+(x^y)*40;p[3]=255;}
    D3D11_TEXTURE2D_DESC td{};td.Width=td.Height=4;td.MipLevels=td.ArraySize=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.SampleDesc.Count=1;td.Usage=D3D11_USAGE_IMMUTABLE;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA data{texels,16,64};hr=device->CreateTexture2D(&td,&data,&texture);
    if(FAILED(hr)){std::fprintf(stderr,"SAREK_TEXTURE_FAIL=0x%08x\n",unsigned(hr));return finish(false);}
    hr=device->CreateShaderResourceView(texture,nullptr,&textureView);if(FAILED(hr))return finish(false);
    hr=device->CreatePixelShader(mavkTexturePS,sizeof(mavkTexturePS),nullptr,&texturePS);if(FAILED(hr))return finish(false);
    context->PSSetShader(texturePS,nullptr,0);context->PSSetShaderResources(0,1,&textureView);
    std::printf("SAREK_STAGE=D3D11TexturedDraw\n");std::fflush(stdout);context->Draw(3,0);context->CopyResource(readback,target);
    hr=context->Map(readback,0,D3D11_MAP_READ,0,&pixels);if(FAILED(hr)){std::fprintf(stderr,"SAREK_TEXTURE_MAP_FAIL=0x%08x\n",unsigned(hr));return finish(false);}
    verified=0;ok=true;
    for(unsigned y=0;y<64&&ok;y++){auto row=static_cast<const unsigned char*>(pixels.pData)+y*pixels.RowPitch;for(unsigned x=0;x<64&&ok;x++)for(unsigned c=0;c<4;c++){
        auto expected=texels[4*((y&3)*4+(x&3))+c];if(std::abs(int(row[4*x+c])-int(expected))>1){std::fprintf(stderr,"SAREK_TEXTURE_PIXEL_FAIL=%u,%u,%u:%u expected=%u\n",x,y,c,unsigned(row[4*x+c]),unsigned(expected));ok=false;break;}verified++;
    }}
    context->Unmap(readback,0);if(ok)std::printf("SAREK_D3D11_TEXTURE_READBACK_PASS components=%u texels=16\n",verified);std::fflush(stdout);if(!ok)return finish(false);
    hr=device->CreatePixelShader(mavkFilteredPS,sizeof(mavkFilteredPS),nullptr,&filteredPS);if(FAILED(hr))return finish(false);
    context->PSSetShader(filteredPS,nullptr,0);
    for(unsigned mode=0;mode<2;mode++){
        D3D11_SAMPLER_DESC sd{};sd.Filter=mode?D3D11_FILTER_MIN_MAG_MIP_LINEAR:D3D11_FILTER_MIN_MAG_MIP_POINT;
        sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;sd.MaxAnisotropy=1;sd.ComparisonFunc=D3D11_COMPARISON_NEVER;
        hr=device->CreateSamplerState(&sd,&filters[mode]);if(FAILED(hr)){std::fprintf(stderr,"SAREK_SAMPLER_FAIL=0x%08x\n",unsigned(hr));return finish(false);}
        context->PSSetSamplers(0,1,&filters[mode]);std::printf("SAREK_STAGE=D3D11FilteredDraw mode=%u\n",mode);std::fflush(stdout);
        context->Draw(3,0);context->CopyResource(readback,target);hr=context->Map(readback,0,D3D11_MAP_READ,0,&pixels);if(FAILED(hr))return finish(false);
        verified=0;ok=true;
        for(unsigned y=0;y<64&&ok;y++){auto row=static_cast<const unsigned char*>(pixels.pData)+y*pixels.RowPitch;for(unsigned x=0;x<64&&ok;x++)for(unsigned c=0;c<4;c++){
            double expected=texels[4*(((y/2)&3)*4+((x/2)&3))+c];
            if(mode){double u=x/2.0-0.25,v=y/2.0-0.25;int ix=int(std::floor(u)),iy=int(std::floor(v));double fx=u-ix,fy=v-iy;expected=0;
                for(int j=0;j<2;j++)for(int i=0;i<2;i++)expected+=texels[4*((unsigned(iy+j)&3)*4+(unsigned(ix+i)&3))+c]*(i?fx:1-fx)*(j?fy:1-fy);
            }
            if(std::abs(double(row[4*x+c])-expected)>2){std::fprintf(stderr,"SAREK_FILTER_PIXEL_FAIL mode=%u pixel=%u,%u,%u got=%u expected=%.2f\n",mode,x,y,c,unsigned(row[4*x+c]),expected);ok=false;break;}verified++;
        }}
        context->Unmap(readback,0);if(!ok)return finish(false);std::printf("SAREK_D3D11_FILTER_READBACK_PASS mode=%u components=%u\n",mode,verified);std::fflush(stdout);
    }
    return finish(true);
}

// Small standard-API lifecycle regression in the REAL consumer startup,not a
// separate Suite workload. The layer must see actual buffer/set writes/copies
// and pool retirement before we trust it in D3D11's more complex API traffic.
static bool descriptorLifecycle(VkPhysicalDevice gpu,VkDevice device,PFN_vkGetInstanceProcAddr gipa,VkInstance instance,PFN_vkGetDeviceProcAddr gdpa){
#define DEVICE_PROC(name) auto name=reinterpret_cast<PFN_##name>(gdpa(device,#name))
    DEVICE_PROC(vkCreateBuffer);DEVICE_PROC(vkDestroyBuffer);DEVICE_PROC(vkGetBufferMemoryRequirements);
    DEVICE_PROC(vkAllocateMemory);DEVICE_PROC(vkFreeMemory);DEVICE_PROC(vkBindBufferMemory);
    DEVICE_PROC(vkCreateImage);DEVICE_PROC(vkDestroyImage);DEVICE_PROC(vkGetImageMemoryRequirements);DEVICE_PROC(vkBindImageMemory);
    DEVICE_PROC(vkCreateImageView);DEVICE_PROC(vkDestroyImageView);DEVICE_PROC(vkCreateSampler);DEVICE_PROC(vkDestroySampler);
    DEVICE_PROC(vkCreateBufferView);DEVICE_PROC(vkDestroyBufferView);
    DEVICE_PROC(vkCreateDescriptorSetLayout);DEVICE_PROC(vkDestroyDescriptorSetLayout);
    DEVICE_PROC(vkCreateDescriptorPool);DEVICE_PROC(vkDestroyDescriptorPool);DEVICE_PROC(vkAllocateDescriptorSets);
    DEVICE_PROC(vkUpdateDescriptorSets);DEVICE_PROC(vkFreeDescriptorSets);DEVICE_PROC(vkResetDescriptorPool);
    DEVICE_PROC(vkCreateDescriptorUpdateTemplate);DEVICE_PROC(vkDestroyDescriptorUpdateTemplate);DEVICE_PROC(vkUpdateDescriptorSetWithTemplate);
#undef DEVICE_PROC
    VkBuffer buffer{};VkDeviceMemory memory{},imageMemory{};VkImage image{};VkImageView view{};VkSampler sampler{};VkBufferView texel{};
    VkDescriptorSetLayout layout{};VkDescriptorPool pool{};VkDescriptorUpdateTemplate descriptorTemplate{};
    auto finish=[&](bool ok){
        if(pool)vkDestroyDescriptorPool(device,pool,nullptr);
        if(descriptorTemplate)vkDestroyDescriptorUpdateTemplate(device,descriptorTemplate,nullptr);
        if(layout)vkDestroyDescriptorSetLayout(device,layout,nullptr);
        if(view)vkDestroyImageView(device,view,nullptr);if(sampler)vkDestroySampler(device,sampler,nullptr);if(texel)vkDestroyBufferView(device,texel,nullptr);
        if(image)vkDestroyImage(device,image,nullptr);if(imageMemory)vkFreeMemory(device,imageMemory,nullptr);
        if(buffer)vkDestroyBuffer(device,buffer,nullptr);if(memory)vkFreeMemory(device,memory,nullptr);
        std::printf("VULKAN_DESCRIPTOR_LIFECYCLE=%s kinds=7 template=1\n",ok?"PASS":"FAIL");std::fflush(stdout);return ok;
    };
    VkPhysicalDeviceMemoryProperties properties{};auto query=reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(gipa(instance,"vkGetPhysicalDeviceMemoryProperties"));query(gpu,&properties);
    auto allocate=[&](const VkMemoryRequirements& requirements,VkDeviceMemory& out){
        uint32_t type=0;while(type<properties.memoryTypeCount&&!(requirements.memoryTypeBits&(1u<<type)))type++;
        if(type==properties.memoryTypeCount)return false;VkMemoryAllocateInfo mi{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};mi.allocationSize=requirements.size;mi.memoryTypeIndex=type;
        return vkAllocateMemory(device,&mi,nullptr,&out)==VK_SUCCESS;
    };
    VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};bi.size=256;bi.usage=VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT|VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
    if(vkCreateBuffer(device,&bi,nullptr,&buffer)!=VK_SUCCESS)return finish(false);
    VkMemoryRequirements requirements{};vkGetBufferMemoryRequirements(device,buffer,&requirements);
    if(!allocate(requirements,memory)||vkBindBufferMemory(device,buffer,memory,0)!=VK_SUCCESS)return finish(false);
    VkBufferViewCreateInfo ti{VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};ti.buffer=buffer;ti.format=VK_FORMAT_R32_UINT;ti.range=64;
    if(vkCreateBufferView(device,&ti,nullptr,&texel)!=VK_SUCCESS)return finish(false);
    VkImageCreateInfo ii{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};ii.imageType=VK_IMAGE_TYPE_2D;ii.format=VK_FORMAT_R8G8B8A8_UNORM;ii.extent={8,8,1};ii.mipLevels=1;ii.arrayLayers=1;ii.samples=VK_SAMPLE_COUNT_1_BIT;ii.tiling=VK_IMAGE_TILING_OPTIMAL;ii.usage=VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_STORAGE_BIT;
    if(vkCreateImage(device,&ii,nullptr,&image)!=VK_SUCCESS)return finish(false);
    vkGetImageMemoryRequirements(device,image,&requirements);
    if(!allocate(requirements,imageMemory)||vkBindImageMemory(device,image,imageMemory,0)!=VK_SUCCESS)return finish(false);
    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};vi.image=image;vi.viewType=VK_IMAGE_VIEW_TYPE_2D;vi.format=ii.format;vi.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
    if(vkCreateImageView(device,&vi,nullptr,&view)!=VK_SUCCESS)return finish(false);
    VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};si.magFilter=si.minFilter=VK_FILTER_NEAREST;si.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST;si.addressModeU=si.addressModeV=si.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;si.maxAnisotropy=1;
    if(vkCreateSampler(device,&si,nullptr,&sampler)!=VK_SUCCESS)return finish(false);
    constexpr VkDescriptorType types[]={VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,VK_DESCRIPTOR_TYPE_SAMPLER,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER};
    VkDescriptorSetLayoutBinding bindings[7]{};VkDescriptorPoolSize sizes[7]{};
    for(uint32_t i=0;i<7;i++){bindings[i]={i,types[i],1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr};sizes[i]={types[i],2};}
    VkDescriptorSetLayoutCreateInfo li{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};li.bindingCount=7;li.pBindings=bindings;
    if(vkCreateDescriptorSetLayout(device,&li,nullptr,&layout)!=VK_SUCCESS)return finish(false);
    struct Data {VkDescriptorBufferInfo buffer;VkDescriptorImageInfo images[4];VkBufferView texels[2];};
    Data data{{buffer,0,64},{{VK_NULL_HANDLE,view,VK_IMAGE_LAYOUT_GENERAL},{sampler,VK_NULL_HANDLE,VK_IMAGE_LAYOUT_UNDEFINED},{sampler,view,VK_IMAGE_LAYOUT_GENERAL},{VK_NULL_HANDLE,view,VK_IMAGE_LAYOUT_GENERAL}},{texel,texel}};
    VkDescriptorUpdateTemplateEntry entries[7]{};
    for(uint32_t i=0;i<7;i++){
        entries[i].dstBinding=i;entries[i].descriptorCount=1;entries[i].descriptorType=types[i];
        entries[i].offset=i==0?offsetof(Data,buffer):i<5?offsetof(Data,images)+sizeof(VkDescriptorImageInfo)*(i-1):offsetof(Data,texels)+sizeof(VkBufferView)*(i-5);
    }
    VkDescriptorUpdateTemplateCreateInfo ci{VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO};ci.descriptorUpdateEntryCount=7;ci.pDescriptorUpdateEntries=entries;ci.templateType=VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET;ci.descriptorSetLayout=layout;
    // Template creation intentionally precedes any set allocation.
    if(!vkCreateDescriptorUpdateTemplate||!vkUpdateDescriptorSetWithTemplate||!vkDestroyDescriptorUpdateTemplate||vkCreateDescriptorUpdateTemplate(device,&ci,nullptr,&descriptorTemplate)!=VK_SUCCESS)return finish(false);
    VkDescriptorPoolCreateInfo pi{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pi.flags=VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;pi.maxSets=2;pi.poolSizeCount=7;pi.pPoolSizes=sizes;
    if(vkCreateDescriptorPool(device,&pi,nullptr,&pool)!=VK_SUCCESS)return finish(false);
    VkDescriptorSetLayout layouts[]={layout,layout};VkDescriptorSet sets[2]{};VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};ai.descriptorPool=pool;ai.descriptorSetCount=2;ai.pSetLayouts=layouts;
    if(vkAllocateDescriptorSets(device,&ai,sets)!=VK_SUCCESS)return finish(false);
    VkWriteDescriptorSet writes[7]{};VkCopyDescriptorSet copies[7]{};
    for(uint32_t i=0;i<7;i++){
        auto& w=writes[i];w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w.dstSet=sets[0];w.dstBinding=i;w.descriptorType=types[i];w.descriptorCount=1;
        if(i==0)w.pBufferInfo=&data.buffer;else if(i<5)w.pImageInfo=&data.images[i-1];else w.pTexelBufferView=&data.texels[i-5];
        auto& c=copies[i];c.sType=VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;c.srcSet=sets[0];c.dstSet=sets[1];c.srcBinding=c.dstBinding=i;c.descriptorCount=1;
    }
    vkUpdateDescriptorSets(device,7,writes,7,copies);data.buffer.range=128;
    vkUpdateDescriptorSetWithTemplate(device,sets[1],descriptorTemplate,&data);
    if(vkFreeDescriptorSets(device,pool,1,sets)!=VK_SUCCESS||vkResetDescriptorPool(device,pool,0)!=VK_SUCCESS)return finish(false);
    return finish(true);
}

// Actual Sarek vertex/fragment SPIR-V from its pinned build,not a new shader
// test. Create a normal native pipeline so the layer receives real pipeline API.
// Draw and read back the real shader pair;guarded execution is separately
// enabled in the explicit layer,not assumed from successful pipeline creation.
static bool shaderPipelineLifecycle(VkPhysicalDevice gpu,VkDevice device,PFN_vkGetInstanceProcAddr gipa,VkInstance instance,PFN_vkGetDeviceProcAddr gdpa,uint32_t family){
#define DEVICE_PROC(name) auto name=reinterpret_cast<PFN_##name>(gdpa(device,#name))
    DEVICE_PROC(vkCreateShaderModule);DEVICE_PROC(vkDestroyShaderModule);DEVICE_PROC(vkCreateDescriptorSetLayout);DEVICE_PROC(vkDestroyDescriptorSetLayout);
    DEVICE_PROC(vkCreatePipelineLayout);DEVICE_PROC(vkDestroyPipelineLayout);DEVICE_PROC(vkCreateRenderPass);DEVICE_PROC(vkDestroyRenderPass);
    DEVICE_PROC(vkCreateGraphicsPipelines);DEVICE_PROC(vkDestroyPipeline);
#undef DEVICE_PROC
    VkShaderModule vertex{},fragment{};VkDescriptorSetLayout set{};VkPipelineLayout layout{};VkRenderPass renderPass{};VkPipeline pipeline{};
    auto finish=[&](bool ok){
        if(pipeline)vkDestroyPipeline(device,pipeline,nullptr);if(renderPass)vkDestroyRenderPass(device,renderPass,nullptr);
        if(layout)vkDestroyPipelineLayout(device,layout,nullptr);if(set)vkDestroyDescriptorSetLayout(device,set,nullptr);
        if(vertex)vkDestroyShaderModule(device,vertex,nullptr);if(fragment)vkDestroyShaderModule(device,fragment,nullptr);
        std::printf("VULKAN_SAREK_PIPELINE_LIFECYCLE=%s nativeShaders=2 drawReadback=1\n",ok?"PASS":"FAIL");std::fflush(stdout);return ok;
    };
    VkShaderModuleCreateInfo sm{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};sm.codeSize=sizeof(d3d11_video_blit_vert);sm.pCode=d3d11_video_blit_vert;
    if(vkCreateShaderModule(device,&sm,nullptr,&vertex)!=VK_SUCCESS)return finish(false);
    sm.codeSize=sizeof(d3d11_video_blit_frag);sm.pCode=d3d11_video_blit_frag;
    if(vkCreateShaderModule(device,&sm,nullptr,&fragment)!=VK_SUCCESS)return finish(false);
    VkDescriptorSetLayoutBinding bindings[]={{0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},{1,VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},{2,VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr}};
    VkDescriptorSetLayoutCreateInfo si{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};si.bindingCount=3;si.pBindings=bindings;
    if(vkCreateDescriptorSetLayout(device,&si,nullptr,&set)!=VK_SUCCESS)return finish(false);
    VkPipelineLayoutCreateInfo li{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};li.setLayoutCount=1;li.pSetLayouts=&set;
    if(vkCreatePipelineLayout(device,&li,nullptr,&layout)!=VK_SUCCESS)return finish(false);
    VkAttachmentDescription attachment{};attachment.format=VK_FORMAT_R8G8B8A8_UNORM;attachment.samples=VK_SAMPLE_COUNT_1_BIT;attachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;attachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;attachment.finalLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkAttachmentReference reference{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};VkSubpassDescription subpass{};subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;subpass.colorAttachmentCount=1;subpass.pColorAttachments=&reference;
    VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};rp.attachmentCount=1;rp.pAttachments=&attachment;rp.subpassCount=1;rp.pSubpasses=&subpass;
    if(vkCreateRenderPass(device,&rp,nullptr,&renderPass)!=VK_SUCCESS)return finish(false);
    VkPipelineShaderStageCreateInfo stages[2]{};for(auto& stage:stages){stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;stage.pName="main";}
    stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;stages[0].module=vertex;stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT;stages[1].module=fragment;
    VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkViewport viewport{0,0,64,64,0,1};VkRect2D scissor{{0,0},{64,64}};VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};vp.viewportCount=1;vp.pViewports=&viewport;vp.scissorCount=1;vp.pScissors=&scissor;
    VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};rs.polygonMode=VK_POLYGON_MODE_FILL;rs.cullMode=VK_CULL_MODE_NONE;rs.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE;rs.lineWidth=1;
    VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
    VkPipelineColorBlendAttachmentState color{};color.colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};blend.attachmentCount=1;blend.pAttachments=&color;
    VkDynamicState scissorDynamic=VK_DYNAMIC_STATE_SCISSOR;VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dynamic.dynamicStateCount=1;dynamic.pDynamicStates=&scissorDynamic;
    VkGraphicsPipelineCreateInfo ci{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};ci.stageCount=2;ci.pStages=stages;ci.pVertexInputState=&input;ci.pInputAssemblyState=&assembly;ci.pViewportState=&vp;ci.pRasterizationState=&rs;ci.pMultisampleState=&ms;ci.pColorBlendState=&blend;ci.layout=layout;ci.renderPass=renderPass;ci.pDynamicState=&dynamic;
    VkResult result=vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&ci,nullptr,&pipeline);
    if(result!=VK_SUCCESS)std::fprintf(stderr,"VULKAN_SAREK_PIPELINE_FAIL=%d\n",int(result));return finish(result==VK_SUCCESS&&drawSarekBlit(gpu,device,gipa,instance,gdpa,family,pipeline,layout,set,renderPass));
}

static bool vulkanPreflight(){
    void* loader=dlopen("libvulkan.so.1",RTLD_NOW|RTLD_LOCAL);
    if(!loader){std::fprintf(stderr,"VULKAN_LOADER_FAIL=%s\n",dlerror());return false;}
    auto gipa=reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(loader,"vkGetInstanceProcAddr"));
    if(!gipa)return false;
    auto create=reinterpret_cast<PFN_vkCreateInstance>(gipa(VK_NULL_HANDLE,"vkCreateInstance"));
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.pApplicationName="Sarek real consumer preflight";app.apiVersion=VK_API_VERSION_1_3;
    VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};ci.pApplicationInfo=&app;VkInstance instance{};
    VkResult result=create?create(&ci,nullptr,&instance):VK_ERROR_INITIALIZATION_FAILED;
    std::printf("VULKAN_CREATE_INSTANCE=%d\n",int(result));std::fflush(stdout);if(result!=VK_SUCCESS)return false;
    auto destroy=reinterpret_cast<PFN_vkDestroyInstance>(gipa(instance,"vkDestroyInstance"));
    auto enumerate=reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(gipa(instance,"vkEnumeratePhysicalDevices"));
    auto properties=reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(gipa(instance,"vkGetPhysicalDeviceProperties"));
    auto queues=reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(gipa(instance,"vkGetPhysicalDeviceQueueFamilyProperties"));
    auto createDevice=reinterpret_cast<PFN_vkCreateDevice>(gipa(instance,"vkCreateDevice"));
    auto gdpa=reinterpret_cast<PFN_vkGetDeviceProcAddr>(gipa(instance,"vkGetDeviceProcAddr"));
    uint32_t count=0;result=enumerate(instance,&count,nullptr);bool ok=false;
    if(result==VK_SUCCESS&&count&&count<=32){std::vector<VkPhysicalDevice> devices(count);result=enumerate(instance,&count,devices.data());
        if(result==VK_SUCCESS)for(auto physical:devices){VkPhysicalDeviceProperties props{};properties(physical,&props);
            std::printf("VULKAN_DEVICE=%s vendor=0x%x type=%u\n",props.deviceName,props.vendorID,unsigned(props.deviceType));
            if(props.vendorID!=0x13b5||props.deviceType==VK_PHYSICAL_DEVICE_TYPE_CPU)continue;
            uint32_t n=0;queues(physical,&n,nullptr);if(!n||n>256)continue;std::vector<VkQueueFamilyProperties> families(n);queues(physical,&n,families.data());
            for(uint32_t i=0;i<n;i++)if(families[i].queueCount&&(families[i].queueFlags&VK_QUEUE_GRAPHICS_BIT)){
                float priority=1;VkDeviceQueueCreateInfo qi{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};qi.queueFamilyIndex=i;qi.queueCount=1;qi.pQueuePriorities=&priority;
                VkPhysicalDeviceFeatures supported{},enabled{};reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures>(gipa(instance,"vkGetPhysicalDeviceFeatures"))(physical,&supported);enabled.robustBufferAccess=supported.robustBufferAccess;enabled.fragmentStoresAndAtomics=supported.fragmentStoresAndAtomics;
                VkPhysicalDeviceVulkan13Features supported13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES},enabled13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
                VkPhysicalDeviceFeatures2 query{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};query.pNext=&supported13;
                reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2>(gipa(instance,"vkGetPhysicalDeviceFeatures2"))(physical,&query);enabled13.synchronization2=supported13.synchronization2;
                VkDeviceCreateInfo di{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};di.queueCreateInfoCount=1;di.pQueueCreateInfos=&qi;di.pEnabledFeatures=&enabled;VkDevice device{};
                di.pNext=&enabled13;if(std::getenv("MAVK_TEST_SUBMIT2")&&!enabled13.synchronization2){std::fprintf(stderr,"required native synchronization2 unavailable\n");continue;}
                result=createDevice(physical,&di,nullptr,&device);std::printf("VULKAN_CREATE_DEVICE=%d\n",int(result));
                if(result==VK_SUCCESS){ok=descriptorLifecycle(physical,device,gipa,instance,gdpa)&&shaderPipelineLifecycle(physical,device,gipa,instance,gdpa,i);auto dd=reinterpret_cast<PFN_vkDestroyDevice>(gdpa(device,"vkDestroyDevice"));dd(device,nullptr);}break;
            }if(ok)break;
        }
    }
    destroy(instance,nullptr);std::printf("VULKAN_PREFLIGHT=%s\n",ok?"PASS":"FAIL");std::fflush(stdout);return ok;
}

int main(int argc,char** argv){
    if(argc!=2){std::fprintf(stderr,"usage: sarek-d3d11-consumer libdxvk_d3d11.so\n");return 2;}
    if(!vulkanPreflight())return 9;
    std::printf("SAREK_STAGE=load_d3d11\n");std::fflush(stdout);
    void* library=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
    if(!library){std::fprintf(stderr,"SAREK_LOAD_FAIL=%s\n",dlerror());return 10;}
    auto create=reinterpret_cast<decltype(&D3D11CreateDevice)>(dlsym(library,"D3D11CreateDevice"));
    if(!create){std::fprintf(stderr,"SAREK_ENTRY_FAIL=%s\n",dlerror());return 11;}
    ID3D11Device* device=nullptr;ID3D11DeviceContext* context=nullptr;D3D_FEATURE_LEVEL chosen{};D3D_FEATURE_LEVEL requested=D3D_FEATURE_LEVEL_11_0;
    std::printf("SAREK_STAGE=D3D11CreateDevice\n");std::fflush(stdout);
    HRESULT hr=create(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,&requested,1,D3D11_SDK_VERSION,&device,&chosen,&context);
    std::printf("SAREK_DEVICE_HRESULT=0x%08x FEATURE_LEVEL=0x%x DEVICE=%d CONTEXT=%d\n",unsigned(hr),unsigned(chosen),device!=nullptr,context!=nullptr);std::fflush(stdout);
    if(FAILED(hr)||!device||!context){if(context)context->Release();if(device)device->Release();return 20;}
    ID3D11Texture2D* target=nullptr;ID3D11Texture2D* readback=nullptr;ID3D11RenderTargetView* view=nullptr;bool mapped=false;
    auto finish=[&](int code){if(mapped)context->Unmap(readback,0);if(view)view->Release();if(readback)readback->Release();if(target)target->Release();context->ClearState();context->Flush();context->Release();device->Release();return code;};
    D3D11_TEXTURE2D_DESC desc{};desc.Width=64;desc.Height=64;desc.MipLevels=1;desc.ArraySize=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_RENDER_TARGET;
    hr=device->CreateTexture2D(&desc,nullptr,&target);if(FAILED(hr)){std::fprintf(stderr,"SAREK_TARGET_FAIL=0x%08x\n",unsigned(hr));return finish(30);}
    hr=device->CreateRenderTargetView(target,nullptr,&view);if(FAILED(hr)){std::fprintf(stderr,"SAREK_RTV_FAIL=0x%08x\n",unsigned(hr));return finish(31);}
    desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;hr=device->CreateTexture2D(&desc,nullptr,&readback);if(FAILED(hr)){std::fprintf(stderr,"SAREK_STAGING_FAIL=0x%08x\n",unsigned(hr));return finish(32);}
    const float color[]={0.25f,0.5f,0.75f,1.0f};context->ClearRenderTargetView(view,color);context->CopyResource(readback,target);D3D11_MAPPED_SUBRESOURCE pixels{};
    hr=context->Map(readback,0,D3D11_MAP_READ,0,&pixels);if(FAILED(hr)){std::fprintf(stderr,"SAREK_MAP_FAIL=0x%08x\n",unsigned(hr));return finish(33);}mapped=true;
    const unsigned expected[]={64,128,191,255};unsigned verified=0;
    for(unsigned y=0;y<64;y++){auto row=static_cast<const unsigned char*>(pixels.pData)+y*pixels.RowPitch;for(unsigned x=0;x<64;x++)for(unsigned c=0;c<4;c++){if(std::abs(int(row[4*x+c])-int(expected[c]))>1){std::fprintf(stderr,"SAREK_PIXEL_FAIL=%u,%u,%u:%u\n",x,y,c,unsigned(row[4*x+c]));return finish(34);}verified++;}}
    std::printf("SAREK_D3D11_CLEAR_READBACK_PASS components=%u\n",verified);std::fflush(stdout);
    context->Unmap(readback,0);mapped=false;
    return finish(d3d11DrawFrame(device,context,view,target,readback)?0:35);
}
