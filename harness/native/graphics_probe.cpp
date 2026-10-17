#define VK_USE_PLATFORM_ANDROID_KHR
#include <jni.h>
#include <vulkan/vulkan.h>
#include <android/native_window_jni.h>
#include <atomic>
#include <chrono>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <algorithm>
#include <memory>
#include <mavk/core.hpp>
#include "validation.hpp"

namespace {
using Clock=std::chrono::steady_clock;
struct Session{std::atomic<bool> stop{false};};
void ok(VkResult r,const char* what){if(r!=VK_SUCCESS)throw std::runtime_error(std::string(what)+" VkResult="+std::to_string(r));}
double milliseconds(Clock::time_point a,Clock::time_point b){return std::chrono::duration<double,std::milli>(b-a).count();}
struct Graphics {
    ValidationCapture* validation{};bool validated=false;
    bool coreMode=false;
    std::unique_ptr<mavk::DeviceContext> core;
    std::unique_ptr<mavk::Buffer> coreBuffer;
    std::unique_ptr<mavk::Image> coreImage;
    std::unique_ptr<mavk::Shader> coreVert,coreFrag;
    std::unique_ptr<mavk::Fence> coreFence;
    std::unique_ptr<mavk::CommandBatch> coreCommands;
    std::vector<std::unique_ptr<mavk::Pipeline>> corePipelines;
    mavk::Counters* retiredCounters{};
    ANativeWindow* window{};VkInstance instance{};VkPhysicalDevice physical{};VkPhysicalDeviceProperties properties{};
    VkDevice device{};VkQueue queue{};uint32_t family{};VkSurfaceKHR surface{};VkSwapchainKHR swapchain{};
    VkShaderModule vert{},frag{};VkPipelineLayout layout{};VkRenderPass offpass{},screenpass{};
    VkPipeline offpipeline{},screenpipeline{};VkImage offimage{};VkDeviceMemory imagememory{},bufferMemory{};
    VkBuffer buffer{};void* mapped{};bool coherent{};VkDeviceSize bufferSize{};
    VkImageView offview{};VkFramebuffer offframe{};VkCommandPool pool{};VkCommandBuffer command{};VkFence fence{};
    VkSemaphore acquired{};std::vector<VkSemaphore> rendered;
    std::vector<VkFence> presentFences;
    std::vector<VkImage> images;std::vector<VkImageView> views;std::vector<VkFramebuffer> frames;
    VkExtent2D extent{};
    ~Graphics(){
        if(device){vkDeviceWaitIdle(device);
            coreCommands.reset();coreFence.reset();
            if(mapped&&!coreMode)vkUnmapMemory(device,bufferMemory);
            for(auto f:frames)vkDestroyFramebuffer(device,f,nullptr);
            if(offframe)vkDestroyFramebuffer(device,offframe,nullptr);
            corePipelines.clear();
            if(!coreMode){if(screenpipeline)vkDestroyPipeline(device,screenpipeline,nullptr);if(offpipeline)vkDestroyPipeline(device,offpipeline,nullptr);}
            if(layout)vkDestroyPipelineLayout(device,layout,nullptr);
            if(screenpass)vkDestroyRenderPass(device,screenpass,nullptr);if(offpass)vkDestroyRenderPass(device,offpass,nullptr);
            for(auto view:views)vkDestroyImageView(device,view,nullptr);if(offview)vkDestroyImageView(device,offview,nullptr);
            coreImage.reset();coreBuffer.reset();coreVert.reset();coreFrag.reset();
            if(!coreMode){if(offimage)vkDestroyImage(device,offimage,nullptr);if(imagememory)vkFreeMemory(device,imagememory,nullptr);
                if(buffer)vkDestroyBuffer(device,buffer,nullptr);if(bufferMemory)vkFreeMemory(device,bufferMemory,nullptr);
                if(vert)vkDestroyShaderModule(device,vert,nullptr);if(frag)vkDestroyShaderModule(device,frag,nullptr);}
            for(auto semaphore:rendered)vkDestroySemaphore(device,semaphore,nullptr);if(acquired)vkDestroySemaphore(device,acquired,nullptr);
            for(auto presentFence:presentFences)vkDestroyFence(device,presentFence,nullptr);
            if(!coreMode){if(fence)vkDestroyFence(device,fence,nullptr);if(pool)vkDestroyCommandPool(device,pool,nullptr);}
            if(core&&retiredCounters)*retiredCounters=core->counters();core.reset();
            if(swapchain)vkDestroySwapchainKHR(device,swapchain,nullptr);vkDestroyDevice(device,nullptr);
        }
        if(surface)vkDestroySurfaceKHR(instance,surface,nullptr);if(instance){validation->detach(instance);vkDestroyInstance(instance,nullptr);}
        if(window)ANativeWindow_release(window);
    }
    uint32_t memoryType(uint32_t bits,VkMemoryPropertyFlags flags){
        VkPhysicalDeviceMemoryProperties p;vkGetPhysicalDeviceMemoryProperties(physical,&p);
        for(uint32_t i=0;i<p.memoryTypeCount;i++)if((bits&(1u<<i))&&(p.memoryTypes[i].propertyFlags&flags)==flags)return i;
        throw std::runtime_error("No required memory type");
    }
    void init(JNIEnv* env,jobject javaSurface){
        window=ANativeWindow_fromSurface(env,javaSurface);if(!window)throw std::runtime_error("No native window");
        const char* exts[]={VK_KHR_SURFACE_EXTENSION_NAME,VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME};
        uint32_t count=0;ok(vkEnumerateInstanceExtensionProperties(nullptr,&count,nullptr),"instance extension count");std::vector<VkExtensionProperties> instanceExtensions(count);ok(vkEnumerateInstanceExtensionProperties(nullptr,&count,instanceExtensions.data()),"instance extensions");
        for(const char* wanted:exts){bool found=false;for(auto& e:instanceExtensions)if(!std::strcmp(e.extensionName,wanted))found=true;if(!found)throw std::runtime_error(std::string("Missing instance extension: ")+wanted);}
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.pApplicationName="Mavk M0 graphics";app.apiVersion=VK_API_VERSION_1_1;
        VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};ici.pApplicationInfo=&app;ici.enabledExtensionCount=4;ici.ppEnabledExtensionNames=exts;
        validation->create(ici,instance,validated);
        VkAndroidSurfaceCreateInfoKHR asci{VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR};asci.window=window;
        ok(vkCreateAndroidSurfaceKHR(instance,&asci,nullptr,&surface),"Android surface");
        uint32_t n=0;ok(vkEnumeratePhysicalDevices(instance,&n,nullptr),"devices count");std::vector<VkPhysicalDevice> candidates(n);
        ok(vkEnumeratePhysicalDevices(instance,&n,candidates.data()),"devices");
        for(auto p:candidates){VkPhysicalDeviceProperties prop;vkGetPhysicalDeviceProperties(p,&prop);
            if(prop.vendorID!=0x13b5||prop.deviceType==VK_PHYSICAL_DEVICE_TYPE_CPU)continue;
            uint32_t count=0;vkGetPhysicalDeviceQueueFamilyProperties(p,&count,nullptr);std::vector<VkQueueFamilyProperties> queues(count);
            vkGetPhysicalDeviceQueueFamilyProperties(p,&count,queues.data());
            for(uint32_t i=0;i<count;i++){VkBool32 supported=VK_FALSE;ok(vkGetPhysicalDeviceSurfaceSupportKHR(p,i,surface,&supported),"surface support");
                if(supported&&(queues[i].queueFlags&VK_QUEUE_GRAPHICS_BIT)){physical=p;properties=prop;family=i;break;}}
            if(physical)break;
        }
        if(!physical)throw std::runtime_error("No ARM hardware graphics+present device");
        float priority=1;VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};qci.queueFamilyIndex=family;qci.queueCount=1;qci.pQueuePriorities=&priority;
        uint32_t extCount=0;ok(vkEnumerateDeviceExtensionProperties(physical,nullptr,&extCount,nullptr),"extension count");std::vector<VkExtensionProperties> extensions(extCount);ok(vkEnumerateDeviceExtensionProperties(physical,nullptr,&extCount,extensions.data()),"extensions");
        bool maintenance=false;for(auto& e:extensions)if(!std::strcmp(e.extensionName,VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME))maintenance=true;
        if(!maintenance)throw std::runtime_error("This bounded Surface lifetime test requires advertised swapchain_maintenance1");
        VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT maintenanceFeature{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT};VkPhysicalDeviceFeatures2 features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};features.pNext=&maintenanceFeature;vkGetPhysicalDeviceFeatures2(physical,&features);
        if(!maintenanceFeature.swapchainMaintenance1)throw std::runtime_error("Present fences feature not supported; refusing unsafe lifetime proof");
        const char* enabled[]={VK_KHR_SWAPCHAIN_EXTENSION_NAME,VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME};VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};dci.queueCreateInfoCount=1;dci.pQueueCreateInfos=&qci;dci.enabledExtensionCount=2;dci.ppEnabledExtensionNames=enabled;dci.pNext=&maintenanceFeature;
        ok(vkCreateDevice(physical,&dci,nullptr,&device),"device");vkGetDeviceQueue(device,family,0,&queue);
        if(coreMode){core=std::make_unique<mavk::DeviceContext>(physical,device,family);coreCommands=std::make_unique<mavk::CommandBatch>(*core);command=coreCommands->handle();coreFence=std::make_unique<mavk::Fence>(*core);fence=coreFence->handle();return;}
        VkCommandPoolCreateInfo cpci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};cpci.queueFamilyIndex=family;cpci.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        ok(vkCreateCommandPool(device,&cpci,nullptr,&pool),"command pool");VkCommandBufferAllocateInfo cbai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};cbai.commandPool=pool;cbai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;cbai.commandBufferCount=1;
        ok(vkAllocateCommandBuffers(device,&cbai,&command),"command buffer");VkFenceCreateInfo fci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};ok(vkCreateFence(device,&fci,nullptr,&fence),"fence");
    }
    VkShaderModule shader(const std::vector<uint32_t>& code){VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};ci.codeSize=code.size()*4;ci.pCode=code.data();VkShaderModule result;ok(vkCreateShaderModule(device,&ci,nullptr,&result),"shader");return result;}
    void shaders(const std::vector<uint32_t>& vertex,const std::vector<uint32_t>& fragment){
        if(coreMode){coreVert=std::make_unique<mavk::Shader>(*core,vertex.data(),vertex.size()*4);coreFrag=std::make_unique<mavk::Shader>(*core,fragment.data(),fragment.size()*4);vert=coreVert->handle();frag=coreFrag->handle();}
        else{vert=shader(vertex);frag=shader(fragment);}VkPushConstantRange push{VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,16};
        VkPipelineLayoutCreateInfo ci{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};ci.pushConstantRangeCount=1;ci.pPushConstantRanges=&push;ok(vkCreatePipelineLayout(device,&ci,nullptr,&layout),"layout");
    }
    VkRenderPass renderPass(VkFormat format){
        VkAttachmentDescription attachment{};attachment.format=format;attachment.samples=VK_SAMPLE_COUNT_1_BIT;attachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;attachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;attachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;attachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;attachment.finalLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkAttachmentReference color{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};VkSubpassDescription subpass{};subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;subpass.colorAttachmentCount=1;subpass.pColorAttachments=&color;
        VkSubpassDependency dep{};dep.srcSubpass=VK_SUBPASS_EXTERNAL;dep.dstSubpass=0;dep.srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;dep.dstStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;dep.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        VkRenderPassCreateInfo ci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};ci.attachmentCount=1;ci.pAttachments=&attachment;ci.subpassCount=1;ci.pSubpasses=&subpass;ci.dependencyCount=1;ci.pDependencies=&dep;
        VkRenderPass result;ok(vkCreateRenderPass(device,&ci,nullptr,&result),"render pass");return result;
    }
    VkPipeline pipeline(VkRenderPass pass){
        VkPipelineShaderStageCreateInfo stages[2]{};for(auto& stage:stages){stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;stage.pName="main";}
        stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;stages[0].module=vert;stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT;stages[1].module=frag;
        VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};ia.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};vp.viewportCount=1;vp.scissorCount=1;
        VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};rs.polygonMode=VK_POLYGON_MODE_FILL;rs.cullMode=VK_CULL_MODE_NONE;rs.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE;rs.lineWidth=1;
        VkPipelineMultisampleStateCreateInfo multi{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};multi.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        VkPipelineColorBlendAttachmentState attachment{};attachment.colorWriteMask=15;
        VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};blend.attachmentCount=1;blend.pAttachments=&attachment;
        VkDynamicState states[]={VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dynamic.dynamicStateCount=2;dynamic.pDynamicStates=states;
        VkGraphicsPipelineCreateInfo ci{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};ci.stageCount=2;ci.pStages=stages;ci.pVertexInputState=&vi;ci.pInputAssemblyState=&ia;ci.pViewportState=&vp;ci.pRasterizationState=&rs;ci.pMultisampleState=&multi;ci.pColorBlendState=&blend;ci.pDynamicState=&dynamic;ci.layout=layout;ci.renderPass=pass;
        if(coreMode){corePipelines.push_back(std::make_unique<mavk::Pipeline>(*core,ci));return corePipelines.back()->handle();}
        VkPipeline result;ok(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&ci,nullptr,&result),"graphics pipeline");return result;
    }
    VkImageView view(VkImage image,VkFormat format){VkImageViewCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};ci.image=image;ci.viewType=VK_IMAGE_VIEW_TYPE_2D;ci.format=format;ci.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};VkImageView result;ok(vkCreateImageView(device,&ci,nullptr,&result),"image view");return result;}
    VkFramebuffer framebuffer(VkRenderPass pass,VkImageView view,VkExtent2D size){VkFramebufferCreateInfo ci{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};ci.renderPass=pass;ci.attachmentCount=1;ci.pAttachments=&view;ci.width=size.width;ci.height=size.height;ci.layers=1;VkFramebuffer result;ok(vkCreateFramebuffer(device,&ci,nullptr,&result),"framebuffer");return result;}
    void begin(){if(coreMode){coreCommands->begin();return;}ok(vkResetCommandBuffer(command,0),"reset command");VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};bi.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;ok(vkBeginCommandBuffer(command,&bi),"begin command");}
    void submit(const VkSubmitInfo& info){if(coreMode)ok(core->submit(queue,info,fence),"Core graphics submit");else ok(vkQueueSubmit(queue,1,&info,fence),"direct graphics submit");}
    void draw(VkRenderPass pass,VkFramebuffer frame,VkPipeline pipeline,VkExtent2D size,const float state[4]){
        VkClearValue clear{};clear.color.float32[3]=1;VkRenderPassBeginInfo bi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};bi.renderPass=pass;bi.framebuffer=frame;bi.renderArea.extent=size;bi.clearValueCount=1;bi.pClearValues=&clear;
        vkCmdBeginRenderPass(command,&bi,VK_SUBPASS_CONTENTS_INLINE);vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);
        VkViewport viewport{0,0,(float)size.width,(float)size.height,0,1};VkRect2D scissor{{0,0},size};vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&scissor);
        vkCmdPushConstants(command,layout,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,16,state);vkCmdDraw(command,3,1,0,0);vkCmdEndRenderPass(command);
    }
    void transition(VkImage image,VkImageLayout layout,VkPipelineStageFlags destination,VkAccessFlags access){
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;barrier.dstAccessMask=access;barrier.oldLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;barrier.newLayout=layout;barrier.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.image=image;barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,destination,0,0,nullptr,0,nullptr,1,&barrier);
    }
    uint32_t offscreen(){
        VkFormatProperties format;vkGetPhysicalDeviceFormatProperties(physical,VK_FORMAT_R8G8B8A8_UNORM,&format);
        if(!(format.optimalTilingFeatures&VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT))throw std::runtime_error("No RGBA8 offscreen attachment");
        VkImageCreateInfo ici{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};ici.imageType=VK_IMAGE_TYPE_2D;ici.format=VK_FORMAT_R8G8B8A8_UNORM;ici.extent={64,64,1};ici.mipLevels=1;ici.arrayLayers=1;ici.samples=VK_SAMPLE_COUNT_1_BIT;ici.tiling=VK_IMAGE_TILING_OPTIMAL;ici.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        VkMemoryRequirements mr;VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        if(coreMode){coreImage=std::make_unique<mavk::Image>(*core,ici);offimage=coreImage->handle();}
        else{ok(vkCreateImage(device,&ici,nullptr,&offimage),"offscreen image");vkGetImageMemoryRequirements(device,offimage,&mr);ai.allocationSize=mr.size;ai.memoryTypeIndex=memoryType(mr.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            ok(vkAllocateMemory(device,&ai,nullptr,&imagememory),"image memory");ok(vkBindImageMemory(device,offimage,imagememory,0),"bind image");}
        offview=view(offimage,VK_FORMAT_R8G8B8A8_UNORM);offpass=renderPass(VK_FORMAT_R8G8B8A8_UNORM);offpipeline=pipeline(offpass);offframe=framebuffer(offpass,offview,{64,64});
        if(coreMode){coreBuffer=std::make_unique<mavk::Buffer>(*core,64*64*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);buffer=coreBuffer->handle();mapped=coreBuffer->map();coherent=core->memoryFlags(coreBuffer->memoryType())&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;}
        else{VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};bci.size=64*64*4;bci.usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT;ok(vkCreateBuffer(device,&bci,nullptr,&buffer),"readback buffer");vkGetBufferMemoryRequirements(device,buffer,&mr);
        ai.allocationSize=mr.size;bufferSize=mr.size;ai.memoryTypeIndex=memoryType(mr.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        VkPhysicalDeviceMemoryProperties mp;vkGetPhysicalDeviceMemoryProperties(physical,&mp);coherent=mp.memoryTypes[ai.memoryTypeIndex].propertyFlags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        ok(vkAllocateMemory(device,&ai,nullptr,&bufferMemory),"readback memory");ok(vkBindBufferMemory(device,buffer,bufferMemory,0),"bind readback");ok(vkMapMemory(device,bufferMemory,0,VK_WHOLE_SIZE,0,&mapped),"map readback");}
        begin();float state[4]={0,0,.25f,0};draw(offpass,offframe,offpipeline,{64,64},state);
        transition(offimage,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_TRANSFER_READ_BIT);
        VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={64,64,1};vkCmdCopyImageToBuffer(command,offimage,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,buffer,1,&copy);
        VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_HOST_READ_BIT;vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&barrier,0,nullptr,0,nullptr);
        ok(vkEndCommandBuffer(command),"end offscreen");VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};si.commandBufferCount=1;si.pCommandBuffers=&command;submit(si);ok(vkWaitForFences(device,1,&fence,VK_TRUE,5'000'000'000ull),"offscreen fence");
        if(coreMode)coreBuffer->invalidate();else if(!coherent){VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};range.memory=bufferMemory;range.size=VK_WHOLE_SIZE;ok(vkInvalidateMappedMemoryRanges(device,1,&range),"offscreen invalidate");}
        const auto* pixels=(const uint8_t*)mapped;uint32_t checked=0;
        for(int y=0;y<64;y++)for(int x=0;x<64;x++){
            float nx=2*(x+.5f)/64-1,ny=2*(y+.5f)/64-1;float side=.375f-.5f*ny-std::abs(nx);
            bool inside=ny>-.67f&&ny<.67f&&side>.08f;bool outside=ny<-.83f||ny>.83f||side<-.08f;
            if(!inside&&!outside)continue;const uint8_t* c=pixels+(y*64+x)*4;
            int expected[4]={inside?255:0,inside?64:0,0,255};for(int k=0;k<4;k++)if(std::abs(int(c[k])-expected[k])>1)throw std::runtime_error("Offscreen pixel mismatch x="+std::to_string(x)+" y="+std::to_string(y));checked++;
        }
        if(checked<3000)throw std::runtime_error("Insufficient offscreen samples");return checked;
    }
    void screen(){
        VkSurfaceCapabilitiesKHR caps;ok(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical,surface,&caps),"surface caps");uint32_t count=0;ok(vkGetPhysicalDeviceSurfaceFormatsKHR(physical,surface,&count,nullptr),"formats count");
        if(!count)throw std::runtime_error("No Surface formats");std::vector<VkSurfaceFormatKHR> formats(count);ok(vkGetPhysicalDeviceSurfaceFormatsKHR(physical,surface,&count,formats.data()),"formats");
        auto format=formats[0];for(auto candidate:formats)if(candidate.format==VK_FORMAT_R8G8B8A8_UNORM||candidate.format==VK_FORMAT_B8G8R8A8_UNORM){format=candidate;break;}
        if(format.format==VK_FORMAT_UNDEFINED)format.format=VK_FORMAT_R8G8B8A8_UNORM;
        extent=caps.currentExtent;if(extent.width==UINT32_MAX){extent.width=std::clamp(uint32_t(ANativeWindow_getWidth(window)),caps.minImageExtent.width,caps.maxImageExtent.width);extent.height=std::clamp(uint32_t(ANativeWindow_getHeight(window)),caps.minImageExtent.height,caps.maxImageExtent.height);}
        if(!extent.width||!extent.height||!(caps.supportedUsageFlags&VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT))throw std::runtime_error("Invalid Surface extent/usage");
        uint32_t imageCount=caps.minImageCount+1;if(caps.maxImageCount)imageCount=std::min(imageCount,caps.maxImageCount);
        VkCompositeAlphaFlagBitsKHR alpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;if(!(caps.supportedCompositeAlpha&alpha)){for(uint32_t bit=1;bit<=8;bit<<=1)if(caps.supportedCompositeAlpha&bit){alpha=(VkCompositeAlphaFlagBitsKHR)bit;break;}}
        VkSwapchainCreateInfoKHR ci{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};ci.surface=surface;ci.minImageCount=imageCount;ci.imageFormat=format.format;ci.imageColorSpace=format.colorSpace;ci.imageExtent=extent;ci.imageArrayLayers=1;ci.imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;ci.imageSharingMode=VK_SHARING_MODE_EXCLUSIVE;ci.preTransform=caps.currentTransform;ci.compositeAlpha=alpha;ci.presentMode=VK_PRESENT_MODE_FIFO_KHR;ci.clipped=VK_TRUE;
        ok(vkCreateSwapchainKHR(device,&ci,nullptr,&swapchain),"swapchain");ok(vkGetSwapchainImagesKHR(device,swapchain,&count,nullptr),"image count");images.resize(count);ok(vkGetSwapchainImagesKHR(device,swapchain,&count,images.data()),"images");
        screenpass=renderPass(format.format);screenpipeline=pipeline(screenpass);
        VkSemaphoreCreateInfo sci{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};ok(vkCreateSemaphore(device,&sci,nullptr,&acquired),"acquire semaphore");
        for(auto image:images){views.push_back(view(image,format.format));frames.push_back(framebuffer(screenpass,views.back(),extent));VkSemaphore semaphore;ok(vkCreateSemaphore(device,&sci,nullptr,&semaphore),"per-image present semaphore");rendered.push_back(semaphore);VkFenceCreateInfo fci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};VkFence retirement;ok(vkCreateFence(device,&fci,nullptr,&retirement),"present retirement fence");presentFences.push_back(retirement);}
    }
    std::string run(Session& session){
        uint32_t verified=offscreen();screen();std::ostringstream samples;uint32_t count=0;auto start=Clock::now();
        while(milliseconds(start,Clock::now())<30000&&!session.stop.load()){
            uint32_t image=0;auto a=Clock::now();VkResult acquiredResult=vkAcquireNextImageKHR(device,swapchain,250'000'000ull,acquired,VK_NULL_HANDLE,&image);
            if(acquiredResult==VK_TIMEOUT)continue;if(acquiredResult==VK_ERROR_OUT_OF_DATE_KHR)break;
            if(acquiredResult!=VK_SUBOPTIMAL_KHR)ok(acquiredResult,"acquire");auto b=Clock::now();
            ok(vkResetFences(device,1,&fence),"reset fence");begin();float t=float(milliseconds(start,b)/1000);float state[4]={.15f*std::sin(t*2),.08f*std::cos(t*2),.25f+.2f*std::sin(t*3),.25f+.2f*std::cos(t*3)};
            draw(screenpass,frames.at(image),screenpipeline,extent,state);transition(images.at(image),VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,0);ok(vkEndCommandBuffer(command),"end screen");
            VkPipelineStageFlags stage=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.waitSemaphoreCount=1;submit.pWaitSemaphores=&acquired;submit.pWaitDstStageMask=&stage;submit.commandBufferCount=1;submit.pCommandBuffers=&command;submit.signalSemaphoreCount=1;submit.pSignalSemaphores=&rendered.at(image);
            auto c=Clock::now();this->submit(submit);VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};present.waitSemaphoreCount=1;present.pWaitSemaphores=&rendered.at(image);present.swapchainCount=1;present.pSwapchains=&swapchain;present.pImageIndices=&image;
            ok(vkResetFences(device,1,&presentFences.at(image)),"reset present fence");VkSwapchainPresentFenceInfoEXT retirement{VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT};retirement.swapchainCount=1;retirement.pFences=&presentFences.at(image);present.pNext=&retirement;
            VkResult result=vkQueuePresentKHR(queue,&present);auto d=Clock::now();ok(vkWaitForFences(device,1,&fence,VK_TRUE,5'000'000'000ull),"render fence");
            if(result==VK_SUCCESS||result==VK_SUBOPTIMAL_KHR)ok(vkWaitForFences(device,1,&presentFences.at(image),VK_TRUE,5'000'000'000ull),"present retirement fence");auto e=Clock::now();
            if(result!=VK_SUBOPTIMAL_KHR&&result!=VK_ERROR_OUT_OF_DATE_KHR)ok(result,"present");
            if(count)samples<<',';samples<<"{\"frame\":"<<count<<",\"acquire_ms\":"<<milliseconds(a,b)<<",\"record_ms\":"<<milliseconds(b,c)<<",\"submit_present_ms\":"<<milliseconds(c,d)<<",\"fence_wait_ms\":"<<milliseconds(d,e)<<",\"elapsed_ms\":"<<milliseconds(start,e)<<'}';count++;
            if(result==VK_ERROR_OUT_OF_DATE_KHR)break;
        }
        auto end=Clock::now();double elapsed=milliseconds(start,end)/1000;ok(vkDeviceWaitIdle(device),"final graphics idle");
        std::ostringstream result;
        result<<"{\"schema\":1,\"core\":"<<(coreMode?"true":"false")<<",\"device\":\""<<properties.deviceName<<"\",\"vendor_id\":"<<properties.vendorID<<",\"device_type\":"<<properties.deviceType
            <<",\"width\":"<<extent.width<<",\"height\":"<<extent.height<<",\"offscreen_pixels_verified\":"<<verified<<",\"swapchain_images\":"<<images.size()
            <<",\"present_mode\":\"FIFO\",\"requested_surface_frame_rate\":60,\"present_retirement_fences\":true,\"present_calls\":"<<count
            <<",\"elapsed_seconds\":"<<elapsed<<",\"present_call_rate\":"<<(count/elapsed)<<",\"cancelled\":"<<(session.stop.load()?"true":"false")
            <<",\"native_start_monotonic_ns\":"<<std::chrono::duration_cast<std::chrono::nanoseconds>(start.time_since_epoch()).count()
            <<",\"native_end_monotonic_ns\":"<<std::chrono::duration_cast<std::chrono::nanoseconds>(end.time_since_epoch()).count()
            <<",\"physical_present_fps_verified\":false,\"samples\":["<<samples.str()<<"],\"result\":\""
            <<(elapsed>=29&&count>=300&&!session.stop.load()?"OFFSCREEN_AND_SURFACE_SUBMISSION_PASS":"INTERRUPTED_NOT_SUSTAINED_PASS")<<"\"}\n";
        return result.str();
    }
};
std::vector<uint32_t> shaderBytes(JNIEnv* env,jbyteArray input){jsize size=env->GetArrayLength(input);if(size<20||size%4)throw std::runtime_error("Invalid shader size");std::vector<uint32_t> result(size/4);env->GetByteArrayRegion(input,0,size,(jbyte*)result.data());if(result[0]!=0x07230203)throw std::runtime_error("Invalid SPIR-V magic");return result;}
}
extern "C" JNIEXPORT jlong JNICALL Java_org_mavk_probe_GraphicsActivity_createSession(JNIEnv*,jclass){return reinterpret_cast<jlong>(new Session);}
extern "C" JNIEXPORT void JNICALL Java_org_mavk_probe_GraphicsActivity_stopSession(JNIEnv*,jclass,jlong handle){reinterpret_cast<Session*>(handle)->stop.store(true);}
extern "C" JNIEXPORT void JNICALL Java_org_mavk_probe_GraphicsActivity_releaseSession(JNIEnv*,jclass,jlong handle){delete reinterpret_cast<Session*>(handle);}
extern "C" JNIEXPORT jstring JNICALL Java_org_mavk_probe_GraphicsActivity_runGraphics(JNIEnv* env,jclass,jlong handle,jobject surface,jbyteArray vertex,jbyteArray fragment,jstring destination,jboolean useCore,jboolean validated){
    ValidationCapture validation;std::string text;try{auto& session=*reinterpret_cast<Session*>(handle);if(session.stop.load())throw std::runtime_error("Cancelled before start");mavk::Counters retired;{Graphics graphics;graphics.validation=&validation;graphics.validated=validated;graphics.coreMode=useCore;graphics.retiredCounters=&retired;graphics.init(env,surface);graphics.shaders(shaderBytes(env,vertex),shaderBytes(env,fragment));text=graphics.run(session);}if(validation.errors.load())throw std::runtime_error("Validation errors in graphics or teardown");if(useCore){if(!retired.empty())throw std::runtime_error("Core graphics resources remain after teardown");auto end=text.rfind('}');text.insert(end,",\"core_live_zero_after_teardown\":true,\"core_submissions\":"+std::to_string(retired.totalSubmissions));}}
    catch(const std::exception& error){text="{\"result\":\"FAIL\",\"error\":\"";for(const char* p=error.what();*p;p++){if(*p=='"'||*p=='\\')text+='\\';text+=*p;}text+="\"}\n";}
    text.insert(text.rfind('}'),validation.fields());
    const char* path=env->GetStringUTFChars(destination,nullptr);{std::ofstream report(path);report<<text;if(!report)text="REPORT_WRITE_FAILED";}env->ReleaseStringUTFChars(destination,path);
    return env->NewStringUTF(text.size()>1000?"Mavk graphics complete — see app-owned JSON report.\nPhysical present FPS and backend overhead are NOT established by present calls.":text.c_str());
}
