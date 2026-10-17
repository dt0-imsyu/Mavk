// Included after the offscreen API definitions in the opt-in WSI build only.
API VkResult VKAPI_CALL vkCreateWin32SurfaceKHR(VkInstance h,const VkWin32SurfaceCreateInfoKHR* ci,const VkAllocationCallbacks* a,VkSurfaceKHR* out){
    *out=VK_NULL_HANDLE;
    if(!as<Instance>(h)->win32Surface)return VK_ERROR_EXTENSION_NOT_PRESENT;
    if(a||ci->pNext||ci->flags)return Unsupported;
    if(!IsWindow(ci->hwnd))return VK_ERROR_SURFACE_LOST_KHR;
    DWORD process{};GetWindowThreadProcessId(ci->hwnd,&process);
    if(process!=GetCurrentProcessId())return Unsupported;
    try{*out=handle<VkSurfaceKHR>(new WindowSurface{ci->hwnd});return VK_SUCCESS;}catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}
}
API void VKAPI_CALL vkDestroySurfaceKHR(VkInstance,VkSurfaceKHR h,const VkAllocationCallbacks*){delete as<WindowSurface>(h);}
API VkBool32 VKAPI_CALL vkGetPhysicalDeviceWin32PresentationSupportKHR(VkPhysicalDevice,uint32_t family){return family==0;}
API VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice,uint32_t family,VkSurfaceKHR h,VkBool32* out){
    *out=VK_FALSE;if(!h||!IsWindow(as<WindowSurface>(h)->hwnd))return VK_ERROR_SURFACE_LOST_KHR;*out=family==0;return VK_SUCCESS;
}
API VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice,VkSurfaceKHR h,VkSurfaceCapabilitiesKHR* out){
    *out={};VkExtent2D extent{};if(!h)return VK_ERROR_SURFACE_LOST_KHR;auto r=windowExtent(as<WindowSurface>(h)->hwnd,extent);if(r)return r;
    out->minImageCount=out->maxImageCount=2;out->currentExtent=extent;out->minImageExtent={16,16};out->maxImageExtent={1024,1024};
    out->maxImageArrayLayers=1;out->supportedTransforms=out->currentTransform=VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    out->supportedCompositeAlpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;out->supportedUsageFlags=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    return VK_SUCCESS;
}
API VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice,VkSurfaceKHR h,uint32_t* count,VkSurfaceFormatKHR* out){
    if(!h||!IsWindow(as<WindowSurface>(h)->hwnd))return VK_ERROR_SURFACE_LOST_KHR;
    if(!out){*count=1;return VK_SUCCESS;}if(!*count)return VK_INCOMPLETE;
    out[0]={VK_FORMAT_R8G8B8A8_UNORM,VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};*count=1;return VK_SUCCESS;
}
API VkResult VKAPI_CALL vkGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice,VkSurfaceKHR h,uint32_t* count,VkPresentModeKHR* out){
    if(!h||!IsWindow(as<WindowSurface>(h)->hwnd))return VK_ERROR_SURFACE_LOST_KHR;
    if(!out){*count=2;return VK_SUCCESS;}uint32_t n=std::min(*count,2u);
    if(n)out[0]=VK_PRESENT_MODE_FIFO_KHR;if(n>1)out[1]=VK_PRESENT_MODE_IMMEDIATE_KHR;*count=n;return n<2?VK_INCOMPLETE:VK_SUCCESS;
}
API VkResult VKAPI_CALL vkCreateSemaphore(VkDevice h,const VkSemaphoreCreateInfo* ci,const VkAllocationCallbacks* a,VkSemaphore* out){
    *out=VK_NULL_HANDLE;if(a||ci->pNext||ci->flags)return Unsupported;
    try{*out=handle<VkSemaphore>(new BinarySemaphore{as<Device>(h),false});return VK_SUCCESS;}catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}
}
API void VKAPI_CALL vkDestroySemaphore(VkDevice,VkSemaphore h,const VkAllocationCallbacks*){delete as<BinarySemaphore>(h);}
API VkResult VKAPI_CALL vkCreateSwapchainKHR(VkDevice h,const VkSwapchainCreateInfoKHR* ci,const VkAllocationCallbacks* a,VkSwapchainKHR* out){
    *out=VK_NULL_HANDLE;auto* d=as<Device>(h);if(!d->swapchain)return VK_ERROR_EXTENSION_NOT_PRESENT;
    if(a||ci->pNext||ci->flags||!ci->surface||ci->minImageCount!=2||ci->imageFormat!=VK_FORMAT_R8G8B8A8_UNORM||ci->imageColorSpace!=VK_COLOR_SPACE_SRGB_NONLINEAR_KHR||ci->imageArrayLayers!=1||ci->imageUsage!=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT||ci->imageSharingMode!=VK_SHARING_MODE_EXCLUSIVE||ci->queueFamilyIndexCount||ci->preTransform!=VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR||ci->compositeAlpha!=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR||(ci->presentMode!=VK_PRESENT_MODE_FIFO_KHR&&ci->presentMode!=VK_PRESENT_MODE_IMMEDIATE_KHR))return Unsupported;
    VkExtent2D extent{};HWND hwnd=as<WindowSurface>(ci->surface)->hwnd;auto r=windowExtent(hwnd,extent);if(r)return r;
    if(extent.width!=ci->imageExtent.width||extent.height!=ci->imageExtent.height)return VK_ERROR_OUT_OF_DATE_KHR;
    auto* old=as<WindowSwapchain>(ci->oldSwapchain);if(old&&(old->device!=d||old->hwnd!=hwnd))return Unsupported;
    std::lock_guard<std::mutex> lock(d->mutex);uint64_t bytes=uint64_t(extent.width)*extent.height*4;
    if(d->allocated+2*bytes>32*1048576)return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    try{
        auto chain=std::make_unique<WindowSwapchain>();chain->hwnd=hwnd;chain->width=extent.width;chain->height=extent.height;chain->mode=ci->presentMode;
        for(unsigned n=0;n<2;n++){
            WindowSwapchain::Slot slot;slot.memory=std::make_unique<Memory>(Memory{d,1,std::vector<uint8_t>(size_t(bytes))});
            slot.image=std::make_unique<Image>(Image{extent.width,extent.height,slot.memory.get()});chain->slots.push_back(std::move(slot));
        }
        chain->device=d;d->allocated+=2*bytes;if(old)old->retired=true;*out=handle<VkSwapchainKHR>(chain.release());return VK_SUCCESS;
    }catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}
}
API void VKAPI_CALL vkDestroySwapchainKHR(VkDevice,VkSwapchainKHR h,const VkAllocationCallbacks*){delete as<WindowSwapchain>(h);}
API VkResult VKAPI_CALL vkGetSwapchainImagesKHR(VkDevice,VkSwapchainKHR h,uint32_t* count,VkImage* out){
    auto* chain=as<WindowSwapchain>(h);if(!out){*count=uint32_t(chain->slots.size());return VK_SUCCESS;}
    uint32_t n=std::min(*count,uint32_t(chain->slots.size()));for(uint32_t i=0;i<n;i++)out[i]=handle<VkImage>(chain->slots[i].image.get());*count=n;return n<chain->slots.size()?VK_INCOMPLETE:VK_SUCCESS;
}
API VkResult VKAPI_CALL vkAcquireNextImageKHR(VkDevice h,VkSwapchainKHR c,uint64_t timeout,VkSemaphore semaphore,VkFence fence,uint32_t* index){
    auto* d=as<Device>(h);auto* chain=as<WindowSwapchain>(c);std::lock_guard<std::mutex> lock(d->mutex);
    if(chain->device!=d||(!semaphore&&!fence)||(semaphore&&!validSemaphores(d,1,&semaphore,false))||(fence&&as<Fence>(fence)->signaled))return Unsupported;
    VkExtent2D extent{};auto r=windowExtent(chain->hwnd,extent);if(r)return r;
    if(chain->retired||extent.width!=chain->width||extent.height!=chain->height)return VK_ERROR_OUT_OF_DATE_KHR;
    for(unsigned n=0;n<chain->slots.size();n++){
        uint32_t i=(chain->next+n)%uint32_t(chain->slots.size());auto& slot=chain->slots[i];if(slot.acquired)continue;
        slot.acquired=true;chain->next=(i+1)%uint32_t(chain->slots.size());*index=i;
        if(semaphore)as<BinarySemaphore>(semaphore)->signaled=true;if(fence)as<Fence>(fence)->signaled=true;return VK_SUCCESS;
    }
    // No asynchronous producer can release an image in this synchronous slice.
    return timeout?VK_TIMEOUT:VK_NOT_READY;
}
API VkResult VKAPI_CALL vkQueuePresentKHR(VkQueue h,const VkPresentInfoKHR* ci){
    auto* d=as<Queue>(h)->device;std::lock_guard<std::mutex> lock(d->mutex);
    if(ci->pNext||ci->swapchainCount!=1||!validSemaphores(d,ci->waitSemaphoreCount,ci->pWaitSemaphores,true))return Unsupported;
    auto* chain=as<WindowSwapchain>(ci->pSwapchains[0]);uint32_t index=ci->pImageIndices[0];
    if(chain->device!=d||index>=chain->slots.size())return Unsupported;auto& slot=chain->slots[index];
    if(!slot.acquired||!slot.image->initialized||!presentLayout(slot.image->layout))return Unsupported;
    VkExtent2D extent{};VkResult r=windowExtent(chain->hwnd,extent);
    if(r==VK_SUCCESS&&(extent.width!=chain->width||extent.height!=chain->height))r=VK_ERROR_OUT_OF_DATE_KHR;
    if(r==VK_SUCCESS){
        try{
            // Guest CPU presentation of actual Mali readback, not CPU rendering.
            auto bgra=slot.memory->bytes;for(size_t i=0;i<bgra.size();i+=4)std::swap(bgra[i],bgra[i+2]);
            if(chain->mode==VK_PRESENT_MODE_FIFO_KHR&&FAILED(DwmFlush()))r=VK_ERROR_SURFACE_LOST_KHR;
            if(r==VK_SUCCESS){
                BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=LONG(chain->width);info.bmiHeader.biHeight=-LONG(chain->height);info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
                HDC dc=GetDC(chain->hwnd);if(!dc)r=VK_ERROR_SURFACE_LOST_KHR;
                else{int rows=StretchDIBits(dc,0,0,int(chain->width),int(chain->height),0,0,int(chain->width),int(chain->height),bgra.data(),&info,DIB_RGB_COLORS,SRCCOPY);ReleaseDC(chain->hwnd,dc);if(rows<=0)r=VK_ERROR_SURFACE_LOST_KHR;}
            }
        }catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}
    }
    // Present waits are consumed even when the window was resized/lost.
    for(uint32_t i=0;i<ci->waitSemaphoreCount;i++)as<BinarySemaphore>(ci->pWaitSemaphores[i])->signaled=false;
    slot.acquired=false;if(ci->pResults)ci->pResults[0]=r;return r;
}
