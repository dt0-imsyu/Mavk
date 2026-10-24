// Real Vulkan consumer through the loader/layer. Reuses Core + shared renderer;
// linker wrappers select commands, not a second rendering implementation.
#include <mavk/core.hpp>
#include "../../bridge/src/graphics.hpp"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace {
void ok(VkResult r){if(r!=VK_SUCCESS)throw std::runtime_error("Vulkan result="+std::to_string(r));}
std::vector<uint32_t> words(const char* name){std::ifstream f(name,std::ios::binary|std::ios::ate);auto size=f.tellg();if(size<20||size>1048576||size%4)throw std::runtime_error("shader size");std::vector<uint32_t> out(size_t(size)/4);f.seekg(0);f.read(reinterpret_cast<char*>(out.data()),size);if(!f)throw std::runtime_error("shader read");return out;}
struct Root {
    VkInstance instance{};VkDevice device{};VkPhysicalDevice gpu{};VkQueue queue{};uint32_t family{};
    VkDescriptorSetLayout layout{};VkDescriptorPool pool{};VkPipelineLayout pipelineLayout{};
    bool groupKHR=false,countKHR=false,countAMD=false,timeline=false,requirementsOnly=false,wideIndices=false,vertexRobust=false,primitiveCull=false;
    ~Root(){if(device){vkDeviceWaitIdle(device);if(pool)vkDestroyDescriptorPool(device,pool,nullptr);if(pipelineLayout)vkDestroyPipelineLayout(device,pipelineLayout,nullptr);if(layout)vkDestroyDescriptorSetLayout(device,layout,nullptr);vkDestroyDevice(device,nullptr);}if(instance)vkDestroyInstance(instance,nullptr);}
    void init(){
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.pApplicationName="Mavk execution routing";app.apiVersion=VK_API_VERSION_1_2;
        VkInstanceCreateInfo i{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};i.pApplicationInfo=&app;ok(vkCreateInstance(&i,nullptr,&instance));
        uint32_t n=0;ok(vkEnumeratePhysicalDevices(instance,&n,nullptr));std::vector<VkPhysicalDevice> devices(n);ok(vkEnumeratePhysicalDevices(instance,&n,devices.data()));
        for(auto p:devices){VkPhysicalDeviceProperties props{};vkGetPhysicalDeviceProperties(p,&props);if(props.vendorID==0x13b5&&props.deviceType!=VK_PHYSICAL_DEVICE_TYPE_CPU){gpu=p;std::cout<<"HARDWARE "<<props.deviceName<<'\n';break;}}
        if(!gpu)throw std::runtime_error("NO-GO: ARM hardware unavailable");
        if(std::getenv("MAVK_TEST_EXTENSION_ENUM")){
            uint32_t count=0;ok(vkEnumerateDeviceExtensionProperties(gpu,nullptr,&count,nullptr));
            std::vector<VkExtensionProperties> extensions(count);uint32_t full=count;ok(vkEnumerateDeviceExtensionProperties(gpu,nullptr,&full,extensions.data()));
            if(full!=count||!count)throw std::runtime_error("unstable extension enumeration");
            VkExtensionProperties limited[2]{};std::strcpy(limited[1].extensionName,"SENTINEL");uint32_t capacity=0;
            if(vkEnumerateDeviceExtensionProperties(gpu,nullptr,&capacity,limited)!=VK_INCOMPLETE||capacity)throw std::runtime_error("zero-capacity enumeration");
            capacity=1;const auto result=vkEnumerateDeviceExtensionProperties(gpu,nullptr,&capacity,limited);
            if(result!=(count>1?VK_INCOMPLETE:VK_SUCCESS)||capacity!=1||std::strcmp(limited[0].extensionName,extensions[0].extensionName)||std::strcmp(limited[1].extensionName,"SENTINEL"))throw std::runtime_error("partial extension enumeration");
            bool push=false;for(const auto& e:extensions)push|=!std::strcmp(e.extensionName,VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
            if(const auto expected=std::getenv("MAVK_TEST_EXPECT_PUSH"))if(push!=(std::strcmp(expected,"1")==0))throw std::runtime_error("push descriptor capability mismatch");
            std::cout<<"EXTENSION_ENUM_PASS count="<<count<<" zeroCapacity=1 partialCapacity=1 sentinelIntact=1 pushDescriptor="<<push<<'\n';
        }
        if(requirementsOnly){
            VkPhysicalDeviceRobustness2FeaturesEXT robust{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT};
            VkPhysicalDeviceVulkan13Features v13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};v13.pNext=&robust;
            VkPhysicalDeviceVulkan12Features v12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};v12.pNext=&v13;
            VkPhysicalDeviceFeatures2 features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};features.pNext=&v12;vkGetPhysicalDeviceFeatures2(gpu,&features);
#define MAVK_REQUIREMENT(group,name) std::cout<<"FEATURE " #name "="<<group.name<<'\n'
            MAVK_REQUIREMENT(features.features,shaderClipDistance);MAVK_REQUIREMENT(features.features,shaderCullDistance);
            MAVK_REQUIREMENT(features.features,multiViewport);MAVK_REQUIREMENT(features.features,fillModeNonSolid);
            MAVK_REQUIREMENT(features.features,textureCompressionBC);MAVK_REQUIREMENT(features.features,geometryShader);
            MAVK_REQUIREMENT(features.features,tessellationShader);MAVK_REQUIREMENT(features.features,vertexPipelineStoresAndAtomics);
            MAVK_REQUIREMENT(v12,descriptorBindingSampledImageUpdateAfterBind);MAVK_REQUIREMENT(v12,descriptorBindingUpdateUnusedWhilePending);
            MAVK_REQUIREMENT(v12,descriptorBindingPartiallyBound);MAVK_REQUIREMENT(v12,runtimeDescriptorArray);MAVK_REQUIREMENT(v12,timelineSemaphore);
            MAVK_REQUIREMENT(v13,dynamicRendering);MAVK_REQUIREMENT(v13,synchronization2);MAVK_REQUIREMENT(v13,inlineUniformBlock);
            MAVK_REQUIREMENT(robust,robustBufferAccess2);MAVK_REQUIREMENT(robust,robustImageAccess2);MAVK_REQUIREMENT(robust,nullDescriptor);
#undef MAVK_REQUIREMENT
            ok(vkEnumerateDeviceExtensionProperties(gpu,nullptr,&n,nullptr));std::vector<VkExtensionProperties> extensions(n);ok(vkEnumerateDeviceExtensionProperties(gpu,nullptr,&n,extensions.data()));
            for(const char* name:{"VK_EXT_robustness2","VK_EXT_descriptor_indexing","VK_KHR_dynamic_rendering","VK_EXT_transform_feedback"}){bool found=false;for(const auto& extension:extensions)found|=!std::strcmp(name,extension.extensionName);std::cout<<"EXTENSION "<<name<<'='<<found<<'\n';}
            for(unsigned format=131;format<=146;format++){VkFormatProperties properties{};vkGetPhysicalDeviceFormatProperties(gpu,VkFormat(format),&properties);VkImageFormatProperties image{};auto result=vkGetPhysicalDeviceImageFormatProperties(gpu,VkFormat(format),VK_IMAGE_TYPE_2D,VK_IMAGE_TILING_OPTIMAL,VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,0,&image);std::cout<<"FORMAT "<<format<<" optimal="<<properties.optimalTilingFeatures<<" sampledTransfer2D="<<result<<'\n';}
            std::cout<<"REQUIREMENTS_QUERY_ONLY noDeviceCreated=1 noGpuWork=1 noConformanceClaim=1\n";return;
        }
        const bool subgroup16=std::getenv("MAVK_TEST_SUBGROUP16")!=nullptr;
        const bool deferredUnused=std::getenv("MAVK_TEST_DEFERRED_UNUSED")!=nullptr;
        const bool standardUbo=std::getenv("MAVK_TEST_STD430_UBO")!=nullptr;
        VkPhysicalDeviceSubgroupSizeControlFeatures subgroup{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES};
        VkPhysicalDeviceVulkan12Features v12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};VkPhysicalDeviceFeatures2 features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};features.pNext=&v12;if(subgroup16)v12.pNext=&subgroup;vkGetPhysicalDeviceFeatures2(gpu,&features);
        if(subgroup16){
            VkPhysicalDeviceSubgroupSizeControlProperties size{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES};
            VkPhysicalDeviceProperties2 properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};properties.pNext=&size;vkGetPhysicalDeviceProperties2(gpu,&properties);
            if(!subgroup.subgroupSizeControl||size.minSubgroupSize>16||size.maxSubgroupSize<16||!(size.requiredSubgroupSizeStages&VK_SHADER_STAGE_COMPUTE_BIT))throw std::runtime_error("NO-GO: subgroup16 unavailable");
            std::cout<<"SUBGROUP_NATIVE enabled=1 size=16 min="<<size.minSubgroupSize<<" max="<<size.maxSubgroupSize<<'\n';
        }
        if(!v12.drawIndirectCount)throw std::runtime_error("NO-GO: native drawIndirectCount unavailable");
        if(standardUbo&&!v12.uniformBufferStandardLayout)throw std::runtime_error("NO-GO: native standard UBO layout unavailable");
        if(deferredUnused&&(!v12.descriptorBindingSampledImageUpdateAfterBind||!v12.descriptorBindingPartiallyBound||!v12.descriptorBindingUpdateUnusedWhilePending))throw std::runtime_error("NO-GO: native deferred-layout features unavailable");
        if(timeline&&!v12.timelineSemaphore)throw std::runtime_error("NO-GO: native timeline unavailable");
        if(wideIndices&&!features.features.shaderInt64)throw std::runtime_error("NO-GO: native shaderInt64 unavailable");
        if(vertexRobust&&!features.features.robustBufferAccess)throw std::runtime_error("NO-GO: native robustBufferAccess unavailable");
        VkPhysicalDeviceFeatures coreFeatures{};coreFeatures.shaderInt64=wideIndices;coreFeatures.robustBufferAccess=vertexRobust;
        if(primitiveCull){if(!features.features.geometryShader)throw std::runtime_error("NO-GO: native geometryShader unavailable");coreFeatures.geometryShader=std::getenv("MAVK_TEST_CULL_NO_GEOMETRY")?VK_FALSE:VK_TRUE;std::cout<<"CULL_NATIVE_FEATURES geometryShader="<<coreFeatures.geometryShader<<" shaderCullDistance="<<features.features.shaderCullDistance<<" requestedCullDistance=0\n";}
        if(primitiveCull&&std::getenv("MAVK_TEST_CULL_TESSELLATION")){if(!features.features.tessellationShader)throw std::runtime_error("NO-GO: native tessellationShader unavailable");coreFeatures.tessellationShader=VK_TRUE;std::cout<<"CULL_TESSELLATION_FEATURE native=1 requested=1\n";}
        v12={VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};v12.drawIndirectCount=VK_TRUE;
        v12.timelineSemaphore=timeline;v12.uniformBufferStandardLayout=standardUbo;
        if(deferredUnused){v12.descriptorBindingSampledImageUpdateAfterBind=VK_TRUE;v12.descriptorBindingPartiallyBound=VK_TRUE;v12.descriptorBindingUpdateUnusedWhilePending=VK_TRUE;}
        if(subgroup16){subgroup={VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES};subgroup.subgroupSizeControl=VK_TRUE;v12.pNext=&subgroup;}
        vkGetPhysicalDeviceQueueFamilyProperties(gpu,&n,nullptr);std::vector<VkQueueFamilyProperties> qs(n);vkGetPhysicalDeviceQueueFamilyProperties(gpu,&n,qs.data());bool found=false;
        for(uint32_t j=0;j<n;j++)if((qs[j].queueFlags&(VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT))==(VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT)){family=j;found=true;break;}if(!found)throw std::runtime_error("queue");
        ok(vkEnumerateDeviceExtensionProperties(gpu,nullptr,&n,nullptr));std::vector<VkExtensionProperties> es(n);ok(vkEnumerateDeviceExtensionProperties(gpu,nullptr,&n,es.data()));std::vector<const char*> enabled;
        for(const char* wanted:{"VK_KHR_device_group","VK_KHR_draw_indirect_count","VK_AMD_draw_indirect_count"})for(const auto& e:es)if(!std::strcmp(e.extensionName,wanted)){enabled.push_back(wanted);break;}
        if(subgroup16){bool found=false;for(const auto& e:es)if(!std::strcmp(e.extensionName,VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME)){enabled.push_back(VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME);found=true;break;}if(!found)throw std::runtime_error("NO-GO: subgroup extension unavailable");}
        for(auto e:enabled){groupKHR|=!std::strcmp(e,"VK_KHR_device_group");countKHR|=!std::strcmp(e,"VK_KHR_draw_indirect_count");countAMD|=!std::strcmp(e,"VK_AMD_draw_indirect_count");}
        float priority=1;VkDeviceQueueCreateInfo q{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};q.queueFamilyIndex=family;q.queueCount=1;q.pQueuePriorities=&priority;
        VkDeviceCreateInfo d{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};d.pNext=&v12;d.pEnabledFeatures=&coreFeatures;d.queueCreateInfoCount=1;d.pQueueCreateInfos=&q;d.enabledExtensionCount=uint32_t(enabled.size());d.ppEnabledExtensionNames=enabled.data();ok(vkCreateDevice(gpu,&d,nullptr,&device));vkGetDeviceQueue(device,family,0,&queue);
        if(const auto expected=std::getenv("MAVK_EXPECT_LAYER_PATH")){std::ifstream maps("/proc/self/maps");std::string line;bool seen=false;while(std::getline(maps,line))if(line.find(expected)!=std::string::npos){seen=true;break;}if(!seen)throw std::runtime_error("candidate layer not mapped");std::cout<<"CANDIDATE_LAYER_MAPPED "<<expected<<'\n';}
    }
};
bool indirect=false;VkBuffer drawBuffer{},countBuffer{};PFN_vkCmdDrawIndirectCount countCall{};PFN_vkCmdDrawIndexedIndirectCount indexedCall{};
uint32_t vertexFirst=0;
bool injectEmptyLayouts=false;unsigned emptyLayoutInjections=0;VkDescriptorSetLayout emptyLayouts[3]{};
uint64_t pixelsVerified=0,wordsVerified=0;uint32_t drawCases=0,dispatchCases=0;
}
extern "C" void __real_vkCmdDraw(VkCommandBuffer,uint32_t,uint32_t,uint32_t,uint32_t);
extern "C" void __real_vkCmdDrawIndexed(VkCommandBuffer,uint32_t,uint32_t,uint32_t,int32_t,uint32_t);
extern "C" VkResult __real_vkCreatePipelineLayout(VkDevice,const VkPipelineLayoutCreateInfo*,const VkAllocationCallbacks*,VkPipelineLayout*);
extern "C" VkResult __real_vkCreateDescriptorSetLayout(VkDevice,const VkDescriptorSetLayoutCreateInfo*,const VkAllocationCallbacks*,VkDescriptorSetLayout*);
extern "C" VkResult __wrap_vkCreateDescriptorSetLayout(VkDevice device,const VkDescriptorSetLayoutCreateInfo* info,const VkAllocationCallbacks* alloc,VkDescriptorSetLayout* out){
    if(!std::getenv("MAVK_TEST_ZERO_LAYOUT_FLAGS")||info->pNext||info->flags)return __real_vkCreateDescriptorSetLayout(device,info,alloc,out);
    std::vector<VkDescriptorBindingFlags> flags(info->bindingCount);
    VkDescriptorSetLayoutBindingFlagsCreateInfo chain{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO};chain.bindingCount=info->bindingCount;chain.pBindingFlags=flags.data();
    auto copy=*info;copy.pNext=&chain;
    if(std::getenv("MAVK_TEST_DEFERRED_USED"))copy.flags=VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    std::cout<<"ZERO_LAYOUT_FLAGS_INJECT bindings="<<info->bindingCount<<'\n';
    return __real_vkCreateDescriptorSetLayout(device,&copy,alloc,out);
}
extern "C" VkResult __wrap_vkCreatePipelineLayout(VkDevice device,const VkPipelineLayoutCreateInfo* info,const VkAllocationCallbacks* allocator,VkPipelineLayout* result){if(!injectEmptyLayouts)return __real_vkCreatePipelineLayout(device,info,allocator,result);auto copy=*info;copy.setLayoutCount=3;copy.pSetLayouts=emptyLayouts;emptyLayoutInjections++;return __real_vkCreatePipelineLayout(device,&copy,allocator,result);}
extern "C" void __wrap_vkCmdDraw(VkCommandBuffer c,uint32_t v,uint32_t i,uint32_t f,uint32_t fi){if(indirect)countCall(c,drawBuffer,16,countBuffer,12,1,sizeof(VkDrawIndirectCommand));else __real_vkCmdDraw(c,v,i,f+vertexFirst,fi);}
extern "C" void __wrap_vkCmdDrawIndexed(VkCommandBuffer c,uint32_t v,uint32_t i,uint32_t f,int32_t o,uint32_t fi){if(indirect)indexedCall(c,drawBuffer,16,countBuffer,12,1,sizeof(VkDrawIndexedIndirectCommand));else __real_vkCmdDrawIndexed(c,v,i,f,o,fi);}

#include "layer-dynamic.hpp"
#include "layer-immutable.hpp"
#include "layer-empty.hpp"
#include "layer-texel.hpp"
#include "layer-texel-components.hpp"
#include "layer-images.hpp"
#include "layer-nested.hpp"
#include "layer-aggregate.hpp"
#include "layer-vertex-robust.hpp"
#include "layer-storage-texel.hpp"
#include "layer-primitive-cull.hpp"
int main(int argc,char** argv){try{
    if(argc!=4&&argc!=5)throw std::runtime_error("compute.spv sampled.vert.spv sampled.frag.spv [--dynamic] required");Root root;root.requirementsOnly=argc==5&&!std::strcmp(argv[4],"--requirements");root.timeline=argc==5&&!std::strcmp(argv[4],"--inflight");root.wideIndices=argc==5&&!std::strcmp(argv[4],"--wide");root.vertexRobust=argc==5&&!std::strcmp(argv[4],"--vertex-robust");root.primitiveCull=argc==5&&!std::strcmp(argv[4],"--primitive-cull");root.init();if(root.requirementsOnly)return 0;mavk::DeviceContext context(root.gpu,root.device,root.family,root.timeline);
    if(root.primitiveCull){primitiveCulling(root,context,argv[1],argv[2],argv[3]);if(!context.counters().empty())throw std::runtime_error("primitive cull Core resource leak");return 0;}
    if(root.vertexRobust){vertexRobustness(root,context,argv[2],argv[3]);if(!context.counters().empty())throw std::runtime_error("vertex robustness Core resource leak");return 0;}
    if(root.wideIndices){const auto valid=std::getenv("MAVK_WIDE_VALID");nestedBuffers(root,context,argv[1],valid&&std::strcmp(valid,"1")==0?0xA5A5A5A5u:0u);if(!context.counters().empty())throw std::runtime_error("wide buffer Core resource leak");return 0;}
    if(argc==5&&!std::strcmp(argv[4],"--nested")){nestedBuffers(root,context,argv[1],std::getenv("MAVK_TEST_STD430_VALID")?0xA5A5A5A5u:0u);if(!context.counters().empty())throw std::runtime_error("nested buffer Core resource leak");return 0;}
    if(argc==5&&!std::strcmp(argv[4],"--record-overhead")){nestedBuffers(root,context,argv[1],0,true);if(!context.counters().empty())throw std::runtime_error("record benchmark Core resource leak");return 0;}
    if(argc==5&&!std::strcmp(argv[4],"--aggregate")){aggregateBuffers(root,context,argv[1]);if(!context.counters().empty())throw std::runtime_error("aggregate Core resource leak");return 0;}
    if(argc==5&&!std::strcmp(argv[4],"--missing-used")){dynamicBuffers(root,context,argv[1],3);if(!context.counters().empty())throw std::runtime_error("negative descriptor Core resource leak");return 0;}
    if(argc==5&&!std::strcmp(argv[4],"--resubmit")){dynamicBuffers(root,context,argv[1],0,true);if(!context.counters().empty())throw std::runtime_error("reusable descriptor Core resource leak");return 0;}
    if(argc==5&&!std::strcmp(argv[4],"--inflight")){dynamicBuffers(root,context,argv[1],0,true,true);if(!context.counters().empty())throw std::runtime_error("inflight descriptor Core resource leak");return 0;}
    if(argc==5&&!std::strcmp(argv[4],"--texel")){nativeTexelFormats(root,context,argv[1],argv[2]);if(!context.counters().empty())throw std::runtime_error("texel format Core resource leak");return 0;}
    if(argc==5&&!std::strcmp(argv[4],"--texel-bounds")){nativeTexelFormats(root,context,argv[1],argv[2],true);if(!context.counters().empty())throw std::runtime_error("texel bounds Core resource leak");return 0;}
    if(argc==5&&!std::strcmp(argv[4],"--texel-components-views")){texelViewComponents(root,context,argv[1],argv[2]);if(!context.counters().empty())throw std::runtime_error("texel view components Core resource leak");return 0;}
    if(argc==5&&!std::strcmp(argv[4],"--storage-texel")){storageTexelBounds(root,context,argv[1]);if(!context.counters().empty())throw std::runtime_error("storage texel Core resource leak");return 0;}
    if(argc==5&&!std::strcmp(argv[4],"--storage-atomic-native")){storageTexelBounds(root,context,argv[1],true);if(!context.counters().empty())throw std::runtime_error("storage atomic Core resource leak");return 0;}
    if(argc==5&&!std::strcmp(argv[4],"--storage-atomic")){storageTexelBounds(root,context,argv[1],true,true);if(!context.counters().empty())throw std::runtime_error("guarded storage atomic Core resource leak");return 0;}
    if(argc==5&&!std::strcmp(argv[4],"--storage-components")){storageTexelBounds(root,context,argv[1],false,false,true);if(!context.counters().empty())throw std::runtime_error("storage components Core resource leak");return 0;}
    if(argc==5&&!std::strcmp(argv[4],"--images")){imageSubresources(root,context,argv[1]);if(!context.counters().empty())throw std::runtime_error("image subresource Core resource leak");return 0;}
    if(argc==5&&(!std::strcmp(argv[4],"--selected")||!std::strcmp(argv[4],"--unwritten-used"))){dynamicBuffers(root,context,argv[1],!std::strcmp(argv[4],"--selected")?4:5);if(!context.counters().empty())throw std::runtime_error("selected descriptor Core resource leak");return 0;}
    if(argc==5){if(!std::strcmp(argv[4],"--dynamic"))dynamicBuffers(root,context,argv[1]);else if(!std::strcmp(argv[4],"--empty")){dynamicBuffers(root,context,argv[1],true);emptyLayoutsDraw(root,context,argv[2],argv[3]);}else if(!std::strcmp(argv[4],"--unused")){dynamicBuffers(root,context,argv[1],2);emptyLayoutsDraw(root,context,argv[2],argv[3],true);}else if(!std::strcmp(argv[4],"--immutable")){const auto control=std::getenv("MAVK_IMMUTABLE_COPY_CONTROL");immutableSamplers(root,context,words(argv[1]),control&&std::strcmp(control,"1")==0);}else throw std::runtime_error("unknown mode");if(!context.counters().empty())throw std::runtime_error("descriptor Core resource leak");return 0;}
    const auto host=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,coherent=VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    {
        mavk::Buffer input(context,4,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,host,coherent),output(context,1024,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,host,coherent);
        *static_cast<uint32_t*>(input.map())=1234;input.flush();auto values=static_cast<uint32_t*>(output.map());
        VkDescriptorSetLayoutBinding bs[]={{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}};
        VkDescriptorSetLayoutCreateInfo l{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};l.bindingCount=2;l.pBindings=bs;ok(vkCreateDescriptorSetLayout(root.device,&l,nullptr,&root.layout));
        VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,2};VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=1;pool.poolSizeCount=1;pool.pPoolSizes=&size;ok(vkCreateDescriptorPool(root.device,&pool,nullptr,&root.pool));
        VkDescriptorSet set{};VkDescriptorSetAllocateInfo a{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};a.descriptorPool=root.pool;a.descriptorSetCount=1;a.pSetLayouts=&root.layout;ok(vkAllocateDescriptorSets(root.device,&a,&set));
        VkDescriptorBufferInfo buffers[]={{input.handle(),0,4},{output.handle(),0,1024}};VkWriteDescriptorSet writes[2]{};for(unsigned j=0;j<2;j++){writes[j].sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;writes[j].dstSet=set;writes[j].dstBinding=j;writes[j].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;writes[j].descriptorCount=1;writes[j].pBufferInfo=&buffers[j];}vkUpdateDescriptorSets(root.device,2,writes,0,nullptr);
        VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};pl.setLayoutCount=1;pl.pSetLayouts=&root.layout;ok(vkCreatePipelineLayout(root.device,&pl,nullptr,&root.pipelineLayout));auto code=words(argv[1]);mavk::Shader shader(context,code.data(),code.size()*4);
        VkComputePipelineCreateInfo p{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};p.flags=VK_PIPELINE_CREATE_DISPATCH_BASE_BIT;p.layout=root.pipelineLayout;p.stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;p.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;p.stage.module=shader.handle();p.stage.pName="main";mavk::Pipeline pipeline(context,p);mavk::CommandBatch cmd(context);mavk::Fence fence(context);
        for(const char* name:{"vkCmdDispatchBase","vkCmdDispatchBaseKHR"}){
            if(std::strstr(name,"KHR")&&!root.groupKHR){std::cout<<"UNAVAILABLE "<<name<<" extension not enabled\n";continue;}
            auto fn=reinterpret_cast<PFN_vkCmdDispatchBase>(vkGetDeviceProcAddr(root.device,name));if(!fn){std::cout<<"UNAVAILABLE "<<name<<'\n';continue;}
            std::fill(values,values+256,0xCCCCCCCCu);output.flush();fence.reset();cmd.begin();vkCmdBindPipeline(cmd.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,pipeline.handle());vkCmdBindDescriptorSets(cmd.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,root.pipelineLayout,0,1,&set,0,nullptr);fn(cmd.handle(),2,3,1,2,2,1);cmd.hostReadBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_WRITE_BIT);cmd.end();auto h=cmd.handle();VkSubmitInfo s{VK_STRUCTURE_TYPE_SUBMIT_INFO};s.commandBufferCount=1;s.pCommandBuffers=&h;ok(context.submit(root.queue,s,fence.handle()));ok(fence.wait(5000000000ull));output.invalidate();
            for(unsigned j=0;j<256;j++){const bool written=j==90||j==91||j==98||j==99;if(values[j]!=(written?1234+j:0xCCCCCCCCu))throw std::runtime_error(std::string(name)+" base/count or descriptor failure index="+std::to_string(j));wordsVerified++;}dispatchCases++;std::cout<<"GPU_PASS "<<name<<'\n';
        }
    }
    {
        mavk::Buffer args(context,64,VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,host,coherent),counts(context,32,VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,host,coherent);drawBuffer=args.handle();countBuffer=counts.handle();auto argument=static_cast<uint8_t*>(args.map());auto count=static_cast<uint32_t*>(counts.map());std::memset(argument,0,64);std::fill(count,count+8,0u);
        mavk::SampledPixels tex;tex.width=tex.height=tex.levels=1;tex.rgba={23,47,91,255};auto vs=words(argv[2]),fs=words(argv[3]);const float state[]={0,0,0,0},clear[]={1,0,1,1};
        mavk::bridge::VertexGeometry geometry;geometry.stride=4;geometry.count=3;geometry.bytes.resize(12);geometry.indexCount=3;geometry.indices.resize(12);uint32_t indices[]={0,1,2};std::memcpy(geometry.indices.data(),indices,12);
        for(bool indexed:{false,true}){
            indirect=false;auto reference=mavk::bridge::drawOffscreen(context,root.queue,16,16,vs,fs,state,clear,indexed?&geometry:nullptr,16,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr,nullptr,&tex);
            if(reference.size()!=1024)throw std::runtime_error("reference size");for(unsigned j=0;j<1024;j++)if(reference[j]!=tex.rgba[j%4])throw std::runtime_error("direct reference pixels");
            for(const char* suffix:{"","KHR","AMD"}){
                const std::string name=std::string(indexed?"vkCmdDrawIndexedIndirectCount":"vkCmdDrawIndirectCount")+suffix;
                if((!std::strcmp(suffix,"KHR")&&!root.countKHR)||(!std::strcmp(suffix,"AMD")&&!root.countAMD)){std::cout<<"UNAVAILABLE "<<name<<" extension not enabled\n";continue;}
                auto fn=vkGetDeviceProcAddr(root.device,name.c_str());if(!fn){std::cout<<"UNAVAILABLE "<<name<<'\n';continue;}
                countCall=reinterpret_cast<PFN_vkCmdDrawIndirectCount>(fn);indexedCall=reinterpret_cast<PFN_vkCmdDrawIndexedIndirectCount>(fn);
                if(indexed){VkDrawIndexedIndirectCommand c{3,1,0,0,0};std::memcpy(argument+16,&c,sizeof(c));}else{VkDrawIndirectCommand c{3,1,0,0};std::memcpy(argument+16,&c,sizeof(c));}args.flush();indirect=true;
                for(uint32_t n:{0u,1u}){count[3]=n;counts.flush();auto frame=mavk::bridge::drawOffscreen(context,root.queue,16,16,vs,fs,state,clear,indexed?&geometry:nullptr,16,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr,nullptr,&tex);
                    for(unsigned pixel=0;pixel<256;pixel++)for(unsigned ch=0;ch<4;ch++){const uint8_t expected=n?reference[4*pixel+ch]:uint8_t((ch==0||ch==2||ch==3)?255:0);if(frame[4*pixel+ch]!=expected)throw std::runtime_error(name+" pixels/count/offset mismatch");pixelsVerified++;}drawCases++;}
                indirect=false;std::cout<<"GPU_PASS "<<name<<" count0/count1 offsets16/12\n";
            }
        }
    }
    if(!context.counters().empty()||dispatchCases<1||drawCases<4)throw std::runtime_error("coverage/resources");
    std::cout<<"LAYER_EXECUTION_GPU_PASS words="<<wordsVerified<<" components="<<pixelsVerified<<" dispatchCases="<<dispatchCases<<" drawCases="<<drawCases<<" coreLiveZero=1\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
