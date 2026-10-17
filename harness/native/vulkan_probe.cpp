#include <jni.h>
#include <vulkan/vulkan.h>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {
using Clock=std::chrono::steady_clock;
double ms(Clock::time_point a,Clock::time_point b){return std::chrono::duration<double,std::milli>(b-a).count();}
void check(VkResult value,const char* operation){if(value!=VK_SUCCESS)throw std::runtime_error(std::string(operation)+": "+std::to_string(value));}
std::string quote(const char* value){std::ostringstream o;o<<'"';for(const unsigned char* p=(const unsigned char*)value;*p;p++){
    if(*p=='"'||*p=='\\')o<<'\\'<<*p;else if(*p<32)o<<"\\u"<<std::hex<<std::setw(4)<<std::setfill('0')<<(unsigned)*p;else o<<*p;}o<<'"';return o.str();}
std::string version(uint32_t v){return std::to_string(VK_API_VERSION_MAJOR(v))+"."+std::to_string(VK_API_VERSION_MINOR(v))+"."+std::to_string(VK_API_VERSION_PATCH(v));}
struct State {
    VkInstance instance{};VkDevice device{};VkBuffer buffer{};VkDeviceMemory memory{};void* mapped{};
    VkShaderModule shader{};VkDescriptorSetLayout setLayout{};VkPipelineLayout layout{};VkPipeline pipeline{};
    VkDescriptorPool descriptors{};VkCommandPool commands{};VkFence fence{};VkQueryPool queries{};
    ~State(){if(device){vkDeviceWaitIdle(device);if(mapped)vkUnmapMemory(device,memory);
        if(queries)vkDestroyQueryPool(device,queries,nullptr);if(fence)vkDestroyFence(device,fence,nullptr);
        if(commands)vkDestroyCommandPool(device,commands,nullptr);if(pipeline)vkDestroyPipeline(device,pipeline,nullptr);
        if(layout)vkDestroyPipelineLayout(device,layout,nullptr);if(descriptors)vkDestroyDescriptorPool(device,descriptors,nullptr);
        if(setLayout)vkDestroyDescriptorSetLayout(device,setLayout,nullptr);if(shader)vkDestroyShaderModule(device,shader,nullptr);
        if(buffer)vkDestroyBuffer(device,buffer,nullptr);if(memory)vkFreeMemory(device,memory,nullptr);vkDestroyDevice(device,nullptr);}
        if(instance)vkDestroyInstance(instance,nullptr);}
};
std::string probe(const std::vector<uint32_t>& spirv){
    State s;
    uint32_t loader=VK_API_VERSION_1_0;
    auto enumerateVersion=(PFN_vkEnumerateInstanceVersion)vkGetInstanceProcAddr(nullptr,"vkEnumerateInstanceVersion");
    if(enumerateVersion)check(enumerateVersion(&loader),"instance version");
    if(loader<VK_API_VERSION_1_1)throw std::runtime_error("Vulkan1.1 required by this bounded probe");
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.pApplicationName="Mavk M0 native probe";app.apiVersion=VK_API_VERSION_1_1;
    VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};ici.pApplicationInfo=&app;
    check(vkCreateInstance(&ici,nullptr,&s.instance),"create instance");
    uint32_t count=0;check(vkEnumeratePhysicalDevices(s.instance,&count,nullptr),"device count");
    std::vector<VkPhysicalDevice> devices(count);check(vkEnumeratePhysicalDevices(s.instance,&count,devices.data()),"devices");
    VkPhysicalDevice physical{};VkPhysicalDeviceProperties properties{};
    for(auto candidate:devices){VkPhysicalDeviceProperties p;vkGetPhysicalDeviceProperties(candidate,&p);
        if(p.vendorID==0x13b5&&p.deviceType!=VK_PHYSICAL_DEVICE_TYPE_CPU){physical=candidate;properties=p;break;}}
    if(!physical)throw std::runtime_error("No ARM hardware Vulkan physical device; refusing CPU/other fallback");
    uint32_t familyCount=0;vkGetPhysicalDeviceQueueFamilyProperties(physical,&familyCount,nullptr);
    std::vector<VkQueueFamilyProperties> families(familyCount);vkGetPhysicalDeviceQueueFamilyProperties(physical,&familyCount,families.data());
    uint32_t family=familyCount;for(uint32_t i=0;i<familyCount;i++)if(families[i].queueFlags&VK_QUEUE_COMPUTE_BIT){family=i;break;}
    if(family==familyCount)throw std::runtime_error("No compute queue");
    VkPhysicalDeviceFeatures features{};vkGetPhysicalDeviceFeatures(physical,&features);
    VkPhysicalDeviceVulkan12Features v12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    VkPhysicalDeviceVulkan13Features v13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    VkPhysicalDeviceFeatures2 featureQuery{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    if(properties.apiVersion>=VK_API_VERSION_1_2){featureQuery.pNext=&v12;if(properties.apiVersion>=VK_API_VERSION_1_3)v12.pNext=&v13;vkGetPhysicalDeviceFeatures2(physical,&featureQuery);}
    VkPhysicalDeviceMemoryProperties memories{};vkGetPhysicalDeviceMemoryProperties(physical,&memories);
    std::ostringstream out;out<<"{\n\"schema\":1,\"scope\":\"direct native Android Vulkan baseline, NOT guest/backend compatibility PASS\",\n"
        <<"\"loader_api\":"<<quote(version(loader).c_str())<<",\"device\":"<<quote(properties.deviceName)
        <<",\"vendor_id\":"<<properties.vendorID<<",\"device_id\":"<<properties.deviceID<<",\"device_type\":"<<properties.deviceType
        <<",\"device_api\":"<<quote(version(properties.apiVersion).c_str())<<",\"driver_version_raw\":"<<properties.driverVersion
        <<",\"enabled_optional_features\":[],\"enabled_device_extensions\":[],\n\"reported_features\":{"
        <<"\"shaderInt64\":"<<features.shaderInt64<<",\"shaderFloat64\":"<<features.shaderFloat64<<",\"sparseBinding\":"<<features.sparseBinding
        <<",\"timelineSemaphore\":"<<v12.timelineSemaphore<<",\"bufferDeviceAddress\":"<<v12.bufferDeviceAddress
        <<",\"descriptorIndexing\":"<<v12.descriptorIndexing<<",\"dynamicRendering\":"<<v13.dynamicRendering<<",\"synchronization2\":"<<v13.synchronization2<<"},\n\"queue_families\":[";
    for(uint32_t i=0;i<familyCount;i++){if(i)out<<',';out<<"{\"flags\":"<<families[i].queueFlags<<",\"count\":"<<families[i].queueCount<<",\"timestamp_valid_bits\":"<<families[i].timestampValidBits<<'}';}
    out<<"],\"memory_types\":[";for(uint32_t i=0;i<memories.memoryTypeCount;i++){if(i)out<<',';out<<"{\"flags\":"<<memories.memoryTypes[i].propertyFlags<<",\"heap\":"<<memories.memoryTypes[i].heapIndex<<'}';}
    uint32_t extensionCount=0;check(vkEnumerateDeviceExtensionProperties(physical,nullptr,&extensionCount,nullptr),"extension count");
    std::vector<VkExtensionProperties> extensions(extensionCount);check(vkEnumerateDeviceExtensionProperties(physical,nullptr,&extensionCount,extensions.data()),"extensions");
    out<<"],\"device_extensions\":[";for(uint32_t i=0;i<extensionCount;i++){if(i)out<<',';out<<quote(extensions[i].extensionName);}out<<"],\n";
    float priority=1;VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};qci.queueFamilyIndex=family;qci.queueCount=1;qci.pQueuePriorities=&priority;
    VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};dci.queueCreateInfoCount=1;dci.pQueueCreateInfos=&qci;
    check(vkCreateDevice(physical,&dci,nullptr,&s.device),"create device");VkQueue queue{};vkGetDeviceQueue(s.device,family,0,&queue);
    constexpr uint32_t words=65536,rounds=128;
    if(properties.limits.maxComputeWorkGroupInvocations<256||properties.limits.maxComputeWorkGroupSize[0]<256||properties.limits.maxComputeWorkGroupCount[0]<words/256)throw std::runtime_error("Probe dispatch exceeds device limits");
    VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};bci.size=words*4;bci.usage=VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    check(vkCreateBuffer(s.device,&bci,nullptr,&s.buffer),"create buffer");VkMemoryRequirements requirements;vkGetBufferMemoryRequirements(s.device,s.buffer,&requirements);
    uint32_t type=memories.memoryTypeCount;for(uint32_t i=0;i<memories.memoryTypeCount;i++)if((requirements.memoryTypeBits&(1u<<i))&&(memories.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)){type=i;if(memories.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)break;}
    if(type==memories.memoryTypeCount)throw std::runtime_error("No compatible host-visible buffer memory");
    VkMemoryAllocateInfo mai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};mai.allocationSize=requirements.size;mai.memoryTypeIndex=type;
    check(vkAllocateMemory(s.device,&mai,nullptr,&s.memory),"allocate memory");check(vkBindBufferMemory(s.device,s.buffer,s.memory,0),"bind memory");
    check(vkMapMemory(s.device,s.memory,0,VK_WHOLE_SIZE,0,&s.mapped),"map memory");
    VkShaderModuleCreateInfo smci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};smci.codeSize=spirv.size()*4;smci.pCode=spirv.data();check(vkCreateShaderModule(s.device,&smci,nullptr,&s.shader),"shader");
    VkDescriptorSetLayoutBinding binding{};binding.binding=0;binding.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;binding.descriptorCount=1;binding.stageFlags=VK_SHADER_STAGE_COMPUTE_BIT;
    VkDescriptorSetLayoutCreateInfo slci{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};slci.bindingCount=1;slci.pBindings=&binding;check(vkCreateDescriptorSetLayout(s.device,&slci,nullptr,&s.setLayout),"descriptor layout");
    VkPushConstantRange push{VK_SHADER_STAGE_COMPUTE_BIT,0,4};VkPipelineLayoutCreateInfo plci{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};plci.setLayoutCount=1;plci.pSetLayouts=&s.setLayout;plci.pushConstantRangeCount=1;plci.pPushConstantRanges=&push;
    check(vkCreatePipelineLayout(s.device,&plci,nullptr,&s.layout),"pipeline layout");VkComputePipelineCreateInfo pci{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};pci.layout=s.layout;pci.stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;pci.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;pci.stage.module=s.shader;pci.stage.pName="main";
    check(vkCreateComputePipelines(s.device,VK_NULL_HANDLE,1,&pci,nullptr,&s.pipeline),"compute pipeline");
    VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1};VkDescriptorPoolCreateInfo dpci{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};dpci.maxSets=1;dpci.poolSizeCount=1;dpci.pPoolSizes=&poolSize;check(vkCreateDescriptorPool(s.device,&dpci,nullptr,&s.descriptors),"descriptor pool");
    VkDescriptorSetAllocateInfo dsai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};dsai.descriptorPool=s.descriptors;dsai.descriptorSetCount=1;dsai.pSetLayouts=&s.setLayout;VkDescriptorSet set;check(vkAllocateDescriptorSets(s.device,&dsai,&set),"descriptor set");
    VkDescriptorBufferInfo dbi{s.buffer,0,words*4};VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=set;write.descriptorCount=1;write.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;write.pBufferInfo=&dbi;vkUpdateDescriptorSets(s.device,1,&write,0,nullptr);
    VkCommandPoolCreateInfo cpci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};cpci.queueFamilyIndex=family;cpci.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;check(vkCreateCommandPool(s.device,&cpci,nullptr,&s.commands),"command pool");
    VkCommandBufferAllocateInfo cbai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};cbai.commandPool=s.commands;cbai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;cbai.commandBufferCount=1;VkCommandBuffer command;check(vkAllocateCommandBuffers(s.device,&cbai,&command),"command buffer");
    VkFenceCreateInfo fci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};check(vkCreateFence(s.device,&fci,nullptr,&s.fence),"fence");
    uint32_t bits=families[family].timestampValidBits;if(bits){VkQueryPoolCreateInfo qpci{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};qpci.queryType=VK_QUERY_TYPE_TIMESTAMP;qpci.queryCount=2;check(vkCreateQueryPool(s.device,&qpci,nullptr,&s.queries),"timestamp pool");}
    out<<"\"selected_queue\":"<<family<<",\"memory_type\":"<<type<<",\"allocation_bytes\":"<<requirements.size<<",\"words_per_round\":"<<words<<",\"rounds\":"<<rounds<<",\"validation_layers_enabled\":false,\n\"samples\":[";
    for(uint32_t round=0;round<rounds;round++){
        auto start=Clock::now();check(vkResetFences(s.device,1,&s.fence),"reset fence");check(vkResetCommandBuffer(command,0),"reset command");
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;check(vkBeginCommandBuffer(command,&begin),"begin");
        if(bits){vkCmdResetQueryPool(command,s.queries,0,2);vkCmdWriteTimestamp(command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,s.queries,0);}
        vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_COMPUTE,s.pipeline);vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_COMPUTE,s.layout,0,1,&set,0,nullptr);
        uint32_t seed=round*747796405u+2891336453u;vkCmdPushConstants(command,s.layout,VK_SHADER_STAGE_COMPUTE_BIT,0,4,&seed);vkCmdDispatch(command,words/256,1,1);
        VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};barrier.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&barrier,0,nullptr,0,nullptr);
        if(bits)vkCmdWriteTimestamp(command,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,s.queries,1);check(vkEndCommandBuffer(command),"end");
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;auto ready=Clock::now();check(vkQueueSubmit(queue,1,&submit,s.fence),"submit");auto submitted=Clock::now();
        check(vkWaitForFences(s.device,1,&s.fence,VK_TRUE,5'000'000'000ull),"bounded fence wait");auto completed=Clock::now();
        if(!(memories.memoryTypes[type].propertyFlags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};range.memory=s.memory;range.size=VK_WHOLE_SIZE;check(vkInvalidateMappedMemoryRanges(s.device,1,&range),"invalidate");}
        auto* data=(const uint32_t*)s.mapped;for(uint32_t i=0;i<words;i++)if(data[i]!=((i*1664525u+1013904223u)^seed))throw std::runtime_error("Compute mismatch round="+std::to_string(round)+" word="+std::to_string(i));
        uint64_t times[2]{};double gpu=-1;if(bits){check(vkGetQueryPoolResults(s.device,s.queries,0,2,sizeof(times),times,8,VK_QUERY_RESULT_64_BIT),"timestamps");uint64_t mask=bits==64?~0ull:((1ull<<bits)-1);gpu=((times[1]-times[0])&mask)*properties.limits.timestampPeriod/1e6;}
        if(round)out<<',';out<<"{\"round\":"<<round<<",\"record_ms\":"<<ms(start,ready)<<",\"submit_ms\":"<<ms(ready,submitted)<<",\"wait_ms\":"<<ms(submitted,completed)<<",\"verify_ms\":"<<ms(completed,Clock::now())<<",\"gpu_ms\":"<<gpu<<'}';
    }
    check(vkDeviceWaitIdle(s.device),"final idle");out<<"],\"correct_words_checked\":"<<uint64_t(words)*rounds<<",\"result\":\"NATIVE_COMPUTE_CORRECTNESS_PASS\"}\n";return out.str();
}
}
extern "C" JNIEXPORT jstring JNICALL Java_org_mavk_probe_ProbeActivity_runProbe(JNIEnv* env,jclass,jbyteArray bytes,jstring destination){
    std::string result;const char* path=env->GetStringUTFChars(destination,nullptr);
    try{jsize size=env->GetArrayLength(bytes);if(size<20||size%4)throw std::runtime_error("Malformed shader byte count");std::vector<uint32_t> shader(size/4);env->GetByteArrayRegion(bytes,0,size,(jbyte*)shader.data());if(shader[0]!=0x07230203)throw std::runtime_error("Invalid SPIR-V magic");result=probe(shader);}
    catch(const std::exception& e){result="{\"result\":\"FAIL\",\"error\":"+quote(e.what())+"}\n";}
    {std::ofstream report(path);report<<result;if(!report)result="REPORT_WRITE_FAILED";}
    env->ReleaseStringUTFChars(destination,path);return env->NewStringUTF(result.c_str());
}
