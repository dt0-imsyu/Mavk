#pragma once
#include <math.h>
typedef struct {
    uint32_t count,width,height;
    VkRenderPass pass;
    VkPipelineLayout layout;
    VkPipeline pipeline;
    VkShaderModule shaders[2];
    VkImageView views[8];
    VkFramebuffer frames[8];
} MavkTriangle;
static MavkTriangle mavk_triangle;
static void mavk_triangle_record(VkCommandBuffer command,unsigned index,unsigned frame){
    VkClearValue clear={.color={{.02f,.04f,.10f,1}}};
    VkRenderPassBeginInfo begin={.sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,.renderPass=mavk_triangle.pass,.framebuffer=mavk_triangle.frames[index],.renderArea={{0,0},{mavk_triangle.width,mavk_triangle.height}},.clearValueCount=1,.pClearValues=&clear};
    vkCmdBeginRenderPass(command,&begin,VK_SUBPASS_CONTENTS_INLINE);vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,mavk_triangle.pipeline);
    VkViewport viewport={0,0,(float)mavk_triangle.width,(float)mavk_triangle.height,0,1};VkRect2D scissor={{0,0},{mavk_triangle.width,mavk_triangle.height}};
    vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&scissor);
    float state[]={.18f*sinf(frame*.05f),.12f*cosf(frame*.04f),.2f+.6f*(frame%90)/90.0f,.1f};
    vkCmdPushConstants(command,mavk_triangle.layout,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,16,state);
    vkCmdDraw(command,3,1,0,0);vkCmdEndRenderPass(command);
}
static int mavk_triangle_setup(const VkImage *images,unsigned count,uint32_t width,uint32_t height,VkFormat format){
    if(!images||!count||count>8||!width||!height)return fail("triangle shape",-1);
    mavk_triangle.count=count;mavk_triangle.width=width;mavk_triangle.height=height;
    VkAttachmentDescription attachment={.format=format,.samples=VK_SAMPLE_COUNT_1_BIT,.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR,.storeOp=VK_ATTACHMENT_STORE_OP_STORE,.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE,.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE,.initialLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,.finalLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference color={0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass={.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS,.colorAttachmentCount=1,.pColorAttachments=&color};
    VkRenderPassCreateInfo ri={.sType=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,.attachmentCount=1,.pAttachments=&attachment,.subpassCount=1,.pSubpasses=&subpass};
    OK(vkCreateRenderPass(device,&ri,NULL,&mavk_triangle.pass));
    for(unsigned i=0;i<mavk_triangle.count;i++){
        VkImageViewCreateInfo vi={.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,.image=images[i],.viewType=VK_IMAGE_VIEW_TYPE_2D,.format=format,.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1}};
        OK(vkCreateImageView(device,&vi,NULL,&mavk_triangle.views[i]));
        VkFramebufferCreateInfo fi={.sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,.renderPass=mavk_triangle.pass,.attachmentCount=1,.pAttachments=&mavk_triangle.views[i],.width=mavk_triangle.width,.height=mavk_triangle.height,.layers=1};
        OK(vkCreateFramebuffer(device,&fi,NULL,&mavk_triangle.frames[i]));
    }
    const char *paths[]={"/data/user/0/com.droiddeck.launcher.mavk/files/mavk-selected/triangle.vert.spv","/data/user/0/com.droiddeck.launcher.mavk/files/mavk-selected/triangle.frag.spv"};
    for(unsigned i=0;i<2;i++){
        size_t bytes=0;uint32_t *code=read_shader(paths[i],&bytes);if(!code)return fail("existing triangle SPIR-V",-1);
        VkShaderModuleCreateInfo si={.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,.codeSize=bytes,.pCode=code};
        VkResult result=vkCreateShaderModule(device,&si,NULL,&mavk_triangle.shaders[i]);free(code);if(result)return fail("AHB triangle shader",result);
    }
    VkPushConstantRange push={VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,16};
    VkPipelineLayoutCreateInfo li={.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,.pushConstantRangeCount=1,.pPushConstantRanges=&push};OK(vkCreatePipelineLayout(device,&li,NULL,&mavk_triangle.layout));
    VkPipelineShaderStageCreateInfo stages[2]={{.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,.stage=VK_SHADER_STAGE_VERTEX_BIT,.module=mavk_triangle.shaders[0],.pName="main"},{.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,.stage=VK_SHADER_STAGE_FRAGMENT_BIT,.module=mavk_triangle.shaders[1],.pName="main"}};
    VkPipelineVertexInputStateCreateInfo vertex={.sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    VkPipelineInputAssemblyStateCreateInfo assembly={.sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
    VkPipelineViewportStateCreateInfo viewport={.sType=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,.viewportCount=1,.scissorCount=1};
    VkPipelineRasterizationStateCreateInfo raster={.sType=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,.polygonMode=VK_POLYGON_MODE_FILL,.cullMode=VK_CULL_MODE_NONE,.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE,.lineWidth=1};
    VkPipelineMultisampleStateCreateInfo multi={.sType=VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT};
    VkPipelineColorBlendAttachmentState mask={.colorWriteMask=15};VkPipelineColorBlendStateCreateInfo blend={.sType=VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,.attachmentCount=1,.pAttachments=&mask};
    VkDynamicState states[]={VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};VkPipelineDynamicStateCreateInfo dynamic={.sType=VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,.dynamicStateCount=2,.pDynamicStates=states};
    VkGraphicsPipelineCreateInfo pi={.sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,.stageCount=2,.pStages=stages,.pVertexInputState=&vertex,.pInputAssemblyState=&assembly,.pViewportState=&viewport,.pRasterizationState=&raster,.pMultisampleState=&multi,.pColorBlendState=&blend,.pDynamicState=&dynamic,.layout=mavk_triangle.layout,.renderPass=mavk_triangle.pass};
    OK(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&pi,NULL,&mavk_triangle.pipeline));
    return 0;
}
static void mavk_triangle_destroy(void){
    if(mavk_triangle.pipeline)vkDestroyPipeline(device,mavk_triangle.pipeline,NULL);
    if(mavk_triangle.layout)vkDestroyPipelineLayout(device,mavk_triangle.layout,NULL);
    for(unsigned i=0;i<mavk_triangle.count;i++){if(mavk_triangle.frames[i])vkDestroyFramebuffer(device,mavk_triangle.frames[i],NULL);if(mavk_triangle.views[i])vkDestroyImageView(device,mavk_triangle.views[i],NULL);}
    for(unsigned i=0;i<2;i++)if(mavk_triangle.shaders[i])vkDestroyShaderModule(device,mavk_triangle.shaders[i],NULL);
    if(mavk_triangle.pass)vkDestroyRenderPass(device,mavk_triangle.pass,NULL);
    mavk_triangle=(MavkTriangle){0};
}
