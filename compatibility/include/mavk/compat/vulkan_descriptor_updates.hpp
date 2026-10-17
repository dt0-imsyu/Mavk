#pragma once
#include <mavk/compat/native_descriptors.hpp>
#include <map>
#include <tuple>
#include <type_traits>

namespace mavk::compat {
namespace descriptor_api {
template<class T>uint64_t key(T h){if constexpr(std::is_pointer_v<T>)return uint64_t(reinterpret_cast<uintptr_t>(h));else return uint64_t(h);}
inline DescriptorKind kind(VkDescriptorType type){switch(type){
case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:return DescriptorKind::UniformBuffer;
case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:return DescriptorKind::StorageBuffer;
case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:return DescriptorKind::CombinedImageSampler;
case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:return DescriptorKind::SampledImage;
case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:return DescriptorKind::StorageImage;
default:throw std::invalid_argument("UNSUPPORTED descriptor type");}}
}
// Per-device trusted handle registry. Native handles never originate from an
// untrusted wire packet. Leases survive logical unregister until snapshots retire.
class VulkanDescriptorResources {
public:
    struct Resource {
        VkBuffer buffer{};VkImageView view{};VkSampler sampler{};VkImageLayout imageLayout{};
        uint64_t bytes=0;VkBufferUsageFlags bufferUsage{};VkImageUsageFlags imageUsage{};
        std::shared_ptr<const void> owner;std::shared_ptr<const Resource> image,sampling;bool active=true;
    };
    void registerBuffer(VkBuffer handle,uint64_t bytes,VkBufferUsageFlags usage,std::shared_ptr<const void> owner){
        if(!handle||!bytes||!owner||buffers_.count(descriptor_api::key(handle)))throw std::invalid_argument("buffer registration");
        auto r=std::make_shared<Resource>();r->buffer=handle;r->bytes=bytes;r->bufferUsage=usage;r->owner=std::move(owner);buffers_.emplace(descriptor_api::key(handle),std::move(r));
    }
    void registerImageView(VkImageView handle,VkImageUsageFlags usage,std::shared_ptr<const void> owner){
        if(!handle||!owner||views_.count(descriptor_api::key(handle)))throw std::invalid_argument("image-view registration");
        auto r=std::make_shared<Resource>();r->view=handle;r->imageUsage=usage;r->owner=std::move(owner);views_.emplace(descriptor_api::key(handle),std::move(r));
    }
    void registerSampler(VkSampler handle,std::shared_ptr<const void> owner){
        if(!handle||!owner||samplers_.count(descriptor_api::key(handle)))throw std::invalid_argument("sampler registration");
        auto r=std::make_shared<Resource>();r->sampler=handle;r->owner=std::move(owner);samplers_.emplace(descriptor_api::key(handle),std::move(r));
    }
    void unregisterBuffer(VkBuffer h){erase(buffers_,descriptor_api::key(h));}
    void unregisterImageView(VkImageView h){erase(views_,descriptor_api::key(h));}
    void unregisterSampler(VkSampler h){erase(samplers_,descriptor_api::key(h));}
    std::shared_ptr<const Resource> buffer(VkBuffer h) const{return lookup(buffers_,descriptor_api::key(h));}
    std::shared_ptr<const Resource> sampler(VkSampler h) const{return lookup(samplers_,descriptor_api::key(h));}
    static std::shared_ptr<const Resource> nullImageSampler(std::shared_ptr<const Resource> sampling,std::shared_ptr<const Resource> previous={}){
        if(previous&&!previous->image&&previous->sampling==sampling)return previous;
        auto r=std::make_shared<Resource>();r->sampler=sampling->sampler;r->sampling=std::move(sampling);return r;
    }
    std::shared_ptr<const Resource> image(const VkDescriptorImageInfo& info,DescriptorKind kind,std::shared_ptr<const Resource> previous={}){
        auto view=lookup(views_,descriptor_api::key(info.imageView));std::shared_ptr<const Resource> sampling;
        bool combined=kind==DescriptorKind::CombinedImageSampler;
        if(combined)sampling=sampler(info.sampler);
        auto usage=kind==DescriptorKind::StorageImage?VK_IMAGE_USAGE_STORAGE_BIT:VK_IMAGE_USAGE_SAMPLED_BIT;
        if(!(view->imageUsage&usage))throw std::invalid_argument("image descriptor usage");
        if(info.imageLayout!=VK_IMAGE_LAYOUT_GENERAL&&info.imageLayout!=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)throw std::invalid_argument("UNSUPPORTED descriptor image layout");
        if(kind==DescriptorKind::StorageImage&&info.imageLayout!=VK_IMAGE_LAYOUT_GENERAL)throw std::invalid_argument("storage descriptor layout");
        // Per-slot reuse,not an ever-growing global tuple cache. Copied slots
        // can reuse the same owned tuple on an identical subsequent write.
        if(previous&&previous->image==view&&previous->sampling==sampling&&previous->imageLayout==info.imageLayout)return previous;
        auto r=std::make_shared<Resource>();r->view=info.imageView;r->sampler=combined?info.sampler:VK_NULL_HANDLE;r->imageLayout=info.imageLayout;r->image=view;r->sampling=sampling;return r;
    }
    static bool alive(const DescriptorValue& value){if(!value.owner)return value.status!=DescriptorStatus::Bound;auto r=std::static_pointer_cast<const Resource>(value.owner);return r->active&&(!r->image||r->image->active)&&(!r->sampling||r->sampling->active);}
    static NativeDescriptorResource resolve(const DescriptorBinding& binding,uint32_t,const DescriptorValue& value){
        if(value.status!=DescriptorStatus::Bound||!alive(value))throw std::invalid_argument("explicit typed null fallback or live descriptor required");
        auto resource=std::static_pointer_cast<const Resource>(value.owner);NativeDescriptorResource out;out.owner=resource;
        if(binding.kind==DescriptorKind::UniformBuffer||binding.kind==DescriptorKind::StorageBuffer){out.buffer={resource->buffer,value.offset,value.range};out.bufferBytes=resource->bytes;}
        else out.image={resource->sampler,resource->view,resource->imageLayout};return out;
    }
private:
    using Map=std::map<uint64_t,std::shared_ptr<Resource>>;
    static std::shared_ptr<const Resource> lookup(const Map& map,uint64_t key){auto at=map.find(key);if(!key||at==map.end()||!at->second->active)throw std::invalid_argument("unknown/live resource handle required");return at->second;}
    static void erase(Map& map,uint64_t key){auto at=map.find(key);if(!key||at==map.end())throw std::invalid_argument("unknown resource unregister");at->second->active=false;map.erase(at);}
    Map buffers_,views_,samplers_;
};
// Vulkan-structure consumer adapter,not exported loader entry points. Logical
// VkDescriptorSet keys are allocated by a future ICD/layer; NEVER pass them to
// the native driver. NativeDescriptorAdapter emits the actual driver sets.
class VulkanDescriptorUpdates {
public:
    VulkanDescriptorUpdates(VulkanDescriptorResources& resources,DescriptorLimits limits={},bool experimentalNullWrites=false):resources_(resources),limits_(limits),allowNull_(experimentalNullWrites){}
    void registerSet(VkDescriptorSet set,const VkDescriptorSetLayoutCreateInfo& layout){
        if(!set||sets_.count(descriptor_api::key(set))||layout.sType!=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO||layout.pNext||layout.flags||!layout.bindingCount||layout.bindingCount>1024||!layout.pBindings)
            throw std::invalid_argument("UNSUPPORTED logical descriptor layout");
        auto record=std::make_shared<Set>();std::vector<DescriptorBinding> common;
        for(uint32_t i=0;i<layout.bindingCount;i++){auto b=layout.pBindings[i];if(record->bindings.count(b.binding))throw std::invalid_argument("duplicate binding");
            if(b.descriptorCount){if(b.pImmutableSamplers||!b.stageFlags)throw std::invalid_argument("UNSUPPORTED immutable/stage binding");common.push_back({0,b.binding,b.descriptorCount,descriptor_api::kind(b.descriptorType)});}
            else{b.pImmutableSamplers=nullptr;b.stageFlags=0;}record->bindings.emplace(b.binding,b);}
        record->state=std::make_shared<DescriptorState>(std::move(common),limits_);sets_.emplace(descriptor_api::key(set),std::move(record));
    }
    void unregisterSet(VkDescriptorSet set){if(!sets_.erase(descriptor_api::key(set)))throw std::invalid_argument("unknown descriptor set");}
    std::shared_ptr<const DescriptorSnapshot> snapshot(VkDescriptorSet set,GuardMetadataAbi abi) const {
        const auto& record=find(set);for(const auto& b:record.bindings)for(uint32_t i=0;i<b.second.descriptorCount;i++)if(!VulkanDescriptorResources::alive(record.state->value(0,b.first,i)))throw std::invalid_argument("undefined descriptor references destroyed resource");return record.state->snapshot(abi);
    }
    // Writes in array order,then copies in array order. Whole call is staged;
    // rejection cannot leave some logical sets partially updated.
    void update(uint32_t writeCount,const VkWriteDescriptorSet* writes,uint32_t copyCount,const VkCopyDescriptorSet* copies){
        if(writeCount>4096||copyCount>4096||(writeCount&&!writes)||(copyCount&&!copies))throw std::invalid_argument("descriptor update array bound");
        std::map<uint64_t,std::shared_ptr<DescriptorState>> staged;
        auto read=[&](VkDescriptorSet set)->const DescriptorState&{auto at=staged.find(descriptor_api::key(set));return at==staged.end()?*find(set).state:*at->second;};
        auto edit=[&](VkDescriptorSet set)->DescriptorState&{auto key=descriptor_api::key(set);auto at=staged.find(key);if(at==staged.end())at=staged.emplace(key,std::make_shared<DescriptorState>(*find(set).state)).first;return *at->second;};
        for(uint32_t n=0;n<writeCount;n++){
            const auto& w=writes[n];if(w.sType!=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET||w.pNext)throw std::invalid_argument("UNSUPPORTED descriptor write chain");
            auto slots=span(find(w.dstSet),w.dstBinding,w.dstArrayElement,w.descriptorCount,w.descriptorType);auto kind=descriptor_api::kind(w.descriptorType);auto& destination=edit(w.dstSet);
            bool buffer=kind==DescriptorKind::UniformBuffer||kind==DescriptorKind::StorageBuffer;
            if((buffer&&!w.pBufferInfo)||(!buffer&&!w.pImageInfo))throw std::invalid_argument("descriptor write data missing");
            for(uint32_t i=0;i<w.descriptorCount;i++){
                const auto slot=slots[i];
                if(buffer){const auto& info=w.pBufferInfo[i];if(!info.buffer){if(!allowNull_||info.offset||info.range!=VK_WHOLE_SIZE)throw std::invalid_argument("null buffer descriptor contract");destination.setNull(0,slot.first,slot.second);continue;}
                    auto resource=resources_.buffer(info.buffer);auto usage=kind==DescriptorKind::UniformBuffer?VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT:VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;if(!(resource->bufferUsage&usage))throw std::invalid_argument("buffer descriptor usage");destination.bindBuffer(0,slot.first,slot.second,resource,resource->bytes,info.offset,info.range);
                }else{const auto& info=w.pImageInfo[i];std::shared_ptr<const VulkanDescriptorResources::Resource> sampling;
                    if(kind==DescriptorKind::CombinedImageSampler)sampling=resources_.sampler(info.sampler);
                    if(!info.imageView){if(!allowNull_)throw std::invalid_argument("null image writes not enabled in adapter");
                        const auto& old=destination.value(0,slot.first,slot.second);auto previous=old.status==DescriptorStatus::Null&&old.owner?std::static_pointer_cast<const VulkanDescriptorResources::Resource>(old.owner):nullptr;
                        destination.setNull(0,slot.first,slot.second,sampling?VulkanDescriptorResources::nullImageSampler(sampling,previous):nullptr);continue;}
                    const auto& old=destination.value(0,slot.first,slot.second);auto previous=old.status==DescriptorStatus::Bound?std::static_pointer_cast<const VulkanDescriptorResources::Resource>(old.owner):nullptr;
                    destination.bindImage(0,slot.first,slot.second,resources_.image(info,kind,previous));
                }
            }
        }
        for(uint32_t n=0;n<copyCount;n++){
            const auto& c=copies[n];if(c.sType!=VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET||c.pNext)throw std::invalid_argument("UNSUPPORTED descriptor copy chain");
            const auto& source=find(c.srcSet);auto at=source.bindings.find(c.srcBinding);if(at==source.bindings.end())throw std::invalid_argument("copy source binding");auto type=at->second.descriptorType;
            auto src=span(source,c.srcBinding,c.srcArrayElement,c.descriptorCount,type),dst=span(find(c.dstSet),c.dstBinding,c.dstArrayElement,c.descriptorCount,type);
            if(c.srcSet==c.dstSet)for(auto a:src)if(std::find(dst.begin(),dst.end(),a)!=dst.end())throw std::invalid_argument("overlapping VkCopyDescriptorSet ranges");
            auto& destination=edit(c.dstSet);const auto& origin=read(c.srcSet);
            for(uint32_t i=0;i<c.descriptorCount;i++){const auto& v=origin.value(0,src[i].first,src[i].second);if(v.status==DescriptorStatus::Unwritten||!VulkanDescriptorResources::alive(v))destination.setUnwritten(0,dst[i].first,dst[i].second);else destination.copyFrom(origin,0,src[i].first,src[i].second,0,dst[i].first,dst[i].second,1);}
        }
        for(auto& state:staged)sets_.at(state.first)->state=std::move(state.second);
    }
private:
    struct Set {std::map<uint32_t,VkDescriptorSetLayoutBinding> bindings;std::shared_ptr<DescriptorState> state;};
    const Set& find(VkDescriptorSet set) const {auto at=sets_.find(descriptor_api::key(set));if(!set||at==sets_.end())throw std::invalid_argument("unknown descriptor set");return *at->second;}
    static std::vector<std::pair<uint32_t,uint32_t>> span(const Set& set,uint32_t binding,uint32_t first,uint32_t count,VkDescriptorType type){
        auto at=set.bindings.find(binding);if(at==set.bindings.end()||!at->second.descriptorCount||first>at->second.descriptorCount||!count||count>65536)throw std::invalid_argument("descriptor binding/array range");
        VkShaderStageFlags stages=at->second.stageFlags;std::vector<std::pair<uint32_t,uint32_t>> slots;slots.reserve(count);
        while(slots.size()<count){if(at==set.bindings.end()||at->first!=binding)throw std::invalid_argument("UNSUPPORTED gap in consecutive bindings");const auto& b=at->second;
            if(b.descriptorCount&&(b.descriptorType!=type||b.stageFlags!=stages))throw std::invalid_argument("consecutive binding type/stage mismatch");
            for(uint32_t i=first;i<b.descriptorCount&&slots.size()<count;i++)slots.emplace_back(binding,i);
            if(binding==UINT32_MAX&&slots.size()<count)throw std::invalid_argument("binding overflow");binding++;first=0;++at;
        }return slots;
    }
    VulkanDescriptorResources& resources_;DescriptorLimits limits_;bool allowNull_;
    std::map<uint64_t,std::shared_ptr<Set>> sets_;
};
}
