// SPDX-License-Identifier: Apache-2.0
// Standard loader integration,tracking default/opt-in guarded execution.
// No emulated feature advertising until complete semantics are implemented.
#include <vulkan/vk_layer.h>
#include <mavk/compat/vulkan_descriptor_updates.hpp>
#include <mavk/compat/shader_guards.hpp>
#include <mavk/compat/null_buffers.hpp>
#include <mavk/compat/null_texels.hpp>
#include <mavk/compat/null_images.hpp>
#include <mavk/compat/shader_descriptors.hpp>
#include <mavk/compat/cull_distance.hpp>
#include <cstdio>
#include <cstring>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>
#include <array>
#include <cstdlib>
#include <chrono>
#include "shader_stage_chain.hpp"
#include "descriptor_layout_shape.hpp"
#include "uniform_metadata.hpp"

#define MAVK_EXPORT extern "C" __attribute__((visibility("default")))
MAVK_EXPORT PFN_vkVoidFunction VKAPI_CALL mavkGetInstanceProcAddr(VkInstance,const char*);
MAVK_EXPORT PFN_vkVoidFunction VKAPI_CALL mavkGetDeviceProcAddr(VkDevice,const char*);
MAVK_EXPORT PFN_vkVoidFunction VKAPI_CALL mavkGetPhysicalDeviceProcAddr(VkInstance,const char*);

namespace {
using namespace mavk::compat;
struct Instance {
    VkInstance handle{};PFN_vkGetInstanceProcAddr gipa{};PFN_GetPhysicalDeviceProcAddr gpdpa{};
    uint32_t apiVersion=VK_API_VERSION_1_0;
    template<class T>T fn(const char* name)const{return reinterpret_cast<T>(gipa(handle,name));}
};
// Real native ownership. Custom allocation callbacks are currently passed through
// without shadow ownership, because delaying their lifetime would require a
// stronger allocator contract than Vulkan gives the layer.
struct BufferOwner {
    VkDevice device{};VkBuffer buffer{};PFN_vkDestroyBuffer destroy{};bool owns=false;
    ~BufferOwner(){if(owns)destroy(device,buffer,nullptr);}
};
template<class Handle,class Destroy>struct NativeOwner {
    VkDevice device{};Handle handle{};Destroy destroy{};bool owns=false;
    std::shared_ptr<const void> backing;
    ~NativeOwner(){if(owns)destroy(device,handle,nullptr);}
};
using ImageOwner=NativeOwner<VkImage,PFN_vkDestroyImage>;
using ViewOwner=NativeOwner<VkImageView,PFN_vkDestroyImageView>;
using SamplerOwner=NativeOwner<VkSampler,PFN_vkDestroySampler>;
using TexelOwner=NativeOwner<VkBufferView,PFN_vkDestroyBufferView>;
using SetLayoutOwner=NativeOwner<VkDescriptorSetLayout,PFN_vkDestroyDescriptorSetLayout>;
using PipelineLayoutOwner=NativeOwner<VkPipelineLayout,PFN_vkDestroyPipelineLayout>;
using PipelineOwner=NativeOwner<VkPipeline,PFN_vkDestroyPipeline>;
using ShaderOwner=NativeOwner<VkShaderModule,PFN_vkDestroyShaderModule>;
struct Image {std::shared_ptr<ImageOwner> owner;VkImageUsageFlags usage{};};
struct View {std::shared_ptr<ViewOwner> owner;VkImage image{};bool registered=false;};
struct Layout {std::vector<VkDescriptorSetLayoutBinding> bindings;std::shared_ptr<SetLayoutOwner> owner;bool trackable=false,nativeSampler=false,deferred=false;std::shared_ptr<const ImmutableSamplerBindings> immutable;};
bool emptyLayout(const Layout& layout){return std::none_of(layout.bindings.begin(),layout.bindings.end(),[](const auto& b){return b.descriptorCount!=0;});}
struct Set {VkDescriptorPool pool{};bool tracked=false,nativeSampler=false,deferred=false,nativeReady=false,nativeSealed=false;};
struct UpdateTemplate {std::unique_ptr<VulkanDescriptorUpdates::UpdateTemplate> shadow;};
struct PipelineSchema {
    std::vector<Layout> sets;std::vector<VkPushConstantRange> push;
    std::shared_ptr<SetLayoutOwner> guard;std::shared_ptr<PipelineLayoutOwner> owner;
    bool uniformMetadata=false;
};
uint64_t metadataWordCount(const PipelineSchema& schema,const std::set<std::pair<uint32_t,uint32_t>>* selected=nullptr){
    uint32_t sets=0;
    auto included=[&](uint32_t set,const VkDescriptorSetLayoutBinding& b){return b.descriptorCount&&!schema.sets[set].nativeSampler&&(!selected||selected->count({set,b.binding}));};
    for(uint32_t set=0;set<schema.sets.size();set++)for(const auto& b:schema.sets[set].bindings)if(included(set,b))sets=set+1;
    if(!sets)return 0;uint64_t words=1+3*sets;
    for(uint32_t set=0;set<sets;set++){
        uint64_t span=1,entries=0,formats=0;
        for(const auto& b:schema.sets[set].bindings)if(included(set,b)){
            span=std::max(span,uint64_t(b.binding)+1);entries+=b.descriptorCount;
            if(b.descriptorType==VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER||b.descriptorType==VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)formats+=b.descriptorCount;
        }
        words+=2*span+entries+(formats?span+formats:0);
    }
    return words;
}
struct PipelineShadow {
    std::shared_ptr<const PipelineSchema> schema;std::vector<std::shared_ptr<const GuardCompiler::Words>> shaders;
    std::shared_ptr<PipelineOwner> owner;std::shared_ptr<NativeDescriptorAdapter> descriptors;size_t bytes=0;
    TexelShaderFormats texelFormats;
    ColorImageShaderTypes imageFormats;
    ImageShaderTypes imageTypes;
    std::array<bool,7> requiredSets{};
    std::set<std::pair<uint32_t,uint32_t>> requiredBindings;
    std::vector<std::pair<uint32_t,uint32_t>> resourceBindings;
};
struct SetBinding {VkDescriptorSet set{};std::shared_ptr<const PipelineSchema> schema;std::vector<uint32_t> dynamicOffsets;};
struct Bound {std::shared_ptr<PipelineShadow> pipeline;std::array<SetBinding,7> sets;};
struct Command {
    VkCommandPool pool{};bool failed=false,recording=false,secondary=false;
    std::array<Bound,2> bound;
    std::vector<std::shared_ptr<PipelineShadow>> pipelines;
    std::vector<std::shared_ptr<NativeDescriptorSnapshot>> generations;
};
struct ImagePrelude {
    NullImage2DResources::Initialization initialization;
    std::unique_ptr<mavk::CommandBatch> commands;
    explicit ImagePrelude(NullImage2DResources::Initialization init):initialization(std::move(init)){}
};
struct QueueInfo {uint32_t family=0;VkDeviceQueueCreateFlags flags=0;};
#include "gpu_profile.hpp"
struct Device {
    VkDevice handle{};std::shared_ptr<Instance> instance;PFN_vkGetDeviceProcAddr gdpa{};
    std::mutex mutex;std::unique_ptr<mavk::DeviceContext> core;
    std::unique_ptr<NullBufferResources> nullBuffers;
    std::unique_ptr<NullTexelResources> nullTexels;
    std::unique_ptr<NullImage2DResources> nullImages;
    std::vector<std::unique_ptr<ImagePrelude>> imagePreludes;
    std::map<VkQueue,QueueInfo> queueInfos;VkQueue imageQueue{};
    bool testSubmitRejected=false;
    VulkanDescriptorResources resources;std::unique_ptr<VulkanDescriptorUpdates> updates;
    std::shared_ptr<DescriptorMetadataArena> metadataArena;
    std::map<VkBuffer,std::shared_ptr<BufferOwner>> buffers;
    std::map<VkImage,Image> images;std::map<VkImageView,View> views;
    std::map<VkSampler,std::shared_ptr<SamplerOwner>> samplers;
    std::map<VkBufferView,std::shared_ptr<TexelOwner>> texels;
    std::map<VkDescriptorUpdateTemplate,UpdateTemplate> templates;
    DescriptorLimits limits;
    std::map<VkShaderModule,std::shared_ptr<const GuardCompiler::Words>> shaders;
    std::map<VkPipelineLayout,std::shared_ptr<const PipelineSchema>> pipelineLayouts;
    std::map<VkPipeline,std::shared_ptr<PipelineShadow>> pipelines;
    std::map<VkCommandBuffer,Command> commands;
    bool guarded=false,fragmentStores=false,vertexStores=false,guardReports=true,profile=false,trace=false;uint64_t guardedDraws=0,submissions=0,diagnosticWords=0,diagnosticReads=0;
    uint64_t profileGuardNs=0,profileWaitNs=0,profilePresentNs=0,profileGuardCalls=0,profileWaitCalls=0,profilePresents=0;
    uint64_t profileSemaphoreNs=0,profileSemaphoreCalls=0,profileSubmitNs=0,profileSubmitCalls=0;
    uint64_t profileSnapshotNs=0,profileMaterializeNs=0,profileMaterializations=0,profileMetadataBytes=0,profileNativeBinds=0,profileDiagnosticAllocations=0,profileArenaAllocations=0,profileNativePublic=0;
    std::chrono::steady_clock::time_point profileSince=std::chrono::steady_clock::now();
    bool gpuProfile=false;std::vector<VkQueueFamilyProperties> gpuFamilies;
    std::vector<std::unique_ptr<GpuSample>> gpuSamples;
    std::chrono::steady_clock::time_point gpuNext=std::chrono::steady_clock::now();
    std::map<uint32_t,std::unique_ptr<GuardCompiler>> compilers;
    size_t shaderBytes=0,pipelineBytes=0;
    std::map<VkDescriptorSetLayout,Layout> layouts;
    std::map<VkDescriptorSet,Set> sets;
    uint64_t commonWrites=0,commonCopies=0,commonTemplates=0,commonPipelines=0,gaps=0;
    bool testTexelMetadata=false;uint32_t testTexelMetadataRecords=0;
    bool cullPipelines=false,nativeGeometry=false,nativeTessellation=false,nativeMultiview=false;
    bool uniformBufferStandardLayout=false;
    bool metadataReadOnly=false,shareQuietLog=false,useMetadataArena=false,useUniformMetadata=false,useNativePublic=false;
    bool optimizeMetadataReads=false;
    size_t shaderCacheEntries=4,shaderCacheBytes=2*1024*1024;
    bool shaderCacheProfile=false;uint64_t shaderCalls=0,shaderCompiled=0,shaderHits=0,shaderNs=0;
    template<class T>T fn(const char* name)const{return reinterpret_cast<T>(gdpa(handle,name));}
    void gap(const char* operation,const char* reason)noexcept{
        ++gaps;
        if(gaps<=32)std::fprintf(stderr,"MAVK_LAYER_GAP operation=%s reason=%s\n",operation,reason);
        else if(gaps==33)std::fprintf(stderr,"MAVK_LAYER_GAP further messages suppressed; total retained in device summary\n");
    }
    void retireSet(VkDescriptorSet set){auto at=sets.find(set);if(at==sets.end())return;if(at->second.tracked)updates->unregisterSet(set);sets.erase(at);}
    void invalidateSet(VkDescriptorSet set){auto at=sets.find(set);if(at!=sets.end()&&at->second.tracked){updates->unregisterSet(set);at->second.tracked=false;}}
    void retirePool(VkDescriptorPool pool){for(auto at=sets.begin();at!=sets.end();){if(at->second.pool==pool){auto h=at++->first;retireSet(h);}else ++at;}}
    void retireCommand(Command& command){
        // Logs are writable. A shared generation may still be used by another
        // pending command buffer;read only when retiring its last recorded owner.
        if(guardReports)for(auto& generation:command.generations)if(generation.use_count()==1){auto words=generation->readDiagnostics()[0];++diagnosticReads;diagnosticWords+=words;std::fprintf(stderr,"MAVK_LAYER_GUARD_LOG words=%u\n",words);}
        command.bound={};command.pipelines.clear();command.generations.clear();command.failed=false;command.recording=false;
    }
    void clear(){gpuSamples.clear();commands.clear();imagePreludes.clear();pipelines.clear();pipelineLayouts.clear();shaders.clear();compilers.clear();sets.clear();updates.reset();templates.clear();resources=VulkanDescriptorResources{};views.clear();texels.clear();samplers.clear();images.clear();buffers.clear();layouts.clear();nullImages.reset();nullTexels.reset();nullBuffers.reset();metadataArena.reset();if(core)std::fprintf(stderr,"MAVK_LAYER_CORE_END liveZero=%d\n",core->counters().empty());core.reset();}
};
std::mutex registryMutex;
std::map<VkInstance,std::shared_ptr<Instance>> instances;
std::map<VkPhysicalDevice,std::shared_ptr<Instance>> physicals;
std::map<VkDevice,std::shared_ptr<Device>> devices;
std::map<VkCommandBuffer,std::weak_ptr<Device>> commandDevices;
std::map<VkQueue,std::weak_ptr<Device>> queueDevices;
std::shared_ptr<Instance> instance(VkInstance h){std::lock_guard lock(registryMutex);auto at=instances.find(h);return at==instances.end()?nullptr:at->second;}
std::shared_ptr<Instance> physical(VkPhysicalDevice h){std::lock_guard lock(registryMutex);auto at=physicals.find(h);return at==physicals.end()?nullptr:at->second;}
std::shared_ptr<Device> device(VkDevice h){std::lock_guard lock(registryMutex);auto at=devices.find(h);return at==devices.end()?nullptr:at->second;}
std::shared_ptr<Device> commandDevice(VkCommandBuffer h){std::lock_guard lock(registryMutex);auto at=commandDevices.find(h);return at==commandDevices.end()?nullptr:at->second.lock();}
std::shared_ptr<Device> queueDevice(VkQueue h){std::lock_guard lock(registryMutex);auto at=queueDevices.find(h);return at==queueDevices.end()?nullptr:at->second.lock();}

template<class T>T* linkInfo(const void* next,VkStructureType type){
    for(auto p=static_cast<const VkBaseInStructure*>(next);p;p=p->pNext)
        if(p->sType==type){auto info=reinterpret_cast<const T*>(p);if(info->function==VK_LAYER_LINK_INFO)return const_cast<T*>(info);}
    return nullptr;
}
VKAPI_ATTR VkResult VKAPI_CALL createInstance(const VkInstanceCreateInfo* ci,const VkAllocationCallbacks* alloc,VkInstance* out){
    auto link=linkInfo<VkLayerInstanceCreateInfo>(ci->pNext,VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO);
    if(!link||!link->u.pLayerInfo)return VK_ERROR_INITIALIZATION_FAILED;
    auto gipa=link->u.pLayerInfo->pfnNextGetInstanceProcAddr;auto gpdpa=link->u.pLayerInfo->pfnNextGetPhysicalDeviceProcAddr;
    auto create=reinterpret_cast<PFN_vkCreateInstance>(gipa(VK_NULL_HANDLE,"vkCreateInstance"));
    link->u.pLayerInfo=link->u.pLayerInfo->pNext;
    VkResult result=create(ci,alloc,out);if(result!=VK_SUCCESS)return result;
    try{auto state=std::make_shared<Instance>();state->handle=*out;state->gipa=gipa;state->gpdpa=gpdpa;
        if(ci->pApplicationInfo&&ci->pApplicationInfo->apiVersion)state->apiVersion=ci->pApplicationInfo->apiVersion;
        std::lock_guard lock(registryMutex);instances.emplace(*out,std::move(state));
    }catch(...){reinterpret_cast<PFN_vkDestroyInstance>(gipa(*out,"vkDestroyInstance"))(*out,alloc);*out=VK_NULL_HANDLE;return VK_ERROR_OUT_OF_HOST_MEMORY;}
    std::fprintf(stderr,"MAVK_LAYER_INSTANCE mode=tracking nullDescriptor=not-advertised\n");return result;
}
VKAPI_ATTR void VKAPI_CALL destroyInstance(VkInstance h,const VkAllocationCallbacks* alloc){
    if(!h)return;auto state=instance(h);if(!state)return;
    {std::lock_guard lock(registryMutex);for(auto at=physicals.begin();at!=physicals.end();)if(at->second==state)at=physicals.erase(at);else ++at;instances.erase(h);}
    state->fn<PFN_vkDestroyInstance>("vkDestroyInstance")(h,alloc);
}
VKAPI_ATTR VkResult VKAPI_CALL enumeratePhysicalDevices(VkInstance h,uint32_t* count,VkPhysicalDevice* out){
    auto state=instance(h);if(!state)return VK_ERROR_INITIALIZATION_FAILED;
    auto result=state->fn<PFN_vkEnumeratePhysicalDevices>("vkEnumeratePhysicalDevices")(h,count,out);
    if(out&&(result==VK_SUCCESS||result==VK_INCOMPLETE))try{std::lock_guard lock(registryMutex);for(uint32_t i=0;i<*count;i++)physicals[out[i]]=state;}catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}
    return result;
}
VkResult groups(VkInstance h,uint32_t* count,VkPhysicalDeviceGroupProperties* out,const char* name){
    auto state=instance(h);if(!state)return VK_ERROR_INITIALIZATION_FAILED;
    auto fn=state->fn<PFN_vkEnumeratePhysicalDeviceGroups>(name);if(!fn)return VK_ERROR_EXTENSION_NOT_PRESENT;
    auto result=fn(h,count,out);
    if(out&&(result==VK_SUCCESS||result==VK_INCOMPLETE))try{std::lock_guard lock(registryMutex);for(uint32_t i=0;i<*count;i++)for(uint32_t n=0;n<out[i].physicalDeviceCount;n++)physicals[out[i].physicalDevices[n]]=state;}catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}
    return result;
}
VKAPI_ATTR VkResult VKAPI_CALL enumerateGroups(VkInstance h,uint32_t* n,VkPhysicalDeviceGroupProperties* p){return groups(h,n,p,"vkEnumeratePhysicalDeviceGroups");}
VKAPI_ATTR VkResult VKAPI_CALL enumerateGroupsKHR(VkInstance h,uint32_t* n,VkPhysicalDeviceGroupProperties* p){return groups(h,n,p,"vkEnumeratePhysicalDeviceGroupsKHR");}

VKAPI_ATTR VkResult VKAPI_CALL createDevice(VkPhysicalDevice gpu,const VkDeviceCreateInfo* ci,const VkAllocationCallbacks* alloc,VkDevice* out){
    auto parent=physical(gpu);if(!parent){std::fprintf(stderr,"MAVK_LAYER_CREATE_DEVICE_FAIL stage=physicalRegistry gpu=%p\n",(void*)gpu);return VK_ERROR_INITIALIZATION_FAILED;}
    auto link=linkInfo<VkLayerDeviceCreateInfo>(ci->pNext,VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO);
    if(!link||!link->u.pLayerInfo){std::fprintf(stderr,"MAVK_LAYER_CREATE_DEVICE_FAIL stage=loaderLink\n");return VK_ERROR_INITIALIZATION_FAILED;}
    auto gipa=link->u.pLayerInfo->pfnNextGetInstanceProcAddr;auto gdpa=link->u.pLayerInfo->pfnNextGetDeviceProcAddr;
    auto create=reinterpret_cast<PFN_vkCreateDevice>(gipa(parent->handle,"vkCreateDevice"));link->u.pLayerInfo=link->u.pLayerInfo->pNext;
    auto result=create(gpu,ci,alloc,out);if(result!=VK_SUCCESS){std::fprintf(stderr,"MAVK_LAYER_CREATE_DEVICE_FAIL stage=downstream result=%d\n",result);return result;}
    try{auto state=std::make_shared<Device>();state->handle=*out;state->instance=parent;state->gdpa=gdpa;
        const auto mode=std::getenv("MAVK_LAYER_GUARDS");state->guarded=mode&&std::strcmp(mode,"1")==0;
        const auto reports=std::getenv("MAVK_GUARD_REPORTS");state->guardReports=!(reports&&std::strcmp(reports,"0")==0);
        const auto profile=std::getenv("MAVK_LAYER_PROFILE");state->profile=profile&&std::strcmp(profile,"1")==0;
        const auto quietLog=std::getenv("MAVK_SHARED_DIAGNOSTICS");state->shareQuietLog=quietLog&&std::strcmp(quietLog,"1")==0;
        if(state->shareQuietLog)std::fprintf(stderr,"MAVK_DIAGNOSTIC_STORAGE sharedReadOnly=%u\n",unsigned(!state->guardReports));
        const auto arena=std::getenv("MAVK_DESCRIPTOR_ARENA");state->useMetadataArena=arena&&std::strcmp(arena,"1")==0;
        const auto uniform=std::getenv("MAVK_UNIFORM_GUARD_METADATA");state->useUniformMetadata=uniform&&std::strcmp(uniform,"1")==0;
        const auto nativePublic=std::getenv("MAVK_NATIVE_BOUND_SETS");state->useNativePublic=nativePublic&&std::strcmp(nativePublic,"1")==0;
        if(state->useUniformMetadata)std::fprintf(stderr,"MAVK_METADATA_UNIFORM requested=1 std140=1\n");
        const auto trace=std::getenv("MAVK_LAYER_TRACE");state->trace=trace&&std::strcmp(trace,"1")==0;
        const auto cull=std::getenv("MAVK_EXPERIMENTAL_CULL_PIPELINES");state->cullPipelines=cull&&std::strcmp(cull,"1")==0;
        std::fprintf(stderr,"MAVK_LAYER_TRACE_POLICY events=%u errors=always\n",unsigned(state->trace));
        const auto texelMetadata=std::getenv("MAVK_TEST_TEXEL_METADATA");state->testTexelMetadata=texelMetadata&&std::strcmp(texelMetadata,"1")==0;
        const auto readOnlyMetadata=std::getenv("MAVK_METADATA_READ_ONLY");state->metadataReadOnly=readOnlyMetadata&&std::strcmp(readOnlyMetadata,"1")==0;
        const auto optimizeMetadata=std::getenv("MAVK_GUARD_CSE");state->optimizeMetadataReads=optimizeMetadata&&std::strcmp(optimizeMetadata,"1")==0;
        state->metadataReadOnly|=state->optimizeMetadataReads;
        if(state->optimizeMetadataReads)std::fprintf(stderr,"MAVK_GUARD_OPTIMIZER experimental=1 inline=1 redundancy=1 guards=preserved\n");
        if(const auto cache=std::getenv("MAVK_SHADER_CACHE_ENTRIES")){
            char* end=nullptr;auto entries=std::strtoul(cache,&end,10);
            if(!end||*end||entries<1||entries>128)throw std::invalid_argument("shader cache entry budget");
            state->shaderCacheEntries=entries;state->shaderCacheBytes=entries>4?8*1024*1024:2*1024*1024;
        }
        const auto cacheProfile=std::getenv("MAVK_SHADER_CACHE_PROFILE");state->shaderCacheProfile=cacheProfile&&std::strcmp(cacheProfile,"1")==0;
        if(state->shaderCacheProfile)std::fprintf(stderr,"MAVK_SHADER_CACHE_POLICY entries=%zu bytesPerCompiler=%zu compilersMax=4\n",state->shaderCacheEntries,state->shaderCacheBytes);
        if(state->guarded)std::fprintf(stderr,"MAVK_LAYER_GUARD_POLICY checks=1 diagnosticReports=%u metadataReadOnly=%u\n",unsigned(state->guardReports),unsigned(state->metadataReadOnly));
        const VkPhysicalDeviceFeatures* enabled=ci->pEnabledFeatures;
        for(auto p=static_cast<const VkBaseInStructure*>(ci->pNext);p;p=p->pNext)if(p->sType==VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2)enabled=&reinterpret_cast<const VkPhysicalDeviceFeatures2*>(p)->features;
        if(enabled){state->fragmentStores=enabled->fragmentStoresAndAtomics;state->vertexStores=enabled->vertexPipelineStoresAndAtomics;}
        state->nativeGeometry=enabled&&enabled->geometryShader;
        state->nativeTessellation=enabled&&enabled->tessellationShader;
        for(auto p=static_cast<const VkBaseInStructure*>(ci->pNext);p;p=p->pNext){
            if(p->sType==VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES)state->uniformBufferStandardLayout|=reinterpret_cast<const VkPhysicalDeviceVulkan12Features*>(p)->uniformBufferStandardLayout;
            if(p->sType==VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFORM_BUFFER_STANDARD_LAYOUT_FEATURES)state->uniformBufferStandardLayout|=reinterpret_cast<const VkPhysicalDeviceUniformBufferStandardLayoutFeatures*>(p)->uniformBufferStandardLayout;
            if(p->sType==VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES)state->nativeMultiview|=reinterpret_cast<const VkPhysicalDeviceVulkan11Features*>(p)->multiview;
            if(p->sType==VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES)state->nativeMultiview|=reinterpret_cast<const VkPhysicalDeviceMultiviewFeatures*>(p)->multiview;
        }
        std::fprintf(stderr,"MAVK_LAYER_LAYOUT_POLICY uniformBufferStandardLayoutEnabled=%u\n",unsigned(state->uniformBufferStandardLayout));
        if(state->cullPipelines)std::fprintf(stderr,"MAVK_LAYER_CULL_POLICY experimental=1 publicFeature=0 nativeGeometryEnabled=%u\n",unsigned(state->nativeGeometry));
        auto dispatch=mavk::VulkanDispatch::next(parent->handle,gipa,*out,gdpa);
        state->core=std::make_unique<mavk::DeviceContext>(gpu,*out,ci->pQueueCreateInfos[0].queueFamilyIndex,dispatch);
        const auto gpuProfile=std::getenv("MAVK_GPU_PROBE");state->gpuProfile=gpuProfile&&std::strcmp(gpuProfile,"1")==0;
        if(state->gpuProfile){
            bool hostReset=false;
            for(auto p=static_cast<const VkBaseInStructure*>(ci->pNext);p;p=p->pNext){
                if(p->sType==VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES)hostReset|=reinterpret_cast<const VkPhysicalDeviceVulkan12Features*>(p)->hostQueryReset;
                if(p->sType==VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES)hostReset|=reinterpret_cast<const VkPhysicalDeviceHostQueryResetFeatures*>(p)->hostQueryReset;
            }
            if(!hostReset){state->gpuProfile=false;std::fprintf(stderr,"MAVK_GPU_PROBE_SKIP reason=hostQueryReset-not-enabled deviceFeatures=unchanged\n");}
        }
        if(state->gpuProfile){
            uint32_t count=0;auto families=parent->fn<PFN_vkGetPhysicalDeviceQueueFamilyProperties>("vkGetPhysicalDeviceQueueFamilyProperties");
            families(gpu,&count,nullptr);state->gpuFamilies.resize(count);families(gpu,&count,state->gpuFamilies.data());state->gpuFamilies.resize(count);
            long warmup=30000;if(const auto value=std::getenv("MAVK_GPU_PROBE_WARMUP_MS")){char* end=nullptr;const auto parsed=std::strtol(value,&end,10);if(end&&!*end&&parsed>=0&&parsed<=60000)warmup=parsed;}
            state->gpuNext=std::chrono::steady_clock::now()+std::chrono::milliseconds(warmup);
            std::fprintf(stderr,"MAVK_GPU_PROBE_POLICY samplesMax=64 intervalMs=100 warmupMs=%ld semaphoreWaitBatches=excluded hostResetBeforePoll=1 explicitCpuWaits=0\n",warmup);
        }
        if(state->guarded)state->nullBuffers=std::make_unique<NullBufferResources>(*state->core);
        if(state->guarded)state->nullTexels=std::make_unique<NullTexelResources>(*state->core);
        if(state->guarded)state->nullImages=std::make_unique<NullImage2DResources>(*state->core);
        // Bounded regression trigger only. Exercises the real queue prelude
        // without pretending the application enabled unsupported nullDescriptor.
        const auto testImages=std::getenv("MAVK_TEST_IMAGE_INIT");
        if(state->guarded&&testImages&&std::strcmp(testImages,"1")==0){
            DescriptorValue null;null.status=DescriptorStatus::Null;
            state->nullImages->resolve({0,0,1,DescriptorKind::SampledImage},null,VK_FORMAT_R8G8B8A8_UNORM);
            state->nullImages->resolve({0,1,1,DescriptorKind::StorageImage},null,VK_FORMAT_R32_UINT);
        }
        VkPhysicalDeviceProperties properties{};parent->fn<PFN_vkGetPhysicalDeviceProperties>("vkGetPhysicalDeviceProperties")(gpu,&properties);
        DescriptorLimits limits{properties.limits.minUniformBufferOffsetAlignment,properties.limits.minStorageBufferOffsetAlignment};
        state->limits=limits;
        // Private guarded profile only;public robustness feature queries stay
        // unchanged. Unsupported typed nulls still reject at materialization.
        state->updates=std::make_unique<VulkanDescriptorUpdates>(state->resources,limits,state->guarded);
        std::lock_guard lock(registryMutex);devices.emplace(*out,std::move(state));
    }catch(...){reinterpret_cast<PFN_vkDestroyDevice>(gdpa(*out,"vkDestroyDevice"))(*out,alloc);*out=VK_NULL_HANDLE;return VK_ERROR_OUT_OF_HOST_MEMORY;}
    std::fprintf(stderr,"MAVK_LAYER_DEVICE sharedCore=1 descriptorFrontend=1 mode=tracking\n");return result;
}
VKAPI_ATTR void VKAPI_CALL destroyDevice(VkDevice h,const VkAllocationCallbacks* alloc){
    if(!h)return;auto state=device(h);if(!state)return;
    {std::lock_guard lock(registryMutex);devices.erase(h);for(auto at=commandDevices.begin();at!=commandDevices.end();)if(at->second.lock()==state)at=commandDevices.erase(at);else ++at;for(auto at=queueDevices.begin();at!=queueDevices.end();)if(at->second.lock()==state)at=queueDevices.erase(at);else ++at;}
    {std::lock_guard lock(state->mutex);
        std::fprintf(stderr,"MAVK_LAYER_DEVICE_END commonWrites=%llu commonCopies=%llu gaps=%llu remainingSets=%zu remainingBuffers=%zu templates=%llu remainingImages=%zu remainingViews=%zu remainingSamplers=%zu remainingTexels=%zu remainingTemplates=%zu\n",
            (unsigned long long)state->commonWrites,(unsigned long long)state->commonCopies,(unsigned long long)state->gaps,state->sets.size(),state->buffers.size(),
            (unsigned long long)state->commonTemplates,state->images.size(),state->views.size(),state->samplers.size(),state->texels.size(),state->templates.size());
        std::fprintf(stderr,"MAVK_LAYER_PIPELINE_END pipelines=%llu remainingShaders=%zu remainingPipelineLayouts=%zu remainingPipelines=%zu shaderBytes=%zu pipelineBytes=%zu\n",
            (unsigned long long)state->commonPipelines,state->shaders.size(),state->pipelineLayouts.size(),state->pipelines.size(),state->shaderBytes,state->pipelineBytes);
        std::fprintf(stderr,"MAVK_LAYER_COMMAND_END guarded=%d draws=%llu submissions=%llu diagnosticWords=%llu remainingCommands=%zu diagnosticReads=%llu\n",state->guarded,(unsigned long long)state->guardedDraws,(unsigned long long)state->submissions,(unsigned long long)state->diagnosticWords,state->commands.size(),(unsigned long long)state->diagnosticReads);
        std::fprintf(stderr,"MAVK_LAYER_IMAGE_END retainedPreludes=%zu\n",state->imagePreludes.size());
        state->clear();}
    state->fn<PFN_vkDestroyDevice>("vkDestroyDevice")(h,alloc);
}
VKAPI_ATTR VkResult VKAPI_CALL createBuffer(VkDevice h,const VkBufferCreateInfo* ci,const VkAllocationCallbacks* alloc,VkBuffer* out){
    auto state=device(h);if(!state)return VK_ERROR_INITIALIZATION_FAILED;
    auto result=state->fn<PFN_vkCreateBuffer>("vkCreateBuffer")(h,ci,alloc,out);if(result!=VK_SUCCESS)return result;
    std::lock_guard lock(state->mutex);
    if(alloc){state->gap("vkCreateBuffer","custom allocator passed through");return result;}
    std::shared_ptr<BufferOwner> owner;
    try{owner=std::make_shared<BufferOwner>();owner->device=h;owner->buffer=*out;owner->destroy=state->fn<PFN_vkDestroyBuffer>("vkDestroyBuffer");
        state->resources.registerBuffer(*out,ci->size,descriptor_api::bufferUsage(*ci),owner);state->buffers.emplace(*out,owner);owner->owns=true;
    }catch(const std::exception& e){if(owner)owner->owns=false;state->buffers.erase(*out);try{state->resources.unregisterBuffer(*out);}catch(...){}state->gap("vkCreateBuffer",e.what());}
    return result;
}
VKAPI_ATTR void VKAPI_CALL destroyBuffer(VkDevice h,VkBuffer buffer,const VkAllocationCallbacks* alloc){
    if(!buffer)return;auto state=device(h);if(!state)return;std::lock_guard lock(state->mutex);
    auto at=state->buffers.find(buffer);if(at==state->buffers.end()){state->fn<PFN_vkDestroyBuffer>("vkDestroyBuffer")(h,buffer,alloc);return;}
    state->resources.unregisterBuffer(buffer);state->buffers.erase(at);
}
VKAPI_ATTR VkResult VKAPI_CALL createImage(VkDevice h,const VkImageCreateInfo* ci,const VkAllocationCallbacks* alloc,VkImage* out){
    auto state=device(h);if(!state)return VK_ERROR_INITIALIZATION_FAILED;
    auto result=state->fn<PFN_vkCreateImage>("vkCreateImage")(h,ci,alloc,out);if(result!=VK_SUCCESS)return result;
    std::lock_guard lock(state->mutex);if(alloc){state->gap("vkCreateImage","custom allocator passed through");return result;}
    std::shared_ptr<ImageOwner> owner;
    try{owner=std::make_shared<ImageOwner>();owner->device=h;owner->handle=*out;owner->destroy=state->fn<PFN_vkDestroyImage>("vkDestroyImage");
        state->images.emplace(*out,Image{owner,ci->usage});owner->owns=true;
    }catch(const std::exception& e){state->gap("vkCreateImage",e.what());}return result;
}
VKAPI_ATTR void VKAPI_CALL destroyImage(VkDevice h,VkImage image,const VkAllocationCallbacks* alloc){
    if(!image)return;auto state=device(h);if(!state)return;std::lock_guard lock(state->mutex);
    auto at=state->images.find(image);if(at==state->images.end()){state->fn<PFN_vkDestroyImage>("vkDestroyImage")(h,image,alloc);return;}
    for(auto& view:state->views)if(view.second.image==image&&view.second.registered){state->resources.unregisterImageView(view.first);view.second.registered=false;}
    state->images.erase(at);
}
VKAPI_ATTR VkResult VKAPI_CALL createImageView(VkDevice h,const VkImageViewCreateInfo* ci,const VkAllocationCallbacks* alloc,VkImageView* out){
    auto state=device(h);if(!state)return VK_ERROR_INITIALIZATION_FAILED;
    auto result=state->fn<PFN_vkCreateImageView>("vkCreateImageView")(h,ci,alloc,out);if(result!=VK_SUCCESS)return result;
    std::lock_guard lock(state->mutex);auto image=state->images.find(ci->image);
    VkImageUsageFlags usage=image==state->images.end()?0:image->second.usage;
    bool knownChain=!ci->pNext;
    if(ci->pNext){auto u=static_cast<const VkImageViewUsageCreateInfo*>(ci->pNext);
        knownChain=u->sType==VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO&&!u->pNext&&u->usage&&!(u->usage&~usage);
        if(knownChain)usage=u->usage;
    }
    if(alloc||!knownChain||image==state->images.end()){state->gap("vkCreateImageView","allocator/extended view/untracked image passed through");return result;}
    std::shared_ptr<ViewOwner> owner;
    try{owner=std::make_shared<ViewOwner>();owner->device=h;owner->handle=*out;owner->destroy=state->fn<PFN_vkDestroyImageView>("vkDestroyImageView");owner->backing=image->second.owner;
        state->resources.registerImageView(*out,usage,owner,ci->subresourceRange.aspectMask);state->views.emplace(*out,View{owner,ci->image,true});owner->owns=true;
    }catch(const std::exception& e){if(owner)owner->owns=false;state->views.erase(*out);try{state->resources.unregisterImageView(*out);}catch(...){}state->gap("vkCreateImageView",e.what());}return result;
}
VKAPI_ATTR void VKAPI_CALL destroyImageView(VkDevice h,VkImageView view,const VkAllocationCallbacks* alloc){
    if(!view)return;auto state=device(h);if(!state)return;std::lock_guard lock(state->mutex);auto at=state->views.find(view);
    if(at==state->views.end()){state->fn<PFN_vkDestroyImageView>("vkDestroyImageView")(h,view,alloc);return;}
    if(at->second.registered)state->resources.unregisterImageView(view);state->views.erase(at);
}
VKAPI_ATTR VkResult VKAPI_CALL createSampler(VkDevice h,const VkSamplerCreateInfo* ci,const VkAllocationCallbacks* alloc,VkSampler* out){
    auto state=device(h);if(!state)return VK_ERROR_INITIALIZATION_FAILED;
    auto result=state->fn<PFN_vkCreateSampler>("vkCreateSampler")(h,ci,alloc,out);if(result!=VK_SUCCESS)return result;
    std::lock_guard lock(state->mutex);if(alloc){state->gap("vkCreateSampler","custom allocator passed through");return result;}
    std::shared_ptr<SamplerOwner> owner;
    try{owner=std::make_shared<SamplerOwner>();owner->device=h;owner->handle=*out;owner->destroy=state->fn<PFN_vkDestroySampler>("vkDestroySampler");
        bool ycbcr=false;for(auto node=static_cast<const VkBaseInStructure*>(ci->pNext);node;node=node->pNext)if(node->sType==VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO)ycbcr=reinterpret_cast<const VkSamplerYcbcrConversionInfo*>(node)->conversion!=VK_NULL_HANDLE;
        state->resources.registerSampler(*out,owner,ycbcr);state->samplers.emplace(*out,owner);owner->owns=true;
    }catch(const std::exception& e){if(owner)owner->owns=false;state->samplers.erase(*out);try{state->resources.unregisterSampler(*out);}catch(...){}state->gap("vkCreateSampler",e.what());}return result;
}
VKAPI_ATTR void VKAPI_CALL destroySampler(VkDevice h,VkSampler sampler,const VkAllocationCallbacks* alloc){
    if(!sampler)return;auto state=device(h);if(!state)return;std::lock_guard lock(state->mutex);auto at=state->samplers.find(sampler);
    if(at==state->samplers.end()){state->fn<PFN_vkDestroySampler>("vkDestroySampler")(h,sampler,alloc);return;}
    state->resources.unregisterSampler(sampler);state->samplers.erase(at);
}
VKAPI_ATTR VkResult VKAPI_CALL createBufferView(VkDevice h,const VkBufferViewCreateInfo* ci,const VkAllocationCallbacks* alloc,VkBufferView* out){
    auto state=device(h);if(!state)return VK_ERROR_INITIALIZATION_FAILED;
    auto result=state->fn<PFN_vkCreateBufferView>("vkCreateBufferView")(h,ci,alloc,out);if(result!=VK_SUCCESS)return result;
    std::lock_guard lock(state->mutex);auto buffer=state->buffers.find(ci->buffer);
    if(alloc||buffer==state->buffers.end()){state->gap("vkCreateBufferView","allocator/untracked buffer passed through");return result;}
    std::shared_ptr<TexelOwner> owner;
    try{owner=std::make_shared<TexelOwner>();owner->device=h;owner->handle=*out;owner->destroy=state->fn<PFN_vkDestroyBufferView>("vkDestroyBufferView");owner->backing=buffer->second;
        const auto usage=descriptor_api::bufferViewUsage(*ci,state->resources.buffer(ci->buffer)->bufferUsage);
        state->resources.registerBufferView(*out,ci->buffer,ci->format,ci->offset,ci->range,owner,usage);state->texels.emplace(*out,owner);owner->owns=true;
        if(state->texels.size()<=4)std::fprintf(stderr,"MAVK_LAYER_TEXEL_VIEW native=1 format=%u usage=%llu elements=%llu\n",ci->format,(unsigned long long)usage,(unsigned long long)state->resources.texelView(*out,usage&VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT?DescriptorKind::UniformTexelBuffer:DescriptorKind::StorageTexelBuffer)->texelElements);
    }catch(const std::exception& e){if(owner)owner->owns=false;state->texels.erase(*out);try{state->resources.unregisterBufferView(*out);}catch(...){}char reason[256];std::snprintf(reason,sizeof(reason),"%s format=%u offset=%llu range=%llu",e.what(),unsigned(ci->format),(unsigned long long)ci->offset,(unsigned long long)ci->range);state->gap("vkCreateBufferView",reason);}return result;
}
VKAPI_ATTR void VKAPI_CALL destroyBufferView(VkDevice h,VkBufferView view,const VkAllocationCallbacks* alloc){
    if(!view)return;auto state=device(h);if(!state)return;std::lock_guard lock(state->mutex);auto at=state->texels.find(view);
    if(at==state->texels.end()){state->fn<PFN_vkDestroyBufferView>("vkDestroyBufferView")(h,view,alloc);return;}
    state->resources.unregisterBufferView(view);state->texels.erase(at);
}
VKAPI_ATTR VkResult VKAPI_CALL createLayout(VkDevice h,const VkDescriptorSetLayoutCreateInfo* ci,const VkAllocationCallbacks* alloc,VkDescriptorSetLayout* out){
    auto state=device(h);if(!state)return VK_ERROR_INITIALIZATION_FAILED;
    auto result=state->fn<PFN_vkCreateDescriptorSetLayout>("vkCreateDescriptorSetLayout")(h,ci,alloc,out);if(result!=VK_SUCCESS)return result;
    std::lock_guard lock(state->mutex);
    try{Layout layout;layout.trackable=ordinaryDescriptorLayout(*ci);
        if(layout.trackable){if(ci->bindingCount)layout.bindings.assign(ci->pBindings,ci->pBindings+ci->bindingCount);std::sort(layout.bindings.begin(),layout.bindings.end(),[](const auto& a,const auto& b){return a.binding<b.binding;});layout.immutable=ImmutableSamplerBindings::capture(state->resources,ci->bindingCount,ci->pBindings);layout.immutable->ownPointers(layout.bindings);}
        // Owning copies only; caller's pImmutableSamplers is never retained.
        // Sarek's live sampler heap is non-nullable and natively supported.
        // Keep update-after-bind/partially-bound semantics in the driver, not
        // an immutable Mavk snapshot. Other admitted extended layouts must remain
        // statically unused; pipeline preparation rejects real shader access.
        if(!alloc&&ci->bindingCount==1&&ci->pBindings&&ci->pBindings[0].descriptorType==VK_DESCRIPTOR_TYPE_SAMPLER&&
            !ci->pBindings[0].pImmutableSamplers&&ci->pBindings[0].descriptorCount&&ci->pBindings[0].descriptorCount<=65536&&
            ci->flags==VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT&&ci->pNext){
            auto f=static_cast<const VkDescriptorSetLayoutBindingFlagsCreateInfo*>(ci->pNext);
            constexpr VkDescriptorBindingFlags allowed=VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT|VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT|VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
            layout.nativeSampler=f->sType==VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO&&!f->pNext&&f->bindingCount==1&&f->pBindingFlags&&f->pBindingFlags[0]==allowed;
        }
        layout.deferred=!layout.trackable&&!layout.nativeSampler&&!alloc&&deferredDescriptorLayout(*ci);
        if(state->guarded&&((!layout.trackable&&!layout.nativeSampler&&!layout.deferred)||alloc)){
            const auto extension=static_cast<const VkBaseInStructure*>(ci->pNext);
            throw std::invalid_argument("guarded descriptor layout/allocator unsupported flags="+std::to_string(ci->flags)+" bindings="+std::to_string(ci->bindingCount)+" sType="+std::to_string(extension?extension->sType:0)+" allocator="+std::to_string(alloc!=nullptr));
        }
        if(layout.nativeSampler)std::fprintf(stderr,"MAVK_LAYER_NATIVE_SAMPLER layout=1 updateAfterBind=preserved count=%u\n",ci->pBindings[0].descriptorCount);
        if(layout.nativeSampler||layout.deferred){if(ci->bindingCount)layout.bindings.assign(ci->pBindings,ci->pBindings+ci->bindingCount);std::sort(layout.bindings.begin(),layout.bindings.end(),[](const auto& a,const auto& b){return a.binding<b.binding;});}
        else if(!layout.trackable)layout.bindings.clear();
        if(!alloc){layout.owner=std::make_shared<SetLayoutOwner>();layout.owner->device=h;layout.owner->handle=*out;layout.owner->destroy=state->fn<PFN_vkDestroyDescriptorSetLayout>("vkDestroyDescriptorSetLayout");layout.owner->backing=layout.immutable;}
        auto owner=layout.owner;state->layouts.emplace(*out,std::move(layout));if(owner)owner->owns=true;
    }catch(const std::exception& e){state->gap("vkCreateDescriptorSetLayout",e.what());if(state->guarded){state->fn<PFN_vkDestroyDescriptorSetLayout>("vkDestroyDescriptorSetLayout")(h,*out,alloc);*out=VK_NULL_HANDLE;return VK_ERROR_FEATURE_NOT_PRESENT;}}return result;
}
VKAPI_ATTR void VKAPI_CALL destroyLayout(VkDevice h,VkDescriptorSetLayout layout,const VkAllocationCallbacks* alloc){
    auto state=device(h);if(!state)return;{std::lock_guard lock(state->mutex);auto at=state->layouts.find(layout);if(at!=state->layouts.end()&&at->second.owner){state->layouts.erase(at);return;}state->layouts.erase(layout);}state->fn<PFN_vkDestroyDescriptorSetLayout>("vkDestroyDescriptorSetLayout")(h,layout,alloc);
}
VKAPI_ATTR VkResult VKAPI_CALL allocateSets(VkDevice h,const VkDescriptorSetAllocateInfo* ci,VkDescriptorSet* out){
    auto state=device(h);if(!state)return VK_ERROR_INITIALIZATION_FAILED;
    auto result=state->fn<PFN_vkAllocateDescriptorSets>("vkAllocateDescriptorSets")(h,ci,out);if(result!=VK_SUCCESS)return result;
    std::lock_guard lock(state->mutex);
    for(uint32_t i=0;i<ci->descriptorSetCount;i++)try{
        auto item=state->sets.emplace(out[i],Set{ci->descriptorPool,false}).first;
        auto layout=state->layouts.find(ci->pSetLayouts[i]);
        if(!ci->pNext&&layout!=state->layouts.end()&&layout->second.nativeSampler){item->second.nativeSampler=true;continue;}
        if(!ci->pNext&&layout!=state->layouts.end()&&layout->second.deferred){item->second.deferred=true;continue;}
        if(ci->pNext||layout==state->layouts.end()||!layout->second.trackable){state->gap("vkAllocateDescriptorSets","opaque/extended layout");continue;}
        VkDescriptorSetLayoutCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};info.bindingCount=layout->second.bindings.size();info.pBindings=layout->second.bindings.data();
        state->updates->registerSet(out[i],info);item->second.tracked=true;item->second.nativeReady=state->useNativePublic;
    }catch(const std::exception& e){state->gap("vkAllocateDescriptorSets",e.what());}
    return result;
}
VKAPI_ATTR VkResult VKAPI_CALL freeSets(VkDevice h,VkDescriptorPool pool,uint32_t count,const VkDescriptorSet* sets){
    auto state=device(h);if(!state)return VK_ERROR_INITIALIZATION_FAILED;
    auto result=state->fn<PFN_vkFreeDescriptorSets>("vkFreeDescriptorSets")(h,pool,count,sets);if(result==VK_SUCCESS){std::lock_guard lock(state->mutex);for(uint32_t i=0;i<count;i++)state->retireSet(sets[i]);}return result;
}
VKAPI_ATTR VkResult VKAPI_CALL resetPool(VkDevice h,VkDescriptorPool pool,VkDescriptorPoolResetFlags flags){
    auto state=device(h);if(!state)return VK_ERROR_INITIALIZATION_FAILED;
    auto result=state->fn<PFN_vkResetDescriptorPool>("vkResetDescriptorPool")(h,pool,flags);if(result==VK_SUCCESS){std::lock_guard lock(state->mutex);state->retirePool(pool);}return result;
}
VKAPI_ATTR void VKAPI_CALL destroyPool(VkDevice h,VkDescriptorPool pool,const VkAllocationCallbacks* alloc){
    auto state=device(h);if(!state)return;{std::lock_guard lock(state->mutex);state->retirePool(pool);}state->fn<PFN_vkDestroyDescriptorPool>("vkDestroyDescriptorPool")(h,pool,alloc);
}
// Optional ordinary-set passthrough maintains native contents at the original
// application update point. Copies/templates/null/extended updates conservatively
// retire eligibility for that set; they retain the established snapshot path.
void mirrorOrdinaryWrites(Device& s,uint32_t wc,const VkWriteDescriptorSet* writes,uint32_t cc,const VkCopyDescriptorSet* copies){
    if(!s.useNativePublic)return;
    for(uint32_t i=0;i<cc;i++){auto at=s.sets.find(copies[i].dstSet);if(at!=s.sets.end())at->second.nativeReady=false;}
    for(uint32_t i=0;i<wc;i++){
        const auto& w=writes[i];auto at=s.sets.find(w.dstSet);if(at==s.sets.end()||!at->second.nativeReady)continue;
        // Once referenced by a recorded command, never overwrite native contents.
        // A later logical update falls back while older command snapshots survive.
        if(at->second.nativeSealed){at->second.nativeReady=false;continue;}
        bool safe=!w.pNext;
        for(uint32_t j=0;j<w.descriptorCount&&safe;j++)switch(w.descriptorType){
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:safe=w.pBufferInfo&&w.pBufferInfo[j].buffer;break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:safe=w.pTexelBufferView&&w.pTexelBufferView[j];break;
        case VK_DESCRIPTOR_TYPE_SAMPLER:safe=w.pImageInfo&&w.pImageInfo[j].sampler;break;
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:safe=w.pImageInfo&&w.pImageInfo[j].imageView&&w.pImageInfo[j].sampler;break;
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:safe=w.pImageInfo&&w.pImageInfo[j].imageView;break;
        default:safe=false;break;
        }
        if(!safe){at->second.nativeReady=false;continue;}
        s.fn<PFN_vkUpdateDescriptorSets>("vkUpdateDescriptorSets")(s.handle,1,&w,0,nullptr);
    }
}
VKAPI_ATTR void VKAPI_CALL updateSets(VkDevice h,uint32_t wc,const VkWriteDescriptorSet* writes,uint32_t cc,const VkCopyDescriptorSet* copies){
    auto state=device(h);if(!state)return;
    // Tracking is not emulation. Preserve valid native operations even where
    // shared frontend coverage is not complete; record the gap,never advertise it.
    // Guarded public sets are logical state only. Materialization emits actual
    // valid descriptors,so unsupported native null handles never reach Mali.
    if(!state->guarded)state->fn<PFN_vkUpdateDescriptorSets>("vkUpdateDescriptorSets")(h,wc,writes,cc,copies);
    std::lock_guard lock(state->mutex);
    const auto originalWrites=writes;
    // Native sampler heaps never enter nullable descriptor emulation. Retain
    // the actual driver update, including updates after bind/pending work.
    if(state->guarded){
        std::vector<VkWriteDescriptorSet> logical;
        try{
            for(uint32_t i=0;i<wc;i++){
                auto at=state->sets.find(writes[i].dstSet);
                if(at==state->sets.end()||!at->second.nativeSampler){logical.push_back(writes[i]);continue;}
                const auto& w=writes[i];
                if(w.pNext||w.descriptorType!=VK_DESCRIPTOR_TYPE_SAMPLER||!w.pImageInfo)throw std::invalid_argument("native sampler heap write shape");
                for(uint32_t j=0;j<w.descriptorCount;j++)if(!w.pImageInfo[j].sampler)throw std::invalid_argument("null sampler is not supported by Vulkan");
                state->fn<PFN_vkUpdateDescriptorSets>("vkUpdateDescriptorSets")(h,1,&w,0,nullptr);
            }
            // Copies involving the native heap require explicit classification.
            for(uint32_t i=0;i<cc;i++)for(auto set:{copies[i].srcSet,copies[i].dstSet}){auto at=state->sets.find(set);if(at!=state->sets.end()&&at->second.nativeSampler)throw std::invalid_argument("native sampler heap copy not implemented");}
            if(logical.size()!=wc){state->updates->update(uint32_t(logical.size()),logical.data(),cc,copies);mirrorOrdinaryWrites(*state,uint32_t(logical.size()),logical.data(),cc,copies);state->commonWrites+=logical.size();state->commonCopies+=cc;return;}
        }catch(const std::exception& e){state->gap("vkUpdateDescriptorSets",e.what());for(uint32_t i=0;i<wc;i++)state->invalidateSet(writes[i].dstSet);return;}
    }
    try{
        // Fault injection in the isolated Sarek shader regression ONLY. Public
        // API inputs remain valid/native-feature queries unchanged. Exercise
        // the actual null resolution branch without advertising a fake feature.
        std::array<VkWriteDescriptorSet,3> injected;std::array<VkDescriptorImageInfo,2> nulls;
        const auto testNull=std::getenv("MAVK_TEST_NULL_IMAGE");
        if(state->guarded&&testNull&&std::strcmp(testNull,"1")==0&&wc==3&&cc==0&&
            writes[0].descriptorType==VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER&&writes[0].dstBinding==0&&writes[0].descriptorCount==1&&
            writes[1].descriptorType==VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE&&writes[1].dstBinding==1&&writes[1].descriptorCount==1&&
            writes[2].descriptorType==VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE&&writes[2].dstBinding==2&&writes[2].descriptorCount==1&&
            writes[0].dstSet==writes[1].dstSet&&writes[0].dstSet==writes[2].dstSet){
            for(unsigned i=0;i<3;i++)injected[i]=writes[i];
            // Keep Y valid:its dimensions are a divisor in the real shader.
            // Null only CbCr;planar-mode RGB must become (0,Y.r,0).
            nulls[0]=writes[2].pImageInfo[0];nulls[0].imageView=VK_NULL_HANDLE;injected[2].pImageInfo=&nulls[0];
            writes=injected.data();std::fprintf(stderr,"MAVK_LAYER_NULL_INJECTION imageSlots=1\n");
        }
        state->updates->update(wc,writes,cc,copies);state->commonWrites+=wc;state->commonCopies+=cc;
        if(state->guarded)mirrorOrdinaryWrites(*state,wc,writes,cc,copies);
        if(state->trace)std::fprintf(stderr,"MAVK_LAYER_UPDATE common=1 writes=%u copies=%u\n",wc,cc);
    }catch(const std::exception& e){state->gap("vkUpdateDescriptorSets",e.what());
        for(uint32_t i=0;i<wc;i++)state->invalidateSet(originalWrites[i].dstSet);for(uint32_t i=0;i<cc;i++)state->invalidateSet(copies[i].dstSet);
    }
}
VkResult createTemplateNamed(VkDevice h,const VkDescriptorUpdateTemplateCreateInfo* ci,const VkAllocationCallbacks* alloc,VkDescriptorUpdateTemplate* out,const char* name){
    auto state=device(h);if(!state)return VK_ERROR_INITIALIZATION_FAILED;
    auto fn=state->fn<PFN_vkCreateDescriptorUpdateTemplate>(name);if(!fn)return VK_ERROR_EXTENSION_NOT_PRESENT;
    auto result=fn(h,ci,alloc,out);if(result!=VK_SUCCESS)return result;
    std::lock_guard lock(state->mutex);
    try{auto record=state->templates.emplace(*out,UpdateTemplate{}).first;auto layout=state->layouts.find(ci->descriptorSetLayout);
        if(layout==state->layouts.end()||!layout->second.trackable)throw std::invalid_argument("opaque template layout");
        // Private common-adapter key ONLY,never forwarded to a Vulkan driver.
        // Template schema must be validated even before any real set is allocated.
        const auto logical=reinterpret_cast<VkDescriptorSet>(uintptr_t(1));
        VulkanDescriptorUpdates schema(state->resources,state->limits,false);
        VkDescriptorSetLayoutCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};info.bindingCount=layout->second.bindings.size();info.pBindings=layout->second.bindings.data();
        schema.registerSet(logical,info);
        record->second.shadow=std::make_unique<VulkanDescriptorUpdates::UpdateTemplate>(schema.compileTemplate(logical,*ci));
    }catch(const std::exception& e){state->gap(name,e.what());}return result;
}
void destroyTemplateNamed(VkDevice h,VkDescriptorUpdateTemplate descriptorTemplate,const VkAllocationCallbacks* alloc,const char* name){
    auto state=device(h);if(!state)return;{std::lock_guard lock(state->mutex);state->templates.erase(descriptorTemplate);}
    if(auto fn=state->fn<PFN_vkDestroyDescriptorUpdateTemplate>(name))fn(h,descriptorTemplate,alloc);
}
void updateTemplateNamed(VkDevice h,VkDescriptorSet set,VkDescriptorUpdateTemplate descriptorTemplate,const void* data,const char* name){
    auto state=device(h);if(!state)return;
    auto fn=state->fn<PFN_vkUpdateDescriptorSetWithTemplate>(name);if(!fn)return;if(!state->guarded)fn(h,set,descriptorTemplate,data);
    std::lock_guard lock(state->mutex);
    try{auto record=state->templates.find(descriptorTemplate);if(record==state->templates.end()||!record->second.shadow)throw std::invalid_argument("untracked descriptor template");
        std::function<void(uint32_t,const VkWriteDescriptorSet*)> mirror;
        if(state->guarded&&state->useNativePublic)mirror=[&](uint32_t count,const VkWriteDescriptorSet* writes){mirrorOrdinaryWrites(*state,count,writes,0,nullptr);};
        const auto& compiled=*record->second.shadow;state->updates->updateWithTemplate(set,compiled,data,compiled.requiredBytes(),mirror);state->commonTemplates++;
        if(state->trace)std::fprintf(stderr,"MAVK_LAYER_TEMPLATE common=1 sourceBytes=%zu\n",compiled.requiredBytes());
    }catch(const std::exception& e){state->gap(name,e.what());state->invalidateSet(set);}
}
VKAPI_ATTR VkResult VKAPI_CALL createTemplate(VkDevice h,const VkDescriptorUpdateTemplateCreateInfo* ci,const VkAllocationCallbacks* a,VkDescriptorUpdateTemplate* out){return createTemplateNamed(h,ci,a,out,"vkCreateDescriptorUpdateTemplate");}
VKAPI_ATTR VkResult VKAPI_CALL createTemplateKHR(VkDevice h,const VkDescriptorUpdateTemplateCreateInfo* ci,const VkAllocationCallbacks* a,VkDescriptorUpdateTemplate* out){return createTemplateNamed(h,ci,a,out,"vkCreateDescriptorUpdateTemplateKHR");}
VKAPI_ATTR void VKAPI_CALL destroyTemplate(VkDevice h,VkDescriptorUpdateTemplate t,const VkAllocationCallbacks* a){destroyTemplateNamed(h,t,a,"vkDestroyDescriptorUpdateTemplate");}
VKAPI_ATTR void VKAPI_CALL destroyTemplateKHR(VkDevice h,VkDescriptorUpdateTemplate t,const VkAllocationCallbacks* a){destroyTemplateNamed(h,t,a,"vkDestroyDescriptorUpdateTemplateKHR");}
VKAPI_ATTR void VKAPI_CALL updateTemplate(VkDevice h,VkDescriptorSet s,VkDescriptorUpdateTemplate t,const void* data){updateTemplateNamed(h,s,t,data,"vkUpdateDescriptorSetWithTemplate");}
VKAPI_ATTR void VKAPI_CALL updateTemplateKHR(VkDevice h,VkDescriptorSet s,VkDescriptorUpdateTemplate t,const void* data){updateTemplateNamed(h,s,t,data,"vkUpdateDescriptorSetWithTemplateKHR");}
VKAPI_ATTR VkResult VKAPI_CALL createShader(VkDevice h,const VkShaderModuleCreateInfo* ci,const VkAllocationCallbacks* alloc,VkShaderModule* out){
    auto state=device(h);if(!state)return VK_ERROR_INITIALIZATION_FAILED;
    auto result=state->fn<PFN_vkCreateShaderModule>("vkCreateShaderModule")(h,ci,alloc,out);if(result!=VK_SUCCESS)return result;
    std::lock_guard lock(state->mutex);
    try{if(ci->pNext||ci->flags||!ci->pCode||ci->codeSize<20||ci->codeSize%4||ci->codeSize>1024*1024||state->shaderBytes>8*1024*1024-ci->codeSize)throw std::invalid_argument("extended/bounded shader shadow unsupported");
        auto words=std::make_shared<const GuardCompiler::Words>(ci->pCode,ci->pCode+ci->codeSize/4);state->shaders.emplace(*out,std::move(words));state->shaderBytes+=ci->codeSize;
    }catch(const std::exception& e){state->gap("vkCreateShaderModule",e.what());}return result;
}
VKAPI_ATTR void VKAPI_CALL destroyShader(VkDevice h,VkShaderModule shader,const VkAllocationCallbacks* alloc){
    auto state=device(h);if(!state)return;{std::lock_guard lock(state->mutex);auto at=state->shaders.find(shader);if(at!=state->shaders.end()){state->shaderBytes-=at->second->size()*4;state->shaders.erase(at);}}
    state->fn<PFN_vkDestroyShaderModule>("vkDestroyShaderModule")(h,shader,alloc);
}
VKAPI_ATTR VkResult VKAPI_CALL createPipelineLayout(VkDevice h,const VkPipelineLayoutCreateInfo* ci,const VkAllocationCallbacks* alloc,VkPipelineLayout* out){
    auto state=device(h);if(!state)return VK_ERROR_INITIALIZATION_FAILED;
    std::lock_guard lock(state->mutex);std::shared_ptr<PipelineSchema> schema;
    try{
        if(ci->pNext||ci->flags||ci->setLayoutCount>7||ci->setLayoutCount>=state->core->properties().limits.maxBoundDescriptorSets||ci->pushConstantRangeCount>32)
            throw std::invalid_argument("extended pipeline layout/private guard set unsupported");
        schema=std::make_shared<PipelineSchema>();
        for(uint32_t i=0;i<ci->setLayoutCount;i++){auto at=state->layouts.find(ci->pSetLayouts[i]);if(at==state->layouts.end()||(!at->second.trackable&&!at->second.nativeSampler&&!at->second.deferred)||(state->guarded&&!at->second.owner))throw std::invalid_argument("pipeline references opaque descriptor schema");schema->sets.push_back(at->second);}
        if(ci->pushConstantRangeCount)schema->push.assign(ci->pPushConstantRanges,ci->pPushConstantRanges+ci->pushConstantRangeCount);
        if(state->guarded&&ci->setLayoutCount){
            if(alloc)throw std::invalid_argument("guarded pipeline layout allocator unsupported");
            auto guard=std::make_shared<SetLayoutOwner>();guard->device=h;guard->destroy=state->fn<PFN_vkDestroyDescriptorSetLayout>("vkDestroyDescriptorSetLayout");
            VkDescriptorSetLayoutBinding bindings[]={{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_ALL,nullptr},{1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_ALL,nullptr}};
            if(state->useUniformMetadata){
                uint64_t uniforms=0;for(const auto& set:schema->sets)for(const auto& b:set.bindings)if(b.descriptorType==VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER||b.descriptorType==VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)uniforms+=b.descriptorCount;
                const auto words=metadataWordCount(*schema);const auto& limits=state->core->properties().limits;
                schema->uniformMetadata=words&&words<=1024&&words*16<=limits.maxUniformBufferRange&&uniforms+1<=limits.maxPerStageDescriptorUniformBuffers&&uniforms+1<=limits.maxDescriptorSetUniformBuffers;
                if(schema->uniformMetadata)bindings[1].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            }
            VkDescriptorSetLayoutCreateInfo gi{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};gi.bindingCount=2;gi.pBindings=bindings;
            descriptor_detail::check(state->fn<PFN_vkCreateDescriptorSetLayout>("vkCreateDescriptorSetLayout")(h,&gi,nullptr,&guard->handle),"private guard layout");guard->owns=true;schema->guard=std::move(guard);
        }
    }catch(const std::exception& e){state->gap("vkCreatePipelineLayout",e.what());if(state->guarded){*out=VK_NULL_HANDLE;return VK_ERROR_FEATURE_NOT_PRESENT;}schema.reset();}
    VkPipelineLayoutCreateInfo native=*ci;std::vector<VkDescriptorSetLayout> layouts;
    if(state->guarded&&ci->setLayoutCount){layouts.assign(ci->pSetLayouts,ci->pSetLayouts+ci->setLayoutCount);layouts.push_back(schema->guard->handle);native.setLayoutCount=layouts.size();native.pSetLayouts=layouts.data();}
    auto result=state->fn<PFN_vkCreatePipelineLayout>("vkCreatePipelineLayout")(h,&native,alloc,out);if(result!=VK_SUCCESS)return result;
    if(schema)try{
        if(!alloc){schema->owner=std::make_shared<PipelineLayoutOwner>();schema->owner->device=h;schema->owner->handle=*out;schema->owner->destroy=state->fn<PFN_vkDestroyPipelineLayout>("vkDestroyPipelineLayout");}
        state->pipelineLayouts.emplace(*out,schema);if(schema->owner)schema->owner->owns=true;
    }catch(const std::exception& e){state->gap("vkCreatePipelineLayout",e.what());if(state->guarded){state->fn<PFN_vkDestroyPipelineLayout>("vkDestroyPipelineLayout")(h,*out,alloc);*out=VK_NULL_HANDLE;return VK_ERROR_OUT_OF_HOST_MEMORY;}}
    return result;
}
VKAPI_ATTR void VKAPI_CALL destroyPipelineLayout(VkDevice h,VkPipelineLayout layout,const VkAllocationCallbacks* alloc){
    auto state=device(h);if(!state)return;{std::lock_guard lock(state->mutex);auto at=state->pipelineLayouts.find(layout);if(at!=state->pipelineLayouts.end()&&at->second->owner){state->pipelineLayouts.erase(at);return;}state->pipelineLayouts.erase(layout);}
    state->fn<PFN_vkDestroyPipelineLayout>("vkDestroyPipelineLayout")(h,layout,alloc);
}
std::shared_ptr<const GuardCompiler::Words> pipelineStageWords(Device& state,const VkPipelineShaderStageCreateInfo& stage){
    shaderStageNativeChain(stage);
    if(!stage.module&&stage.pNext){
        auto ci=static_cast<const VkShaderModuleCreateInfo*>(stage.pNext);
        if(ci->sType!=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO||ci->pNext||ci->flags||!ci->pCode||ci->codeSize<20||ci->codeSize%4||ci->codeSize>1024*1024||ci->pCode[0]!=0x07230203)
            throw std::invalid_argument("unsupported inline shader module shape");
        return std::make_shared<const GuardCompiler::Words>(ci->pCode,ci->pCode+ci->codeSize/4);
    }
    auto at=state.shaders.find(stage.module);if(at==state.shaders.end())throw std::invalid_argument("untracked original shader module");return at->second;
}
std::shared_ptr<PipelineShadow> preparePipeline(Device& state,VkPipelineLayout layout,uint32_t count,const VkPipelineShaderStageCreateInfo* stages,bool preserveOriginals=false){
    auto schema=state.pipelineLayouts.find(layout);if(schema==state.pipelineLayouts.end()||!count||count>8||!stages)throw std::invalid_argument("untracked pipeline layout/stages");
    auto shadow=std::make_shared<PipelineShadow>();shadow->schema=schema->second;
    std::vector<std::shared_ptr<const GuardCompiler::Words>> originals;
    std::vector<bool> stageUses;
    std::vector<std::set<std::pair<uint32_t,uint32_t>>> stageBindings(count);
    const auto target=state.instance->apiVersion>=VK_API_VERSION_1_3?GuardTarget::Vulkan13:state.instance->apiVersion>=VK_API_VERSION_1_2?GuardTarget::Vulkan12:GuardTarget::Vulkan11;
    for(uint32_t i=0;i<count;i++){
        uint32_t model;
        switch(stages[i].stage){case VK_SHADER_STAGE_VERTEX_BIT:model=0;break;case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:model=1;break;case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:model=2;break;case VK_SHADER_STAGE_GEOMETRY_BIT:model=3;break;case VK_SHADER_STAGE_FRAGMENT_BIT:model=4;break;case VK_SHADER_STAGE_COMPUTE_BIT:model=5;break;default:throw std::invalid_argument("unsupported static-use shader stage");}
        if(!stages[i].pName)throw std::invalid_argument("shader entry point missing");
        auto source=pipelineStageWords(state,stages[i]);auto uses=staticDescriptorUses(*source,stages[i].pName,model,target,state.uniformBufferStandardLayout);
        for(const auto& use:uses){
            if(use.set>=schema->second->sets.size())throw std::invalid_argument("statically used set outside pipeline layout");
            if(schema->second->sets[use.set].deferred)throw std::invalid_argument("statically used extended descriptor schema unsupported");
            const auto& bindings=schema->second->sets[use.set].bindings;
            auto binding=std::find_if(bindings.begin(),bindings.end(),[&](const auto& b){return b.binding==use.binding;});
            if(binding==bindings.end()||!binding->descriptorCount||!(binding->stageFlags&stages[i].stage))throw std::invalid_argument("statically used descriptor missing/stage mismatch");
            shadow->requiredSets[use.set]=true;
            shadow->requiredBindings.insert({use.set,use.binding});
            stageBindings[i].insert({use.set,use.binding});
        }
        originals.push_back(std::move(source));stageUses.push_back(!uses.empty());
    }
    if(preserveOriginals||std::none_of(shadow->requiredSets.begin(),shadow->requiredSets.end(),[](bool used){return used;})){
        for(const auto& source:originals){shadow->shaders.push_back(source);shadow->bytes+=source->size()*4;}
        if(shadow->bytes>32*1024*1024||state.pipelineBytes>32*1024*1024-shadow->bytes)throw std::invalid_argument("bounded pipeline shadow exceeded");
        if(preserveOriginals)std::fprintf(stderr,"MAVK_LAYER_TRACKED_ORIGINAL stages=%u guarded=0\n",count);
        else std::fprintf(stderr,"MAVK_LAYER_DESCRIPTOR_FREE stages=%u native=1\n",count);return shadow;
    }
    const auto publicSets=uint32_t(schema->second->sets.size());
    const auto uniformWords=schema->second->uniformMetadata?metadataWordCount(*schema->second,&shadow->requiredBindings):0;
    for(uint32_t i=0;i<count;i++){
        if(!stageUses[i]){shadow->shaders.push_back(originals[i]);shadow->bytes+=originals[i]->size()*4;continue;}
        const bool reports=state.guardReports&&(stages[i].stage==VK_SHADER_STAGE_COMPUTE_BIT||(stages[i].stage==VK_SHADER_STAGE_FRAGMENT_BIT?state.fragmentStores:state.vertexStores));
        const auto key=publicSets+(reports?0:8);auto compiler=state.compilers.find(key);
        if(compiler==state.compilers.end()){
            if(state.compilers.size()>=4)state.compilers.erase(state.compilers.begin());
            GuardCompilerOptions options{publicSets};options.reportErrors=reports;options.texelFormatMetadata=true;
            options.target=target;
            options.uniformBufferStandardLayout=state.uniformBufferStandardLayout;
            options.metadataReadOnly=state.metadataReadOnly;
            options.optimizeMetadataReads=state.optimizeMetadataReads;
            compiler=state.compilers.emplace(key,std::make_unique<GuardCompiler>(state.shaderCacheEntries,state.shaderCacheBytes,options)).first;
        }
        auto source=originals[i];
        const auto shaderBefore=state.shaderCacheProfile?compiler->second->stats():GuardCompilerStats{};
        const auto shaderStart=state.shaderCacheProfile?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};
        auto guarded=compiler->second->compile(*source);
        if(uniformWords)
            guarded=std::make_shared<const GuardCompiler::Words>(uniformGuardMetadata(*guarded,publicSets,uniformWords,target,state.uniformBufferStandardLayout));
        if(state.shaderCacheProfile){
            const auto after=compiler->second->stats();++state.shaderCalls;
            state.shaderCompiled+=after.compiled-shaderBefore.compiled;state.shaderHits+=after.hits-shaderBefore.hits;
            state.shaderNs+=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-shaderStart).count();
            if(state.shaderCalls<=4||state.shaderCalls%32==0)std::fprintf(stderr,"MAVK_SHADER_CACHE calls=%llu compiled=%llu hits=%llu cpuMs=%.3f retainedBytes=%zu\n",(unsigned long long)state.shaderCalls,(unsigned long long)state.shaderCompiled,(unsigned long long)state.shaderHits,double(state.shaderNs)/1000000.0,after.retainedBytes);
        }
        ImageShaderTypes declarations;
        if(state.guarded)for(auto [key,format]:reflectImageFormats(*source,true,1,&declarations)){
            if(!stageBindings[i].count(key))continue;
            auto [at,added]=shadow->texelFormats.emplace(key,format);
            if(!added&&at->second!=format)throw std::invalid_argument("incompatible texel format across shader stages");
        }
        for(const auto& [key,type]:declarations)if(stageBindings[i].count(key))shadow->imageTypes.emplace(key,type);
        if(state.guarded)for(auto [key,type]:reflectColorImageTypes(*source)){
            if(!stageBindings[i].count(key))continue;
            auto [at,added]=shadow->imageFormats.emplace(key,type);
            if(!added&&(at->second.format!=type.format||at->second.viewType!=type.viewType))throw std::invalid_argument("incompatible color image shape across shader stages");
        }
        bool metadata=false;for(size_t p=5;p<guarded->size();p+=(*guarded)[p]>>16)if(((*guarded)[p]&65535)==71&&((*guarded)[p]>>16)==4&&(*guarded)[p+2]==34&&(*guarded)[p+3]==publicSets)metadata=true;
        if(state.guarded&&metadata&&!reports)std::fprintf(stderr,"MAVK_LAYER_READ_ONLY_GUARD stage=%u diagnosticWrites=0\n",stages[i].stage);
        shadow->bytes+=guarded->size()*4;shadow->shaders.push_back(std::move(guarded));
    }
    if(shadow->bytes>32*1024*1024||state.pipelineBytes>32*1024*1024-shadow->bytes)throw std::invalid_argument("bounded pipeline shadow exceeded");
    if(state.guarded){
        if(!schema->second->owner||!schema->second->guard)throw std::invalid_argument("augmented native layout missing");
        std::vector<DescriptorBinding> common;BorrowedDescriptorLayouts native;
        native.diagnosticWrites=state.guardReports||!state.shareQuietLog;
        if(state.useMetadataArena){if(!state.metadataArena)state.metadataArena=std::make_shared<DescriptorMetadataArena>(*state.core);native.metadataArena=state.metadataArena;}
        native.uniformMetadataWords=uint32_t(uniformWords);
        for(uint32_t set=0;set<publicSets;set++){
            const auto& s=schema->second->sets[set];native.sets.push_back(s.owner->handle);
            if(!shadow->requiredSets[set]){native.externalSets.push_back(set);continue;}
            if(s.nativeSampler){native.externalSets.push_back(set);continue;}
            for(const auto& b:s.bindings)if(b.descriptorCount){
                DescriptorBinding binding{set,b.binding,b.descriptorCount,descriptor_api::kind(b.descriptorType)};
                native.allocationBindings.push_back(binding);
                if(shadow->requiredBindings.count({set,b.binding})){common.push_back(binding);shadow->resourceBindings.emplace_back(set,b.binding);}
                if(descriptor_api::dynamic(b.descriptorType))native.dynamicBindings.emplace_back(set,b.binding);
                if(b.pImmutableSamplers)native.immutableBindings.emplace_back(set,b.binding);
            }
        }
        native.sets.push_back(schema->second->guard->handle);native.pipeline=schema->second->owner->handle;native.owner=schema->second;
        if(!common.empty())shadow->descriptors=std::make_shared<NativeDescriptorAdapter>(*state.core,std::move(common),GuardMetadataAbi::IndexInitFormats,std::move(native));
        else if(!std::all_of(schema->second->sets.begin(),schema->second->sets.end(),emptyLayout))throw std::invalid_argument("UNSUPPORTED guarded external-sampler-only schema");
    }
    return shadow;
}
std::vector<VkPipelineShaderStageCreateInfo> instrumentStages(Device& state,const PipelineShadow& shadow,uint32_t count,const VkPipelineShaderStageCreateInfo* stages,std::vector<std::shared_ptr<ShaderOwner>>& temporary){
    std::vector<VkPipelineShaderStageCreateInfo> out(stages,stages+count);
    for(uint32_t i=0;i<count;i++){
        auto owner=std::make_shared<ShaderOwner>();owner->device=state.handle;owner->destroy=state.fn<PFN_vkDestroyShaderModule>("vkDestroyShaderModule");
        VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};ci.codeSize=shadow.shaders[i]->size()*4;ci.pCode=shadow.shaders[i]->data();
        descriptor_detail::check(state.fn<PFN_vkCreateShaderModule>("vkCreateShaderModule")(state.handle,&ci,nullptr,&owner->handle),"guarded shader module");owner->owns=true;out[i]=shaderStageWithModule(stages[i],owner->handle);temporary.push_back(std::move(owner));
    }return out;
}
void registerPipeline(Device& state,VkPipeline handle,std::shared_ptr<PipelineShadow> shadow){
    if(state.guarded){shadow->owner=std::make_shared<PipelineOwner>();shadow->owner->device=state.handle;shadow->owner->handle=handle;shadow->owner->destroy=state.fn<PFN_vkDestroyPipeline>("vkDestroyPipeline");}
    state.pipelines.emplace(handle,shadow);if(shadow->owner)shadow->owner->owns=true;state.pipelineBytes+=shadow->bytes;state.commonPipelines++;
    std::fprintf(stderr,"MAVK_LAYER_PIPELINE guards=%s stages=%zu publicSets=%zu\n",state.guarded?"native-instrumented":"compiled-only",shadow->shaders.size(),shadow->schema->sets.size());
}
#include "cull_pipelines.hpp"
VKAPI_ATTR VkResult VKAPI_CALL createGraphicsPipelines(VkDevice h,VkPipelineCache cache,uint32_t count,const VkGraphicsPipelineCreateInfo* ci,const VkAllocationCallbacks* alloc,VkPipeline* out){
    auto state=device(h);if(!state)return VK_ERROR_INITIALIZATION_FAILED;std::lock_guard lock(state->mutex);
    std::vector<VkGraphicsPipelineCreateInfo> native;std::vector<std::shared_ptr<PipelineShadow>> shadows;
    std::vector<std::vector<VkPipelineShaderStageCreateInfo>> stages;std::vector<std::shared_ptr<ShaderOwner>> temporary;
    std::vector<CullPipelineStages> linked;
    if(state->guarded||state->cullPipelines)try{
        if(alloc||count>4096)throw std::invalid_argument("guarded graphics batch/allocator unsupported");
        native.assign(ci,ci+count);shadows.resize(count);stages.resize(count);linked.resize(count);
        for(uint32_t i=0;i<count;i++){
            if(ci[i].flags&(VK_PIPELINE_CREATE_LIBRARY_BIT_KHR|VK_PIPELINE_CREATE_DERIVATIVE_BIT))throw std::invalid_argument("guarded pipeline libraries/derivatives unsupported");
            const bool transformed=prepareCullPipeline(*state,ci[i],linked[i]);
            if(state->guarded||transformed){
                const auto n=transformed?uint32_t(linked[i].stages.size()):ci[i].stageCount;const auto input=transformed?linked[i].stages.data():ci[i].pStages;
                shadows[i]=preparePipeline(*state,ci[i].layout,n,input,!state->guarded);
                stages[i]=instrumentStages(*state,*shadows[i],n,input,temporary);native[i].pStages=stages[i].data();native[i].stageCount=n;
            }
        }
    }catch(const std::exception& e){state->gap("vkCreateGraphicsPipelines",e.what());for(uint32_t i=0;i<count;i++)out[i]=VK_NULL_HANDLE;return VK_ERROR_FEATURE_NOT_PRESENT;}
    auto result=state->fn<PFN_vkCreateGraphicsPipelines>("vkCreateGraphicsPipelines")(h,cache,count,state->guarded||state->cullPipelines?native.data():ci,alloc,out);
    for(uint32_t i=0;i<count;i++)if(out[i])try{registerPipeline(*state,out[i],!shadows.empty()&&shadows[i]?shadows[i]:preparePipeline(*state,ci[i].layout,ci[i].stageCount,ci[i].pStages));}
    catch(const std::exception& e){state->gap("vkCreateGraphicsPipelines",e.what());if(state->guarded||(!linked.empty()&&!linked[i].stages.empty())){state->fn<PFN_vkDestroyPipeline>("vkDestroyPipeline")(h,out[i],alloc);out[i]=VK_NULL_HANDLE;result=VK_ERROR_OUT_OF_HOST_MEMORY;}}
    return result;
}
VKAPI_ATTR VkResult VKAPI_CALL createComputePipelines(VkDevice h,VkPipelineCache cache,uint32_t count,const VkComputePipelineCreateInfo* ci,const VkAllocationCallbacks* alloc,VkPipeline* out){
    auto state=device(h);if(!state)return VK_ERROR_INITIALIZATION_FAILED;std::lock_guard lock(state->mutex);
    std::vector<VkComputePipelineCreateInfo> native;std::vector<std::shared_ptr<PipelineShadow>> shadows;std::vector<std::shared_ptr<ShaderOwner>> temporary;
    if(state->guarded)try{
        if(alloc||count>4096)throw std::invalid_argument("guarded compute batch/allocator unsupported");native.assign(ci,ci+count);shadows.resize(count);
        for(uint32_t i=0;i<count;i++){if(ci[i].flags&(VK_PIPELINE_CREATE_LIBRARY_BIT_KHR|VK_PIPELINE_CREATE_DERIVATIVE_BIT))throw std::invalid_argument("guarded pipeline derivatives unsupported");shadows[i]=preparePipeline(*state,ci[i].layout,1,&ci[i].stage);native[i].stage=instrumentStages(*state,*shadows[i],1,&ci[i].stage,temporary)[0];}
    }catch(const std::exception& e){state->gap("vkCreateComputePipelines",e.what());for(uint32_t i=0;i<count;i++)out[i]=VK_NULL_HANDLE;return VK_ERROR_FEATURE_NOT_PRESENT;}
    auto result=state->fn<PFN_vkCreateComputePipelines>("vkCreateComputePipelines")(h,cache,count,state->guarded?native.data():ci,alloc,out);
    for(uint32_t i=0;i<count;i++)if(out[i])try{registerPipeline(*state,out[i],state->guarded?shadows[i]:preparePipeline(*state,ci[i].layout,1,&ci[i].stage));}
    catch(const std::exception& e){state->gap("vkCreateComputePipelines",e.what());if(state->guarded){state->fn<PFN_vkDestroyPipeline>("vkDestroyPipeline")(h,out[i],alloc);out[i]=VK_NULL_HANDLE;result=VK_ERROR_OUT_OF_HOST_MEMORY;}}
    return result;
}
VKAPI_ATTR void VKAPI_CALL destroyPipeline(VkDevice h,VkPipeline pipeline,const VkAllocationCallbacks* alloc){
    auto state=device(h);if(!state)return;{std::lock_guard lock(state->mutex);auto at=state->pipelines.find(pipeline);if(at!=state->pipelines.end()){state->pipelineBytes-=at->second->bytes;bool owned=bool(at->second->owner);state->pipelines.erase(at);if(owned)return;}}
    state->fn<PFN_vkDestroyPipeline>("vkDestroyPipeline")(h,pipeline,alloc);
}
#include "gpu_profile_commands.hpp"
#include "gpu_profile_submit.hpp"
#include "commands.hpp"
PFN_vkVoidFunction instanceHook(const char* n){
#define MATCH(name,fn) if(!std::strcmp(n,name))return reinterpret_cast<PFN_vkVoidFunction>(fn)
    MATCH("vkGetInstanceProcAddr",mavkGetInstanceProcAddr);MATCH("vkGetDeviceProcAddr",mavkGetDeviceProcAddr);
    MATCH("vkCreateInstance",createInstance);MATCH("vkDestroyInstance",destroyInstance);
    MATCH("vkEnumeratePhysicalDevices",enumeratePhysicalDevices);MATCH("vkEnumeratePhysicalDeviceGroups",enumerateGroups);MATCH("vkEnumeratePhysicalDeviceGroupsKHR",enumerateGroupsKHR);
    MATCH("vkCreateDevice",createDevice);
    return nullptr;
}
PFN_vkVoidFunction deviceHook(const char* n){
    MATCH("vkGetDeviceProcAddr",mavkGetDeviceProcAddr);MATCH("vkDestroyDevice",destroyDevice);
    MATCH("vkCreateBuffer",createBuffer);MATCH("vkDestroyBuffer",destroyBuffer);
    MATCH("vkCreateImage",createImage);MATCH("vkDestroyImage",destroyImage);MATCH("vkCreateImageView",createImageView);MATCH("vkDestroyImageView",destroyImageView);
    MATCH("vkCreateSampler",createSampler);MATCH("vkDestroySampler",destroySampler);MATCH("vkCreateBufferView",createBufferView);MATCH("vkDestroyBufferView",destroyBufferView);
    MATCH("vkCreateDescriptorSetLayout",createLayout);MATCH("vkDestroyDescriptorSetLayout",destroyLayout);
    MATCH("vkAllocateDescriptorSets",allocateSets);MATCH("vkFreeDescriptorSets",freeSets);
    MATCH("vkResetDescriptorPool",resetPool);MATCH("vkDestroyDescriptorPool",destroyPool);MATCH("vkUpdateDescriptorSets",updateSets);
    MATCH("vkCreateDescriptorUpdateTemplate",createTemplate);MATCH("vkCreateDescriptorUpdateTemplateKHR",createTemplateKHR);
    MATCH("vkDestroyDescriptorUpdateTemplate",destroyTemplate);MATCH("vkDestroyDescriptorUpdateTemplateKHR",destroyTemplateKHR);
    MATCH("vkUpdateDescriptorSetWithTemplate",updateTemplate);MATCH("vkUpdateDescriptorSetWithTemplateKHR",updateTemplateKHR);
    MATCH("vkCreateShaderModule",createShader);MATCH("vkDestroyShaderModule",destroyShader);MATCH("vkCreatePipelineLayout",createPipelineLayout);MATCH("vkDestroyPipelineLayout",destroyPipelineLayout);
    MATCH("vkCreateGraphicsPipelines",createGraphicsPipelines);MATCH("vkCreateComputePipelines",createComputePipelines);MATCH("vkDestroyPipeline",destroyPipeline);
    MATCH("vkGetDeviceQueue",getQueue);MATCH("vkGetDeviceQueue2",getQueue2);MATCH("vkAllocateCommandBuffers",allocateCommands);
    MATCH("vkBeginCommandBuffer",beginCommand);MATCH("vkEndCommandBuffer",endCommand);MATCH("vkResetCommandBuffer",resetCommand);MATCH("vkFreeCommandBuffers",freeCommands);
    MATCH("vkResetCommandPool",resetCommandPool);MATCH("vkDestroyCommandPool",destroyCommandPool);MATCH("vkCmdBindPipeline",bindPipeline);
    MATCH("vkCmdExecuteCommands",executeCommands);
    MATCH("vkCmdBindDescriptorSets",bindSets);MATCH("vkCmdBindDescriptorSets2",bindSets2);MATCH("vkCmdBindDescriptorSets2KHR",bindSets2KHR);
    MATCH("vkCmdDraw",draw);MATCH("vkCmdDrawIndexed",drawIndexed);MATCH("vkCmdDispatch",dispatch);
    MATCH("vkCmdDrawIndirect",drawIndirect);MATCH("vkCmdDrawIndexedIndirect",drawIndexedIndirect);MATCH("vkCmdDispatchIndirect",dispatchIndirect);
    MATCH("vkCmdDispatchBase",dispatchBase);MATCH("vkCmdDispatchBaseKHR",dispatchBaseKHR);
    MATCH("vkCmdDrawIndirectCount",drawIndirectCount);MATCH("vkCmdDrawIndirectCountKHR",drawIndirectCountKHR);MATCH("vkCmdDrawIndirectCountAMD",drawIndirectCountAMD);
    MATCH("vkCmdDrawIndexedIndirectCount",drawIndexedIndirectCount);MATCH("vkCmdDrawIndexedIndirectCountKHR",drawIndexedIndirectCountKHR);MATCH("vkCmdDrawIndexedIndirectCountAMD",drawIndexedIndirectCountAMD);
    MATCH("vkQueueSubmit",queueSubmit);MATCH("vkQueueSubmit2",queueSubmit2);MATCH("vkQueueSubmit2KHR",queueSubmit2KHR);
    MATCH("vkQueuePresentKHR",profilePresent);MATCH("vkWaitForFences",profileWaitForFences);
    MATCH("vkWaitSemaphores",profileWaitSemaphores);MATCH("vkWaitSemaphoresKHR",profileWaitSemaphoresKHR);
#undef MATCH
    return nullptr;
}
}
MAVK_EXPORT PFN_vkVoidFunction VKAPI_CALL mavkGetInstanceProcAddr(VkInstance h,const char* n){
    if(!n)return nullptr;
    if(!h){
        if(!std::strcmp(n,"vkCreateInstance"))return reinterpret_cast<PFN_vkVoidFunction>(createInstance);
        // The upstream WSI layer resolves its next create-device entry with NULL.
        // The physical-device registry supplies the instance at invocation time.
        if(!std::strcmp(n,"vkCreateDevice"))return reinterpret_cast<PFN_vkVoidFunction>(createDevice);
        if(!std::strcmp(n,"vkGetInstanceProcAddr"))return reinterpret_cast<PFN_vkVoidFunction>(mavkGetInstanceProcAddr);
        return nullptr;
    }
    if(auto p=instanceHook(n))return p;
    if(h){auto state=instance(h);if(state){auto next=state->gipa(h,n);if(next)if(auto p=deviceHook(n))return p;return next;}}return nullptr;
}
MAVK_EXPORT PFN_vkVoidFunction VKAPI_CALL mavkGetDeviceProcAddr(VkDevice h,const char* n){
    if(!h||!n)return nullptr;auto state=device(h);if(!state)return nullptr;
    auto next=state->gdpa(h,n);if(next)if(auto p=deviceHook(n))return p;return next;
}
MAVK_EXPORT PFN_vkVoidFunction VKAPI_CALL mavkGetPhysicalDeviceProcAddr(VkInstance h,const char* n){
    if(!h||!n)return nullptr;if(auto p=instanceHook(n))return p;auto state=instance(h);return state?(state->gpdpa?state->gpdpa(h,n):state->gipa(h,n)):nullptr;
}
MAVK_EXPORT VkResult VKAPI_CALL vkNegotiateLoaderLayerInterfaceVersion(VkNegotiateLayerInterface* info){
    if(!info||info->sType!=LAYER_NEGOTIATE_INTERFACE_STRUCT||info->loaderLayerInterfaceVersion<2)return VK_ERROR_INITIALIZATION_FAILED;
    info->loaderLayerInterfaceVersion=2;info->pfnGetInstanceProcAddr=mavkGetInstanceProcAddr;info->pfnGetDeviceProcAddr=mavkGetDeviceProcAddr;info->pfnGetPhysicalDeviceProcAddr=mavkGetPhysicalDeviceProcAddr;return VK_SUCCESS;
}
