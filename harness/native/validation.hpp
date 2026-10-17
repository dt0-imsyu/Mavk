#pragma once
#include <vulkan/vulkan.h>
#include <atomic>
#include <cstring>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <vector>

// Harness-only diagnostics. Never changes Android global layer settings.
struct ValidationCapture {
    bool enabled=false;uint32_t implementationVersion=0;
    std::atomic<uint32_t> errors{0},warnings{0},callbackTests{0},expectedHazards{0};
    std::atomic<bool> negativeControl{false};
    VkDebugUtilsMessengerEXT messenger{};
    std::mutex mutex;std::vector<std::string> messages;
    static VKAPI_ATTR VkBool32 VKAPI_CALL callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,VkDebugUtilsMessageTypeFlagsEXT,const VkDebugUtilsMessengerCallbackDataEXT* data,void* user){
        auto& s=*static_cast<ValidationCapture*>(user);
        if(data->pMessageIdName&&!std::strcmp(data->pMessageIdName,"MAVK_CALLBACK_SELFTEST")){s.callbackTests++;return VK_FALSE;}
        if(s.negativeControl.load()&&data->pMessageIdName&&std::strstr(data->pMessageIdName,"SYNC-HAZARD-WRITE-AFTER-WRITE")){s.expectedHazards++;return VK_FALSE;}
        if(severity&VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)s.errors++;
        else if(severity&VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)s.warnings++;else return VK_FALSE;
        std::lock_guard<std::mutex> guard(s.mutex);
        if(s.messages.size()<64)s.messages.push_back(data->pMessage?data->pMessage:"empty validation message");
        return VK_FALSE;
    }
    void create(VkInstanceCreateInfo ci,VkInstance& instance,bool requested){
        if(!requested){check(vkCreateInstance(&ci,nullptr,&instance),"instance");return;}
        const char* layer="VK_LAYER_KHRONOS_validation";uint32_t n=0;
        check(vkEnumerateInstanceLayerProperties(&n,nullptr),"layer count");std::vector<VkLayerProperties> layers(n);check(vkEnumerateInstanceLayerProperties(&n,layers.data()),"layers");bool found=false;
        for(const auto& l:layers)if(!std::strcmp(l.layerName,layer)){found=true;implementationVersion=l.implementationVersion;}
        if(!found)throw std::runtime_error("Requested app-local Khronos validation missing; refusing silent fallback");
        std::vector<const char*> extensions;if(ci.enabledExtensionCount)extensions.assign(ci.ppEnabledExtensionNames,ci.ppEnabledExtensionNames+ci.enabledExtensionCount);
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);extensions.push_back(VK_EXT_LAYER_SETTINGS_EXTENSION_NAME);
        VkBool32 sync=VK_TRUE;VkLayerSettingEXT setting{layer,"validate_sync",VK_LAYER_SETTING_TYPE_BOOL32_EXT,1,&sync};
        VkLayerSettingsCreateInfoEXT settings{VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT};settings.settingCount=1;settings.pSettings=&setting;settings.pNext=ci.pNext;
        VkDebugUtilsMessengerCreateInfoEXT debug{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};debug.pNext=&settings;
        debug.messageSeverity=VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug.messageType=VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;debug.pfnUserCallback=callback;debug.pUserData=this;
        ci.enabledLayerCount=1;ci.ppEnabledLayerNames=&layer;ci.enabledExtensionCount=uint32_t(extensions.size());ci.ppEnabledExtensionNames=extensions.data();ci.pNext=&debug;
        check(vkCreateInstance(&ci,nullptr,&instance),"validated instance");enabled=true;
        auto createMessenger=reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkCreateDebugUtilsMessengerEXT"));
        auto submitMessage=reinterpret_cast<PFN_vkSubmitDebugUtilsMessageEXT>(vkGetInstanceProcAddr(instance,"vkSubmitDebugUtilsMessageEXT"));
        if(!createMessenger||!submitMessage)throw std::runtime_error("Validation debug-utils entrypoints missing");
        debug.pNext=nullptr;check(createMessenger(instance,&debug,nullptr,&messenger),"validation messenger");
        VkDebugUtilsMessengerCallbackDataEXT test{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT};test.pMessageIdName="MAVK_CALLBACK_SELFTEST";test.pMessage="Harness callback liveness only, not GPU work";
        submitMessage(instance,VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT,&test);
        if(callbackTests.load()!=1)throw std::runtime_error("Validation callback self-test failed");
    }
    // Record an intentional WAW hazard but NEVER submit it to the GPU. Discard
    // this command buffer before real work. Confirms SyncVal is actually active.
    void probeSynchronization(VkCommandBuffer command,VkBuffer buffer){
        if(!enabled||expectedHazards.load())return;
        check(vkResetCommandBuffer(command,0),"probe reset");VkCommandBufferBeginInfo b{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};check(vkBeginCommandBuffer(command,&b),"probe begin");
        negativeControl=true;vkCmdFillBuffer(command,buffer,0,4,1);vkCmdFillBuffer(command,buffer,0,4,2);negativeControl=false;
        check(vkEndCommandBuffer(command),"probe end");check(vkResetCommandBuffer(command,0),"discard never-submitted probe");
        if(expectedHazards.load()!=1)throw std::runtime_error("SyncVal negative control did not detect exactly one WAW hazard");
    }
    void detach(VkInstance instance){if(messenger){auto destroy=reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkDestroyDebugUtilsMessengerEXT"));if(destroy)destroy(instance,messenger,nullptr);messenger=VK_NULL_HANDLE;}}
    std::string fields(){
        std::ostringstream o;o<<",\"validation_layers_enabled\":"<<(enabled?"true":"false")<<",\"validation_implementation_version\":"<<implementationVersion<<",\"synchronization_validation_requested\":"<<(enabled?"true":"false")<<",\"validation_callback_tests\":"<<callbackTests.load()<<",\"syncval_expected_unsubmitted_hazards\":"<<expectedHazards.load()<<",\"validation_errors\":"<<errors.load()<<",\"validation_warnings\":"<<warnings.load()<<",\"validation_messages\":[";
        std::lock_guard<std::mutex> guard(mutex);for(size_t i=0;i<messages.size();i++){if(i)o<<',';o<<'"';for(unsigned char c:messages[i]){if(c=='"'||c=='\\')o<<'\\';if(c>=32)o<<char(c);}o<<'"';}o<<']';return o.str();
    }
private:static void check(VkResult r,const char* op){if(r!=VK_SUCCESS)throw std::runtime_error(std::string(op)+" VkResult="+std::to_string(r));}
};
