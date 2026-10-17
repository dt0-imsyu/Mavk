// Research offscreen ICD slice. Not a conformant/general-purpose Vulkan driver.
// Unsupported operations are rejected, never replaced by fixed host shaders.
#include "transport.hpp"
#define VK_NO_PROTOTYPES
#ifdef MAVK_EXPERIMENTAL_WIN32_WSI
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vk_icd.h>
#include <vulkan/vk_layer.h>
#include <memory>
#include <string>
#include <cstring>
#include <algorithm>
#include <mutex>
#include <cstdio>
#include "../../bridge/src/vertex_geometry.hpp"
#include "../../bridge/src/scene.hpp"
#include <cmath>
#include "../../bridge/src/compute-shape.hpp"
#ifdef MAVK_EXPERIMENTAL_SAMPLED
#include <mavk/sampled.hpp>
#endif

#define API extern "C" __declspec(dllexport) VKAPI_ATTR
namespace {
using namespace mavk::icd;
constexpr VkResult Unsupported=VK_ERROR_FEATURE_NOT_PRESENT;
#ifdef MAVK_EXPERIMENTAL_UI_LAYERS
constexpr bool UiLayersEnabled=true;
#else
constexpr bool UiLayersEnabled=false;
#endif
struct Physical;
struct Instance{VK_LOADER_DATA dispatch;std::unique_ptr<Transport> wire;std::unique_ptr<Physical> physical;bool win32Surface{};};
struct Physical{VK_LOADER_DATA dispatch;Instance* instance{};uint32_t nativeApi{},driver{},vendor{},id{};char name[256]{};};
struct Device;
struct Queue{VK_LOADER_DATA dispatch;Device* device;};
struct Device{VK_LOADER_DATA dispatch;Physical* physical;Queue queue;std::mutex mutex;uint64_t allocated=0;bool swapchain{};};
struct Memory{Device* device;uint32_t type;std::vector<uint8_t> bytes;};
struct Buffer{uint64_t bytes;Memory* memory{};uint64_t offset{};VkBufferUsageFlags usage{};};
struct Image{uint32_t w,h;Memory* memory{};VkFormat format{VK_FORMAT_R8G8B8A8_UNORM};bool linear{},initialized{};VkImageLayout layout{VK_IMAGE_LAYOUT_UNDEFINED};uint32_t levels{1},uploadedMips{};bool sampled{};uint64_t revision{},remoteTexture{},remoteRevision{};Device* owner{};std::vector<uint8_t> remoteSampler;};
struct View{Image* image;};
struct RenderPass{bool depth{};VkImageLayout finalLayout{VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};};
struct Layout{uint32_t bytes{};VkShaderStageFlags stages{};bool uniform{},storage{},sampled{};uint32_t sampledBinding=1;};
struct EmptyDescriptorLayout{bool uniform{},storage{},sampled{};uint32_t sampledBinding=1;};
struct Sampler;
struct DescriptorPool;
struct DescriptorSet{DescriptorPool* pool;Buffer* buffer{};VkDeviceSize offset{},range{};bool bad{};View* imageView{};Sampler* sampler{};};
struct DescriptorPool{Device* device{};uint32_t limit{};bool freeable{},storage{};std::vector<std::unique_ptr<DescriptorSet>> sets;bool sampled{};bool fontOnly{};uint32_t samplerCapacity{};};
struct EmptyPipelineCache{}; // Valid empty cache; caching/serialization not implemented.
struct Shader{std::vector<uint8_t> code;};
struct Pipeline{Layout pushLayout{};std::unique_ptr<Shader> vs,fs,cs;uint32_t stride{};std::vector<VkVertexInputAttributeDescription> attributes;bool depth{};VkCullModeFlags cull{};VkFrontFace front{};uint32_t blendMode=0;};
struct Frame{Image* image;Image* depth{};};
struct Command;
struct Pool{std::vector<Command*> commands;VkCommandPoolCreateFlags flags{};};
// Recording snapshots retain Vulkan object references, not their mutable mapped
// bytes: buffers/UBOs must be read at submit. Push/scissor/binding offsets are
// value snapshots. Objects must remain valid per Vulkan command-buffer rules.
struct DrawSnapshot {
    Pipeline* pipeline{};DescriptorSet* descriptor{};Buffer* vertices{};Buffer* indices{};
    VkDeviceSize vertexBufferOffset{},indexBufferOffset{};VkIndexType indexType{};
    uint32_t firstIndex{},indexCount{},pushBytes{};int32_t baseVertex{};
    VkShaderStageFlags pushStages{};VkRect2D clip{};std::vector<uint8_t> push;
};
struct Command {
    VK_LOADER_DATA dispatch;Device* device;Pool* pool;
    bool recording=false,ready=false,bad=false,inPass=false,drew=false,viewport=false,scissor=false;
    Frame* frame{};Pipeline* pipeline{};DescriptorSet* descriptor{};Buffer* output{};VkDeviceSize offset{};
    uint8_t push[64]{};uint32_t pushBytes{};VkShaderStageFlags pushStages{};float clear[4]{};float clearDepth{1};
    std::vector<mavk::bridge::SceneDraw> indexedDraws;
    Buffer* vertices{};VkDeviceSize vertexOffset{};uint32_t vertexCount{};
    Buffer* indices{};VkDeviceSize indexOffset{};VkIndexType indexType{};uint32_t indexCount{},firstIndex{};
    struct Copy{Buffer* src;Buffer* dst;VkBufferCopy region;};std::vector<Copy> copies;
    struct ImageOp{Image* src;Image* dst;VkImageLayout oldLayout,newLayout;Buffer* buffer{};VkBufferImageCopy region{};};std::vector<ImageOp> imageOps;
    VkImageLayout renderFinal{VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};uint32_t computeGroups{};
    std::vector<DrawSnapshot> drawSnapshots;VkRect2D currentClip{};bool directDraw=false;
};
struct Fence{bool signaled;};
template<class T,class H>T* as(H h){return reinterpret_cast<T*>(h);}
template<class H,class T>H handle(T* p){return reinterpret_cast<H>(p);}
template<class T>T* dispatch(T* p){set_loader_magic_value(p);return p;}
bool loaderOnly(const void* next,VkStructureType allowed){
    auto* p=static_cast<const VkBaseInStructure*>(next);
    for(unsigned n=0;p&&n<16;n++,p=p->pNext)if(p->sType!=allowed)return false;
    return p==nullptr;
}
bool flagsZero(const VkPhysicalDeviceFeatures* p){if(!p)return true;const auto* b=reinterpret_cast<const uint8_t*>(p);for(size_t i=0;i<sizeof(*p);i++)if(b[i])return false;return true;}
[[maybe_unused]] VkResult extensionList(const char* layer,uint32_t* n,VkExtensionProperties*){if(layer)return VK_ERROR_LAYER_NOT_PRESENT;*n=0;return VK_SUCCESS;}
bool sizeMatch(Command* c){return c->frame&&c->frame->image;}
uint64_t imageByteSize(const Image* i){uint64_t bytes=0;uint32_t w=i->w,h=i->h;for(uint32_t level=0;level<i->levels;level++){bytes+=uint64_t(w)*h*4;w=std::max(1u,w/2);h=std::max(1u,h/2);}return bytes;}
bool drawStateFrozen(const Command* c){return c->drew&&(!UiLayersEnabled||c->directDraw||!c->inPass);}
bool captureUiIndexed(Command* c,uint32_t count,uint32_t instances,uint32_t first,int32_t base,uint32_t firstInstance){
    if(!c->recording||!c->inPass||!c->pipeline||c->pipeline->cs||!c->pipeline->stride||!c->vertices||!c->indices||!c->viewport||!c->scissor||c->directDraw||c->drawSnapshots.size()>=64||!count||count>12288||instances!=1||firstInstance)return false;
    auto& range=c->pipeline->pushLayout;if(range.bytes&&(c->pushBytes!=range.bytes||c->pushStages!=range.stages))return false;
    if((range.uniform||range.sampled)&&!c->descriptor)return false;
    DrawSnapshot draw;draw.pipeline=c->pipeline;draw.descriptor=c->descriptor;draw.vertices=c->vertices;draw.indices=c->indices;
    draw.vertexBufferOffset=c->vertexOffset;draw.indexBufferOffset=c->indexOffset;draw.indexType=c->indexType;draw.firstIndex=first;draw.indexCount=count;draw.baseVertex=base;draw.clip=c->currentClip;
    draw.pushBytes=range.bytes;draw.pushStages=range.stages;draw.push.assign(c->push,c->push+range.bytes);c->drawSnapshots.push_back(std::move(draw));c->drew=true;return true;
}
#ifdef MAVK_EXPERIMENTAL_SAMPLED
constexpr uint64_t MaxObjectBytes=8*1048576;
#else
constexpr uint64_t MaxObjectBytes=4*1048576;
#endif
}

#ifdef MAVK_EXPERIMENTAL_SAMPLED
#include "sampled-resources.hpp"
#include "sampled-descriptors.hpp"
#endif
#ifdef MAVK_EXPERIMENTAL_WIN32_WSI
#include "wsi-state.hpp"
#endif

API VkResult VKAPI_CALL vkCreateInstance(const VkInstanceCreateInfo* ci,const VkAllocationCallbacks* allocator,VkInstance* out){
    std::fprintf(stderr,"MAVK_ICD_CREATE_INSTANCE flags=%u allocator=%u next_type=%u extensions=%u\n",ci->flags,allocator?1u:0u,ci->pNext?uint32_t(static_cast<const VkBaseInStructure*>(ci->pNext)->sType):0u,ci->enabledExtensionCount);
    *out=VK_NULL_HANDLE;if(allocator||ci->flags||!loaderOnly(ci->pNext,VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO))return Unsupported;
#ifdef MAVK_EXPERIMENTAL_WIN32_WSI
    if(!wsiExtensions(ci->enabledExtensionCount,ci->ppEnabledExtensionNames,false))return VK_ERROR_EXTENSION_NOT_PRESENT;
#else
    if(ci->enabledExtensionCount)return VK_ERROR_EXTENSION_NOT_PRESENT;
#endif
    if(ci->pApplicationInfo&&ci->pApplicationInfo->apiVersion>VK_API_VERSION_1_0)return VK_ERROR_INCOMPATIBLE_DRIVER;
    try{auto i=std::make_unique<Instance>();dispatch(i.get());
#ifdef MAVK_EXPERIMENTAL_WIN32_WSI
        for(uint32_t n=0;n<ci->enabledExtensionCount;n++)if(!std::strcmp(ci->ppEnabledExtensionNames[n],VK_KHR_WIN32_SURFACE_EXTENSION_NAME))i->win32Surface=true;
#endif
        i->wire=std::make_unique<Transport>();i->wire->open();auto p=i->wire->exchange(7,{});if(p.size()!=276||get32(p.data()+8)!=0x13b5||get32(p.data()+16)==VK_PHYSICAL_DEVICE_TYPE_CPU)return VK_ERROR_INCOMPATIBLE_DRIVER;
        i->physical=std::make_unique<Physical>();auto* phy=dispatch(i->physical.get());phy->instance=i.get();phy->nativeApi=get32(p.data());phy->driver=get32(p.data()+4);phy->vendor=get32(p.data()+8);phy->id=get32(p.data()+12);std::memcpy(phy->name,p.data()+20,255);*out=handle<VkInstance>(i.release());return VK_SUCCESS;
    }catch(const std::exception& error){std::fprintf(stderr,"MAVK_ICD_INSTANCE_TRANSPORT=%s\n",error.what());return VK_ERROR_INITIALIZATION_FAILED;}
}
API void VKAPI_CALL vkDestroyInstance(VkInstance i,const VkAllocationCallbacks*){delete as<Instance>(i);}
API VkResult VKAPI_CALL vkEnumeratePhysicalDevices(VkInstance i,uint32_t* n,VkPhysicalDevice* p){if(!p){*n=1;return VK_SUCCESS;}if(!*n)return VK_INCOMPLETE;*n=1;*p=handle<VkPhysicalDevice>(as<Instance>(i)->physical.get());return VK_SUCCESS;}
API void VKAPI_CALL vkGetPhysicalDeviceFeatures(VkPhysicalDevice,VkPhysicalDeviceFeatures* p){std::memset(p,0,sizeof(*p));}
API void VKAPI_CALL vkGetPhysicalDeviceProperties(VkPhysicalDevice p,VkPhysicalDeviceProperties* out){
    auto* phy=as<Physical>(p);std::memset(out,0,sizeof(*out));out->apiVersion=VK_API_VERSION_1_0;out->driverVersion=1;out->vendorID=phy->vendor;out->deviceID=phy->id;out->deviceType=VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU;std::string name="Mavk experimental / "+std::string(phy->name);std::strncpy(out->deviceName,name.c_str(),255);
    // Deliberately restricted virtual-device limits, not the native GPU limits.
    out->limits.maxPerStageResources=1;
    out->limits.maxStorageBufferRange=524288;out->limits.minStorageBufferOffsetAlignment=16;
    out->limits.maxPerStageDescriptorStorageBuffers=out->limits.maxDescriptorSetStorageBuffers=1;
    out->limits.maxComputeWorkGroupCount[0]=256;out->limits.maxComputeWorkGroupCount[1]=out->limits.maxComputeWorkGroupCount[2]=1;
    out->limits.maxComputeWorkGroupSize[0]=256;out->limits.maxComputeWorkGroupSize[1]=out->limits.maxComputeWorkGroupSize[2]=1;out->limits.maxComputeWorkGroupInvocations=256;
    auto& l=out->limits;l.maxImageDimension2D=1024;l.maxFramebufferWidth=1024;l.maxFramebufferHeight=1024;l.maxFramebufferLayers=1;l.maxImageArrayLayers=1;l.maxPushConstantsSize=64;l.maxMemoryAllocationCount=16;l.maxBoundDescriptorSets=1;l.maxUniformBufferRange=4096;l.minUniformBufferOffsetAlignment=16;l.maxPerStageDescriptorUniformBuffers=1;l.maxDescriptorSetUniformBuffers=1;l.maxColorAttachments=1;l.maxViewports=1;l.maxViewportDimensions[0]=1024;l.maxViewportDimensions[1]=1024;l.viewportBoundsRange[0]=0;l.viewportBoundsRange[1]=1024;l.framebufferColorSampleCounts=VK_SAMPLE_COUNT_1_BIT;l.framebufferDepthSampleCounts=VK_SAMPLE_COUNT_1_BIT;l.sampledImageColorSampleCounts=VK_SAMPLE_COUNT_1_BIT;l.nonCoherentAtomSize=1;l.minMemoryMapAlignment=8;l.bufferImageGranularity=1;l.maxVertexInputBindings=1;l.maxVertexInputAttributes=4;l.maxVertexInputBindingStride=64;l.maxVertexInputAttributeOffset=60;
#ifdef MAVK_EXPERIMENTAL_SAMPLED
    l.maxPerStageDescriptorSamplers=l.maxDescriptorSetSamplers=1;l.maxPerStageDescriptorSampledImages=l.maxDescriptorSetSampledImages=1;l.maxSamplerAllocationCount=16;l.maxSamplerLodBias=2;
#endif
}
API void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice,uint32_t* n,VkQueueFamilyProperties* out){if(out&&*n){std::memset(out,0,sizeof(*out));out->queueFlags=VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_TRANSFER_BIT|VK_QUEUE_COMPUTE_BIT;out->queueCount=1;out->minImageTransferGranularity={1,1,1};}*n=1;}
API void VKAPI_CALL vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice,VkPhysicalDeviceMemoryProperties* out){std::memset(out,0,sizeof(*out));out->memoryHeapCount=1;out->memoryHeaps[0].size=32*1048576;out->memoryTypeCount=2;out->memoryTypes[0]={VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,0};out->memoryTypes[1]={VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,0};}
API void VKAPI_CALL vkGetPhysicalDeviceFormatProperties(VkPhysicalDevice,VkFormat format,VkFormatProperties* out){
    std::memset(out,0,sizeof(*out));
    if(format==VK_FORMAT_R8G8B8A8_UNORM)out->optimalTilingFeatures=VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT|VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
#ifdef MAVK_EXPERIMENTAL_SAMPLED
    if(format==VK_FORMAT_R8G8B8A8_UNORM)out->optimalTilingFeatures|=VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT|VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
#endif
    if(format==VK_FORMAT_R8G8B8A8_UNORM)out->linearTilingFeatures=VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
    if(format==VK_FORMAT_D32_SFLOAT)out->optimalTilingFeatures=VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
}
API VkResult VKAPI_CALL vkGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice,VkFormat f,VkImageType t,VkImageTiling til,VkImageUsageFlags usage,VkImageCreateFlags flags,VkImageFormatProperties* out){
#ifdef MAVK_EXPERIMENTAL_SAMPLED
    if(f==VK_FORMAT_R8G8B8A8_UNORM&&t==VK_IMAGE_TYPE_2D&&til==VK_IMAGE_TILING_OPTIMAL&&usage==(VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT)&&!flags){*out={{1024,1024,1},11,1,VK_SAMPLE_COUNT_1_BIT,5592404};return VK_SUCCESS;}
#endif
    VkImageUsageFlags supported=til==VK_IMAGE_TILING_LINEAR?(f==VK_FORMAT_R8G8B8A8_UNORM?VkImageUsageFlags(VK_IMAGE_USAGE_TRANSFER_DST_BIT):0):f==VK_FORMAT_R8G8B8A8_UNORM?(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT):f==VK_FORMAT_D32_SFLOAT?VkImageUsageFlags(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT):0;
    if(!supported||t!=VK_IMAGE_TYPE_2D||(til!=VK_IMAGE_TILING_OPTIMAL&&til!=VK_IMAGE_TILING_LINEAR)||flags||(usage&~supported))return VK_ERROR_FORMAT_NOT_SUPPORTED;
    *out={{1024,1024,1},1,1,VK_SAMPLE_COUNT_1_BIT,4194304};return VK_SUCCESS;
}
API VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(const char* layer,uint32_t* n,VkExtensionProperties* p){
#ifdef MAVK_EXPERIMENTAL_WIN32_WSI
return wsiExtensionList(layer,n,p,false);
#else
return extensionList(layer,n,p);
#endif
}
API void VKAPI_CALL vkGetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice,VkFormat,VkImageType,VkSampleCountFlagBits,VkImageUsageFlags,VkImageTiling,uint32_t* count,VkSparseImageFormatProperties*){*count=0;}
API VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(VkPhysicalDevice,const char* layer,uint32_t* n,VkExtensionProperties* p){
#ifdef MAVK_EXPERIMENTAL_WIN32_WSI
return wsiExtensionList(layer,n,p,true);
#else
return extensionList(layer,n,p);
#endif
}
API VkResult VKAPI_CALL vkCreateDevice(VkPhysicalDevice p,const VkDeviceCreateInfo* ci,const VkAllocationCallbacks* allocator,VkDevice* out){*out=VK_NULL_HANDLE;if(allocator||ci->flags||!loaderOnly(ci->pNext,VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO)||!flagsZero(ci->pEnabledFeatures)||ci->queueCreateInfoCount!=1||ci->pQueueCreateInfos[0].queueFamilyIndex||ci->pQueueCreateInfos[0].queueCount!=1||ci->pQueueCreateInfos[0].flags||ci->pQueueCreateInfos[0].pNext)return Unsupported;
#ifdef MAVK_EXPERIMENTAL_WIN32_WSI
if(!wsiExtensions(ci->enabledExtensionCount,ci->ppEnabledExtensionNames,true)||(ci->enabledExtensionCount&&!as<Physical>(p)->instance->win32Surface))return VK_ERROR_EXTENSION_NOT_PRESENT;
#else
if(ci->enabledExtensionCount)return VK_ERROR_EXTENSION_NOT_PRESENT;
#endif
try{auto d=std::make_unique<Device>();dispatch(d.get());d->physical=as<Physical>(p);d->swapchain=ci->enabledExtensionCount!=0;dispatch(&d->queue);d->queue.device=d.get();*out=handle<VkDevice>(d.release());return VK_SUCCESS;}catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}}
API void VKAPI_CALL vkDestroyDevice(VkDevice d,const VkAllocationCallbacks*){delete as<Device>(d);}
API void VKAPI_CALL vkGetDeviceQueue(VkDevice d,uint32_t f,uint32_t i,VkQueue* out){*out=(f||i)?VK_NULL_HANDLE:handle<VkQueue>(&as<Device>(d)->queue);}
API VkResult VKAPI_CALL vkDeviceWaitIdle(VkDevice){return VK_SUCCESS;}
API VkResult VKAPI_CALL vkQueueWaitIdle(VkQueue){return VK_SUCCESS;}
API VkResult VKAPI_CALL vkAllocateMemory(VkDevice d,const VkMemoryAllocateInfo* ci,const VkAllocationCallbacks* a,VkDeviceMemory* out){auto* device=as<Device>(d);if(a||ci->pNext||ci->memoryTypeIndex>1||!ci->allocationSize||ci->allocationSize>MaxObjectBytes||device->allocated+ci->allocationSize>32*1048576)return VK_ERROR_OUT_OF_DEVICE_MEMORY;try{auto m=std::make_unique<Memory>();m->device=device;m->type=ci->memoryTypeIndex;m->bytes.resize(size_t(ci->allocationSize));device->allocated+=ci->allocationSize;*out=handle<VkDeviceMemory>(m.release());return VK_SUCCESS;}catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}}
API void VKAPI_CALL vkFreeMemory(VkDevice,VkDeviceMemory h,const VkAllocationCallbacks*){if(h){auto* m=as<Memory>(h);m->device->allocated-=m->bytes.size();delete m;}}
API VkResult VKAPI_CALL vkMapMemory(VkDevice,VkDeviceMemory h,VkDeviceSize offset,VkDeviceSize bytes,VkMemoryMapFlags flags,void** out){auto* m=as<Memory>(h);if(m->type||flags||offset>m->bytes.size()||(bytes!=VK_WHOLE_SIZE&&bytes>m->bytes.size()-offset))return VK_ERROR_MEMORY_MAP_FAILED;*out=m->bytes.data()+offset;return VK_SUCCESS;}
API void VKAPI_CALL vkUnmapMemory(VkDevice,VkDeviceMemory){}
API VkResult VKAPI_CALL vkFlushMappedMemoryRanges(VkDevice,uint32_t,const VkMappedMemoryRange*){return VK_SUCCESS;}
API VkResult VKAPI_CALL vkInvalidateMappedMemoryRanges(VkDevice,uint32_t,const VkMappedMemoryRange*){return VK_SUCCESS;}
API VkResult VKAPI_CALL vkCreateBuffer(VkDevice,const VkBufferCreateInfo* ci,const VkAllocationCallbacks* a,VkBuffer* out){if(a||ci->pNext||ci->flags||!ci->size||ci->size>MaxObjectBytes||!ci->usage||(ci->usage&~(VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_INDEX_BUFFER_BIT|VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT|VK_BUFFER_USAGE_STORAGE_BUFFER_BIT))||ci->sharingMode!=VK_SHARING_MODE_EXCLUSIVE)return Unsupported;try{auto b=std::make_unique<Buffer>();b->bytes=ci->size;b->usage=ci->usage;*out=handle<VkBuffer>(b.release());return VK_SUCCESS;}catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}}
API void VKAPI_CALL vkDestroyBuffer(VkDevice,VkBuffer h,const VkAllocationCallbacks*){delete as<Buffer>(h);}
API void VKAPI_CALL vkCmdCopyBuffer(VkCommandBuffer h,VkBuffer src,VkBuffer dst,uint32_t count,const VkBufferCopy* regions){
    auto* c=as<Command>(h);auto* s=as<Buffer>(src);auto* d=as<Buffer>(dst);
    if(!c->recording||c->frame||c->inPass||c->drew||!c->imageOps.empty()||!count||count>64||count>64-c->copies.size()||!(s->usage&VK_BUFFER_USAGE_TRANSFER_SRC_BIT)||!(d->usage&VK_BUFFER_USAGE_TRANSFER_DST_BIT)){c->bad=true;return;}
    for(uint32_t i=0;i<count;i++){auto& r=regions[i];if(!r.size||r.size%4||r.srcOffset%4||r.dstOffset%4||r.srcOffset>s->bytes||r.size>s->bytes-r.srcOffset||r.dstOffset>d->bytes||r.size>d->bytes-r.dstOffset){c->bad=true;return;}}
    try{for(uint32_t i=0;i<count;i++)c->copies.push_back({s,d,regions[i]});}catch(...){c->bad=true;}
}
API void VKAPI_CALL vkGetBufferMemoryRequirements(VkDevice,VkBuffer h,VkMemoryRequirements* out){*out={as<Buffer>(h)->bytes,4,3};}
API VkResult VKAPI_CALL vkBindBufferMemory(VkDevice,VkBuffer h,VkDeviceMemory memory,VkDeviceSize offset){auto* b=as<Buffer>(h);auto* m=as<Memory>(memory);if(offset%4||offset>m->bytes.size()||b->bytes>m->bytes.size()-offset)return VK_ERROR_OUT_OF_DEVICE_MEMORY;b->memory=m;b->offset=offset;return VK_SUCCESS;}
API VkResult VKAPI_CALL vkCreateImage(VkDevice d,const VkImageCreateInfo* ci,const VkAllocationCallbacks* a,VkImage* out){
#ifdef MAVK_EXPERIMENTAL_SAMPLED
    *out=VK_NULL_HANDLE;if(ci->usage==(VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT))return createSampledImage(as<Device>(d),*ci,a,out);
#else
    (void)d;
#endif
    *out=VK_NULL_HANDLE;bool linear=ci->tiling==VK_IMAGE_TILING_LINEAR;
    VkImageUsageFlags usage=linear?VkImageUsageFlags(VK_IMAGE_USAGE_TRANSFER_DST_BIT):ci->format==VK_FORMAT_D32_SFLOAT?VkImageUsageFlags(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT):VkImageUsageFlags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    if(a||ci->pNext||ci->flags||ci->imageType!=VK_IMAGE_TYPE_2D||(ci->format!=VK_FORMAT_R8G8B8A8_UNORM&&ci->format!=VK_FORMAT_D32_SFLOAT)||ci->extent.width<16||ci->extent.width>1024||ci->extent.height<16||ci->extent.height>1024||ci->extent.depth!=1||ci->mipLevels!=1||ci->arrayLayers!=1||ci->samples!=VK_SAMPLE_COUNT_1_BIT||(ci->tiling!=VK_IMAGE_TILING_OPTIMAL&&!linear)||(linear&&ci->format!=VK_FORMAT_R8G8B8A8_UNORM)||ci->sharingMode!=VK_SHARING_MODE_EXCLUSIVE||ci->initialLayout!=VK_IMAGE_LAYOUT_UNDEFINED||ci->usage!=usage)return Unsupported;
    try{auto image=std::make_unique<Image>();image->w=ci->extent.width;image->h=ci->extent.height;image->format=ci->format;image->linear=linear;*out=handle<VkImage>(image.release());return VK_SUCCESS;}catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}
}
API void VKAPI_CALL vkDestroyImage(VkDevice,VkImage h,const VkAllocationCallbacks*){
#ifdef MAVK_EXPERIMENTAL_SAMPLED
    if(h)try{dropRemoteImage(as<Image>(h));}catch(...){} // Lost transport is reclaimed by session EOF.
#endif
    delete as<Image>(h);
}
API void VKAPI_CALL vkGetImageMemoryRequirements(VkDevice,VkImage h,VkMemoryRequirements* out){auto* i=as<Image>(h);*out={imageByteSize(i),4,i->linear?1u:2u};}
API VkResult VKAPI_CALL vkBindImageMemory(VkDevice,VkImage h,VkDeviceMemory memory,VkDeviceSize offset){auto* i=as<Image>(h);auto* m=as<Memory>(memory);if(m->type!=(i->linear?0u:1u)||offset||m->bytes.size()<imageByteSize(i))return VK_ERROR_OUT_OF_DEVICE_MEMORY;i->memory=m;return VK_SUCCESS;}
namespace {
bool identityComponents(const VkComponentMapping& c){
    // Explicit R/G/B/A and per-component IDENTITY have identical semantics.
    return (c.r==VK_COMPONENT_SWIZZLE_IDENTITY||c.r==VK_COMPONENT_SWIZZLE_R)&&
           (c.g==VK_COMPONENT_SWIZZLE_IDENTITY||c.g==VK_COMPONENT_SWIZZLE_G)&&
           (c.b==VK_COMPONENT_SWIZZLE_IDENTITY||c.b==VK_COMPONENT_SWIZZLE_B)&&
           (c.a==VK_COMPONENT_SWIZZLE_IDENTITY||c.a==VK_COMPONENT_SWIZZLE_A);
}
}
API VkResult VKAPI_CALL vkCreateImageView(VkDevice,const VkImageViewCreateInfo* ci,const VkAllocationCallbacks* a,VkImageView* out){*out=VK_NULL_HANDLE;auto r=ci->subresourceRange;if(a||ci->pNext||ci->flags||ci->viewType!=VK_IMAGE_VIEW_TYPE_2D||ci->format!=as<Image>(ci->image)->format||r.aspectMask!=(ci->format==VK_FORMAT_D32_SFLOAT?VK_IMAGE_ASPECT_DEPTH_BIT:VK_IMAGE_ASPECT_COLOR_BIT)||r.baseMipLevel||r.levelCount!=as<Image>(ci->image)->levels||r.baseArrayLayer||r.layerCount!=1||!identityComponents(ci->components))return Unsupported;try{*out=handle<VkImageView>(new View{as<Image>(ci->image)});return VK_SUCCESS;}catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}}
API void VKAPI_CALL vkDestroyImageView(VkDevice,VkImageView h,const VkAllocationCallbacks*){delete as<View>(h);}
API VkResult VKAPI_CALL vkCreateShaderModule(VkDevice,const VkShaderModuleCreateInfo* ci,const VkAllocationCallbacks* a,VkShaderModule* out){if(a||ci->pNext||ci->flags||ci->codeSize<20||ci->codeSize>65536||ci->codeSize%4||ci->pCode[0]!=0x07230203)return Unsupported;try{auto s=std::make_unique<Shader>();auto* bytes=reinterpret_cast<const uint8_t*>(ci->pCode);s->code.assign(bytes,bytes+ci->codeSize);*out=handle<VkShaderModule>(s.release());return VK_SUCCESS;}catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}}
API void VKAPI_CALL vkDestroyShaderModule(VkDevice,VkShaderModule h,const VkAllocationCallbacks*){delete as<Shader>(h);}
API VkResult VKAPI_CALL vkCreateDescriptorSetLayout(VkDevice,const VkDescriptorSetLayoutCreateInfo* ci,const VkAllocationCallbacks* a,VkDescriptorSetLayout* out){
#ifdef MAVK_EXPERIMENTAL_SAMPLED
    *out=VK_NULL_HANDLE;if(!a&&!ci->pNext&&!ci->flags&&(ci->bindingCount==2||(UiLayersEnabled&&ci->bindingCount==1&&ci->pBindings&&ci->pBindings[0].descriptorType==VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)))return sampledLayout(*ci,out);
#endif
    *out=VK_NULL_HANDLE;if(a||ci->pNext||ci->flags||ci->bindingCount>1)return Unsupported;
    bool storage=false;
    if(ci->bindingCount){if(!ci->pBindings)return Unsupported;const auto& b=ci->pBindings[0];storage=b.descriptorType==VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;if(b.binding||(!storage&&b.descriptorType!=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)||b.descriptorCount!=1||b.stageFlags!=(storage?VK_SHADER_STAGE_COMPUTE_BIT:VK_SHADER_STAGE_VERTEX_BIT)||b.pImmutableSamplers)return Unsupported;}
    try{*out=handle<VkDescriptorSetLayout>(new EmptyDescriptorLayout{ci->bindingCount!=0&&!storage,storage});return VK_SUCCESS;}catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}
}
API void VKAPI_CALL vkDestroyDescriptorSetLayout(VkDevice,VkDescriptorSetLayout h,const VkAllocationCallbacks*){delete as<EmptyDescriptorLayout>(h);}
API VkResult VKAPI_CALL vkCreatePipelineCache(VkDevice,const VkPipelineCacheCreateInfo* ci,const VkAllocationCallbacks* a,VkPipelineCache* out){
    *out=VK_NULL_HANDLE;if(a||ci->pNext||ci->flags||ci->initialDataSize)return Unsupported;
    try{*out=handle<VkPipelineCache>(new EmptyPipelineCache);return VK_SUCCESS;}catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}
}
API void VKAPI_CALL vkDestroyPipelineCache(VkDevice,VkPipelineCache h,const VkAllocationCallbacks*){delete as<EmptyPipelineCache>(h);}
API VkResult VKAPI_CALL vkCreatePipelineLayout(VkDevice,const VkPipelineLayoutCreateInfo* ci,const VkAllocationCallbacks* a,VkPipelineLayout* out){
    *out=VK_NULL_HANDLE;if(a||ci->pNext||ci->flags||ci->setLayoutCount>1||ci->pushConstantRangeCount>1)return Unsupported;
    bool uniform=ci->setLayoutCount&&as<EmptyDescriptorLayout>(ci->pSetLayouts[0])->uniform;
    bool storage=ci->setLayoutCount&&as<EmptyDescriptorLayout>(ci->pSetLayouts[0])->storage;
    if(uniform){if(ci->pushConstantRangeCount)return Unsupported;}
    else if(!storage&&(ci->pushConstantRangeCount!=1||(ci->setLayoutCount&&!(UiLayersEnabled&&as<EmptyDescriptorLayout>(ci->pSetLayouts[0])->sampled&&as<EmptyDescriptorLayout>(ci->pSetLayouts[0])->sampledBinding==0))))return Unsupported;
    Layout result{};result.uniform=uniform;result.storage=storage;
    result.sampled=ci->setLayoutCount&&as<EmptyDescriptorLayout>(ci->pSetLayouts[0])->sampled;
    if(ci->setLayoutCount)result.sampledBinding=as<EmptyDescriptorLayout>(ci->pSetLayouts[0])->sampledBinding;
    if(ci->pushConstantRangeCount){const auto& range=ci->pPushConstantRanges[0];
        if(range.offset||!range.size||range.size>64||range.size%4||!range.stageFlags||(storage?range.stageFlags!=VK_SHADER_STAGE_COMPUTE_BIT:bool(range.stageFlags&~(VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT))))return Unsupported;
        result.bytes=range.size;result.stages=range.stageFlags;
    }
    try{*out=handle<VkPipelineLayout>(new Layout(result));return VK_SUCCESS;}catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}
}
#include "uniform-descriptors.hpp"
#include "compute-commands.hpp"
API void VKAPI_CALL vkDestroyPipelineLayout(VkDevice,VkPipelineLayout h,const VkAllocationCallbacks*){delete as<Layout>(h);}
API VkResult VKAPI_CALL vkCreateRenderPass(VkDevice,const VkRenderPassCreateInfo* ci,const VkAllocationCallbacks* a,VkRenderPass* out){
    if(a||ci->pNext||ci->flags||(ci->attachmentCount!=1&&ci->attachmentCount!=2)||ci->subpassCount!=1)return Unsupported;
    const auto& color=ci->pAttachments[0];bool depth=ci->attachmentCount==2;
    if(color.format!=VK_FORMAT_R8G8B8A8_UNORM||color.samples!=VK_SAMPLE_COUNT_1_BIT||color.loadOp!=VK_ATTACHMENT_LOAD_OP_CLEAR||color.storeOp!=VK_ATTACHMENT_STORE_OP_STORE||color.initialLayout!=VK_IMAGE_LAYOUT_UNDEFINED||(color.finalLayout!=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL&&color.finalLayout!=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
#ifdef MAVK_EXPERIMENTAL_WIN32_WSI
&&color.finalLayout!=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
#endif
))return Unsupported;
    const auto& s=ci->pSubpasses[0];
    if(s.flags||s.pipelineBindPoint!=VK_PIPELINE_BIND_POINT_GRAPHICS||s.inputAttachmentCount||s.colorAttachmentCount!=1||s.pColorAttachments[0].attachment||s.pColorAttachments[0].layout!=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL||s.pResolveAttachments||s.preserveAttachmentCount)return Unsupported;
    if(depth){
        const auto& z=ci->pAttachments[1];
        if(z.format!=VK_FORMAT_D32_SFLOAT||z.samples!=VK_SAMPLE_COUNT_1_BIT||z.loadOp!=VK_ATTACHMENT_LOAD_OP_CLEAR||(z.storeOp!=VK_ATTACHMENT_STORE_OP_DONT_CARE&&z.storeOp!=VK_ATTACHMENT_STORE_OP_STORE)||z.initialLayout!=VK_IMAGE_LAYOUT_UNDEFINED||z.finalLayout!=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL||!s.pDepthStencilAttachment||s.pDepthStencilAttachment->attachment!=1||s.pDepthStencilAttachment->layout!=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)return Unsupported;
    }else if(s.pDepthStencilAttachment)return Unsupported;
    try{*out=handle<VkRenderPass>(new RenderPass{depth,color.finalLayout});return VK_SUCCESS;}catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}
}
API void VKAPI_CALL vkDestroyRenderPass(VkDevice,VkRenderPass h,const VkAllocationCallbacks*){delete as<RenderPass>(h);}
API VkResult VKAPI_CALL vkCreateFramebuffer(VkDevice,const VkFramebufferCreateInfo* ci,const VkAllocationCallbacks* a,VkFramebuffer* out){
    bool depth=as<RenderPass>(ci->renderPass)->depth;
    if(a||ci->pNext||ci->flags||ci->attachmentCount!=(depth?2u:1u)||ci->layers!=1)return Unsupported;
    auto* i=as<View>(ci->pAttachments[0])->image;Image* z=depth?as<View>(ci->pAttachments[1])->image:nullptr;
    if(ci->width!=i->w||ci->height!=i->h||i->format!=VK_FORMAT_R8G8B8A8_UNORM||(z&&(z->format!=VK_FORMAT_D32_SFLOAT||z->w!=i->w||z->h!=i->h)))return Unsupported;
    try{*out=handle<VkFramebuffer>(new Frame{i,z});return VK_SUCCESS;}catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}
}
API void VKAPI_CALL vkDestroyFramebuffer(VkDevice,VkFramebuffer h,const VkAllocationCallbacks*){delete as<Frame>(h);}
API VkResult VKAPI_CALL vkCreateGraphicsPipelines(VkDevice,VkPipelineCache,uint32_t count,const VkGraphicsPipelineCreateInfo* list,const VkAllocationCallbacks* a,VkPipeline* out){for(uint32_t i=0;i<count;i++)out[i]=VK_NULL_HANDLE;if(a||count!=1)return Unsupported;const auto& p=*list;if(p.pNext||p.flags||p.basePipelineHandle||p.stageCount!=2||p.subpass||p.pTessellationState||!p.pVertexInputState||!p.pInputAssemblyState||p.pInputAssemblyState->topology!=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST||p.pInputAssemblyState->primitiveRestartEnable||!p.pRasterizationState||p.pRasterizationState->depthClampEnable||p.pRasterizationState->rasterizerDiscardEnable||p.pRasterizationState->polygonMode!=VK_POLYGON_MODE_FILL||(p.pRasterizationState->cullMode&~3u)||uint32_t(p.pRasterizationState->frontFace)>1||p.pRasterizationState->depthBiasEnable||p.pRasterizationState->lineWidth!=1||!p.pMultisampleState||p.pMultisampleState->rasterizationSamples!=VK_SAMPLE_COUNT_1_BIT||p.pMultisampleState->sampleShadingEnable||p.pMultisampleState->alphaToCoverageEnable||p.pMultisampleState->alphaToOneEnable||!p.pColorBlendState||p.pColorBlendState->logicOpEnable||p.pColorBlendState->attachmentCount!=1||p.pColorBlendState->pAttachments[0].blendEnable||p.pColorBlendState->pAttachments[0].colorWriteMask!=15||!p.pDynamicState||p.pDynamicState->dynamicStateCount!=2||!p.pViewportState||p.pViewportState->viewportCount!=1||p.pViewportState->scissorCount!=1)return Unsupported;
    bool depth=as<RenderPass>(p.renderPass)->depth;
    if(depth){const auto* z=p.pDepthStencilState;if(!z||z->pNext||z->flags||z->depthTestEnable!=VK_TRUE||z->depthWriteEnable!=VK_TRUE||z->depthCompareOp!=VK_COMPARE_OP_LESS_OR_EQUAL||z->depthBoundsTestEnable||z->stencilTestEnable)return Unsupported;}
    else if(p.pDepthStencilState)return Unsupported;
    auto& vi=*p.pVertexInputState;uint32_t stride=0,locations=0;
    if(vi.pNext||vi.flags||vi.vertexBindingDescriptionCount>1||vi.vertexAttributeDescriptionCount>4)return Unsupported;
    if(vi.vertexBindingDescriptionCount){auto& binding=vi.pVertexBindingDescriptions[0];stride=binding.stride;if(binding.binding||binding.inputRate!=VK_VERTEX_INPUT_RATE_VERTEX||!stride||stride>64||stride%4||!vi.vertexAttributeDescriptionCount)return Unsupported;}
    else if(vi.vertexAttributeDescriptionCount)return Unsupported;
    for(uint32_t i=0;i<vi.vertexAttributeDescriptionCount;i++){auto& attribute=vi.pVertexAttributeDescriptions[i];auto width=mavk::bridge::vertexFormatBytes(attribute.format);if(attribute.location>=4||attribute.binding||(locations&(1u<<attribute.location))||!width||attribute.offset%4||attribute.offset>stride||width>stride-attribute.offset)return Unsupported;locations|=1u<<attribute.location;}
    bool vp=false,sc=false;for(uint32_t i=0;i<2;i++){vp|=p.pDynamicState->pDynamicStates[i]==VK_DYNAMIC_STATE_VIEWPORT;sc|=p.pDynamicState->pDynamicStates[i]==VK_DYNAMIC_STATE_SCISSOR;}if(!vp||!sc)return Unsupported;Shader* vs=nullptr;Shader* fs=nullptr;for(uint32_t i=0;i<2;i++){const auto& s=p.pStages[i];if(s.pNext||s.flags||s.pSpecializationInfo||std::strcmp(s.pName,"main"))return Unsupported;if(s.stage==VK_SHADER_STAGE_VERTEX_BIT)vs=as<Shader>(s.module);else if(s.stage==VK_SHADER_STAGE_FRAGMENT_BIT)fs=as<Shader>(s.module);else return Unsupported;}if(!vs||!fs)return Unsupported;
    try{auto pipeline=std::make_unique<Pipeline>();pipeline->pushLayout=*as<Layout>(p.layout);pipeline->depth=depth;pipeline->cull=p.pRasterizationState->cullMode;pipeline->front=p.pRasterizationState->frontFace;pipeline->vs=std::make_unique<Shader>(*vs);pipeline->fs=std::make_unique<Shader>(*fs);pipeline->stride=stride;if(vi.vertexAttributeDescriptionCount)pipeline->attributes.assign(vi.pVertexAttributeDescriptions,vi.pVertexAttributeDescriptions+vi.vertexAttributeDescriptionCount);*out=handle<VkPipeline>(pipeline.release());return VK_SUCCESS;}catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}
}
API void VKAPI_CALL vkDestroyPipeline(VkDevice,VkPipeline h,const VkAllocationCallbacks*){delete as<Pipeline>(h);}
API VkResult VKAPI_CALL vkCreateCommandPool(VkDevice,const VkCommandPoolCreateInfo* ci,const VkAllocationCallbacks* a,VkCommandPool* out){if(a||ci->pNext||ci->queueFamilyIndex||(ci->flags&~VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT))return Unsupported;try{auto p=std::make_unique<Pool>();p->flags=ci->flags;*out=handle<VkCommandPool>(p.release());return VK_SUCCESS;}catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}}
API void VKAPI_CALL vkDestroyCommandPool(VkDevice,VkCommandPool h,const VkAllocationCallbacks*){if(h){auto* p=as<Pool>(h);for(auto* c:p->commands)delete c;delete p;}}
API VkResult VKAPI_CALL vkAllocateCommandBuffers(VkDevice d,const VkCommandBufferAllocateInfo* ci,VkCommandBuffer* out){
    if(ci->pNext||ci->level!=VK_COMMAND_BUFFER_LEVEL_PRIMARY||!ci->commandBufferCount||ci->commandBufferCount>16)return Unsupported;
    for(uint32_t i=0;i<ci->commandBufferCount;i++)out[i]=VK_NULL_HANDLE;
    auto* pool=as<Pool>(ci->commandPool);if(ci->commandBufferCount>64-pool->commands.size())return VK_ERROR_OUT_OF_HOST_MEMORY;
    try{std::vector<std::unique_ptr<Command>> pending;pending.reserve(ci->commandBufferCount);pool->commands.reserve(pool->commands.size()+ci->commandBufferCount);
        for(uint32_t i=0;i<ci->commandBufferCount;i++){auto owner=std::make_unique<Command>();auto* c=dispatch(owner.get());c->device=as<Device>(d);c->pool=pool;pending.push_back(std::move(owner));}
        for(uint32_t i=0;i<ci->commandBufferCount;i++){auto* c=pending[i].release();pool->commands.push_back(c);out[i]=handle<VkCommandBuffer>(c);}return VK_SUCCESS;
    }catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}
}
API VkResult VKAPI_CALL vkResetCommandBuffer(VkCommandBuffer h,VkCommandBufferResetFlags flags){
    auto* c=as<Command>(h);if((flags&~VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT)||!(c->pool->flags&VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT))return Unsupported;
    // All submissions in this limited ICD complete synchronously; none are pending.
    auto loader=c->dispatch;auto* device=c->device;auto* pool=c->pool;*c=Command{};c->dispatch=loader;c->device=device;c->pool=pool;return VK_SUCCESS;
}
API void VKAPI_CALL vkFreeCommandBuffers(VkDevice,VkCommandPool,uint32_t n,const VkCommandBuffer* p){for(uint32_t i=0;i<n;i++){auto* c=as<Command>(p[i]);auto& list=c->pool->commands;list.erase(std::remove(list.begin(),list.end(),c),list.end());delete c;}}
API VkResult VKAPI_CALL vkBeginCommandBuffer(VkCommandBuffer h,const VkCommandBufferBeginInfo* ci){auto* c=as<Command>(h);if(ci->pNext||ci->pInheritanceInfo||(ci->flags&~VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT))return Unsupported;c->recording=true;c->ready=c->bad=c->inPass=c->drew=c->viewport=c->scissor=false;c->frame=nullptr;c->pipeline=nullptr;c->descriptor=nullptr;c->output=nullptr;c->vertices=nullptr;c->vertexOffset=0;c->vertexCount=0;c->indices=nullptr;c->indexCount=0;c->firstIndex=0;c->copies.clear();c->imageOps.clear();c->indexedDraws.clear();c->drawSnapshots.clear();c->currentClip={};c->directDraw=false;c->clearDepth=1;c->pushBytes=0;c->pushStages=0;c->computeGroups=0;std::memset(c->push,0,sizeof(c->push));return VK_SUCCESS;}
API VkResult VKAPI_CALL vkEndCommandBuffer(VkCommandBuffer h){auto* c=as<Command>(h);c->recording=false;c->ready=!c->bad&&!c->inPass&&((c->computeGroups&&!c->frame&&c->copies.empty()&&c->imageOps.empty())||(c->drew&&c->frame&&c->pipeline&&c->copies.empty()&&c->imageOps.empty())||(!c->frame&&(!c->copies.empty()||!c->imageOps.empty())));return c->ready?VK_SUCCESS:Unsupported;}
API void VKAPI_CALL vkCmdBeginRenderPass(VkCommandBuffer h,const VkRenderPassBeginInfo* ci,VkSubpassContents contents){
    auto* c=as<Command>(h);c->frame=as<Frame>(ci->framebuffer);
    if(!c->recording||c->inPass||c->drew||ci->pNext||contents!=VK_SUBPASS_CONTENTS_INLINE||ci->renderArea.offset.x||ci->renderArea.offset.y||!sizeMatch(c)||ci->renderArea.extent.width!=c->frame->image->w||ci->renderArea.extent.height!=c->frame->image->h||ci->clearValueCount!=(c->frame->depth?2u:1u)){c->bad=true;return;}
    if(c->frame->depth){c->clearDepth=ci->pClearValues[1].depthStencil.depth;if(!std::isfinite(c->clearDepth)||c->clearDepth<0||c->clearDepth>1){c->bad=true;return;}}
    c->renderFinal=as<RenderPass>(ci->renderPass)->finalLayout;std::memcpy(c->clear,ci->pClearValues[0].color.float32,16);c->inPass=true;
}
API void VKAPI_CALL vkCmdEndRenderPass(VkCommandBuffer h){auto* c=as<Command>(h);if(!c->inPass)c->bad=true;c->inPass=false;}
API void VKAPI_CALL vkCmdBindPipeline(VkCommandBuffer h,VkPipelineBindPoint point,VkPipeline p){auto* c=as<Command>(h);auto* pipeline=as<Pipeline>(p);if(!c->recording||c->computeGroups||drawStateFrozen(c)||point!=(pipeline->cs?VK_PIPELINE_BIND_POINT_COMPUTE:VK_PIPELINE_BIND_POINT_GRAPHICS))c->bad=true;else c->pipeline=pipeline;}
API void VKAPI_CALL vkCmdSetViewport(VkCommandBuffer h,uint32_t first,uint32_t n,const VkViewport* v){auto* c=as<Command>(h);if(first||n!=1||!sizeMatch(c)||v->x||v->y||v->width!=c->frame->image->w||v->height!=c->frame->image->h||v->minDepth||v->maxDepth!=1)c->bad=true;else c->viewport=true;}
API void VKAPI_CALL vkCmdSetScissor(VkCommandBuffer h,uint32_t first,uint32_t n,const VkRect2D* s){
    auto* c=as<Command>(h);bool valid=!first&&n==1&&sizeMatch(c)&&s;
    if(valid){if(UiLayersEnabled)valid=s->offset.x>=0&&s->offset.y>=0&&s->extent.width&&s->extent.height&&uint64_t(s->offset.x)+s->extent.width<=c->frame->image->w&&uint64_t(s->offset.y)+s->extent.height<=c->frame->image->h;
        else valid=!s->offset.x&&!s->offset.y&&s->extent.width==c->frame->image->w&&s->extent.height==c->frame->image->h;}
    if(!valid)c->bad=true;else {c->scissor=true;c->currentClip=*s;}
}
API void VKAPI_CALL vkCmdPushConstants(VkCommandBuffer h,VkPipelineLayout layout,VkShaderStageFlags stages,uint32_t offset,uint32_t bytes,const void* values){
    auto* c=as<Command>(h);auto* range=as<Layout>(layout);
    if(!c->recording||c->computeGroups||(UiLayersEnabled?drawStateFrozen(c):(c->drew&&(c->indexedDraws.empty()||!c->inPass)))||!bytes||bytes%4||offset%4||offset>range->bytes||bytes>range->bytes-offset||stages!=range->stages||(!UiLayersEnabled&&c->pushBytes&&(c->pushBytes!=range->bytes||c->pushStages!=stages))){c->bad=true;return;}
    c->pushBytes=range->bytes;c->pushStages=stages;std::memcpy(c->push+offset,values,bytes);
}
API void VKAPI_CALL vkCmdBindVertexBuffers(VkCommandBuffer h,uint32_t first,uint32_t count,const VkBuffer* buffers,const VkDeviceSize* offsets){auto* c=as<Command>(h);if(!c->recording||drawStateFrozen(c)||first||count!=1){c->bad=true;return;}auto* b=as<Buffer>(buffers[0]);if(!(b->usage&VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)||offsets[0]>=b->bytes||offsets[0]%4){c->bad=true;return;}c->vertices=b;c->vertexOffset=offsets[0];}
API void VKAPI_CALL vkCmdDraw(VkCommandBuffer h,uint32_t vertices,uint32_t instances,uint32_t firstVertex,uint32_t firstInstance){auto* c=as<Command>(h);if(!c->recording||!c->inPass||!c->pipeline||!c->viewport||!c->scissor||c->drew||!vertices||vertices>4096||instances!=1||firstVertex||firstInstance||(c->pipeline->stride?!c->vertices:vertices!=3))c->bad=true;c->vertexCount=vertices;c->drew=true;c->directDraw=true;}
API void VKAPI_CALL vkCmdBindIndexBuffer(VkCommandBuffer h,VkBuffer buffer,VkDeviceSize offset,VkIndexType type){
    auto* c=as<Command>(h);auto* b=as<Buffer>(buffer);uint32_t width=type==VK_INDEX_TYPE_UINT16?2:type==VK_INDEX_TYPE_UINT32?4:0;
    if(!c->recording||drawStateFrozen(c)||!width||!(b->usage&VK_BUFFER_USAGE_INDEX_BUFFER_BIT)||offset>=b->bytes||offset%width){c->bad=true;return;}
    c->indices=b;c->indexOffset=offset;c->indexType=type;
}
API void VKAPI_CALL vkCmdDrawIndexed(VkCommandBuffer h,uint32_t count,uint32_t instances,uint32_t first,int32_t vertexOffset,uint32_t firstInstance){
    auto* c=as<Command>(h);
    if(UiLayersEnabled){try{if(!captureUiIndexed(c,count,instances,first,vertexOffset,firstInstance))c->bad=true;}catch(...){c->bad=true;}return;}
    if(!c->recording||!c->inPass||!c->pipeline||!c->pipeline->stride||!c->vertices||!c->indices||!c->viewport||!c->scissor||(c->drew&&c->indexedDraws.empty())||c->indexedDraws.size()>=16||!count||count>12288||instances!=1||vertexOffset||firstInstance||c->pushBytes!=c->pipeline->pushLayout.bytes||c->pushStages!=c->pipeline->pushLayout.stages){c->bad=true;return;}
    try{mavk::bridge::SceneDraw draw;draw.firstIndex=first;draw.indexCount=count;draw.push.assign(c->push,c->push+c->pushBytes);c->indexedDraws.push_back(std::move(draw));}
    catch(...){c->bad=true;return;}
    c->indexCount=count;c->firstIndex=first;c->drew=true;
}
API void VKAPI_CALL vkCmdPipelineBarrier(VkCommandBuffer h,VkPipelineStageFlags,VkPipelineStageFlags,VkDependencyFlags flags,uint32_t,const VkMemoryBarrier*,uint32_t bn,const VkBufferMemoryBarrier*,uint32_t count,const VkImageMemoryBarrier* images){
    auto* c=as<Command>(h);if(!c->recording||flags||bn||count>1||c->inPass){c->bad=true;return;}
    if(!count)return;const auto& b=images[0];auto* image=as<Image>(b.image);const auto& r=b.subresourceRange;
    if(b.pNext||b.srcQueueFamilyIndex!=VK_QUEUE_FAMILY_IGNORED||b.dstQueueFamilyIndex!=VK_QUEUE_FAMILY_IGNORED||r.aspectMask!=VK_IMAGE_ASPECT_COLOR_BIT||r.baseMipLevel||r.levelCount!=image->levels||r.baseArrayLayer||r.layerCount!=1){c->bad=true;return;}
#ifdef MAVK_EXPERIMENTAL_SAMPLED
    if(image->sampled){
        bool allowed=((b.oldLayout==VK_IMAGE_LAYOUT_UNDEFINED||b.oldLayout==VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)&&b.newLayout==VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)||(b.oldLayout==VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL&&b.newLayout==VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        if(c->frame||!allowed||!c->copies.empty()||c->imageOps.size()>=40){c->bad=true;return;}
        try{c->imageOps.push_back({image,nullptr,b.oldLayout,b.newLayout});}catch(...){c->bad=true;}return;
    }
#endif
    if(c->frame){if(image!=c->frame->image||c->renderFinal!=b.oldLayout||b.oldLayout!=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL||b.newLayout!=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)c->bad=true;else c->renderFinal=b.newLayout;return;}
    if(!image->linear||c->imageOps.size()>=16||!c->copies.empty()||!((b.oldLayout==VK_IMAGE_LAYOUT_UNDEFINED&&b.newLayout==VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)||(b.oldLayout==VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL&&b.newLayout==VK_IMAGE_LAYOUT_GENERAL))){c->bad=true;return;}
    try{c->imageOps.push_back({image,nullptr,b.oldLayout,b.newLayout});}catch(...){c->bad=true;}
}
API void VKAPI_CALL vkCmdCopyImage(VkCommandBuffer h,VkImage src,VkImageLayout srcLayout,VkImage dst,VkImageLayout dstLayout,uint32_t count,const VkImageCopy* regions){
    auto* c=as<Command>(h);auto* source=as<Image>(src);auto* target=as<Image>(dst);
    if(!c->recording||c->frame||c->inPass||c->drew||!c->copies.empty()||count!=1||c->imageOps.size()>=16||source->linear||!target->linear||source->format!=VK_FORMAT_R8G8B8A8_UNORM||target->format!=source->format||srcLayout!=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL||dstLayout!=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL){c->bad=true;return;}
    const auto& r=regions[0];
    auto valid=[](const VkImageSubresourceLayers& s){return s.aspectMask==VK_IMAGE_ASPECT_COLOR_BIT&&!s.mipLevel&&!s.baseArrayLayer&&s.layerCount==1;};
    if(!valid(r.srcSubresource)||!valid(r.dstSubresource)||r.srcOffset.x||r.srcOffset.y||r.srcOffset.z||r.dstOffset.x||r.dstOffset.y||r.dstOffset.z||r.extent.width!=source->w||r.extent.height!=source->h||r.extent.depth!=1||source->w!=target->w||source->h!=target->h){c->bad=true;return;}
    try{c->imageOps.push_back({source,target,srcLayout,dstLayout});}catch(...){c->bad=true;}
}
API void VKAPI_CALL vkGetImageSubresourceLayout(VkDevice,VkImage h,const VkImageSubresource* sub,VkSubresourceLayout* out){
    auto* image=as<Image>(h);std::memset(out,0,sizeof(*out));
    if(!image->linear||image->format!=VK_FORMAT_R8G8B8A8_UNORM||sub->aspectMask!=VK_IMAGE_ASPECT_COLOR_BIT||sub->mipLevel||sub->arrayLayer)return;
    out->rowPitch=uint64_t(image->w)*4;out->size=out->rowPitch*image->h;out->depthPitch=out->arrayPitch=out->size;
}
API void VKAPI_CALL vkCmdCopyImageToBuffer(VkCommandBuffer h,VkImage image,VkImageLayout layout,VkBuffer buffer,uint32_t n,const VkBufferImageCopy* r){auto* c=as<Command>(h);if(!c->drew||c->inPass||c->output||!c->frame||as<Image>(image)!=c->frame->image||layout!=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL||n!=1||r->bufferRowLength||r->bufferImageHeight||r->imageOffset.x||r->imageOffset.y||r->imageOffset.z||r->imageExtent.width!=c->frame->image->w||r->imageExtent.height!=c->frame->image->h||r->imageExtent.depth!=1||r->imageSubresource.aspectMask!=VK_IMAGE_ASPECT_COLOR_BIT||r->imageSubresource.mipLevel||r->imageSubresource.baseArrayLayer||r->imageSubresource.layerCount!=1)c->bad=true;else{c->output=as<Buffer>(buffer);if(!(c->output->usage&VK_BUFFER_USAGE_TRANSFER_DST_BIT))c->bad=true;c->offset=r->bufferOffset;}}
API VkResult VKAPI_CALL vkCreateFence(VkDevice,const VkFenceCreateInfo* ci,const VkAllocationCallbacks* a,VkFence* out){if(a||ci->pNext||(ci->flags&~VK_FENCE_CREATE_SIGNALED_BIT))return Unsupported;try{*out=handle<VkFence>(new Fence{bool(ci->flags&VK_FENCE_CREATE_SIGNALED_BIT)});return VK_SUCCESS;}catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}}
API void VKAPI_CALL vkDestroyFence(VkDevice,VkFence h,const VkAllocationCallbacks*){delete as<Fence>(h);}
API VkResult VKAPI_CALL vkGetFenceStatus(VkDevice,VkFence h){return as<Fence>(h)->signaled?VK_SUCCESS:VK_NOT_READY;}
API VkResult VKAPI_CALL vkResetFences(VkDevice,uint32_t n,const VkFence* f){for(uint32_t i=0;i<n;i++)as<Fence>(f[i])->signaled=false;return VK_SUCCESS;}
API VkResult VKAPI_CALL vkWaitForFences(VkDevice,uint32_t n,const VkFence* f,VkBool32 all,uint64_t){bool any=false,every=true;for(uint32_t i=0;i<n;i++){any|=as<Fence>(f[i])->signaled;every&=as<Fence>(f[i])->signaled;}return (all?every:any)?VK_SUCCESS:VK_TIMEOUT;}
#include "ui-commands.hpp"
API VkResult VKAPI_CALL vkQueueSubmit(VkQueue q,uint32_t n,const VkSubmitInfo* list,VkFence fence){
    auto* d=as<Queue>(q)->device;std::lock_guard<std::mutex> lock(d->mutex);
    try{for(uint32_t i=0;i<n;i++){
        const auto& s=list[i];if(s.pNext||s.commandBufferCount!=1)return Unsupported;
#ifdef MAVK_EXPERIMENTAL_WIN32_WSI
        if(!validSemaphores(d,s.waitSemaphoreCount,s.pWaitSemaphores,true)||!validSemaphores(d,s.signalSemaphoreCount,s.pSignalSemaphores,false))return Unsupported;
#else
        if(s.waitSemaphoreCount||s.signalSemaphoreCount)return Unsupported;
#endif
        const auto execute=[&]()->VkResult {
        auto* c=as<Command>(s.pCommandBuffers[0]);if(c->device!=d||!c->ready||c->bad)return Unsupported;
        if(c->computeGroups)return executeCompute(c,d);
        if(!c->drawSnapshots.empty())return executeUiLayers(c,d);
        if(!c->imageOps.empty()){
            if(!c->copies.empty())return Unsupported;
            for(const auto& op:c->imageOps){
                if(!op.src->memory||(op.dst&&!op.dst->memory))return Unsupported;
#ifdef MAVK_EXPERIMENTAL_SAMPLED
                if(op.buffer){auto r=executeSampledCopy(op);if(r!=VK_SUCCESS)return r;continue;}
#endif
                if(!op.dst){
                    if(op.oldLayout!=VK_IMAGE_LAYOUT_UNDEFINED&&op.src->layout!=op.oldLayout)return Unsupported;
                    if(op.oldLayout==VK_IMAGE_LAYOUT_UNDEFINED){op.src->initialized=false;op.src->uploadedMips=0;op.src->revision++;}
#ifdef MAVK_EXPERIMENTAL_SAMPLED
                    if(op.src->sampled&&op.newLayout==VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL&&!op.src->initialized)return Unsupported;
#endif
                    op.src->layout=op.newLayout;
                }else{
                    if(!op.src->initialized||op.src->layout!=op.oldLayout||op.dst->layout!=op.newLayout)return Unsupported;
                    std::memcpy(op.dst->memory->bytes.data(),op.src->memory->bytes.data(),size_t(op.src->w)*op.src->h*4);op.dst->initialized=true;
                }
            }
            return VK_SUCCESS;
        }
        if(!c->copies.empty()){
            for(const auto& copy:c->copies)if(!copy.src->memory||!copy.dst->memory)return Unsupported;
            // Synchronous shadow-memory transfer; not a native GPU copy or zero-copy.
            for(const auto& copy:c->copies)std::memmove(copy.dst->memory->bytes.data()+copy.dst->offset+copy.region.dstOffset,copy.src->memory->bytes.data()+copy.src->offset+copy.region.srcOffset,size_t(copy.region.size));
            return VK_SUCCESS;
        }
        if(!c->frame||!c->pipeline||!c->frame->image->memory||bool(c->frame->depth)!=c->pipeline->depth||(c->frame->depth&&!c->frame->depth->memory))return Unsupported;
        auto* b=c->output;if(b&&(!b->memory||c->offset>b->bytes))return Unsupported;
        uint64_t bytes=uint64_t(c->frame->image->w)*c->frame->image->h*4;if(b&&bytes>b->bytes-c->offset)return Unsupported;
        if(c->pushBytes!=c->pipeline->pushLayout.bytes||c->pushStages!=c->pipeline->pushLayout.stages)return Unsupported;
        bool indexed=!c->indexedDraws.empty(),uniform=c->pipeline->pushLayout.uniform;
        std::vector<uint8_t> uniformData;if(uniform&&(!indexed||!uniformSnapshot(c,uniformData)))return Unsupported;
        if(!indexed&&(c->pipeline->depth||c->pipeline->cull||bytes>1048576))return Unsupported;
        bool scene=indexed&&(uniform||c->indexedDraws.size()>1||c->pipeline->depth||c->pipeline->cull||!b||bytes>1048576);
        bool extended=c->pushBytes!=16||c->pushStages!=(VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT);
        if((scene||extended)&&!indexed)return Unsupported;
        auto draws=c->indexedDraws;
        std::vector<uint8_t> indexData,geometry;uint32_t vertices=c->vertexCount,totalIndices=0;
        if(indexed){
            auto* index=c->indices;if(!index||!index->memory)return Unsupported;
            uint32_t width=c->indexType==VK_INDEX_TYPE_UINT16?2:4;vertices=0;
            for(auto& draw:draws){
                if(draw.indexCount>12288-totalIndices||draw.push.size()!=c->pushBytes)return Unsupported;
                uint64_t at=c->indexOffset+uint64_t(draw.firstIndex)*width,amount=uint64_t(draw.indexCount)*width;
                if(at>index->bytes||amount>index->bytes-at)return Unsupported;
                const auto* data=index->memory->bytes.data()+index->offset+at;
                for(uint32_t j=0;j<draw.indexCount;j++){
                    uint32_t value=width==4?get32(data+j*width):(uint32_t(data[j*width])|(uint32_t(data[j*width+1])<<8));
                    if(value>=4096)return Unsupported;vertices=std::max(vertices,value+1);
                }
                indexData.insert(indexData.end(),data,data+amount);draw.firstIndex=totalIndices;totalIndices+=draw.indexCount;
            }
        }
        if(c->pipeline->stride){
            auto* v=c->vertices;uint64_t size=uint64_t(vertices)*c->pipeline->stride;
            if(!v||!v->memory||c->vertexOffset>v->bytes||size>v->bytes-c->vertexOffset)return Unsupported;
            if(indexed){put32(geometry,uint32_t(16+c->pipeline->attributes.size()*12+size));put32(geometry,uint32_t(c->indexType));put32(geometry,totalIndices);put32(geometry,uint32_t(indexData.size()));}
            put32(geometry,c->pipeline->stride);put32(geometry,uint32_t(c->pipeline->attributes.size()));put32(geometry,vertices);put32(geometry,uint32_t(size));
            for(const auto& a:c->pipeline->attributes){put32(geometry,a.location);put32(geometry,uint32_t(a.format));put32(geometry,a.offset);}
            const auto* data=v->memory->bytes.data()+v->offset+c->vertexOffset;geometry.insert(geometry.end(),data,data+size);
            geometry.insert(geometry.end(),indexData.begin(),indexData.end());
        }
        const uint8_t* push=indexed?draws[0].push.data():c->push;
        std::vector<uint8_t> payload;
        auto putFloat=[&](float f){uint32_t value;std::memcpy(&value,&f,4);put32(payload,value);};
        put32(payload,c->frame->image->w);put32(payload,c->frame->image->h);
        put32(payload,uint32_t(c->pipeline->vs->code.size()));put32(payload,uint32_t(c->pipeline->fs->code.size()));
        if(scene){
            put32(payload,c->pushStages);put32(payload,c->pushBytes);put32(payload,c->pipeline->cull);put32(payload,uint32_t(c->pipeline->front));
            put32(payload,c->frame->depth?uint32_t(c->frame->depth->format):0);put32(payload,uint32_t(draws.size()));put32(payload,uint32_t(geometry.size()));
            for(float f:c->clear)putFloat(f);putFloat(c->clearDepth);
        }else{
            if(extended){put32(payload,c->pushStages);put32(payload,c->pushBytes);}
            else payload.insert(payload.end(),push,push+16);
            for(float f:c->clear)putFloat(f);
            if(extended)payload.insert(payload.end(),push,push+c->pushBytes);
        }
        payload.insert(payload.end(),c->pipeline->vs->code.begin(),c->pipeline->vs->code.end());payload.insert(payload.end(),c->pipeline->fs->code.begin(),c->pipeline->fs->code.end());
        payload.insert(payload.end(),geometry.begin(),geometry.end());
        if(scene)for(const auto& draw:draws){put32(payload,draw.firstIndex);put32(payload,draw.indexCount);payload.insert(payload.end(),draw.push.begin(),draw.push.end());}
        auto* wire=d->physical->instance->wire.get();std::vector<uint8_t> result;
        if(scene){
            if(uniform){std::vector<uint8_t> combined;put32(combined,uint32_t(uniformData.size()));combined.insert(combined.end(),uniformData.begin(),uniformData.end());combined.insert(combined.end(),payload.begin(),payload.end());payload=std::move(combined);}
            bool sampled=false;
#ifdef MAVK_EXPERIMENTAL_SAMPLED
            sampled=c->pipeline->pushLayout.sampled;
            if(sampled){uint64_t texture{};auto status=sampledRemote(c,d,texture);if(status!=VK_SUCCESS)return status;std::vector<uint8_t> combined;putId(combined,texture);combined.insert(combined.end(),payload.begin(),payload.end());payload=std::move(combined);}
#endif
            auto descriptor=wire->exchange(sampled?23:uniform?17:14,payload);
            if(descriptor.size()!=16||get32(descriptor.data()+8)!=bytes||get32(descriptor.data()+12)!=c->frame->image->w*4)return VK_ERROR_DEVICE_LOST;
            std::vector<uint8_t> id(descriptor.begin(),descriptor.begin()+8);
            try{
                result.resize(size_t(bytes));
                for(uint32_t offset=0;offset<bytes;){
                    uint32_t amount=uint32_t(std::min<uint64_t>(1048576,bytes-offset));
                    auto request=id;put32(request,offset);put32(request,amount);auto chunk=wire->exchange(15,request);
                    if(chunk.size()!=amount)throw std::runtime_error("Snapshot chunk size");
                    std::memcpy(result.data()+offset,chunk.data(),amount);offset+=amount;
                }
            }catch(...){try{wire->exchange(16,id);}catch(...){}throw;}
            wire->exchange(16,id);
        }else result=wire->exchange(extended?12:indexed?11:c->pipeline->stride?10:8,payload);
        if(result.size()!=bytes)return VK_ERROR_DEVICE_LOST;
        auto* image=c->frame->image;std::memcpy(image->memory->bytes.data(),result.data(),result.size());image->initialized=true;image->layout=c->renderFinal;
        if(b)std::memcpy(b->memory->bytes.data()+b->offset+c->offset,result.data(),result.size());
        return VK_SUCCESS;
        };
        auto result=execute();if(result!=VK_SUCCESS)return result;
#ifdef MAVK_EXPERIMENTAL_WIN32_WSI
        for(uint32_t j=0;j<s.waitSemaphoreCount;j++)as<BinarySemaphore>(s.pWaitSemaphores[j])->signaled=false;
        for(uint32_t j=0;j<s.signalSemaphoreCount;j++)as<BinarySemaphore>(s.pSignalSemaphores[j])->signaled=true;
#endif
    }
    if(fence)as<Fence>(fence)->signaled=true;return VK_SUCCESS;
    }catch(...){return VK_ERROR_DEVICE_LOST;}
}
#ifdef MAVK_EXPERIMENTAL_WIN32_WSI
#include "wsi-impl.hpp"
#endif
API PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance,const char*);
API PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice,const char*);
API VkResult VKAPI_CALL vk_icdNegotiateLoaderICDInterfaceVersion(uint32_t* version){if(!version||*version<4)return VK_ERROR_INCOMPATIBLE_DRIVER;*version=std::min(*version,5u);return VK_SUCCESS;}
namespace {
PFN_vkVoidFunction lookup(const char* name){
#define ENTRY(f) if(!std::strcmp(name,#f))return reinterpret_cast<PFN_vkVoidFunction>(f)
#ifdef MAVK_EXPERIMENTAL_SAMPLED
    ENTRY(vkCreateSampler);ENTRY(vkDestroySampler);ENTRY(vkCmdCopyBufferToImage);
#endif
#ifdef MAVK_EXPERIMENTAL_WIN32_WSI
    ENTRY(vkCreateWin32SurfaceKHR);ENTRY(vkDestroySurfaceKHR);ENTRY(vkGetPhysicalDeviceWin32PresentationSupportKHR);
    ENTRY(vkGetPhysicalDeviceSurfaceSupportKHR);ENTRY(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    ENTRY(vkGetPhysicalDeviceSurfaceFormatsKHR);ENTRY(vkGetPhysicalDeviceSurfacePresentModesKHR);
    ENTRY(vkCreateSwapchainKHR);ENTRY(vkDestroySwapchainKHR);ENTRY(vkGetSwapchainImagesKHR);
    ENTRY(vkAcquireNextImageKHR);ENTRY(vkQueuePresentKHR);ENTRY(vkCreateSemaphore);ENTRY(vkDestroySemaphore);
#endif
    ENTRY(vkGetPhysicalDeviceSparseImageFormatProperties);ENTRY(vkCmdBindVertexBuffers);ENTRY(vkCmdBindIndexBuffer);ENTRY(vkCmdDrawIndexed);ENTRY(vkCmdCopyBuffer);
    ENTRY(vkCmdCopyImage);ENTRY(vkGetImageSubresourceLayout);ENTRY(vkResetCommandBuffer);ENTRY(vkCreateComputePipelines);ENTRY(vkCmdDispatch);
    ENTRY(vkCreateDescriptorPool);ENTRY(vkDestroyDescriptorPool);ENTRY(vkResetDescriptorPool);ENTRY(vkAllocateDescriptorSets);ENTRY(vkFreeDescriptorSets);ENTRY(vkUpdateDescriptorSets);ENTRY(vkCmdBindDescriptorSets);
    ENTRY(vkCreateDescriptorSetLayout);ENTRY(vkDestroyDescriptorSetLayout);ENTRY(vkCreatePipelineCache);ENTRY(vkDestroyPipelineCache);
    ENTRY(vkCreateInstance);ENTRY(vkDestroyInstance);ENTRY(vkEnumeratePhysicalDevices);ENTRY(vkGetPhysicalDeviceFeatures);ENTRY(vkGetPhysicalDeviceProperties);ENTRY(vkGetPhysicalDeviceQueueFamilyProperties);ENTRY(vkGetPhysicalDeviceMemoryProperties);ENTRY(vkGetPhysicalDeviceFormatProperties);ENTRY(vkGetPhysicalDeviceImageFormatProperties);ENTRY(vkEnumerateInstanceExtensionProperties);ENTRY(vkEnumerateDeviceExtensionProperties);ENTRY(vkCreateDevice);ENTRY(vkDestroyDevice);ENTRY(vkGetDeviceQueue);ENTRY(vkDeviceWaitIdle);ENTRY(vkQueueWaitIdle);ENTRY(vkAllocateMemory);ENTRY(vkFreeMemory);ENTRY(vkMapMemory);ENTRY(vkUnmapMemory);ENTRY(vkFlushMappedMemoryRanges);ENTRY(vkInvalidateMappedMemoryRanges);ENTRY(vkCreateBuffer);ENTRY(vkDestroyBuffer);ENTRY(vkGetBufferMemoryRequirements);ENTRY(vkBindBufferMemory);ENTRY(vkCreateImage);ENTRY(vkDestroyImage);ENTRY(vkGetImageMemoryRequirements);ENTRY(vkBindImageMemory);ENTRY(vkCreateImageView);ENTRY(vkDestroyImageView);ENTRY(vkCreateShaderModule);ENTRY(vkDestroyShaderModule);ENTRY(vkCreatePipelineLayout);ENTRY(vkDestroyPipelineLayout);ENTRY(vkCreateRenderPass);ENTRY(vkDestroyRenderPass);ENTRY(vkCreateFramebuffer);ENTRY(vkDestroyFramebuffer);ENTRY(vkCreateGraphicsPipelines);ENTRY(vkDestroyPipeline);ENTRY(vkCreateCommandPool);ENTRY(vkDestroyCommandPool);ENTRY(vkAllocateCommandBuffers);ENTRY(vkFreeCommandBuffers);ENTRY(vkBeginCommandBuffer);ENTRY(vkEndCommandBuffer);ENTRY(vkCmdBeginRenderPass);ENTRY(vkCmdEndRenderPass);ENTRY(vkCmdBindPipeline);ENTRY(vkCmdSetViewport);ENTRY(vkCmdSetScissor);ENTRY(vkCmdPushConstants);ENTRY(vkCmdDraw);ENTRY(vkCmdPipelineBarrier);ENTRY(vkCmdCopyImageToBuffer);ENTRY(vkCreateFence);ENTRY(vkDestroyFence);ENTRY(vkGetFenceStatus);ENTRY(vkResetFences);ENTRY(vkWaitForFences);ENTRY(vkQueueSubmit);ENTRY(vkGetInstanceProcAddr);ENTRY(vkGetDeviceProcAddr);ENTRY(vk_icdNegotiateLoaderICDInterfaceVersion);
#undef ENTRY
    return nullptr;
}
}
API PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance,const char* name){return name?lookup(name):nullptr;}
API PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice,const char* name){return name?lookup(name):nullptr;}
API PFN_vkVoidFunction VKAPI_CALL vk_icdGetInstanceProcAddr(VkInstance i,const char* name){return vkGetInstanceProcAddr(i,name);}
API PFN_vkVoidFunction VKAPI_CALL vk_icdGetPhysicalDeviceProcAddr(VkInstance i,const char* name){return vkGetInstanceProcAddr(i,name);}
