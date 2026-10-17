// Standard Vulkan/Win32 consumer, no Mavk commands. Bounded WSI functional test.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <cstdio>
#include <stdexcept>
#include <set>
static const uint32_t vertex[]=
#include "offscreen.vert.inc"
;
static const uint32_t fragment[]=
#include "offscreen.frag.inc"
;
static void check(VkResult r,const char* operation){if(r!=VK_SUCCESS){std::printf("VK_FAILURE=%s:%d\n",operation,int(r));throw std::runtime_error(operation);}}
int main(){try{
    const HINSTANCE module=GetModuleHandleW(nullptr);RECT rect{0,0,320,240};const DWORD style=WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX;
    if(!AdjustWindowRect(&rect,style,FALSE))throw std::runtime_error("Window rectangle");
    HWND window=CreateWindowExW(0,L"STATIC",L"Standard Vulkan window - hardware render test",style,200,180,rect.right-rect.left,rect.bottom-rect.top,nullptr,nullptr,module,nullptr);
    if(!window)throw std::runtime_error("CreateWindow");ShowWindow(window,SW_SHOW);UpdateWindow(window);
    HMODULE loader=LoadLibraryW(L"vulkan-1.dll");if(!loader)throw std::runtime_error("Standard loader unavailable");
    auto get=reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(loader,"vkGetInstanceProcAddr"));if(!get)throw std::runtime_error("Loader export");
    auto create=reinterpret_cast<PFN_vkCreateInstance>(get({},"vkCreateInstance"));if(!create)throw std::runtime_error("Loader CreateInstance");
    const char* instanceExtensions[]={VK_KHR_SURFACE_EXTENSION_NAME,VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.pApplicationName="Standard Vulkan window functional check";app.apiVersion=VK_API_VERSION_1_0;
    VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};ici.pApplicationInfo=&app;ici.enabledExtensionCount=2;ici.ppEnabledExtensionNames=instanceExtensions;
    VkInstance instance{};check(create(&ici,nullptr,&instance),"CreateInstance");
#define LOAD(name) auto name=reinterpret_cast<PFN_##name>(get(instance,#name));if(!name)throw std::runtime_error("Missing " #name)
    LOAD(vkDestroyInstance);LOAD(vkEnumeratePhysicalDevices);LOAD(vkGetPhysicalDeviceProperties);LOAD(vkCreateWin32SurfaceKHR);LOAD(vkDestroySurfaceKHR);LOAD(vkGetPhysicalDeviceSurfaceSupportKHR);LOAD(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);LOAD(vkGetPhysicalDeviceSurfaceFormatsKHR);LOAD(vkCreateDevice);LOAD(vkDestroyDevice);LOAD(vkGetDeviceQueue);LOAD(vkCreateSwapchainKHR);LOAD(vkDestroySwapchainKHR);LOAD(vkGetSwapchainImagesKHR);LOAD(vkAcquireNextImageKHR);LOAD(vkQueuePresentKHR);LOAD(vkCreateSemaphore);LOAD(vkDestroySemaphore);LOAD(vkCreateImageView);LOAD(vkDestroyImageView);LOAD(vkCreateRenderPass);LOAD(vkDestroyRenderPass);LOAD(vkCreateFramebuffer);LOAD(vkDestroyFramebuffer);LOAD(vkCreateShaderModule);LOAD(vkDestroyShaderModule);LOAD(vkCreatePipelineLayout);LOAD(vkDestroyPipelineLayout);LOAD(vkCreateGraphicsPipelines);LOAD(vkDestroyPipeline);LOAD(vkCreateCommandPool);LOAD(vkDestroyCommandPool);LOAD(vkAllocateCommandBuffers);LOAD(vkBeginCommandBuffer);LOAD(vkEndCommandBuffer);LOAD(vkCmdBeginRenderPass);LOAD(vkCmdEndRenderPass);LOAD(vkCmdBindPipeline);LOAD(vkCmdSetViewport);LOAD(vkCmdSetScissor);LOAD(vkCmdPushConstants);LOAD(vkCmdDraw);LOAD(vkQueueSubmit);
#undef LOAD
    uint32_t n=1;VkPhysicalDevice physical{};check(vkEnumeratePhysicalDevices(instance,&n,&physical),"PhysicalDevice");if(n!=1)throw std::runtime_error("Physical count");
    VkPhysicalDeviceProperties props{};vkGetPhysicalDeviceProperties(physical,&props);if(props.deviceType==VK_PHYSICAL_DEVICE_TYPE_CPU)throw std::runtime_error("CPU renderer");std::printf("VULKAN_DEVICE=%s\n",props.deviceName);
    VkWin32SurfaceCreateInfoKHR sci{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};sci.hinstance=module;sci.hwnd=window;VkSurfaceKHR surface{};check(vkCreateWin32SurfaceKHR(instance,&sci,nullptr,&surface),"CreateWin32Surface");
    VkBool32 supported{};check(vkGetPhysicalDeviceSurfaceSupportKHR(physical,0,surface,&supported),"SurfaceSupport");if(!supported)throw std::runtime_error("No presentation queue");
    VkSurfaceCapabilitiesKHR caps{};check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical,surface,&caps),"SurfaceCapabilities");
    VkSurfaceFormatKHR format{};n=1;check(vkGetPhysicalDeviceSurfaceFormatsKHR(physical,surface,&n,&format),"SurfaceFormat");if(format.format!=VK_FORMAT_R8G8B8A8_UNORM)throw std::runtime_error("Expected RGBA8");
    float priority=1;VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};qci.queueCount=1;qci.pQueuePriorities=&priority;
    const char* deviceExtension=VK_KHR_SWAPCHAIN_EXTENSION_NAME;VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};dci.queueCreateInfoCount=1;dci.pQueueCreateInfos=&qci;dci.enabledExtensionCount=1;dci.ppEnabledExtensionNames=&deviceExtension;VkDevice device{};check(vkCreateDevice(physical,&dci,nullptr,&device),"CreateDevice");VkQueue queue{};vkGetDeviceQueue(device,0,0,&queue);
    VkSwapchainCreateInfoKHR chainInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};chainInfo.surface=surface;chainInfo.minImageCount=2;chainInfo.imageFormat=format.format;chainInfo.imageColorSpace=format.colorSpace;chainInfo.imageExtent=caps.currentExtent;chainInfo.imageArrayLayers=1;chainInfo.imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;chainInfo.preTransform=VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;chainInfo.compositeAlpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;chainInfo.presentMode=VK_PRESENT_MODE_FIFO_KHR;chainInfo.clipped=VK_TRUE;
    VkSwapchainKHR chain{};check(vkCreateSwapchainKHR(device,&chainInfo,nullptr,&chain),"CreateSwapchain");VkImage images[2]{};n=2;check(vkGetSwapchainImagesKHR(device,chain,&n,images),"SwapchainImages");
    VkAttachmentDescription attachment{};attachment.format=format.format;attachment.samples=VK_SAMPLE_COUNT_1_BIT;attachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;attachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;attachment.finalLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference color{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;sub.colorAttachmentCount=1;sub.pColorAttachments=&color;VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};rp.attachmentCount=1;rp.pAttachments=&attachment;rp.subpassCount=1;rp.pSubpasses=&sub;VkRenderPass pass{};check(vkCreateRenderPass(device,&rp,nullptr,&pass),"RenderPass");
    VkImageView views[2]{};VkFramebuffer frames[2]{};
    for(unsigned i=0;i<2;i++){
        VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};vi.image=images[i];vi.viewType=VK_IMAGE_VIEW_TYPE_2D;vi.format=format.format;vi.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};check(vkCreateImageView(device,&vi,nullptr,&views[i]),"ImageView");
        VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};fb.renderPass=pass;fb.attachmentCount=1;fb.pAttachments=&views[i];fb.width=caps.currentExtent.width;fb.height=caps.currentExtent.height;fb.layers=1;check(vkCreateFramebuffer(device,&fb,nullptr,&frames[i]),"Framebuffer");
    }
    VkShaderModuleCreateInfo sm{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};sm.codeSize=sizeof(vertex);sm.pCode=vertex;VkShaderModule vs{},fs{};check(vkCreateShaderModule(device,&sm,nullptr,&vs),"VertexShader");sm.codeSize=sizeof(fragment);sm.pCode=fragment;check(vkCreateShaderModule(device,&sm,nullptr,&fs),"FragmentShader");
    VkPushConstantRange push{VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,16};VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};pl.pushConstantRangeCount=1;pl.pPushConstantRanges=&push;VkPipelineLayout layout{};check(vkCreatePipelineLayout(device,&pl,nullptr,&layout),"PipelineLayout");
    VkPipelineShaderStageCreateInfo stages[2]{};for(auto& stage:stages){stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;stage.pName="main";}stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;stages[0].module=vs;stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT;stages[1].module=fs;
    VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;VkPipelineViewportStateCreateInfo vps{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};vps.viewportCount=vps.scissorCount=1;VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};raster.polygonMode=VK_POLYGON_MODE_FILL;raster.lineWidth=1;VkPipelineMultisampleStateCreateInfo multi{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};multi.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
    VkPipelineColorBlendAttachmentState ba{};ba.colorWriteMask=15;VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};blend.attachmentCount=1;blend.pAttachments=&ba;VkDynamicState dynamics[]={VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};VkPipelineDynamicStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};ds.dynamicStateCount=2;ds.pDynamicStates=dynamics;
    VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};pipelineInfo.stageCount=2;pipelineInfo.pStages=stages;pipelineInfo.pVertexInputState=&input;pipelineInfo.pInputAssemblyState=&assembly;pipelineInfo.pViewportState=&vps;pipelineInfo.pRasterizationState=&raster;pipelineInfo.pMultisampleState=&multi;pipelineInfo.pColorBlendState=&blend;pipelineInfo.pDynamicState=&ds;pipelineInfo.layout=layout;pipelineInfo.renderPass=pass;VkPipeline pipeline{};check(vkCreateGraphicsPipelines(device,{},1,&pipelineInfo,nullptr,&pipeline),"Pipeline");
    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};poolInfo.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;VkCommandPool pool{};check(vkCreateCommandPool(device,&poolInfo,nullptr,&pool),"CommandPool");VkCommandBufferAllocateInfo ca{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ca.commandPool=pool;ca.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ca.commandBufferCount=1;VkCommandBuffer cmd{};check(vkAllocateCommandBuffers(device,&ca,&cmd),"CommandBuffer");
    VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};VkSemaphore ready{},done{};check(vkCreateSemaphore(device,&semaphoreInfo,nullptr,&ready),"AcquireSemaphore");check(vkCreateSemaphore(device,&semaphoreInfo,nullptr,&done),"RenderSemaphore");
    std::set<uint64_t> windowHashes;
    for(unsigned frame=0;frame<12;frame++){
        MSG message{};while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}
        uint32_t index{};check(vkAcquireNextImageKHR(device,chain,UINT64_MAX,ready,{},&index),"Acquire");
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;check(vkBeginCommandBuffer(cmd,&begin),"Begin");
        VkClearValue clear{};clear.color.float32[3]=1;VkRenderPassBeginInfo bi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};bi.renderPass=pass;bi.framebuffer=frames[index];bi.renderArea.extent=caps.currentExtent;bi.clearValueCount=1;bi.pClearValues=&clear;vkCmdBeginRenderPass(cmd,&bi,VK_SUBPASS_CONTENTS_INLINE);vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);
        VkViewport viewport{0,0,float(caps.currentExtent.width),float(caps.currentExtent.height),0,1};VkRect2D scissor{{0,0},caps.currentExtent};vkCmdSetViewport(cmd,0,1,&viewport);vkCmdSetScissor(cmd,0,1,&scissor);float state[4]={-.15f+.03f*frame,0,0,0};vkCmdPushConstants(cmd,layout,push.stageFlags,0,16,state);vkCmdDraw(cmd,3,1,0,0);vkCmdEndRenderPass(cmd);check(vkEndCommandBuffer(cmd),"End");
        VkPipelineStageFlags wait=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.waitSemaphoreCount=1;submit.pWaitSemaphores=&ready;submit.pWaitDstStageMask=&wait;submit.commandBufferCount=1;submit.pCommandBuffers=&cmd;submit.signalSemaphoreCount=1;submit.pSignalSemaphores=&done;check(vkQueueSubmit(queue,1,&submit,{}),"Submit");
        VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};present.waitSemaphoreCount=1;present.pWaitSemaphores=&done;present.swapchainCount=1;present.pSwapchains=&chain;present.pImageIndices=&index;check(vkQueuePresentKHR(queue,&present),"Present");
        HDC dc=GetDC(window);if(!dc)throw std::runtime_error("Window verification DC");uint64_t hash=14695981039346656037ull;
        for(int y=8;y<240;y+=16)for(int x=8;x<320;x+=16){COLORREF pixel=GetPixel(dc,x,y);if(pixel==CLR_INVALID){ReleaseDC(window,dc);throw std::runtime_error("Window pixel read");}hash=(hash^pixel)*1099511628211ull;}
        COLORREF center=GetPixel(dc,160,120);ReleaseDC(window,dc);if(GetRValue(center)>2||GetGValue(center)<62||GetGValue(center)>66||GetBValue(center)<253)throw std::runtime_error("Presented blue pixel mismatch");windowHashes.insert(hash);
    }
    if(windowHashes.size()<8)throw std::runtime_error("Presented images did not change");
    std::printf("WINDOW_VULKAN_PRESENT_PASS frames=12 distinct_window_grid_hashes=%zu size=320x240; functional only, NOT FPS or third-party WSI PASS\n",windowHashes.size());
    vkDestroySemaphore(device,ready,nullptr);vkDestroySemaphore(device,done,nullptr);vkDestroyCommandPool(device,pool,nullptr);vkDestroyPipeline(device,pipeline,nullptr);vkDestroyPipelineLayout(device,layout,nullptr);vkDestroyShaderModule(device,vs,nullptr);vkDestroyShaderModule(device,fs,nullptr);for(unsigned i=0;i<2;i++){vkDestroyFramebuffer(device,frames[i],nullptr);vkDestroyImageView(device,views[i],nullptr);}vkDestroyRenderPass(device,pass,nullptr);vkDestroySwapchainKHR(device,chain,nullptr);vkDestroyDevice(device,nullptr);vkDestroySurfaceKHR(instance,surface,nullptr);vkDestroyInstance(instance,nullptr);
    std::fflush(stdout);Sleep(12000);DestroyWindow(window);FreeLibrary(loader);return 0;
}catch(const std::exception& e){std::fprintf(stderr,"WINDOW_TEST_ERROR=%s\n",e.what());return 1;}}
