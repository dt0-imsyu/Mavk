// Standalone Vulkan API consumer. No Mavk headers, protocol, extension or DLL API.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <cstdio>
#include <stdexcept>
#include <cmath>
#include <cstring>

static const uint32_t vertex[]=
#ifdef TEST_UNIFORM_BUFFER
#include "uniform.vert.inc"
#elif defined(TEST_MATRIX)
#include "matrix.vert.inc"
#elif defined(TEST_VERTEX_BUFFER)
#include "vertex-buffer.vert.inc"
#else
#include "offscreen.vert.inc"
#endif
;
static const uint32_t fragment[]=
#include "offscreen.frag.inc"
;
static void check(VkResult r,const char* operation){if(r!=VK_SUCCESS){std::printf("VK_FAILURE=%s:%d\n",operation,int(r));throw std::runtime_error(operation);}}
int main(){try{
    HMODULE loader=LoadLibraryW(L"vulkan-1.dll");if(!loader)throw std::runtime_error("Standard vulkan-1.dll unavailable");
    auto get=reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(loader,"vkGetInstanceProcAddr"));if(!get)throw std::runtime_error("Loader export");
    auto create=reinterpret_cast<PFN_vkCreateInstance>(get(VK_NULL_HANDLE,"vkCreateInstance"));if(!create)throw std::runtime_error("Loader CreateInstance");
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.pApplicationName="Vulkan offscreen pixel check";app.apiVersion=VK_API_VERSION_1_0;
    VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};ici.pApplicationInfo=&app;VkInstance instance{};check(create(&ici,nullptr,&instance),"CreateInstance");
#define LOAD(name) auto name=reinterpret_cast<PFN_##name>(get(instance,#name));if(!name)throw std::runtime_error("Missing " #name)
    LOAD(vkDestroyInstance);LOAD(vkEnumeratePhysicalDevices);LOAD(vkGetPhysicalDeviceProperties);LOAD(vkGetPhysicalDeviceQueueFamilyProperties);LOAD(vkCreateDevice);LOAD(vkDestroyDevice);LOAD(vkGetDeviceQueue);LOAD(vkCreateImage);LOAD(vkGetImageMemoryRequirements);LOAD(vkAllocateMemory);LOAD(vkBindImageMemory);LOAD(vkCreateBuffer);LOAD(vkGetBufferMemoryRequirements);LOAD(vkBindBufferMemory);LOAD(vkCreateImageView);LOAD(vkCreateRenderPass);LOAD(vkCreateFramebuffer);LOAD(vkCreateShaderModule);LOAD(vkCreatePipelineLayout);LOAD(vkCreateGraphicsPipelines);LOAD(vkCreateCommandPool);LOAD(vkAllocateCommandBuffers);LOAD(vkBeginCommandBuffer);LOAD(vkCmdBeginRenderPass);LOAD(vkCmdBindPipeline);LOAD(vkCmdSetViewport);LOAD(vkCmdSetScissor);LOAD(vkCmdPushConstants);LOAD(vkCmdDraw);LOAD(vkCmdEndRenderPass);LOAD(vkCmdPipelineBarrier);LOAD(vkCmdCopyImageToBuffer);LOAD(vkEndCommandBuffer);LOAD(vkCreateFence);LOAD(vkQueueSubmit);LOAD(vkWaitForFences);LOAD(vkMapMemory);LOAD(vkInvalidateMappedMemoryRanges);LOAD(vkUnmapMemory);LOAD(vkDestroyFence);LOAD(vkFreeCommandBuffers);LOAD(vkDestroyCommandPool);LOAD(vkDestroyPipeline);LOAD(vkDestroyPipelineLayout);LOAD(vkDestroyShaderModule);LOAD(vkDestroyFramebuffer);LOAD(vkDestroyRenderPass);LOAD(vkDestroyImageView);LOAD(vkDestroyBuffer);LOAD(vkDestroyImage);LOAD(vkFreeMemory);
#undef LOAD
#ifdef TEST_VERTEX_BUFFER
    auto vkCmdBindVertexBuffers=reinterpret_cast<PFN_vkCmdBindVertexBuffers>(get(instance,"vkCmdBindVertexBuffers"));if(!vkCmdBindVertexBuffers)throw std::runtime_error("Missing vkCmdBindVertexBuffers");
#endif
#ifdef TEST_INDEXED_BUFFER
#define EXTRA(name) auto name=reinterpret_cast<PFN_##name>(get(instance,#name));if(!name)throw std::runtime_error("Missing " #name)
    EXTRA(vkCmdCopyBuffer);EXTRA(vkCmdBindIndexBuffer);EXTRA(vkCmdDrawIndexed);EXTRA(vkGetPhysicalDeviceMemoryProperties);
#ifdef TEST_MATRIX
    EXTRA(vkCreateDescriptorSetLayout);EXTRA(vkDestroyDescriptorSetLayout);EXTRA(vkCreatePipelineCache);EXTRA(vkDestroyPipelineCache);
#ifdef TEST_UNIFORM_BUFFER
    EXTRA(vkCreateDescriptorPool);EXTRA(vkDestroyDescriptorPool);EXTRA(vkAllocateDescriptorSets);EXTRA(vkUpdateDescriptorSets);EXTRA(vkCmdBindDescriptorSets);
#endif
#endif
#undef EXTRA
#endif
    uint32_t n=0;check(vkEnumeratePhysicalDevices(instance,&n,nullptr),"EnumeratePhysicalDevices");if(!n)throw std::runtime_error("No Vulkan device");VkPhysicalDevice physical{};n=1;check(vkEnumeratePhysicalDevices(instance,&n,&physical),"PhysicalDevice");
    VkPhysicalDeviceProperties props{};vkGetPhysicalDeviceProperties(physical,&props);std::printf("VULKAN_DEVICE=%s\n",props.deviceName);if(props.deviceType==VK_PHYSICAL_DEVICE_TYPE_CPU)throw std::runtime_error("CPU renderer is not accepted");
    VkQueueFamilyProperties queues[16]{};n=16;vkGetPhysicalDeviceQueueFamilyProperties(physical,&n,queues);uint32_t family=0;while(family<n&&!(queues[family].queueFlags&VK_QUEUE_GRAPHICS_BIT))family++;if(family==n)throw std::runtime_error("No graphics queue");
    float priority=1;VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};qci.queueFamilyIndex=family;qci.queueCount=1;qci.pQueuePriorities=&priority;VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};dci.queueCreateInfoCount=1;dci.pQueueCreateInfos=&qci;VkDevice device{};check(vkCreateDevice(physical,&dci,nullptr,&device),"CreateDevice");VkQueue queue{};vkGetDeviceQueue(device,family,0,&queue);
    VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};imageInfo.imageType=VK_IMAGE_TYPE_2D;imageInfo.format=VK_FORMAT_R8G8B8A8_UNORM;imageInfo.extent={64,64,1};imageInfo.mipLevels=1;imageInfo.arrayLayers=1;imageInfo.samples=VK_SAMPLE_COUNT_1_BIT;imageInfo.tiling=VK_IMAGE_TILING_OPTIMAL;imageInfo.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;VkImage image{};check(vkCreateImage(device,&imageInfo,nullptr,&image),"CreateImage");
    VkMemoryRequirements ir{};vkGetImageMemoryRequirements(device,image,&ir);uint32_t it=0;while(!(ir.memoryTypeBits&(1u<<it)))it++;VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};ai.allocationSize=ir.size;ai.memoryTypeIndex=it;VkDeviceMemory im{};check(vkAllocateMemory(device,&ai,nullptr,&im),"AllocateImageMemory");check(vkBindImageMemory(device,image,im,0),"BindImageMemory");
    VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};bi.size=64*64*4;bi.usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT;VkBuffer buffer{};check(vkCreateBuffer(device,&bi,nullptr,&buffer),"CreateBuffer");VkMemoryRequirements br{};vkGetBufferMemoryRequirements(device,buffer,&br);uint32_t bt=0;while(!(br.memoryTypeBits&(1u<<bt)))bt++;ai.allocationSize=br.size;ai.memoryTypeIndex=bt;VkDeviceMemory bm{};check(vkAllocateMemory(device,&ai,nullptr,&bm),"AllocateBufferMemory");check(vkBindBufferMemory(device,buffer,bm,0),"BindBufferMemory");
    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};vi.image=image;vi.viewType=VK_IMAGE_VIEW_TYPE_2D;vi.format=imageInfo.format;vi.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};VkImageView view{};check(vkCreateImageView(device,&vi,nullptr,&view),"CreateImageView");
    VkAttachmentDescription attachment{};attachment.format=imageInfo.format;attachment.samples=VK_SAMPLE_COUNT_1_BIT;attachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;attachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;attachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;attachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;attachment.finalLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;VkAttachmentReference color{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;sub.colorAttachmentCount=1;sub.pColorAttachments=&color;
    VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};rp.attachmentCount=1;rp.pAttachments=&attachment;rp.subpassCount=1;rp.pSubpasses=&sub;VkRenderPass pass{};check(vkCreateRenderPass(device,&rp,nullptr,&pass),"CreateRenderPass");VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};fb.renderPass=pass;fb.attachmentCount=1;fb.pAttachments=&view;fb.width=64;fb.height=64;fb.layers=1;VkFramebuffer frame{};check(vkCreateFramebuffer(device,&fb,nullptr,&frame),"CreateFramebuffer");
    VkShaderModuleCreateInfo si{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};si.codeSize=sizeof(vertex);si.pCode=vertex;VkShaderModule vs{},fs{};check(vkCreateShaderModule(device,&si,nullptr,&vs),"VertexShader");si.codeSize=sizeof(fragment);si.pCode=fragment;check(vkCreateShaderModule(device,&si,nullptr,&fs),"FragmentShader");VkPushConstantRange push{VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,16};
#ifdef TEST_MATRIX
    push={VK_SHADER_STAGE_VERTEX_BIT,0,64};
#endif
    VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};pl.pushConstantRangeCount=1;pl.pPushConstantRanges=&push;
#ifdef TEST_UNIFORM_BUFFER
    VkDescriptorSetLayoutBinding uniformBinding{0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT,nullptr};
    VkDescriptorSetLayoutCreateInfo uniformLayoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};uniformLayoutInfo.bindingCount=1;uniformLayoutInfo.pBindings=&uniformBinding;
    VkDescriptorSetLayout descriptorLayout{};check(vkCreateDescriptorSetLayout(device,&uniformLayoutInfo,nullptr,&descriptorLayout),"UniformLayout");
    VkDescriptorPoolSize uniformPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1};VkDescriptorPoolCreateInfo uniformPoolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};uniformPoolInfo.maxSets=1;uniformPoolInfo.poolSizeCount=1;uniformPoolInfo.pPoolSizes=&uniformPoolSize;
    VkDescriptorPool uniformPool{};check(vkCreateDescriptorPool(device,&uniformPoolInfo,nullptr,&uniformPool),"UniformPool");
    VkDescriptorSetAllocateInfo uniformSetInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};uniformSetInfo.descriptorPool=uniformPool;uniformSetInfo.descriptorSetCount=1;uniformSetInfo.pSetLayouts=&descriptorLayout;
    VkDescriptorSet uniformSet{};check(vkAllocateDescriptorSets(device,&uniformSetInfo,&uniformSet),"UniformSet");
    VkBufferCreateInfo uniformInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};uniformInfo.size=224;uniformInfo.usage=VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;VkBuffer uniformBuffer{};check(vkCreateBuffer(device,&uniformInfo,nullptr,&uniformBuffer),"UniformBuffer");
    VkMemoryRequirements ur{};vkGetBufferMemoryRequirements(device,uniformBuffer,&ur);ai.allocationSize=ur.size;ai.memoryTypeIndex=0;
    VkDeviceMemory uniformMemory{};check(vkAllocateMemory(device,&ai,nullptr,&uniformMemory),"UniformMemory");check(vkBindBufferMemory(device,uniformBuffer,uniformMemory,0),"BindUniformMemory");
    void* uniformMapped{};check(vkMapMemory(device,uniformMemory,0,VK_WHOLE_SIZE,0,&uniformMapped),"MapUniform");
    float matrices[48]{};for(unsigned m=0;m<3;m++)for(unsigned j=0;j<4;j++)matrices[m*16+j*5]=1;
    std::memset(uniformMapped,0xcc,224);std::memcpy(static_cast<uint8_t*>(uniformMapped)+32,matrices,192);
    VkDescriptorBufferInfo uniformRange{uniformBuffer,32,192};VkWriteDescriptorSet uniformWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};uniformWrite.dstSet=uniformSet;uniformWrite.descriptorCount=1;uniformWrite.descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;uniformWrite.pBufferInfo=&uniformRange;vkUpdateDescriptorSets(device,1,&uniformWrite,0,nullptr);
    pl.pushConstantRangeCount=0;pl.pPushConstantRanges=nullptr;pl.setLayoutCount=1;pl.pSetLayouts=&descriptorLayout;
#endif
VkPipelineLayout layout{};check(vkCreatePipelineLayout(device,&pl,nullptr,&layout),"PipelineLayout");
    VkPipelineShaderStageCreateInfo stages[2]{};for(auto& s:stages){s.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;s.pName="main";}stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;stages[0].module=vs;stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT;stages[1].module=fs;
    VkPipelineVertexInputStateCreateInfo vertexInput{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;VkPipelineViewportStateCreateInfo viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};viewportState.viewportCount=1;viewportState.scissorCount=1;VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};raster.polygonMode=VK_POLYGON_MODE_FILL;raster.cullMode=VK_CULL_MODE_NONE;raster.lineWidth=1;VkPipelineMultisampleStateCreateInfo multi{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};multi.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;VkPipelineColorBlendAttachmentState ba{};ba.colorWriteMask=15;VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};blend.attachmentCount=1;blend.pAttachments=&ba;VkDynamicState states[]={VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dynamic.dynamicStateCount=2;dynamic.pDynamicStates=states;
#ifdef TEST_VERTEX_BUFFER
    // Padded interleaved storage and nonzero binding offset catch byte/stride errors.
    VkVertexInputBindingDescription binding{0,16,VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputAttributeDescription attribute{0,0,VK_FORMAT_R32G32_SFLOAT,4};
    vertexInput.vertexBindingDescriptionCount=1;vertexInput.pVertexBindingDescriptions=&binding;
    vertexInput.vertexAttributeDescriptionCount=1;vertexInput.pVertexAttributeDescriptions=&attribute;
    VkBufferCreateInfo vbi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};vbi.size=64;vbi.usage=VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
#ifdef TEST_INDEXED_BUFFER
    vbi.size=80;vbi.usage|=VK_BUFFER_USAGE_TRANSFER_DST_BIT;
#endif
    VkBuffer vertexBuffer{};VkDeviceMemory vertexMemory{};check(vkCreateBuffer(device,&vbi,nullptr,&vertexBuffer),"VertexBuffer");
    VkMemoryRequirements vr{};vkGetBufferMemoryRequirements(device,vertexBuffer,&vr);uint32_t vt=0;while(!(vr.memoryTypeBits&(1u<<vt)))vt++;
#ifdef TEST_INDEXED_BUFFER
    VkPhysicalDeviceMemoryProperties memoryProperties{};vkGetPhysicalDeviceMemoryProperties(physical,&memoryProperties);
    auto memoryType=[&](uint32_t bits,VkMemoryPropertyFlags flags){for(uint32_t i=0;i<memoryProperties.memoryTypeCount;i++)if((bits&(1u<<i))&&(memoryProperties.memoryTypes[i].propertyFlags&flags)==flags)return i;throw std::runtime_error("Required memory type absent");};
    vt=memoryType(vr.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
#endif
    ai.allocationSize=vr.size;ai.memoryTypeIndex=vt;check(vkAllocateMemory(device,&ai,nullptr,&vertexMemory),"VertexMemory");check(vkBindBufferMemory(device,vertexBuffer,vertexMemory,0),"BindVertexMemory");
#ifndef TEST_INDEXED_BUFFER
    void* vertexMapped{};check(vkMapMemory(device,vertexMemory,0,VK_WHOLE_SIZE,0,&vertexMapped),"MapVertexMemory");
    const float vertexData[16]={123,123,123,123, 99,-.75f,-.75f,99, 99,.75f,-.75f,99, 99,0,.75f,99};
    std::memcpy(vertexMapped,vertexData,sizeof(vertexData));vkUnmapMemory(device,vertexMemory);
#endif
#endif
    VkPipelineCache pipelineCache{};
#if defined(TEST_MATRIX) && !defined(TEST_UNIFORM_BUFFER)
    VkDescriptorSetLayoutCreateInfo empty{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};VkDescriptorSetLayout descriptorLayout{};
    check(vkCreateDescriptorSetLayout(device,&empty,nullptr,&descriptorLayout),"EmptyDescriptorLayout");
    VkPipelineCacheCreateInfo cacheInfo{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
    check(vkCreatePipelineCache(device,&cacheInfo,nullptr,&pipelineCache),"EmptyPipelineCache");
#endif
    VkGraphicsPipelineCreateInfo gp{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};gp.stageCount=2;gp.pStages=stages;gp.pVertexInputState=&vertexInput;gp.pInputAssemblyState=&assembly;gp.pViewportState=&viewportState;gp.pRasterizationState=&raster;gp.pMultisampleState=&multi;gp.pColorBlendState=&blend;gp.pDynamicState=&dynamic;gp.layout=layout;gp.renderPass=pass;VkPipeline pipeline{};check(vkCreateGraphicsPipelines(device,pipelineCache,1,&gp,nullptr,&pipeline),"GraphicsPipeline");
    VkCommandPoolCreateInfo pci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pci.queueFamilyIndex=family;pci.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;VkCommandPool pool{};check(vkCreateCommandPool(device,&pci,nullptr,&pool),"CommandPool");VkCommandBufferAllocateInfo ca{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ca.commandPool=pool;ca.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ca.commandBufferCount=1;VkCommandBuffer command{};check(vkAllocateCommandBuffers(device,&ca,&command),"CommandBuffer");
#ifdef TEST_INDEXED_BUFFER
    // Staging is submitted through standard Vulkan calls; no Bridge API in this app.
    auto upload=[&](VkBuffer destination,const void* data,VkDeviceSize size){
        VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};info.size=size;info.usage=VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        VkBuffer staging{};check(vkCreateBuffer(device,&info,nullptr,&staging),"StagingBuffer");
        VkMemoryRequirements req{};vkGetBufferMemoryRequirements(device,staging,&req);
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=req.size;
        allocation.memoryTypeIndex=memoryType(req.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        VkDeviceMemory memory{};check(vkAllocateMemory(device,&allocation,nullptr,&memory),"StagingMemory");
        check(vkBindBufferMemory(device,staging,memory,0),"BindStaging");void* address{};
        check(vkMapMemory(device,memory,0,size,0,&address),"MapStaging");std::memcpy(address,data,size);vkUnmapMemory(device,memory);
        VkCommandBufferBeginInfo start{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};start.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        check(vkBeginCommandBuffer(command,&start),"BeginUpload");VkBufferCopy region{0,0,size};vkCmdCopyBuffer(command,staging,destination,1,&region);
        check(vkEndCommandBuffer(command),"EndUpload");VkFenceCreateInfo fci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};VkFence done{};
        check(vkCreateFence(device,&fci,nullptr,&done),"UploadFence");VkSubmitInfo job{VK_STRUCTURE_TYPE_SUBMIT_INFO};job.commandBufferCount=1;job.pCommandBuffers=&command;
        check(vkQueueSubmit(queue,1,&job,done),"SubmitUpload");check(vkWaitForFences(device,1,&done,VK_TRUE,10000000000ull),"WaitUpload");
        vkDestroyFence(device,done,nullptr);vkDestroyBuffer(device,staging,nullptr);vkFreeMemory(device,memory,nullptr);
    };
    const float vertexData[20]={123,123,123,123,99,9,9,99,99,.75f,-.75f,99,99,0,.75f,99,99,-.75f,-.75f,99};
    upload(vertexBuffer,vertexData,sizeof(vertexData));
    VkBufferCreateInfo indexInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};indexInfo.size=20;indexInfo.usage=VK_BUFFER_USAGE_INDEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VkBuffer indexBuffer{};VkDeviceMemory indexMemory{};check(vkCreateBuffer(device,&indexInfo,nullptr,&indexBuffer),"IndexBuffer");
    VkMemoryRequirements indexReq{};vkGetBufferMemoryRequirements(device,indexBuffer,&indexReq);
    ai.allocationSize=indexReq.size;ai.memoryTypeIndex=memoryType(indexReq.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    check(vkAllocateMemory(device,&ai,nullptr,&indexMemory),"IndexMemory");check(vkBindBufferMemory(device,indexBuffer,indexMemory,0),"BindIndex");
    const uint32_t indexData[5]={99,99,3,1,2};upload(indexBuffer,indexData,sizeof(indexData));
#endif
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;check(vkBeginCommandBuffer(command,&begin),"BeginCommandBuffer");
#ifdef TEST_VERTEX_BUFFER
    VkDeviceSize vertexOffset=16;vkCmdBindVertexBuffers(command,0,1,&vertexBuffer,&vertexOffset);
#endif
#ifdef TEST_INDEXED_BUFFER
    vkCmdBindIndexBuffer(command,indexBuffer,4,VK_INDEX_TYPE_UINT32);
#endif
    VkClearValue clear{};clear.color.float32[3]=1;VkRenderPassBeginInfo render{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};render.renderPass=pass;render.framebuffer=frame;render.renderArea.extent={64,64};render.clearValueCount=1;render.pClearValues=&clear;vkCmdBeginRenderPass(command,&render,VK_SUBPASS_CONTENTS_INLINE);vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);VkViewport viewport{0,0,64,64,0,1};VkRect2D scissor{{0,0},{64,64}};vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&scissor);
#ifdef TEST_UNIFORM_BUFFER
    vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,layout,0,1,&uniformSet,0,nullptr);
#elif defined(TEST_MATRIX)
    const float state[16]={.5f,0,0,0,0,.5f,0,0,0,0,1,0,.25f,-.125f,0,1};
    // Two aligned partial updates must preserve the entire matrix.
    vkCmdPushConstants(command,layout,push.stageFlags,0,32,state);
    vkCmdPushConstants(command,layout,push.stageFlags,32,32,state+8);
#else
    float state[4]={0,0,0,0};vkCmdPushConstants(command,layout,push.stageFlags,0,16,state);
#endif

#ifdef TEST_INDEXED_BUFFER
    vkCmdDrawIndexed(command,3,1,1,0,0);
#else
    vkCmdDraw(command,3,1,0,0);
#endif
    vkCmdEndRenderPass(command);
#ifdef TEST_UNIFORM_BUFFER
    // Change persistently mapped bytes AFTER recording/binding. Submit reads current data.
    matrices[16]=matrices[21]=.5f;matrices[44]=.25f;matrices[45]=-.125f;
    std::memcpy(static_cast<uint8_t*>(uniformMapped)+32,matrices,192);
#endif
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;barrier.oldLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barrier.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.image=image;barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={64,64,1};vkCmdCopyImageToBuffer(command,image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,buffer,1,&copy);VkMemoryBarrier hostBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};hostBarrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;hostBarrier.dstAccessMask=VK_ACCESS_HOST_READ_BIT;vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&hostBarrier,0,nullptr,0,nullptr);check(vkEndCommandBuffer(command),"EndCommandBuffer");VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};VkFence fence{};check(vkCreateFence(device,&fi,nullptr,&fence),"Fence");VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;check(vkQueueSubmit(queue,1,&submit,fence),"QueueSubmit");check(vkWaitForFences(device,1,&fence,VK_TRUE,10000000000ull),"WaitForFences");
    void* mapped{};check(vkMapMemory(device,bm,0,VK_WHOLE_SIZE,0,&mapped),"MapMemory");VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};range.memory=bm;range.size=VK_WHOLE_SIZE;check(vkInvalidateMappedMemoryRanges(device,1,&range),"Invalidate");auto* pixels=static_cast<const uint8_t*>(mapped);unsigned checked=0;for(int y=0;y<64;y++)for(int x=0;x<64;x++){float nx=2*(x+.5f)/64-1,ny=2*(y+.5f)/64-1;
#ifdef TEST_MATRIX
    nx=2*(nx-.25f);ny=2*(ny+.125f);
#endif
    float side=.375f-.5f*ny-std::abs(nx);bool inside=ny>-.67f&&ny<.67f&&side>.08f,outside=ny<-.83f||ny>.83f||side<-.08f;if(!inside&&!outside)continue;int expected[4]={0,inside?64:0,inside?255:0,255};for(int k=0;k<4;k++)if(std::abs(int(pixels[(y*64+x)*4+k])-expected[k])>1)throw std::runtime_error("Blue offscreen pixel mismatch");checked++;}
#ifdef TEST_MATRIX
    if(checked!=3906)throw std::runtime_error("Matrix sample count");
#else
    if(checked!=3330)throw std::runtime_error("Sample count");
#endif
#ifdef TEST_VERTEX_BUFFER
    vkDestroyBuffer(device,vertexBuffer,nullptr);vkFreeMemory(device,vertexMemory,nullptr);
#endif
#ifdef TEST_INDEXED_BUFFER
    vkDestroyBuffer(device,indexBuffer,nullptr);vkFreeMemory(device,indexMemory,nullptr);
#endif
#ifdef TEST_UNIFORM_BUFFER
    vkDestroyDescriptorPool(device,uniformPool,nullptr);vkUnmapMemory(device,uniformMemory);vkDestroyBuffer(device,uniformBuffer,nullptr);vkFreeMemory(device,uniformMemory,nullptr);
#endif
#ifdef TEST_MATRIX
    vkDestroyDescriptorSetLayout(device,descriptorLayout,nullptr);vkDestroyPipelineCache(device,pipelineCache,nullptr);
#endif
    vkUnmapMemory(device,bm);vkDestroyFence(device,fence,nullptr);vkFreeCommandBuffers(device,pool,1,&command);vkDestroyCommandPool(device,pool,nullptr);vkDestroyPipeline(device,pipeline,nullptr);vkDestroyPipelineLayout(device,layout,nullptr);vkDestroyShaderModule(device,vs,nullptr);vkDestroyShaderModule(device,fs,nullptr);vkDestroyFramebuffer(device,frame,nullptr);vkDestroyRenderPass(device,pass,nullptr);vkDestroyImageView(device,view,nullptr);vkDestroyBuffer(device,buffer,nullptr);vkDestroyImage(device,image,nullptr);vkFreeMemory(device,bm,nullptr);vkFreeMemory(device,im,nullptr);vkDestroyDevice(device,nullptr);vkDestroyInstance(instance,nullptr);FreeLibrary(loader);
#ifdef TEST_UNIFORM_BUFFER
    std::printf("STANDARD_VULKAN_UNIFORM_PIXELS=%u\nSTANDARD_VULKAN_UNIFORM_PASS\n",checked);
#elif defined(TEST_MATRIX)
    std::printf("STANDARD_VULKAN_MATRIX_PIXELS=%u\nSTANDARD_VULKAN_MATRIX_PASS\n",checked);
#elif defined(TEST_INDEXED_BUFFER)
    std::printf("STANDARD_VULKAN_INDEXED_STAGING_PIXELS=%u\nSTANDARD_VULKAN_INDEXED_STAGING_PASS\n",checked);
#elif defined(TEST_VERTEX_BUFFER)
    std::printf("STANDARD_VULKAN_VERTEX_BUFFER_PIXELS=%u\nSTANDARD_VULKAN_VERTEX_BUFFER_PASS\n",checked);
#else
    std::printf("STANDARD_VULKAN_BLUE_PIXELS=%u\nSTANDARD_VULKAN_OFFSCREEN_PASS\n",checked);
#endif
    return 0;
}catch(const std::exception& e){std::printf("STANDARD_VULKAN_OFFSCREEN_FAIL=%s\n",e.what());return 1;}}
