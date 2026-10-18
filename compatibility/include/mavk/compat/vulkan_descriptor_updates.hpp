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
case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:return DescriptorKind::UniformTexelBuffer;
case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:return DescriptorKind::StorageTexelBuffer;
default:throw std::invalid_argument("UNSUPPORTED descriptor type");}}
}
// Per-device trusted handle registry. Native handles never originate from an
// untrusted wire packet. Leases survive logical unregister until snapshots retire.
class VulkanDescriptorResources {
public:
    struct Resource {
        VkBuffer buffer{};VkImageView view{};VkSampler sampler{};VkImageLayout imageLayout{};
        uint64_t bytes=0;VkBufferUsageFlags bufferUsage{};VkImageUsageFlags imageUsage{};VkBufferView texelView{};uint64_t texelElements=0;
        std::shared_ptr<const Resource> backing;
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
    // Caller already created this real view. Currently R32 scalar formats only;
    // native format features/alignment are checked at actual view creation.
    void registerBufferView(VkBufferView view,VkBuffer bufferHandle,VkFormat format,uint64_t offset,uint64_t range,std::shared_ptr<const void> owner){
        auto buffer=lookup(buffers_,descriptor_api::key(bufferHandle));
        if(!view||!owner||texelViews_.count(descriptor_api::key(view))||(format!=VK_FORMAT_R32_UINT&&format!=VK_FORMAT_R32_SINT&&format!=VK_FORMAT_R32_SFLOAT)||offset>=buffer->bytes||offset%4)throw std::invalid_argument("UNSUPPORTED texel buffer view");
        if(range==VK_WHOLE_SIZE)range=buffer->bytes-offset;
        if(!range||range>buffer->bytes-offset||range%4||range/4>UINT32_MAX||!(buffer->bufferUsage&(VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT|VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)))throw std::invalid_argument("texel view range/usage");
        auto r=std::make_shared<Resource>();r->texelView=view;r->texelElements=range/4;r->backing=std::move(buffer);r->owner=std::move(owner);texelViews_.emplace(descriptor_api::key(view),std::move(r));
    }
    void registerSampler(VkSampler handle,std::shared_ptr<const void> owner){
        if(!handle||!owner||samplers_.count(descriptor_api::key(handle)))throw std::invalid_argument("sampler registration");
        auto r=std::make_shared<Resource>();r->sampler=handle;r->owner=std::move(owner);samplers_.emplace(descriptor_api::key(handle),std::move(r));
    }
    void unregisterBuffer(VkBuffer h){erase(buffers_,descriptor_api::key(h));}
    void unregisterImageView(VkImageView h){erase(views_,descriptor_api::key(h));}
    void unregisterSampler(VkSampler h){erase(samplers_,descriptor_api::key(h));}
    void unregisterBufferView(VkBufferView h){erase(texelViews_,descriptor_api::key(h));}
    std::shared_ptr<const Resource> buffer(VkBuffer h) const{return lookup(buffers_,descriptor_api::key(h));}
    std::shared_ptr<const Resource> sampler(VkSampler h) const{return lookup(samplers_,descriptor_api::key(h));}
    std::shared_ptr<const Resource> texelView(VkBufferView h,DescriptorKind kind) const{auto r=lookup(texelViews_,descriptor_api::key(h));auto usage=kind==DescriptorKind::UniformTexelBuffer?VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT:VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;if(!r->backing->active||!(r->backing->bufferUsage&usage))throw std::invalid_argument("texel view backing/descriptor usage");return r;}
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
    static bool alive(const DescriptorValue& value){if(!value.owner)return value.status!=DescriptorStatus::Bound;auto r=std::static_pointer_cast<const Resource>(value.owner);return r->active&&(!r->image||r->image->active)&&(!r->sampling||r->sampling->active)&&(!r->backing||r->backing->active);}
    static NativeDescriptorResource resolve(const DescriptorBinding& binding,uint32_t,const DescriptorValue& value){
        if(value.status!=DescriptorStatus::Bound||!alive(value))throw std::invalid_argument("explicit typed null fallback or live descriptor required");
        auto resource=std::static_pointer_cast<const Resource>(value.owner);NativeDescriptorResource out;out.owner=resource;
        if(binding.kind==DescriptorKind::UniformBuffer||binding.kind==DescriptorKind::StorageBuffer){out.buffer={resource->buffer,value.offset,value.range};out.bufferBytes=resource->bytes;}
        else if(binding.kind==DescriptorKind::UniformTexelBuffer||binding.kind==DescriptorKind::StorageTexelBuffer){out.texelView=resource->texelView;out.texelElements=resource->texelElements;}
        else out.image={resource->sampler,resource->view,resource->imageLayout};return out;
    }
private:
    using Map=std::map<uint64_t,std::shared_ptr<Resource>>;
    static std::shared_ptr<const Resource> lookup(const Map& map,uint64_t key){auto at=map.find(key);if(!key||at==map.end()||!at->second->active)throw std::invalid_argument("unknown/live resource handle required");return at->second;}
    static void erase(Map& map,uint64_t key){auto at=map.find(key);if(!key||at==map.end())throw std::invalid_argument("unknown resource unregister");at->second->active=false;map.erase(at);}
    Map buffers_,views_,samplers_,texelViews_;
};
// Vulkan-structure consumer adapter,not exported loader entry points. Logical
// VkDescriptorSet keys are allocated by a future ICD/layer; NEVER pass them to
// the native driver. NativeDescriptorAdapter emits the actual driver sets.
class VulkanDescriptorUpdates {
public:
    // Compiled template owns only schema/offsets,not caller pointers or a set.
    // The future loader layer resolves VkDescriptorSetLayout ownership before
    // supplying a representative logical set with that exact layout.
    class UpdateTemplate {
    public:
        size_t requiredBytes() const{return required_;}
    private:
        friend class VulkanDescriptorUpdates;
        std::map<uint32_t,VkDescriptorSetLayoutBinding> bindings_;
        std::vector<VkDescriptorUpdateTemplateEntry> entries_;
        size_t required_=0;
    };
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
    // Pipeline-bound logical sets ->one immutable metadata/resource generation.
    // Null positions are empty public sets,NOT logically null descriptors.
    // The pipeline adapter still verifies the exact complete binding schema.
    std::shared_ptr<const DescriptorSnapshot> snapshotBindings(const std::vector<VkDescriptorSet>& bound,GuardMetadataAbi abi){
        if(bound.empty()||bound.size()>7)throw std::invalid_argument("bound public set count");
        std::vector<std::shared_ptr<const DescriptorSnapshot>> sources;for(auto set:bound)sources.push_back(set?snapshot(set,abi):nullptr);
        bool cached=bound==boundKeys_&&abi==boundAbi_&&sources.size()==boundSources_.size();
        if(cached)for(size_t i=0;i<sources.size();i++)if(sources[i]!=boundSources_[i].lock()){cached=false;break;}
        if(cached)if(auto result=boundCache_.lock())return result;
        std::vector<DescriptorBinding> bindings;
        for(uint32_t set=0;set<sources.size();set++)if(sources[set])for(auto binding:sources[set]->bindings){binding.set=set;bindings.push_back(binding);}
        DescriptorState state(std::move(bindings),limits_);
        for(uint32_t set=0;set<sources.size();set++)if(sources[set])for(size_t row=0;row<sources[set]->bindings.size();row++){
            const auto& binding=sources[set]->bindings[row];for(uint32_t i=0;i<binding.count;i++){
                const auto& value=sources[set]->values[row][i];
                if(value.status==DescriptorStatus::Null)state.setNull(set,binding.binding,i,value.owner);
                else if(binding.kind==DescriptorKind::UniformBuffer||binding.kind==DescriptorKind::StorageBuffer)state.bindBuffer(set,binding.binding,i,value.owner,value.offset+value.range,value.offset,value.range);
                else if(binding.kind==DescriptorKind::UniformTexelBuffer||binding.kind==DescriptorKind::StorageTexelBuffer)state.bindTexelBuffer(set,binding.binding,i,value.owner,value.range);
                else state.bindImage(set,binding.binding,i,value.owner);
            }
        }
        auto result=state.snapshot(abi);boundKeys_=bound;boundAbi_=abi;boundSources_.assign(sources.begin(),sources.end());boundCache_=result;return result;
    }
    UpdateTemplate compileTemplate(VkDescriptorSet representative,const VkDescriptorUpdateTemplateCreateInfo& info) const {
        if(info.sType!=VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO||info.pNext||info.flags||info.templateType!=VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET||!info.descriptorSetLayout||!info.descriptorUpdateEntryCount||info.descriptorUpdateEntryCount>4096||!info.pDescriptorUpdateEntries)
            throw std::invalid_argument("UNSUPPORTED descriptor update template");
        const auto& set=find(representative);UpdateTemplate out;out.bindings_=set.bindings;size_t total=0;
        for(uint32_t n=0;n<info.descriptorUpdateEntryCount;n++){
            auto entry=info.pDescriptorUpdateEntries[n];auto kind=descriptor_api::kind(entry.descriptorType);
            span(set,entry.dstBinding,entry.dstArrayElement,entry.descriptorCount,entry.descriptorType);
            if(entry.descriptorCount>65536-total)throw std::invalid_argument("template descriptor budget");total+=entry.descriptorCount;
            size_t bytes=kind==DescriptorKind::UniformBuffer||kind==DescriptorKind::StorageBuffer?sizeof(VkDescriptorBufferInfo):(kind==DescriptorKind::UniformTexelBuffer||kind==DescriptorKind::StorageTexelBuffer?sizeof(VkBufferView):sizeof(VkDescriptorImageInfo));
            constexpr size_t limit=16*1024*1024;
            if(entry.offset>limit-bytes||(entry.descriptorCount>1&&entry.stride>(limit-entry.offset-bytes)/(entry.descriptorCount-1)))throw std::invalid_argument("template source extent/overflow");
            out.required_=std::max(out.required_,entry.offset+entry.stride*(entry.descriptorCount-1)+bytes);out.entries_.push_back(entry);
        }
        return out;
    }
    // Bounded counterpart of vkUpdateDescriptorSetWithTemplate. Source may be
    // unaligned/interleaved; memcpy avoids typed alias/alignment assumptions.
    // Expand first,then reuse the existing whole-call transactional update path.
    void updateWithTemplate(VkDescriptorSet set,const UpdateTemplate& descriptorTemplate,const void* data,size_t dataBytes){
        const auto& bindings=find(set).bindings;
        if(descriptorTemplate.entries_.empty()||bindings.size()!=descriptorTemplate.bindings_.size())throw std::invalid_argument("template/layout mismatch");
        auto a=bindings.begin(),b=descriptorTemplate.bindings_.begin();
        for(;a!=bindings.end();++a,++b)if(a->first!=b->first||a->second.descriptorType!=b->second.descriptorType||a->second.descriptorCount!=b->second.descriptorCount||a->second.stageFlags!=b->second.stageFlags)
            throw std::invalid_argument("template/layout mismatch");
        if(!data||dataBytes<descriptorTemplate.required_)throw std::invalid_argument("template source bytes missing");
        std::vector<VkWriteDescriptorSet> writes(descriptorTemplate.entries_.size());
        std::vector<std::vector<VkDescriptorBufferInfo>> buffers(writes.size());std::vector<std::vector<VkDescriptorImageInfo>> images(writes.size());
        std::vector<std::vector<VkBufferView>> texels(writes.size());
        auto source=static_cast<const unsigned char*>(data);
        for(size_t n=0;n<writes.size();n++){
            const auto& e=descriptorTemplate.entries_[n];auto kind=descriptor_api::kind(e.descriptorType);auto& w=writes[n];
            w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w.dstSet=set;w.dstBinding=e.dstBinding;w.dstArrayElement=e.dstArrayElement;w.descriptorType=e.descriptorType;w.descriptorCount=e.descriptorCount;
            if(kind==DescriptorKind::UniformBuffer||kind==DescriptorKind::StorageBuffer){buffers[n].resize(e.descriptorCount);for(uint32_t i=0;i<e.descriptorCount;i++)std::memcpy(&buffers[n][i],source+e.offset+e.stride*i,sizeof(VkDescriptorBufferInfo));w.pBufferInfo=buffers[n].data();}
            else if(kind==DescriptorKind::UniformTexelBuffer||kind==DescriptorKind::StorageTexelBuffer){texels[n].resize(e.descriptorCount);for(uint32_t i=0;i<e.descriptorCount;i++)std::memcpy(&texels[n][i],source+e.offset+e.stride*i,sizeof(VkBufferView));w.pTexelBufferView=texels[n].data();}
            else{images[n].resize(e.descriptorCount);for(uint32_t i=0;i<e.descriptorCount;i++)std::memcpy(&images[n][i],source+e.offset+e.stride*i,sizeof(VkDescriptorImageInfo));w.pImageInfo=images[n].data();}
        }
        update(uint32_t(writes.size()),writes.data(),0,nullptr);
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
            bool texel=kind==DescriptorKind::UniformTexelBuffer||kind==DescriptorKind::StorageTexelBuffer;
            if((buffer&&!w.pBufferInfo)||(texel&&!w.pTexelBufferView)||(!buffer&&!texel&&!w.pImageInfo))throw std::invalid_argument("descriptor write data missing");
            for(uint32_t i=0;i<w.descriptorCount;i++){
                const auto slot=slots[i];
                if(buffer){const auto& info=w.pBufferInfo[i];if(!info.buffer){if(!allowNull_||info.offset||info.range!=VK_WHOLE_SIZE)throw std::invalid_argument("null buffer descriptor contract");destination.setNull(0,slot.first,slot.second);continue;}
                    auto resource=resources_.buffer(info.buffer);auto usage=kind==DescriptorKind::UniformBuffer?VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT:VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;if(!(resource->bufferUsage&usage))throw std::invalid_argument("buffer descriptor usage");destination.bindBuffer(0,slot.first,slot.second,resource,resource->bytes,info.offset,info.range);
                }else if(texel){auto h=w.pTexelBufferView[i];if(!h){if(!allowNull_)throw std::invalid_argument("null texel descriptor not enabled");destination.setNull(0,slot.first,slot.second);}
                    else{auto r=resources_.texelView(h,kind);destination.bindTexelBuffer(0,slot.first,slot.second,r,r->texelElements);}
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
    std::vector<VkDescriptorSet> boundKeys_;GuardMetadataAbi boundAbi_{};
    std::vector<std::weak_ptr<const DescriptorSnapshot>> boundSources_;
    std::weak_ptr<const DescriptorSnapshot> boundCache_;
};
}
