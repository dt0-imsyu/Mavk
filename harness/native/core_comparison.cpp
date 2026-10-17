#include <jni.h>
#include <mavk/core.hpp>
#include <mavk/compat/bc.hpp>
#include <mavk/compat/sampled.hpp>
#include <chrono>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>
#include "validation.hpp"
#include <mavk/bridge.hpp>
#include "../../bridge/src/graphics.hpp"
#include <cmath>
#include <cstring>
#include <algorithm>
#ifdef MAVK_NATIVE_UI
#include <imgui.h>
#endif

namespace {
using Clock=std::chrono::steady_clock;
double ms(Clock::time_point a,Clock::time_point b){return std::chrono::duration<double,std::milli>(b-a).count();}
void ok(VkResult r,const char* what){if(r!=VK_SUCCESS)throw std::runtime_error(std::string(what)+" VkResult="+std::to_string(r));}
std::string quote(const std::string& s){std::string r="\"";for(unsigned char c:s){if(c=='"'||c=='\\')r+='\\';if(c>=32)r+=c;}return r+'"';}
struct Root {
    ValidationCapture& validation;explicit Root(ValidationCapture& v):validation(v){}
    VkInstance instance{};VkPhysicalDevice physical{};VkDevice device{};VkQueue queue{};uint32_t family{},bits{};
    VkPhysicalDeviceProperties properties{};VkDescriptorSetLayout setLayout{};VkPipelineLayout layout{};VkQueryPool query{};
    ~Root(){if(device){vkDeviceWaitIdle(device);if(query)vkDestroyQueryPool(device,query,nullptr);if(layout)vkDestroyPipelineLayout(device,layout,nullptr);if(setLayout)vkDestroyDescriptorSetLayout(device,setLayout,nullptr);vkDestroyDevice(device,nullptr);}if(instance){validation.detach(instance);vkDestroyInstance(instance,nullptr);}}
    void init(bool validated,bool timeline=false){
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.pApplicationName="Mavk Core A/B";app.apiVersion=timeline?VK_API_VERSION_1_2:VK_API_VERSION_1_1;
        VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};ici.pApplicationInfo=&app;validation.create(ici,instance,validated);
        uint32_t n=0;ok(vkEnumeratePhysicalDevices(instance,&n,nullptr),"physical count");std::vector<VkPhysicalDevice> devices(n);ok(vkEnumeratePhysicalDevices(instance,&n,devices.data()),"physical devices");
        for(auto p:devices){VkPhysicalDeviceProperties prop;vkGetPhysicalDeviceProperties(p,&prop);if(prop.vendorID==0x13b5&&prop.deviceType!=VK_PHYSICAL_DEVICE_TYPE_CPU){physical=p;properties=prop;break;}}
        if(!physical)throw std::runtime_error("ARM hardware required; no CPU fallback");
        VkPhysicalDeviceTimelineSemaphoreFeatures timelineFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES};
        if(timeline){
            if(properties.apiVersion<VK_API_VERSION_1_2)throw std::runtime_error("UNSUPPORTED: native Vulkan1.2 timeline path");
            VkPhysicalDeviceFeatures2 features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};features.pNext=&timelineFeatures;
            vkGetPhysicalDeviceFeatures2(physical,&features);
            if(!timelineFeatures.timelineSemaphore)throw std::runtime_error("UNSUPPORTED: timelineSemaphore hardware feature");
        }
        vkGetPhysicalDeviceQueueFamilyProperties(physical,&n,nullptr);std::vector<VkQueueFamilyProperties> queues(n);vkGetPhysicalDeviceQueueFamilyProperties(physical,&n,queues.data());
        bool found=false;for(uint32_t i=0;i<n;i++)if((queues[i].queueFlags&(VK_QUEUE_COMPUTE_BIT|VK_QUEUE_GRAPHICS_BIT))==(VK_QUEUE_COMPUTE_BIT|VK_QUEUE_GRAPHICS_BIT)){family=i;bits=queues[i].timestampValidBits;found=true;break;}
        if(!found||!bits)throw std::runtime_error("Compute queue with GPU timestamps required");
        float priority=1;VkDeviceQueueCreateInfo q{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};q.queueFamilyIndex=family;q.queueCount=1;q.pQueuePriorities=&priority;
        VkDeviceCreateInfo ci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};ci.queueCreateInfoCount=1;ci.pQueueCreateInfos=&q;if(timeline)ci.pNext=&timelineFeatures;ok(vkCreateDevice(physical,&ci,nullptr,&device),"device");vkGetDeviceQueue(device,family,0,&queue);
        VkDescriptorSetLayoutBinding binding{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr};VkDescriptorSetLayoutCreateInfo sl{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};sl.bindingCount=1;sl.pBindings=&binding;ok(vkCreateDescriptorSetLayout(device,&sl,nullptr,&setLayout),"descriptor layout");
        VkPushConstantRange range{VK_SHADER_STAGE_COMPUTE_BIT,0,4};VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};pl.setLayoutCount=1;pl.pSetLayouts=&setLayout;pl.pushConstantRangeCount=1;pl.pPushConstantRanges=&range;ok(vkCreatePipelineLayout(device,&pl,nullptr,&layout),"layout");
        VkQueryPoolCreateInfo qp{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};qp.queryType=VK_QUERY_TYPE_TIMESTAMP;qp.queryCount=2;ok(vkCreateQueryPool(device,&qp,nullptr,&query),"query");
    }
};
#include "texture_comparison.hpp"
#include "bc_comparison.hpp"
#include "bc_channels_comparison.hpp"
#include "bc67_comparison.hpp"
#include "bc_texture_comparison.hpp"
#include "null_buffers_comparison.hpp"
#include "null_images_comparison.hpp"
#include "timeline_comparison.hpp"
#include "reuse_comparison.hpp"
#include "raster_comparison.hpp"
#include "ui_wire.hpp"
#include "ui_comparison.hpp"
// Both arms share shader/layout/device, barriers, verification and timestamp scope.
// Owned resources are retired only after queue completion, including error paths.
struct Arm {
    Root& r;mavk::DeviceContext& context;bool core;
    std::unique_ptr<mavk::Buffer> cb;std::unique_ptr<mavk::Image> ci;
    std::unique_ptr<mavk::Shader> cs;std::unique_ptr<mavk::Pipeline> cp;
    std::unique_ptr<mavk::Fence> cf;std::unique_ptr<mavk::CommandBatch> cc;
    VkBuffer buffer{};VkDeviceMemory memory{},imageMemory{};void* mapped{};uint32_t type{};
    VkImage image{};VkShaderModule shader{};VkPipeline pipeline{};VkFence fence{};VkCommandPool pool{};VkCommandBuffer command{};VkDescriptorPool descriptors{};VkDescriptorSet set{};
    Arm(Root& root,mavk::DeviceContext& c,bool isCore):r(root),context(c),core(isCore){}
    ~Arm(){
        vkDeviceWaitIdle(r.device);
        if(descriptors)vkDestroyDescriptorPool(r.device,descriptors,nullptr);
        if(!core){if(pool)vkDestroyCommandPool(r.device,pool,nullptr);if(fence)vkDestroyFence(r.device,fence,nullptr);if(pipeline)vkDestroyPipeline(r.device,pipeline,nullptr);if(shader)vkDestroyShaderModule(r.device,shader,nullptr);if(mapped)vkUnmapMemory(r.device,memory);if(buffer)vkDestroyBuffer(r.device,buffer,nullptr);if(memory)vkFreeMemory(r.device,memory,nullptr);if(image)vkDestroyImage(r.device,image,nullptr);if(imageMemory)vkFreeMemory(r.device,imageMemory,nullptr);}
    }
    void resources(VkDeviceSize bytes,VkBufferUsageFlags usage){
        if(core){cb=std::make_unique<mavk::Buffer>(context,bytes,usage,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);buffer=cb->handle();mapped=cb->map();type=cb->memoryType();cf=std::make_unique<mavk::Fence>(context);fence=cf->handle();cc=std::make_unique<mavk::CommandBatch>(context);command=cc->handle();}
        else {
            VkBufferCreateInfo b{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};b.size=bytes;b.usage=usage;ok(vkCreateBuffer(r.device,&b,nullptr,&buffer),"buffer");VkMemoryRequirements req;vkGetBufferMemoryRequirements(r.device,buffer,&req);type=context.memoryType(req.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};ai.allocationSize=req.size;ai.memoryTypeIndex=type;ok(vkAllocateMemory(r.device,&ai,nullptr,&memory),"buffer allocation");ok(vkBindBufferMemory(r.device,buffer,memory,0),"bind buffer");ok(vkMapMemory(r.device,memory,0,VK_WHOLE_SIZE,0,&mapped),"map");
            VkFenceCreateInfo f{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};ok(vkCreateFence(r.device,&f,nullptr,&fence),"fence");VkCommandPoolCreateInfo p{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};p.queueFamilyIndex=r.family;p.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;ok(vkCreateCommandPool(r.device,&p,nullptr,&pool),"pool");VkCommandBufferAllocateInfo ca{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ca.commandPool=pool;ca.commandBufferCount=1;ca.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ok(vkAllocateCommandBuffers(r.device,&ca,&command),"command");
        }
    }
    void compute(const std::vector<uint32_t>& code,uint32_t words){
        resources(words*4,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        if(core){cs=std::make_unique<mavk::Shader>(context,code.data(),code.size()*4);shader=cs->handle();}
        else {VkShaderModuleCreateInfo s{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};s.codeSize=code.size()*4;s.pCode=code.data();ok(vkCreateShaderModule(r.device,&s,nullptr,&shader),"shader");}
        VkComputePipelineCreateInfo p{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};p.layout=r.layout;p.stage={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};p.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;p.stage.module=shader;p.stage.pName="main";
        if(core){cp=std::make_unique<mavk::Pipeline>(context,p);pipeline=cp->handle();}else ok(vkCreateComputePipelines(r.device,VK_NULL_HANDLE,1,&p,nullptr,&pipeline),"pipeline");
        VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1};VkDescriptorPoolCreateInfo dp{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};dp.maxSets=1;dp.poolSizeCount=1;dp.pPoolSizes=&size;ok(vkCreateDescriptorPool(r.device,&dp,nullptr,&descriptors),"descriptor pool");VkDescriptorSetAllocateInfo da{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};da.descriptorPool=descriptors;da.descriptorSetCount=1;da.pSetLayouts=&r.setLayout;ok(vkAllocateDescriptorSets(r.device,&da,&set),"set");VkDescriptorBufferInfo bi{buffer,0,words*4};VkWriteDescriptorSet wr{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};wr.dstSet=set;wr.descriptorCount=1;wr.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;wr.pBufferInfo=&bi;vkUpdateDescriptorSets(r.device,1,&wr,0,nullptr);
    }
    void imageResources(uint32_t w,uint32_t h){
        resources(uint64_t(w)*h*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        VkImageCreateInfo i{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};i.imageType=VK_IMAGE_TYPE_2D;i.format=VK_FORMAT_R8G8B8A8_UNORM;i.extent={w,h,1};i.mipLevels=1;i.arrayLayers=1;i.samples=VK_SAMPLE_COUNT_1_BIT;i.tiling=VK_IMAGE_TILING_OPTIMAL;i.usage=VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        if(core){ci=std::make_unique<mavk::Image>(context,i);image=ci->handle();}
        else{ok(vkCreateImage(r.device,&i,nullptr,&image),"image");VkMemoryRequirements req;vkGetImageMemoryRequirements(r.device,image,&req);VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=req.size;a.memoryTypeIndex=context.memoryType(req.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);ok(vkAllocateMemory(r.device,&a,nullptr,&imageMemory),"image allocation");ok(vkBindImageMemory(r.device,image,imageMemory,0),"bind image");}
    }
    void begin(){
        if(core){cf->reset();cc->begin();}else{ok(vkResetFences(r.device,1,&fence),"reset fence");ok(vkResetCommandBuffer(command,0),"reset command");VkCommandBufferBeginInfo b{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};b.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;ok(vkBeginCommandBuffer(command,&b),"begin");}
        vkCmdResetQueryPool(command,r.query,0,2);vkCmdWriteTimestamp(command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,r.query,0);
    }
    void hostBarrier(VkPipelineStageFlags stage,VkAccessFlags access){if(core)cc->hostReadBarrier(stage,access);else{VkMemoryBarrier b{VK_STRUCTURE_TYPE_MEMORY_BARRIER};b.srcAccessMask=access;b.dstAccessMask=VK_ACCESS_HOST_READ_BIT;vkCmdPipelineBarrier(command,stage,VK_PIPELINE_STAGE_HOST_BIT,0,1,&b,0,nullptr,0,nullptr);}}
    struct Timing{double record,submit,wait,gpu,verify;};
    Timing finish(Clock::time_point start){
        vkCmdWriteTimestamp(command,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,r.query,1);if(core)cc->end();else ok(vkEndCommandBuffer(command),"end");auto ready=Clock::now();VkSubmitInfo s{VK_STRUCTURE_TYPE_SUBMIT_INFO};s.commandBufferCount=1;s.pCommandBuffers=&command;
        if(core)ok(context.submit(r.queue,s,fence),"Core submit");else ok(vkQueueSubmit(r.queue,1,&s,fence),"direct submit");auto submitted=Clock::now();
        ok(core?cf->wait(5'000'000'000ull):vkWaitForFences(r.device,1,&fence,VK_TRUE,5'000'000'000ull),"bounded fence");auto waited=Clock::now();
        if(core)cb->invalidate();else if(!(context.memoryFlags(type)&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};range.memory=memory;range.size=VK_WHOLE_SIZE;ok(vkInvalidateMappedMemoryRanges(r.device,1,&range),"invalidate");}
        uint64_t ticks[2];ok(vkGetQueryPoolResults(r.device,r.query,0,2,sizeof(ticks),ticks,8,VK_QUERY_RESULT_64_BIT),"timestamps");uint64_t mask=r.bits==64?~0ull:((1ull<<r.bits)-1);
        return {ms(start,ready),ms(ready,submitted),ms(submitted,waited),double((ticks[1]-ticks[0])&mask)*r.properties.limits.timestampPeriod/1e6,0};
    }
};
void sample(std::ostream& out,uint32_t round,const Arm::Timing& t){out<<"{\"round\":"<<round<<",\"record_ms\":"<<t.record<<",\"submit_ms\":"<<t.submit<<",\"wait_ms\":"<<t.wait<<",\"gpu_ms\":"<<t.gpu<<",\"verify_ms\":"<<t.verify<<'}';}
std::string run(const std::vector<uint32_t>& code,bool validated,ValidationCapture& validation){
    Root r(validation);r.init(validated);mavk::DeviceContext context(r.physical,r.device,r.family);
    std::ostringstream out;out.precision(9);out<<"{\"schema\":1,\"device\":"<<quote(r.properties.deviceName)<<",\"vendor_id\":"<<r.properties.vendorID<<",\"same_device_queue\":true,\"order\":\"ABBA per size\",\"warmup_rounds\":16,\"phases\":[";
    uint64_t verifiedWords=0,verifiedPixels=0;uint32_t phase=0;
    for(uint32_t words:{4096u,65536u,262144u})for(bool core:{false,true,true,false}){
        if(words/256>r.properties.limits.maxComputeWorkGroupCount[0]||r.properties.limits.maxComputeWorkGroupInvocations<256||words*4>r.properties.limits.maxStorageBufferRange)throw std::runtime_error("Workload exceeds hardware limits");
        if(phase++)out<<',';out<<"{\"kind\":\"compute\",\"core\":"<<(core?"true":"false")<<",\"words\":"<<words;
        auto created=Clock::now();auto arm=std::make_unique<Arm>(r,context,core);arm->compute(code,words);out<<",\"create_ms\":"<<ms(created,Clock::now())<<",\"memory_type\":"<<arm->type<<",\"samples\":[";
        for(uint32_t round=0;round<128;round++){
            auto start=Clock::now();arm->begin();vkCmdBindPipeline(arm->command,VK_PIPELINE_BIND_POINT_COMPUTE,arm->pipeline);vkCmdBindDescriptorSets(arm->command,VK_PIPELINE_BIND_POINT_COMPUTE,r.layout,0,1,&arm->set,0,nullptr);uint32_t seed=round*747796405u+2891336453u;vkCmdPushConstants(arm->command,r.layout,VK_SHADER_STAGE_COMPUTE_BIT,0,4,&seed);vkCmdDispatch(arm->command,words/256,1,1);arm->hostBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_WRITE_BIT);auto t=arm->finish(start);auto verifyStart=Clock::now();
            auto data=static_cast<const uint32_t*>(arm->mapped);for(uint32_t i=0;i<words;i++)if(data[i]!=((i*1664525u+1013904223u)^seed))throw std::runtime_error("Compute mismatch");verifiedWords+=words;t.verify=ms(verifyStart,Clock::now());if(round)out<<',';sample(out,round,t);
        }
        auto teardown=Clock::now();arm.reset();out<<"],\"teardown_ms\":"<<ms(teardown,Clock::now())<<",\"core_live_zero\":"<<(context.counters().empty()?"true":"false")<<'}';if(!context.counters().empty())throw std::runtime_error("Core live resources remain");
    }
    out<<"],\"image_phases\":[";phase=0;
    for(VkExtent2D extent: {VkExtent2D{64,64},VkExtent2D{640,360},VkExtent2D{1920,1200}})for(bool core:{false,true,true,false}){
        if(phase++)out<<',';auto arm=std::make_unique<Arm>(r,context,core);arm->imageResources(extent.width,extent.height);validation.probeSynchronization(arm->command,arm->buffer);
        out<<"{\"core\":"<<(core?"true":"false")<<",\"width\":"<<extent.width<<",\"height\":"<<extent.height<<",\"samples\":[";
        for(uint32_t round=0;round<8;round++){
            auto start=Clock::now();arm->begin();VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};b.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;b.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;b.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;b.image=arm->image;b.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};vkCmdPipelineBarrier(arm->command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&b);
            // Exact 0/1 UNORM values avoid ambiguous quantization across devices.
            VkClearColorValue color{};color.float32[round%3]=1;color.float32[3]=1;vkCmdClearColorImage(arm->command,arm->image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&color,1,&b.subresourceRange);
            b.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;b.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;b.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;b.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;vkCmdPipelineBarrier(arm->command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&b);
            VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={extent.width,extent.height,1};vkCmdCopyImageToBuffer(arm->command,arm->image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,arm->buffer,1,&copy);arm->hostBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_TRANSFER_WRITE_BIT);auto t=arm->finish(start);auto verifyStart=Clock::now();auto data=static_cast<const uint8_t*>(arm->mapped);
            for(uint64_t p=0;p<uint64_t(extent.width)*extent.height;p++)for(uint32_t c=0;c<4;c++)if(data[p*4+c]!=((c==3||c==round%3)?255:0))throw std::runtime_error("Image transfer pixel mismatch");verifiedPixels+=uint64_t(extent.width)*extent.height;t.verify=ms(verifyStart,Clock::now());if(round)out<<',';sample(out,round,t);
        }
        arm.reset();if(!context.counters().empty())throw std::runtime_error("Image resources remain");out<<"],\"core_live_zero\":true}";
    }
    out<<"],\"verified_words\":"<<verifiedWords<<",\"verified_image_pixels\":"<<verifiedPixels<<",\"core_live_zero\":"<<(context.counters().empty()?"true":"false")<<",\"core_allocations\":"<<context.counters().totalAllocations<<",\"core_submissions\":"<<context.counters().totalSubmissions<<",\"scope\":\"compute and image transfer A/B; graphics pipeline and Surface Core comparison pending; no validation-layer PASS\",\"result\":\"CORE_COMPUTE_AND_IMAGE_AB_PASS\"}";return out.str();
}
}
extern "C" JNIEXPORT jstring JNICALL Java_org_mavk_probe_CoreComparisonActivity_runComparison(JNIEnv* env,jclass,jbyteArray input,jstring destination,jboolean validated){
    ValidationCapture validation;std::string result;try{jsize bytes=env->GetArrayLength(input);if(bytes<20||bytes%4)throw std::runtime_error("Invalid shader bytes");std::vector<uint32_t> shader(bytes/4);env->GetByteArrayRegion(input,0,bytes,(jbyte*)shader.data());if(shader[0]!=0x07230203)throw std::runtime_error("Invalid SPIR-V");result=run(shader,validated,validation);if(validation.errors.load())throw std::runtime_error("Validation errors in actual workload or teardown");}catch(const std::exception& e){result="{\"result\":\"FAIL\",\"error\":"+quote(e.what())+"}";}
    result.insert(result.rfind('}'),validation.fields());
    const char* path=env->GetStringUTFChars(destination,nullptr);{std::ofstream f(path);f<<result;if(!f)result="REPORT_WRITE_FAILED";}env->ReleaseStringUTFChars(destination,path);return env->NewStringUTF(result.size()>1000?"Core compute/image A/B finished. See report; full graphics/synchronization milestone is not yet claimed.":result.c_str());
}

extern "C" JNIEXPORT jstring JNICALL Java_org_mavk_probe_CoreComparisonActivity_runTexture(JNIEnv* env,jclass,jbyteArray vertex,jbyteArray fragment,jstring destination,jboolean validated){
    ValidationCapture validation;std::string result;
    try{
        auto shader=[&](jbyteArray input){jsize bytes=env->GetArrayLength(input);if(bytes<20||bytes>65536||bytes%4)throw std::runtime_error("Sampled shader byte count");std::vector<uint32_t> words(bytes/4);env->GetByteArrayRegion(input,0,bytes,reinterpret_cast<jbyte*>(words.data()));return words;};
        result=sampledGate(shader(vertex),shader(fragment),validated,validation);
        if(validation.errors.load()||validation.warnings.load())throw std::runtime_error("Sampled validation errors/warnings");
    }catch(const std::exception& e){result="{\"result\":\"FAIL\",\"error\":"+quote(e.what())+"}";}
    result.insert(result.rfind('}'),validation.fields());
    const char* path=env->GetStringUTFChars(destination,nullptr);{std::ofstream f(path);f<<result;if(!f)result="REPORT_WRITE_FAILED";}env->ReleaseStringUTFChars(destination,path);return env->NewStringUTF(result.c_str());
}

extern "C" JNIEXPORT jstring JNICALL Java_org_mavk_probe_CoreComparisonActivity_runBc(JNIEnv* env,jclass,jbyteArray vertex,jbyteArray fragment,jstring destination,jboolean validated){
    ValidationCapture validation;std::string result;
    try{
        auto shader=[&](jbyteArray input){jsize bytes=env->GetArrayLength(input);if(bytes<20||bytes>65536||bytes%4)throw std::runtime_error("BC shader byte count");std::vector<uint32_t> words(bytes/4);env->GetByteArrayRegion(input,0,bytes,reinterpret_cast<jbyte*>(words.data()));return words;};
        result=bcGate(shader(vertex),shader(fragment),validated,validation);
        if(validation.errors.load()||validation.warnings.load())throw std::runtime_error("BC validation errors/warnings");
    }catch(const std::exception& e){result="{\"result\":\"FAIL\",\"error\":"+quote(e.what())+"}";}
    result.insert(result.rfind('}'),validation.fields());
    const char* path=env->GetStringUTFChars(destination,nullptr);{std::ofstream f(path);f<<result;if(!f)result="REPORT_WRITE_FAILED";}env->ReleaseStringUTFChars(destination,path);return env->NewStringUTF(result.c_str());
}

extern "C" JNIEXPORT jstring JNICALL Java_org_mavk_probe_CoreComparisonActivity_runBcChannels(JNIEnv* env,jclass,jbyteArray vertex,jbyteArray fragment,jstring destination,jboolean validated){
    ValidationCapture validation;std::string result;
    try{
        auto shader=[&](jbyteArray input){jsize bytes=env->GetArrayLength(input);if(bytes<20||bytes>65536||bytes%4)throw std::runtime_error("BC45 shader byte count");std::vector<uint32_t> words(bytes/4);env->GetByteArrayRegion(input,0,bytes,reinterpret_cast<jbyte*>(words.data()));return words;};
        result=bcChannelsGate(shader(vertex),shader(fragment),validated,validation);
        if(validation.errors.load()||validation.warnings.load())throw std::runtime_error("BC45 validation errors/warnings");
    }catch(const std::exception& e){result="{\"result\":\"FAIL\",\"error\":"+quote(e.what())+"}";}
    result.insert(result.rfind('}'),validation.fields());
    const char* path=env->GetStringUTFChars(destination,nullptr);{std::ofstream f(path);f<<result;if(!f)result="REPORT_WRITE_FAILED";}env->ReleaseStringUTFChars(destination,path);return env->NewStringUTF(result.c_str());
}

extern "C" JNIEXPORT jstring JNICALL Java_org_mavk_probe_CoreComparisonActivity_runBc67(JNIEnv* env,jclass,jbyteArray vertex,jbyteArray fragment,jbyteArray hdrFragment,jstring destination,jboolean validated){
    ValidationCapture validation;std::string result;
    try{
        auto shader=[&](jbyteArray input){jsize bytes=env->GetArrayLength(input);if(bytes<20||bytes>65536||bytes%4)throw std::runtime_error("BC67 shader byte count");std::vector<uint32_t> words(bytes/4);env->GetByteArrayRegion(input,0,bytes,reinterpret_cast<jbyte*>(words.data()));return words;};
        result=bc67Gate(shader(vertex),shader(fragment),shader(hdrFragment),validated,validation);
        if(validation.errors.load()||validation.warnings.load())throw std::runtime_error("BC67 validation errors/warnings");
    }catch(const std::exception& e){result="{\"result\":\"FAIL\",\"error\":"+quote(e.what())+"}";}
    result.insert(result.rfind('}'),validation.fields());
    const char* path=env->GetStringUTFChars(destination,nullptr);{std::ofstream f(path);f<<result;if(!f)result="REPORT_WRITE_FAILED";}env->ReleaseStringUTFChars(destination,path);return env->NewStringUTF(result.c_str());
}

extern "C" JNIEXPORT jstring JNICALL Java_org_mavk_probe_CoreComparisonActivity_runBcTexture(JNIEnv* env,jclass,jbyteArray vertex,jbyteArray fragment,jstring destination,jboolean validated){
    ValidationCapture validation;std::string result;
    try{
        auto shader=[&](jbyteArray input){jsize bytes=env->GetArrayLength(input);if(bytes<20||bytes>65536||bytes%4)throw std::runtime_error("BC texture shader byte count");std::vector<uint32_t> words(bytes/4);env->GetByteArrayRegion(input,0,bytes,reinterpret_cast<jbyte*>(words.data()));return words;};
        result=bcTextureGate(shader(vertex),shader(fragment),validated,validation);
        if(validation.errors.load()||validation.warnings.load())throw std::runtime_error("BC texture validation errors/warnings");
    }catch(const std::exception& e){result="{\"result\":\"FAIL\",\"error\":"+quote(e.what())+"}";}
    result.insert(result.rfind('}'),validation.fields());
    const char* path=env->GetStringUTFChars(destination,nullptr);{std::ofstream f(path);f<<result;if(!f)result="REPORT_WRITE_FAILED";}env->ReleaseStringUTFChars(destination,path);return env->NewStringUTF(result.c_str());
}

extern "C" JNIEXPORT jstring JNICALL Java_org_mavk_probe_CoreComparisonActivity_runTimeline(JNIEnv* env,jclass,jstring destination,jboolean validated){
    ValidationCapture validation;std::string result;
    try{result=timelineGate(validated,validation);if(validation.errors.load()||validation.warnings.load())throw std::runtime_error("Timeline validation errors/warnings");}
    catch(const std::exception& e){result="{\"result\":\"FAIL\",\"error\":"+quote(e.what())+"}";}
    result.insert(result.rfind('}'),validation.fields());
    const char* path=env->GetStringUTFChars(destination,nullptr);{std::ofstream f(path);f<<result;if(!f)result="REPORT_WRITE_FAILED";}env->ReleaseStringUTFChars(destination,path);return env->NewStringUTF(result.c_str());
}

extern "C" JNIEXPORT jstring JNICALL Java_org_mavk_probe_CoreComparisonActivity_runReuse(JNIEnv* env,jclass,jbyteArray vertex,jbyteArray fragment,jstring destination,jboolean validated){
    ValidationCapture validation;std::string result;
    try{
        auto shader=[&](jbyteArray input){jsize bytes=env->GetArrayLength(input);if(bytes<20||bytes>65536||bytes%4)throw std::runtime_error("Reuse shader byte count");std::vector<uint32_t> words(bytes/4);env->GetByteArrayRegion(input,0,bytes,reinterpret_cast<jbyte*>(words.data()));return words;};
        result=reuseGate(shader(vertex),shader(fragment),validated,validation);if(validation.errors.load()||validation.warnings.load())throw std::runtime_error("Reuse validation errors/warnings");
    }catch(const std::exception& e){result="{\"result\":\"FAIL\",\"error\":"+quote(e.what())+"}";}
    result.insert(result.rfind('}'),validation.fields());
    const char* path=env->GetStringUTFChars(destination,nullptr);{std::ofstream f(path);f<<result;if(!f)result="REPORT_WRITE_FAILED";}env->ReleaseStringUTFChars(destination,path);return env->NewStringUTF(result.c_str());
}

extern "C" JNIEXPORT jstring JNICALL Java_org_mavk_probe_CoreComparisonActivity_runRaster(JNIEnv* env,jclass,jbyteArray vertex,jbyteArray fragment,jstring destination,jboolean validated){
    ValidationCapture validation;std::string result;
    try{
        auto shader=[&](jbyteArray input){jsize bytes=env->GetArrayLength(input);if(bytes<20||bytes>65536||bytes%4)throw std::runtime_error("Raster shader byte count");std::vector<uint32_t> words(bytes/4);env->GetByteArrayRegion(input,0,bytes,reinterpret_cast<jbyte*>(words.data()));return words;};
        result=rasterGate(shader(vertex),shader(fragment),validated,validation);if(validation.errors.load()||validation.warnings.load())throw std::runtime_error("Raster validation errors/warnings");
    }catch(const std::exception& e){result="{\"result\":\"FAIL\",\"error\":"+quote(e.what())+"}";}
    result.insert(result.rfind('}'),validation.fields());
    const char* path=env->GetStringUTFChars(destination,nullptr);{std::ofstream f(path);f<<result;if(!f)result="REPORT_WRITE_FAILED";}env->ReleaseStringUTFChars(destination,path);return env->NewStringUTF(result.c_str());
}

extern "C" JNIEXPORT jstring JNICALL Java_org_mavk_probe_CoreComparisonActivity_runUi(JNIEnv* env,jclass,jbyteArray vertex,jbyteArray fragment,jstring destination,jboolean validated){
    ValidationCapture validation;std::string result;
    try{
#ifdef MAVK_NATIVE_UI
        auto shader=[&](jbyteArray input){jsize bytes=env->GetArrayLength(input);if(bytes<20||bytes>65536||bytes%4)throw std::runtime_error("UI shader byte count");std::vector<uint32_t> words(bytes/4);env->GetByteArrayRegion(input,0,bytes,reinterpret_cast<jbyte*>(words.data()));return words;};
        result=uiGate(shader(vertex),shader(fragment),validated,validation);if(validation.errors.load()||validation.warnings.load())throw std::runtime_error("UI validation errors/warnings");
#else
        (void)vertex;(void)fragment;(void)validated;throw std::runtime_error("NOT_BUILT: optional pinned upstream UI dependency");
#endif
    }catch(const std::exception& e){result="{\"result\":\"FAIL\",\"error\":"+quote(e.what())+"}";}
    result.insert(result.rfind('}'),validation.fields());
    const char* path=env->GetStringUTFChars(destination,nullptr);{std::ofstream f(path);f<<result;if(!f)result="REPORT_WRITE_FAILED";}env->ReleaseStringUTFChars(destination,path);return env->NewStringUTF(result.c_str());
}

extern "C" JNIEXPORT jstring JNICALL Java_org_mavk_probe_CoreComparisonActivity_runUiLayers(JNIEnv* env,jclass,jbyteArray vertex,jbyteArray fragment,jbyteArray backgroundVertex,jbyteArray backgroundFragment,jstring destination,jboolean validated){
    ValidationCapture validation;std::string result;
    try{
#ifdef MAVK_NATIVE_UI
        auto shader=[&](jbyteArray input){jsize bytes=env->GetArrayLength(input);if(bytes<20||bytes>65536||bytes%4)throw std::runtime_error("Layer shader byte count");std::vector<uint32_t> words(bytes/4);env->GetByteArrayRegion(input,0,bytes,reinterpret_cast<jbyte*>(words.data()));return words;};
        auto bv=shader(backgroundVertex),bf=shader(backgroundFragment);result=uiGate(shader(vertex),shader(fragment),validated,validation,&bv,&bf);if(validation.errors.load()||validation.warnings.load())throw std::runtime_error("Layer validation errors/warnings");
#else
        (void)vertex;(void)fragment;(void)backgroundVertex;(void)backgroundFragment;(void)validated;throw std::runtime_error("NOT_BUILT: optional pinned upstream UI dependency");
#endif
    }catch(const std::exception& e){result="{\"result\":\"FAIL\",\"error\":"+quote(e.what())+"}";}
    result.insert(result.rfind('}'),validation.fields());
    const char* path=env->GetStringUTFChars(destination,nullptr);{std::ofstream f(path);f<<result;if(!f)result="REPORT_WRITE_FAILED";}env->ReleaseStringUTFChars(destination,path);return env->NewStringUTF(result.c_str());
}

extern "C" JNIEXPORT jstring JNICALL Java_org_mavk_probe_CoreComparisonActivity_runUiBridge(JNIEnv* env,jclass,jbyteArray vertex,jbyteArray fragment,jbyteArray backgroundVertex,jbyteArray backgroundFragment,jbyteArray compute,jstring destination,jboolean validated){
    ValidationCapture validation;std::string result;
    try{
#ifdef MAVK_NATIVE_UI
        auto shader=[&](jbyteArray input){jsize bytes=env->GetArrayLength(input);if(bytes<20||bytes>65536||bytes%4)throw std::runtime_error("UI Bridge shader byte count");std::vector<uint32_t> words(bytes/4);env->GetByteArrayRegion(input,0,bytes,reinterpret_cast<jbyte*>(words.data()));return words;};
        auto bv=shader(backgroundVertex),bf=shader(backgroundFragment),cs=shader(compute);result=uiGate(shader(vertex),shader(fragment),validated,validation,&bv,&bf,&cs);if(validation.errors.load()||validation.warnings.load())throw std::runtime_error("UI Bridge validation errors/warnings");
#else
        (void)vertex;(void)fragment;(void)backgroundVertex;(void)backgroundFragment;(void)compute;(void)validated;throw std::runtime_error("NOT_BUILT: optional pinned upstream UI dependency");
#endif
    }catch(const std::exception& e){result="{\"result\":\"FAIL\",\"error\":"+quote(e.what())+"}";}
    result.insert(result.rfind('}'),validation.fields());
    const char* path=env->GetStringUTFChars(destination,nullptr);{std::ofstream f(path);f<<result;if(!f)result="REPORT_WRITE_FAILED";}env->ReleaseStringUTFChars(destination,path);return env->NewStringUTF(result.c_str());
}

extern "C" JNIEXPORT jstring JNICALL Java_org_mavk_probe_CoreComparisonActivity_runNullBuffers(JNIEnv* env,jclass,jbyteArray input,jstring destination,jboolean validated){
    ValidationCapture validation;std::string result;
    try{jsize bytes=env->GetArrayLength(input);if(bytes<20||bytes>65536||bytes%4)throw std::runtime_error("null shader bytes");std::vector<uint32_t> words(bytes/4);env->GetByteArrayRegion(input,0,bytes,reinterpret_cast<jbyte*>(words.data()));result=nullBuffersGate(words,validated,validation);if(validation.errors.load()||validation.warnings.load())throw std::runtime_error("null validation errors/warnings");}
    catch(const std::exception& e){result="{\"result\":\"FAIL\",\"error\":"+quote(e.what())+"}";}
    result.insert(result.rfind('}'),validation.fields());const char* path=env->GetStringUTFChars(destination,nullptr);{std::ofstream f(path);f<<result;if(!f)result="REPORT_WRITE_FAILED";}env->ReleaseStringUTFChars(destination,path);return env->NewStringUTF(result.c_str());
}

extern "C" JNIEXPORT jstring JNICALL Java_org_mavk_probe_CoreComparisonActivity_runNullImages(JNIEnv* env,jclass,jbyteArray input,jstring destination,jboolean validated){
    ValidationCapture validation;std::string result;
    try{jsize bytes=env->GetArrayLength(input);if(bytes<20||bytes>65536||bytes%4)throw std::runtime_error("null image shader bytes");std::vector<uint32_t> words(bytes/4);env->GetByteArrayRegion(input,0,bytes,reinterpret_cast<jbyte*>(words.data()));result=nullImagesGate(words,validated,validation);if(validation.errors.load()||validation.warnings.load())throw std::runtime_error("null image validation errors/warnings");}
    catch(const std::exception& e){result="{\"result\":\"FAIL\",\"error\":"+quote(e.what())+"}";}
    result.insert(result.rfind('}'),validation.fields());const char* path=env->GetStringUTFChars(destination,nullptr);{std::ofstream f(path);f<<result;if(!f)result="REPORT_WRITE_FAILED";}env->ReleaseStringUTFChars(destination,path);return env->NewStringUTF(result.c_str());
}

extern "C" JNIEXPORT jstring JNICALL Java_org_mavk_probe_CoreComparisonActivity_runNullAtomics(JNIEnv* env,jclass,jbyteArray input,jstring destination,jboolean validated){
    ValidationCapture validation;std::string result;
    try{jsize bytes=env->GetArrayLength(input);if(bytes<20||bytes>65536||bytes%4)throw std::runtime_error("null atomic shader bytes");std::vector<uint32_t> words(bytes/4);env->GetByteArrayRegion(input,0,bytes,reinterpret_cast<jbyte*>(words.data()));result=nullBuffersGate(words,validated,validation,true);if(validation.errors.load()||validation.warnings.load())throw std::runtime_error("null atomic validation errors/warnings");}
    catch(const std::exception& e){result="{\"result\":\"FAIL\",\"error\":"+quote(e.what())+"}";}
    result.insert(result.rfind('}'),validation.fields());const char* path=env->GetStringUTFChars(destination,nullptr);{std::ofstream f(path);f<<result;if(!f)result="REPORT_WRITE_FAILED";}env->ReleaseStringUTFChars(destination,path);return env->NewStringUTF(result.c_str());
}

namespace {
struct BridgeOwner {
    ValidationCapture validation;
    std::unique_ptr<Root> root;
    std::unique_ptr<mavk::DeviceContext> context;
    std::unique_ptr<mavk::bridge::Session> session;
    explicit BridgeOwner(const std::vector<uint32_t>& code,bool validated){
        root=std::make_unique<Root>(validation);root->init(validated);
        const auto& l=root->properties.limits;
        if(l.maxComputeWorkGroupCount[0]<1024||l.maxComputeWorkGroupSize[0]<256||l.maxComputeWorkGroupInvocations<256||l.maxStorageBufferRange<mavk::bridge::MaxPayload)throw std::runtime_error("Bridge slice exceeds hardware limits");
        context=std::make_unique<mavk::DeviceContext>(root->physical,root->device,root->family);
        session=std::make_unique<mavk::bridge::Session>(*context,root->queue,code.data(),code.size()*4);
    }
};
void javaError(JNIEnv* env,const char* text){env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),text);}
}
extern "C" JNIEXPORT jlong JNICALL Java_org_mavk_probe_BridgeActivity_createBridge(JNIEnv* env,jclass,jbyteArray input,jboolean validated){
    try{jsize n=env->GetArrayLength(input);if(n<20||n%4)throw std::runtime_error("Invalid trusted shader bytes");std::vector<uint32_t> code(n/4);env->GetByteArrayRegion(input,0,n,(jbyte*)code.data());return reinterpret_cast<jlong>(new BridgeOwner(code,validated));}
    catch(const std::exception& e){javaError(env,e.what());return 0;}
}
extern "C" JNIEXPORT jbyteArray JNICALL Java_org_mavk_probe_BridgeActivity_exchangeBridge(JNIEnv* env,jclass,jlong handle,jbyteArray input){
    try{jsize n=env->GetArrayLength(input);if(n<0||uint32_t(n)>mavk::bridge::MaxPayload+mavk::bridge::HeaderBytes)throw std::runtime_error("Bridge frame too large");std::vector<uint8_t> request(n);env->GetByteArrayRegion(input,0,n,(jbyte*)request.data());auto response=reinterpret_cast<BridgeOwner*>(handle)->session->exchange(request);jbyteArray result=env->NewByteArray(jsize(response.size()));if(result)env->SetByteArrayRegion(result,0,jsize(response.size()),(jbyte*)response.data());return result;}
    catch(const std::exception& e){javaError(env,e.what());return nullptr;}
}
extern "C" JNIEXPORT jstring JNICALL Java_org_mavk_probe_BridgeActivity_finishBridge(JNIEnv* env,jclass,jlong handle){
    std::unique_ptr<BridgeOwner> owner(reinterpret_cast<BridgeOwner*>(handle));owner->session.reset();bool zero=owner->context->counters().empty();uint64_t submitted=owner->context->counters().totalSubmissions,created=owner->context->counters().totalPipelineCreates;std::string device=owner->root->properties.deviceName;owner->context.reset();owner->root.reset();
    std::string text="{\"device\":"+quote(device)+",\"core_live_zero_after_disconnect\":"+(zero?"true":"false")+",\"gpu_submissions\":"+std::to_string(submitted)+",\"pipeline_creates\":"+std::to_string(created)+owner->validation.fields()+"}";return env->NewStringUTF(text.c_str());
}
extern "C" JNIEXPORT jlong JNICALL Java_org_mavk_bridge_BridgeNative_create(JNIEnv* env,jclass cls,jbyteArray shader,jboolean validation){return Java_org_mavk_probe_BridgeActivity_createBridge(env,cls,shader,validation);}
extern "C" JNIEXPORT jbyteArray JNICALL Java_org_mavk_bridge_BridgeNative_exchange(JNIEnv* env,jclass cls,jlong handle,jbyteArray frame){return Java_org_mavk_probe_BridgeActivity_exchangeBridge(env,cls,handle,frame);}
extern "C" JNIEXPORT jstring JNICALL Java_org_mavk_bridge_BridgeNative_finish(JNIEnv* env,jclass cls,jlong handle){return Java_org_mavk_probe_BridgeActivity_finishBridge(env,cls,handle);}
