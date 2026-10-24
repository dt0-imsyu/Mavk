// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <mavk/compat/native_descriptors.hpp>
#include <map>

namespace mavk::compat {
// Typed,valid native backing. Matching shader guards are mandatory. No queue
// submissions/waits;native resources live as long as recorded snapshot leases.
class NullTexelResources {
    struct View {
        VkDevice device{};PFN_vkDestroyBufferView destroy{};VkBufferView handle{};
        std::shared_ptr<mavk::Buffer> buffer;
        ~View(){if(handle)destroy(device,handle,nullptr);}
    };
public:
    explicit NullTexelResources(mavk::DeviceContext& context):context_(context){}
    NativeDescriptorResource resolve(const DescriptorBinding& binding,const DescriptorValue& value,VkFormat format){
        if(value.status!=DescriptorStatus::Null||(binding.kind!=DescriptorKind::UniformTexelBuffer&&binding.kind!=DescriptorKind::StorageTexelBuffer)||
           (format!=VK_FORMAT_R32_UINT&&format!=VK_FORMAT_R32_SINT&&format!=VK_FORMAT_R32_SFLOAT))throw std::invalid_argument("typed null R32 texel descriptor required");
        auto at=views_.find(format);
        if(at==views_.end()){
            VkFormatProperties properties{};context_.dispatch().vkGetPhysicalDeviceFormatProperties(context_.physicalDevice(),format,&properties);
            constexpr auto required=VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT|VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT;
            if((properties.bufferFeatures&required)!=required)throw std::invalid_argument("native R32 texel format unavailable");
            auto view=std::make_shared<View>();view->device=context_.vkDevice();view->destroy=context_.dispatch().vkDestroyBufferView;
            view->buffer=std::make_shared<mavk::Buffer>(context_,16,VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT|VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            std::memset(view->buffer->map(),0,16);view->buffer->flush();
            VkBufferViewCreateInfo ci{VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};ci.buffer=view->buffer->handle();ci.format=format;ci.range=16;
            descriptor_detail::check(context_.dispatch().vkCreateBufferView(view->device,&ci,nullptr,&view->handle),"null texel view");
            at=views_.emplace(format,std::move(view)).first;
        }
        NativeDescriptorResource out;out.owner=at->second;out.texelView=at->second->handle;out.texelElements=4;return out;
    }
private:
    mavk::DeviceContext& context_;std::map<VkFormat,std::shared_ptr<View>> views_;
};
}
