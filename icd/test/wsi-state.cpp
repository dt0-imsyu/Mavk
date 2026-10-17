// Host CPU-only ownership/state tests; artificial pixels, NOT guest/GPU PASS.
#define MAVK_EXPERIMENTAL_WIN32_WSI
#include "../windows/icd.cpp"
#include <cassert>
int main(){
    HWND window=CreateWindowExW(0,L"STATIC",L"Mavk hidden WSI state unit",WS_POPUP,0,0,128,96,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    assert(window);Instance instance{};instance.win32Surface=true;Device device{};device.swapchain=true;device.queue.device=&device;
    const auto dh=handle<VkDevice>(&device);const auto q=handle<VkQueue>(&device.queue);
    VkWin32SurfaceCreateInfoKHR surfaceInfo{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};surfaceInfo.hwnd=window;surfaceInfo.hinstance=GetModuleHandleW(nullptr);
    VkSurfaceKHR surface{};assert(vkCreateWin32SurfaceKHR(handle<VkInstance>(&instance),&surfaceInfo,nullptr,&surface)==VK_SUCCESS);
    VkSurfaceCapabilitiesKHR caps{};assert(vkGetPhysicalDeviceSurfaceCapabilitiesKHR({},surface,&caps)==VK_SUCCESS&&caps.currentExtent.width==128&&caps.currentExtent.height==96);
    uint32_t count=0;assert(vkGetPhysicalDeviceSurfaceFormatsKHR({},surface,&count,nullptr)==VK_SUCCESS&&count==1);
    VkSurfaceFormatKHR format{};count=0;assert(vkGetPhysicalDeviceSurfaceFormatsKHR({},surface,&count,&format)==VK_INCOMPLETE);
    VkSwapchainCreateInfoKHR info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};info.surface=surface;info.minImageCount=2;info.imageFormat=VK_FORMAT_R8G8B8A8_UNORM;info.imageColorSpace=VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;info.imageExtent={128,96};info.imageArrayLayers=1;info.imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;info.preTransform=VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;info.compositeAlpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;info.presentMode=VK_PRESENT_MODE_IMMEDIATE_KHR;
    VkSwapchainKHR chain{};assert(vkCreateSwapchainKHR(dh,&info,nullptr,&chain)==VK_SUCCESS&&device.allocated==128*96*8);
    VkSemaphoreCreateInfo sci{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};VkSemaphore ready{},done{};
    assert(vkCreateSemaphore(dh,&sci,nullptr,&ready)==VK_SUCCESS&&vkCreateSemaphore(dh,&sci,nullptr,&done)==VK_SUCCESS);
    uint32_t index=99;assert(vkAcquireNextImageKHR(dh,chain,0,ready,{},&index)==VK_SUCCESS&&index==0);
    assert(vkAcquireNextImageKHR(dh,chain,0,ready,{},&index)==Unsupported); // Cannot signal a signaled binary semaphore.
    VkImage images[2]{};count=1;assert(vkGetSwapchainImagesKHR(dh,chain,&count,images)==VK_INCOMPLETE&&count==1);
    count=2;assert(vkGetSwapchainImagesKHR(dh,chain,&count,images)==VK_SUCCESS&&count==2);
    VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};viewInfo.image=images[0];viewInfo.viewType=VK_IMAGE_VIEW_TYPE_2D;viewInfo.format=VK_FORMAT_R8G8B8A8_UNORM;viewInfo.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
    const VkComponentSwizzle explicitChannels[]={VK_COMPONENT_SWIZZLE_R,VK_COMPONENT_SWIZZLE_G,VK_COMPONENT_SWIZZLE_B,VK_COMPONENT_SWIZZLE_A};
    for(unsigned mask=0;mask<16;mask++){
        viewInfo.components={mask&1?explicitChannels[0]:VK_COMPONENT_SWIZZLE_IDENTITY,mask&2?explicitChannels[1]:VK_COMPONENT_SWIZZLE_IDENTITY,mask&4?explicitChannels[2]:VK_COMPONENT_SWIZZLE_IDENTITY,mask&8?explicitChannels[3]:VK_COMPONENT_SWIZZLE_IDENTITY};
        VkImageView view{};assert(vkCreateImageView(dh,&viewInfo,nullptr,&view)==VK_SUCCESS);vkDestroyImageView(dh,view,nullptr);
    }
    for(auto swizzle:{VK_COMPONENT_SWIZZLE_B,VK_COMPONENT_SWIZZLE_ZERO,VK_COMPONENT_SWIZZLE_ONE}){
        viewInfo.components={swizzle,VK_COMPONENT_SWIZZLE_G,VK_COMPONENT_SWIZZLE_B,VK_COMPONENT_SWIZZLE_A};VkImageView view{};assert(vkCreateImageView(dh,&viewInfo,nullptr,&view)==Unsupported&&!view);
    }
    auto* image=as<Image>(images[0]);image->initialized=true;image->layout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    Command command{};command.device=&device;command.ready=true;command.imageOps.push_back({image,nullptr,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_PRESENT_SRC_KHR});
    VkCommandBuffer cb=handle<VkCommandBuffer>(&command);VkPipelineStageFlags stage=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&cb;submit.waitSemaphoreCount=1;submit.pWaitSemaphores=&ready;submit.pWaitDstStageMask=&stage;submit.signalSemaphoreCount=1;submit.pSignalSemaphores=&done;
    assert(vkQueueSubmit(q,1,&submit,{})==VK_SUCCESS&&!as<BinarySemaphore>(ready)->signaled&&as<BinarySemaphore>(done)->signaled);
    VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};present.waitSemaphoreCount=1;present.pWaitSemaphores=&done;present.swapchainCount=1;present.pSwapchains=&chain;index=0;present.pImageIndices=&index;
    assert(vkQueuePresentKHR(q,&present)==VK_SUCCESS&&!as<BinarySemaphore>(done)->signaled);
    assert(vkQueuePresentKHR(q,&present)==Unsupported); // Not acquired; no completed render signal.
    assert(vkAcquireNextImageKHR(dh,chain,0,ready,{},&index)==VK_SUCCESS&&index==1);
    assert(SetWindowPos(window,nullptr,0,0,160,96,SWP_NOZORDER|SWP_NOACTIVATE));
    as<BinarySemaphore>(done)->signaled=true;auto* second=as<Image>(images[1]);second->initialized=true;second->layout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    assert(vkQueuePresentKHR(q,&present)==VK_ERROR_OUT_OF_DATE_KHR&&!as<BinarySemaphore>(done)->signaled);
    as<BinarySemaphore>(ready)->signaled=false;assert(vkAcquireNextImageKHR(dh,chain,0,ready,{},&index)==VK_ERROR_OUT_OF_DATE_KHR&&!as<BinarySemaphore>(ready)->signaled);
    info.oldSwapchain=chain;info.imageExtent={160,96};VkSwapchainKHR replacement{};assert(vkCreateSwapchainKHR(dh,&info,nullptr,&replacement)==VK_SUCCESS);
    assert(as<WindowSwapchain>(chain)->retired);vkDestroySwapchainKHR(dh,chain,nullptr);vkDestroySwapchainKHR(dh,replacement,nullptr);assert(device.allocated==0);
    assert(DestroyWindow(window));assert(vkGetPhysicalDeviceSurfaceCapabilitiesKHR({},surface,&caps)==VK_ERROR_SURFACE_LOST_KHR);
    vkDestroySemaphore(dh,ready,nullptr);vkDestroySemaphore(dh,done,nullptr);vkDestroySurfaceKHR({},surface,nullptr);
    std::puts("WSI_CPU_STATE_PASS artificial pixels; hidden host window only; NOT Windows guest or Mali render PASS");
}
