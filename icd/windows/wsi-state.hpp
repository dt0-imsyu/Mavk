// Isolated, compile-time opt-in Win32 presentation experiment.
// GPU rendering remains on Mali; completed pixels are copied to a guest window.
// This is not zero-copy, DWM acceleration or a conformant general WSI driver.
#pragma once
#include <dwmapi.h>
#include <set>
namespace {
struct BinarySemaphore { Device* device; bool signaled{}; };
struct WindowSurface { HWND hwnd; };
struct WindowSwapchain {
    Device* device{}; HWND hwnd{}; uint32_t width{},height{},next{};
    bool retired{}; VkPresentModeKHR mode{};
    struct Slot { std::unique_ptr<Memory> memory; std::unique_ptr<Image> image; bool acquired{}; };
    std::vector<Slot> slots;
    ~WindowSwapchain(){if(device)for(auto& slot:slots)device->allocated-=slot.memory->bytes.size();}
};
VkResult windowExtent(HWND hwnd,VkExtent2D& extent){
    RECT rect{};
    if(!IsWindow(hwnd)||!GetClientRect(hwnd,&rect))return VK_ERROR_SURFACE_LOST_KHR;
    extent={uint32_t(rect.right-rect.left),uint32_t(rect.bottom-rect.top)};
    if(extent.width<16||extent.height<16||extent.width>1024||extent.height>1024)return VK_ERROR_OUT_OF_DATE_KHR;
    return VK_SUCCESS;
}
bool wsiExtensions(uint32_t count,const char* const* names,bool device){
    std::set<std::string> unique;
    for(uint32_t i=0;i<count;i++){
        if(!names||!names[i]||!unique.insert(names[i]).second)return false;
        if(device?std::strcmp(names[i],VK_KHR_SWAPCHAIN_EXTENSION_NAME)!=0:
           (std::strcmp(names[i],VK_KHR_SURFACE_EXTENSION_NAME)&&std::strcmp(names[i],VK_KHR_WIN32_SURFACE_EXTENSION_NAME)))return false;
    }
    return device||!unique.count(VK_KHR_WIN32_SURFACE_EXTENSION_NAME)||unique.count(VK_KHR_SURFACE_EXTENSION_NAME);
}
VkResult wsiExtensionList(const char* layer,uint32_t* count,VkExtensionProperties* out,bool device){
    if(layer)return VK_ERROR_LAYER_NOT_PRESENT;
    const VkExtensionProperties instanceList[]={{VK_KHR_SURFACE_EXTENSION_NAME,25},{VK_KHR_WIN32_SURFACE_EXTENSION_NAME,6}};
    const VkExtensionProperties deviceList[]={{VK_KHR_SWAPCHAIN_EXTENSION_NAME,70}};
    uint32_t total=device?1:2;
    if(!out){*count=total;return VK_SUCCESS;}
    uint32_t written=std::min(*count,total);std::copy_n(device?deviceList:instanceList,written,out);*count=written;
    return written<total?VK_INCOMPLETE:VK_SUCCESS;
}
bool validSemaphores(Device* device,uint32_t count,const VkSemaphore* list,bool signaled){
    if(count>8||(count&&!list))return false;
    for(uint32_t i=0;i<count;i++){
        auto* sem=as<BinarySemaphore>(list[i]);
        if(!sem||sem->device!=device||sem->signaled!=signaled)return false;
        for(uint32_t j=0;j<i;j++)if(list[i]==list[j])return false;
    }
    return true;
}
bool presentLayout(VkImageLayout layout){return layout==VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;}
}
