#pragma once
#include <mavk/core.hpp>
#include <mavk/compat/descriptors.hpp>
#include <array>
#include <algorithm>
#include <cstring>
#include <functional>
#include <map>
#include <string>
#include <utility>

namespace mavk::compat {
struct NativeDescriptorResource {
    // Owns the REAL native resource/view/sampler dependencies,including typed
    // fallbacks for logical null. Caller must not use a non-owning placeholder.
    std::shared_ptr<const void> owner;
    VkDescriptorBufferInfo buffer{};
    uint64_t bufferBytes=0;
    VkDescriptorImageInfo image{};
    VkBufferView texelView{};uint64_t texelElements=0;
    VkImageAspectFlags imageAspects=VK_IMAGE_ASPECT_COLOR_BIT;
};
struct NativeDescriptorCounters {uint64_t poolCreates=0,metadataUploads=0,updates=0,liveSnapshots=0,diagnosticAllocations=0;};
class DescriptorMetadataArena {
public:
    struct Slice {std::shared_ptr<mavk::Buffer> buffer;VkDeviceSize offset=0,bytes=0;};
    explicit DescriptorMetadataArena(mavk::DeviceContext& context):context_(context),alignment_(std::max({VkDeviceSize(1),context.properties().limits.minStorageBufferOffsetAlignment,context.properties().limits.minUniformBufferOffsetAlignment})){}
    Slice allocate(VkDeviceSize bytes){
        if(!bytes||bytes>context_.properties().limits.maxStorageBufferRange)throw std::invalid_argument("metadata arena range");
        if(!poolable_||bytes>pageBytes_)return {make(bytes),0,bytes};
        // Recorded generations retain the buffer through every permitted resubmit.
        if(current_&&current_.use_count()==1)cursor_=0;
        const auto remainder=cursor_%alignment_;
        const auto offset=remainder?cursor_+alignment_-remainder:cursor_;
        if(!current_||offset>current_->size()||bytes>current_->size()-offset){
            current_=make(std::max(pageBytes_,alignment_));
            // Whole-allocation noncoherent flushes would overlap live slices.
            if(!(context_.memoryFlags(current_->memoryType())&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){poolable_=false;current_.reset();cursor_=0;return {make(bytes),0,bytes};}
            cursor_=bytes;return {current_,0,bytes};
        }
        cursor_=offset+bytes;return {current_,offset,bytes};
    }
    uint64_t allocations() const{return allocations_;}
private:
    std::shared_ptr<mavk::Buffer> make(VkDeviceSize bytes){
        auto result=std::make_shared<mavk::Buffer>(context_,bytes,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        allocations_++;return result;
    }
    mavk::DeviceContext& context_;VkDeviceSize alignment_,cursor_=0;
    const VkDeviceSize pageBytes_=65536;
    std::shared_ptr<mavk::Buffer> current_;uint64_t allocations_=0;bool poolable_=true;
};
// Exact native layouts from an API adapter. Owner must retain every public and
// private layout plus pipeline layout until all recorded generations retire.
struct BorrowedDescriptorLayouts {
    std::vector<VkDescriptorSetLayout> sets;VkPipelineLayout pipeline{};
    std::shared_ptr<const void> owner;
    // Non-nullable native sets (e.g. live sampler heaps). Never snapshot,
    // allocate, rewrite or cache their descriptors; caller binds originals.
    std::vector<uint32_t> externalSets;
    std::vector<std::pair<uint32_t,uint32_t>> dynamicBindings;
    std::vector<std::pair<uint32_t,uint32_t>> immutableBindings;
    std::vector<DescriptorBinding> allocationBindings;
    bool diagnosticWrites=true;
    std::shared_ptr<DescriptorMetadataArena> metadataArena;
    uint32_t uniformMetadataWords=0;
};
namespace descriptor_detail {
inline bool readableImageLayout(VkImageLayout layout,VkImageAspectFlags aspects){switch(layout){
case VK_IMAGE_LAYOUT_GENERAL:case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:case VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL:return true;
case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:return aspects&&!(aspects&~(VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT));
case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:return aspects==VK_IMAGE_ASPECT_DEPTH_BIT;
case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:return aspects==VK_IMAGE_ASPECT_STENCIL_BIT;
default:return false;
}}
inline void check(VkResult r,const char* message){if(r!=VK_SUCCESS)throw std::runtime_error(std::string(message)+": "+std::to_string(r));}
inline VkDescriptorType type(DescriptorKind k){switch(k){
    case DescriptorKind::UniformBuffer:return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case DescriptorKind::StorageBuffer:return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case DescriptorKind::CombinedImageSampler:return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    case DescriptorKind::SampledImage:return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    case DescriptorKind::StorageImage:return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    case DescriptorKind::UniformTexelBuffer:return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    case DescriptorKind::StorageTexelBuffer:return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    case DescriptorKind::Sampler:return VK_DESCRIPTOR_TYPE_SAMPLER;
}throw std::invalid_argument("native descriptor kind");}
inline void validateDynamicBufferOffset(const NativeDescriptorResource& resource,DescriptorKind kind,DescriptorStatus status,uint32_t offset,const VkPhysicalDeviceLimits& limits){
    if(kind!=DescriptorKind::UniformBuffer&&kind!=DescriptorKind::StorageBuffer)throw std::invalid_argument("dynamic buffer kind");
    const uint64_t alignment=kind==DescriptorKind::UniformBuffer?limits.minUniformBufferOffsetAlignment:limits.minStorageBufferOffsetAlignment;
    if(!alignment||offset%alignment)throw std::invalid_argument("dynamic buffer offset alignment");
    // A logical null was written with WHOLE_SIZE; Vulkan requires offset zero.
    if(status==DescriptorStatus::Null){if(offset)throw std::invalid_argument("null dynamic buffer offset");return;}
    if(status!=DescriptorStatus::Bound||!resource.buffer.buffer||resource.buffer.offset>resource.bufferBytes||offset>resource.bufferBytes-resource.buffer.offset)throw std::invalid_argument("dynamic buffer effective offset");
    const uint64_t remaining=resource.bufferBytes-resource.buffer.offset-offset;
    if(!resource.buffer.range||resource.buffer.range>remaining)throw std::invalid_argument("dynamic buffer effective range");
}
struct Layouts {
    VkDevice device{};mavk::VulkanDispatch dispatch;std::vector<DescriptorBinding> bindings;std::vector<VkDescriptorSetLayout> sets;VkPipelineLayout pipeline{};
    VkPhysicalDeviceLimits limits{};std::shared_ptr<NativeDescriptorCounters> counters=std::make_shared<NativeDescriptorCounters>();
    bool ownsNative=true;std::shared_ptr<const void> nativeOwner;std::vector<uint32_t> externalSets;
    std::vector<std::pair<uint32_t,uint32_t>> dynamicBindings;
    std::vector<std::pair<uint32_t,uint32_t>> immutableBindings;
    std::vector<DescriptorBinding> allocationBindings;
    bool diagnosticWrites=true;
    std::shared_ptr<DescriptorMetadataArena> metadataArena;
    uint32_t uniformMetadataWords=0;
    std::shared_ptr<mavk::Buffer> quietLog;
    struct DynamicSlot {DescriptorKind kind;size_t row,element,resource;};
    std::vector<DynamicSlot> dynamicSlots;
    bool immutable(const DescriptorBinding& b) const{return std::binary_search(immutableBindings.begin(),immutableBindings.end(),std::make_pair(b.set,b.binding));}
    bool dynamic(const DescriptorBinding& b) const{return std::binary_search(dynamicBindings.begin(),dynamicBindings.end(),std::make_pair(b.set,b.binding));}
    VkDescriptorType nativeType(const DescriptorBinding& b) const{return dynamic(b)?(b.kind==DescriptorKind::UniformBuffer?VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC):type(b.kind);}
    void configureAllocation(std::vector<DescriptorBinding> declared){
        if(sets.size()<2||sets.size()>8)throw std::invalid_argument("native allocation layout count");
        if(declared.empty())declared=bindings;
        DescriptorState validated(declared);(void)validated;
        std::sort(declared.begin(),declared.end(),[](const auto& a,const auto& b){return std::make_pair(a.set,a.binding)<std::make_pair(b.set,b.binding);});
        std::map<std::pair<uint32_t,uint32_t>,std::pair<size_t,size_t>> selected;size_t base=0;
        for(size_t row=0;row<bindings.size();row++){selected[{bindings[row].set,bindings[row].binding}]={row,base};base+=bindings[row].count;}
        for(const auto& b:bindings){auto at=std::find_if(declared.begin(),declared.end(),[&](const auto& d){return d.set==b.set&&d.binding==b.binding;});if(at==declared.end()||at->kind!=b.kind||at->count!=b.count)throw std::invalid_argument("selected/allocation descriptor mismatch");}
        dynamicSlots.clear();
        for(const auto& b:declared){
            if(b.set>=sets.size()-1||std::find(externalSets.begin(),externalSets.end(),b.set)!=externalSets.end())throw std::invalid_argument("allocation descriptor set index");
            if(!dynamic(b))continue;
            auto at=selected.find({b.set,b.binding});
            for(uint32_t i=0;i<b.count;i++)dynamicSlots.push_back({b.kind,at==selected.end()?SIZE_MAX:at->second.first,i,at==selected.end()?SIZE_MAX:at->second.second+i});
        }
        allocationBindings=std::move(declared);
    }
    ~Layouts(){if(ownsNative){if(pipeline)dispatch.vkDestroyPipelineLayout(device,pipeline,nullptr);for(auto s:sets)if(s)dispatch.vkDestroyDescriptorSetLayout(device,s,nullptr);}}
};
}
// Immutable native generation. Do not release until every GPU user completes.
// No destructor wait/queue idle: the submission owner explicitly controls life.
class NativeDescriptorSnapshot {
public:
    NativeDescriptorSnapshot(const NativeDescriptorSnapshot&)=delete;
    ~NativeDescriptorSnapshot(){if(pool_)layouts_->dispatch.vkDestroyDescriptorPool(layouts_->device,pool_,nullptr);if(counted_)layouts_->counters->liveSnapshots--;}
    // External native set positions are NULL; the API adapter must substitute
    // the application's original live sets before issuing a native bind.
    const std::vector<VkDescriptorSet>& sets() const{return sets_;}
    VkPipelineLayout pipelineLayout() const{return layouts_->pipeline;}
    const DescriptorSnapshot& cpu() const{return *cpu_;}
    void validateDynamicOffsets(const std::vector<uint32_t>& offsets) const {
        if(offsets.size()!=layouts_->dynamicSlots.size())throw std::invalid_argument("dynamic buffer offset count");
        for(size_t i=0;i<offsets.size();i++){
            const auto& slot=layouts_->dynamicSlots[i];
            const uint64_t alignment=slot.kind==DescriptorKind::UniformBuffer?layouts_->limits.minUniformBufferOffsetAlignment:layouts_->limits.minStorageBufferOffsetAlignment;
            if(!alignment||offsets[i]%alignment)throw std::invalid_argument("dynamic buffer offset alignment");
            if(slot.resource!=SIZE_MAX)descriptor_detail::validateDynamicBufferOffset(resources_.at(slot.resource),slot.kind,cpu_->values[slot.row][slot.element].status,offsets[i],layouts_->limits);
        }
    }
    // Caller must have completed GPU use before host read/reset of diagnostics.
    const uint32_t* readDiagnostics(){if(!layouts_->diagnosticWrites)throw std::logic_error("diagnostic writes disabled");log_->invalidate();return static_cast<const uint32_t*>(log_->map());}
private:
    friend class NativeDescriptorAdapter;
    NativeDescriptorSnapshot(std::shared_ptr<descriptor_detail::Layouts> layouts,std::shared_ptr<const DescriptorSnapshot> cpu):layouts_(std::move(layouts)),cpu_(std::move(cpu)){}
    std::shared_ptr<descriptor_detail::Layouts> layouts_;std::shared_ptr<const DescriptorSnapshot> cpu_;
    std::vector<NativeDescriptorResource> resources_;std::shared_ptr<mavk::Buffer> metadata_,log_;
    VkDeviceSize metadataOffset_=0,metadataBytes_=0;
    VkDescriptorPool pool_{};std::vector<VkDescriptorSet> sets_;bool counted_=false,nativePublic_=false;
};
// Bounded native backend API,not a loader layer or complete VkDescriptor API.
// Public sets followed by one private guard set; caller supplies real shader/layout
// admission and typed fallbacks. Serialized/external queue synchronization.
class NativeDescriptorAdapter {
public:
    using Resolver=std::function<NativeDescriptorResource(const DescriptorBinding&,uint32_t,const DescriptorValue&)>;
    NativeDescriptorAdapter(mavk::DeviceContext& context,std::vector<DescriptorBinding> bindings,GuardMetadataAbi abi,BorrowedDescriptorLayouts native):context_(context),layouts_(std::make_shared<descriptor_detail::Layouts>()),abi_(abi){
        if(bindings.empty())throw std::invalid_argument("native descriptor adapter needs resource bindings");
        DescriptorState validated(bindings);(void)validated;
        if((abi!=GuardMetadataAbi::BufferRanges&&abi!=GuardMetadataAbi::IndexAndInit&&abi!=GuardMetadataAbi::IndexInitFormats)||!native.owner||!native.pipeline||native.sets.size()<2||native.sets.size()>8)throw std::invalid_argument("borrowed descriptor layouts/ABI");
        std::sort(bindings.begin(),bindings.end(),[](const auto& a,const auto& b){return a.set<b.set||(a.set==b.set&&a.binding<b.binding);});
        if(bindings.back().set>=native.sets.size()-1||native.sets.size()>context.properties().limits.maxBoundDescriptorSets)throw std::invalid_argument("borrowed descriptor public set count");
        for(auto layout:native.sets)if(!layout)throw std::invalid_argument("borrowed descriptor layout handle");
        std::sort(native.externalSets.begin(),native.externalSets.end());
        for(size_t i=0;i<native.externalSets.size();i++){
            auto set=native.externalSets[i];if(set>=native.sets.size()-1||(i&&set==native.externalSets[i-1]))throw std::invalid_argument("external native set index");
            for(const auto& b:bindings)if(b.set==set)throw std::invalid_argument("external set must not be snapshotted");
        }
        std::sort(native.dynamicBindings.begin(),native.dynamicBindings.end());
        const auto& declared=native.allocationBindings.empty()?bindings:native.allocationBindings;
        for(size_t i=0;i<native.dynamicBindings.size();i++){
            const auto key=native.dynamicBindings[i];auto b=std::find_if(declared.begin(),declared.end(),[&](const auto& b){return b.set==key.first&&b.binding==key.second;});
            if((i&&key==native.dynamicBindings[i-1])||b==declared.end()||(b->kind!=DescriptorKind::UniformBuffer&&b->kind!=DescriptorKind::StorageBuffer))throw std::invalid_argument("dynamic native binding schema");
        }
        layouts_->dynamicBindings=std::move(native.dynamicBindings);
        std::sort(native.immutableBindings.begin(),native.immutableBindings.end());
        for(size_t i=0;i<native.immutableBindings.size();i++){
            auto key=native.immutableBindings[i];auto b=std::find_if(declared.begin(),declared.end(),[&](const auto& b){return b.set==key.first&&b.binding==key.second;});
            if((i&&key==native.immutableBindings[i-1])||b==declared.end()||(b->kind!=DescriptorKind::Sampler&&b->kind!=DescriptorKind::CombinedImageSampler))throw std::invalid_argument("immutable native binding schema");
        }
        layouts_->immutableBindings=std::move(native.immutableBindings);
        layouts_->diagnosticWrites=native.diagnosticWrites;
        layouts_->metadataArena=std::move(native.metadataArena);
        layouts_->uniformMetadataWords=native.uniformMetadataWords;
        if(native.uniformMetadataWords>1024||uint64_t(native.uniformMetadataWords)*16>context.properties().limits.maxUniformBufferRange)throw std::invalid_argument("uniform metadata range");
        layouts_->ownsNative=false;layouts_->device=context.vkDevice();layouts_->dispatch=context.dispatch();layouts_->limits=context.properties().limits;
        layouts_->bindings=std::move(bindings);layouts_->sets=std::move(native.sets);layouts_->pipeline=native.pipeline;layouts_->nativeOwner=std::move(native.owner);layouts_->externalSets=std::move(native.externalSets);
        layouts_->configureAllocation(std::move(native.allocationBindings));
    }
    NativeDescriptorAdapter(mavk::DeviceContext& context,std::vector<DescriptorBinding> bindings,GuardMetadataAbi abi,VkShaderStageFlags stages=VK_SHADER_STAGE_COMPUTE_BIT,uint32_t publicSetCount=0):context_(context),layouts_(std::make_shared<descriptor_detail::Layouts>()),abi_(abi){
        if(bindings.empty())throw std::invalid_argument("native descriptor adapter needs resource bindings");
        // Reuse common layout validation before any native resource creation.
        DescriptorState validated(bindings);(void)validated;
        if(abi!=GuardMetadataAbi::BufferRanges&&abi!=GuardMetadataAbi::IndexAndInit&&abi!=GuardMetadataAbi::IndexInitFormats)throw std::invalid_argument("native shader metadata ABI");
        std::sort(bindings.begin(),bindings.end(),[](const auto& a,const auto& b){return a.set<b.set||(a.set==b.set&&a.binding<b.binding);});
        if(!publicSetCount)publicSetCount=bindings.back().set+1;
        if(publicSetCount>7||bindings.back().set>=publicSetCount)throw std::invalid_argument("native public descriptor set count");
        if(!stages)throw std::invalid_argument("native descriptor stage visibility");
        layouts_->device=context.vkDevice();layouts_->dispatch=context.dispatch();layouts_->bindings=std::move(bindings);
        const auto& properties=context.properties();layouts_->limits=properties.limits;
        if(properties.limits.maxBoundDescriptorSets<publicSetCount+1)throw std::runtime_error("guard descriptor set unavailable");
        layouts_->sets.resize(publicSetCount+1,VK_NULL_HANDLE);
        layouts_->configureAllocation({});
        VkDescriptorSetLayoutCreateInfo create{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        for(uint32_t set=0;set<publicSetCount;set++){
            std::vector<VkDescriptorSetLayoutBinding> native;for(const auto& b:layouts_->bindings)if(b.set==set)native.push_back({b.binding,descriptor_detail::type(b.kind),b.count,stages,nullptr});
            create.bindingCount=uint32_t(native.size());create.pBindings=native.data();descriptor_detail::check(layouts_->dispatch.vkCreateDescriptorSetLayout(layouts_->device,&create,nullptr,&layouts_->sets[set]),"native public descriptor layout");
        }
        VkDescriptorSetLayoutBinding guards[]={{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,stages,nullptr},{1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,stages,nullptr}};
        create.bindingCount=2;create.pBindings=guards;descriptor_detail::check(layouts_->dispatch.vkCreateDescriptorSetLayout(layouts_->device,&create,nullptr,&layouts_->sets.back()),"native guard descriptor layout");
        VkPipelineLayoutCreateInfo pipeline{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};pipeline.setLayoutCount=uint32_t(layouts_->sets.size());pipeline.pSetLayouts=layouts_->sets.data();descriptor_detail::check(layouts_->dispatch.vkCreatePipelineLayout(layouts_->device,&pipeline,nullptr,&layouts_->pipeline),"native descriptor pipeline layout");
    }
    VkPipelineLayout pipelineLayout() const{return layouts_->pipeline;}
    uint32_t guardSetIndex() const{return uint32_t(layouts_->sets.size()-1);}
    NativeDescriptorCounters counters() const{return *layouts_->counters;}
    // Resolver/resource mapping must remain stable for a CPU generation. If a
    // caller replaces its typed fallback mapping,explicitly drop this weak key.
    void invalidateResolverCache(){cache_.reset();}
    std::shared_ptr<NativeDescriptorSnapshot> materialize(std::shared_ptr<const DescriptorSnapshot> cpu,const Resolver& resolve,bool nativePublic=false){
        if(!cpu||cpu->abi!=abi_||!resolve||cpu->bindings.size()!=layouts_->bindings.size()||cpu->values.size()!=cpu->bindings.size()||cpu->metadata.empty()||cpu->metadata.size()>131072)
            throw std::invalid_argument("native descriptor snapshot shape");
        for(size_t i=0;i<cpu->bindings.size();i++){
            const auto& a=cpu->bindings[i];const auto& b=layouts_->bindings[i];if(a.set!=b.set||a.binding!=b.binding||a.count!=b.count||a.kind!=b.kind||cpu->values[i].size()!=b.count)throw std::invalid_argument("native snapshot/layout mismatch");
        }
        // Only borrowed Vulkan layouts admit application-owned ordinary sets.
        // Null descriptors still require the existing typed snapshot fallback.
        if(nativePublic){
            if(layouts_->ownsNative)throw std::invalid_argument("native public sets require borrowed layouts");
            for(const auto& row:cpu->values)for(const auto& value:row)if(value.status!=DescriptorStatus::Bound)throw std::invalid_argument("native public sets require bound resources");
        }
        if(auto cached=cache_.lock())if(cached->cpu_.get()==cpu.get()&&cached->nativePublic_==nativePublic)return cached;
        auto result=std::shared_ptr<NativeDescriptorSnapshot>(new NativeDescriptorSnapshot(layouts_,std::move(cpu)));
        result->nativePublic_=nativePublic;
        // Resolve/check everything before allocations/descriptor updates. Native
        // API never receives null handles: shader guards supply logical nulls.
        for(size_t row=0;row<layouts_->bindings.size();row++)for(uint32_t i=0;i<layouts_->bindings[row].count;i++){
            const auto& binding=layouts_->bindings[row];const auto& value=result->cpu_->values[row][i];
            if(value.status!=DescriptorStatus::Null&&value.status!=DescriptorStatus::Bound)throw std::invalid_argument("unwritten native descriptor");
            auto resource=resolve(binding,i,value);if(!resource.owner)throw std::invalid_argument("native resource lease missing");
            if(binding.kind==DescriptorKind::UniformBuffer||binding.kind==DescriptorKind::StorageBuffer){
                auto& b=resource.buffer;bool uniform=binding.kind==DescriptorKind::UniformBuffer;
                auto alignment=uniform?layouts_->limits.minUniformBufferOffsetAlignment:layouts_->limits.minStorageBufferOffsetAlignment;
                uint64_t maxRange=uniform?layouts_->limits.maxUniformBufferRange:layouts_->limits.maxStorageBufferRange;
                if(!b.buffer||b.offset>=resource.bufferBytes||b.offset%alignment)throw std::invalid_argument("native buffer handle/offset");
                if(b.range==VK_WHOLE_SIZE)b.range=resource.bufferBytes-b.offset;
                if(!b.range||b.range>resource.bufferBytes-b.offset||b.range>maxRange)throw std::invalid_argument("native buffer range");
                if(value.status==DescriptorStatus::Bound&&(value.offset!=b.offset||value.range!=b.range))throw std::invalid_argument("native buffer/guard mismatch");
            }else if(binding.kind==DescriptorKind::Sampler){
                if(value.status!=DescriptorStatus::Bound||!resource.image.sampler)throw std::invalid_argument("native sampler descriptor");
            }else if(binding.kind==DescriptorKind::UniformTexelBuffer||binding.kind==DescriptorKind::StorageTexelBuffer){
                if(!resource.texelView||!resource.texelElements||resource.texelElements>layouts_->limits.maxTexelBufferElements)throw std::invalid_argument("native texel buffer view/size");
                if(value.status==DescriptorStatus::Bound&&(value.offset||value.range!=resource.texelElements))throw std::invalid_argument("native texel/guard mismatch");
            }else{
                const auto& image=resource.image;
                if(!image.imageView||(binding.kind==DescriptorKind::CombinedImageSampler&&!image.sampler))throw std::invalid_argument("native image/view/sampler");
                if(!descriptor_detail::readableImageLayout(image.imageLayout,resource.imageAspects))throw std::invalid_argument("native image layout/aspect not supported");
                if(binding.kind==DescriptorKind::StorageImage&&image.imageLayout!=VK_IMAGE_LAYOUT_GENERAL)throw std::invalid_argument("native storage image layout");
            }
            result->resources_.push_back(std::move(resource));
        }
        auto host=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,coherent=VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        if(layouts_->uniformMetadataWords&&result->cpu_->metadata.size()!=layouts_->uniformMetadataWords)throw std::invalid_argument("uniform metadata ABI word count");
        result->metadataBytes_=layouts_->uniformMetadataWords?((result->cpu_->metadata.size()+3)/4)*16:result->cpu_->metadata.size()*sizeof(uint32_t);
        if(layouts_->metadataArena){auto slice=layouts_->metadataArena->allocate(result->metadataBytes_);result->metadata_=std::move(slice.buffer);result->metadataOffset_=slice.offset;}
        else result->metadata_=std::make_shared<mavk::Buffer>(context_,result->metadataBytes_,layouts_->uniformMetadataWords?VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT:VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,host,coherent);
        auto target=static_cast<unsigned char*>(result->metadata_->map())+result->metadataOffset_;
        if(layouts_->uniformMetadataWords){std::memset(target,0,result->metadataBytes_);std::memcpy(target,result->cpu_->metadata.data(),result->cpu_->metadata.size()*4);}
        else std::memcpy(target,result->cpu_->metadata.data(),result->metadataBytes_);result->metadata_->flush();
        if(!layouts_->diagnosticWrites&&layouts_->quietLog)result->log_=layouts_->quietLog;
        else{
            result->log_=std::make_shared<mavk::Buffer>(context_,32768,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,host,coherent);
            std::memset(result->log_->map(),0,32768);result->log_->flush();layouts_->counters->diagnosticAllocations++;
            if(!layouts_->diagnosticWrites)layouts_->quietLog=result->log_;
        }
        std::vector<VkDescriptorPoolSize> sizes{{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,layouts_->uniformMetadataWords?1u:2u}};
        if(layouts_->uniformMetadataWords)sizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1});
        if(!nativePublic)for(const auto& b:layouts_->allocationBindings){auto type=layouts_->nativeType(b);auto at=std::find_if(sizes.begin(),sizes.end(),[&](const auto& s){return s.type==type;});if(at==sizes.end())sizes.push_back({type,b.count});else at->descriptorCount+=b.count;}
        result->sets_.resize(layouts_->sets.size(),VK_NULL_HANDLE);
        std::vector<VkDescriptorSetLayout> ownedLayouts;std::vector<uint32_t> ownedIndices;
        for(uint32_t i=0;i<layouts_->sets.size();i++)if((!nativePublic||i+1==layouts_->sets.size())&&std::find(layouts_->externalSets.begin(),layouts_->externalSets.end(),i)==layouts_->externalSets.end()){ownedIndices.push_back(i);ownedLayouts.push_back(layouts_->sets[i]);}
        std::vector<VkDescriptorSet> ownedSets(ownedLayouts.size());
        VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=uint32_t(ownedLayouts.size());pool.poolSizeCount=uint32_t(sizes.size());pool.pPoolSizes=sizes.data();descriptor_detail::check(layouts_->dispatch.vkCreateDescriptorPool(layouts_->device,&pool,nullptr,&result->pool_),"native descriptor pool");
        VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};allocate.descriptorPool=result->pool_;allocate.descriptorSetCount=uint32_t(ownedLayouts.size());allocate.pSetLayouts=ownedLayouts.data();descriptor_detail::check(layouts_->dispatch.vkAllocateDescriptorSets(layouts_->device,&allocate,ownedSets.data()),"native descriptor sets");
        for(size_t i=0;i<ownedSets.size();i++)result->sets_[ownedIndices[i]]=ownedSets[i];
        std::vector<VkWriteDescriptorSet> writes;size_t index=0;
        if(!nativePublic)for(const auto& b:layouts_->bindings)for(uint32_t i=0;i<b.count;i++,index++){
            if(b.kind==DescriptorKind::Sampler&&layouts_->immutable(b))continue;
            VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=result->sets_[b.set];write.dstBinding=b.binding;write.dstArrayElement=i;write.descriptorCount=1;write.descriptorType=layouts_->nativeType(b);
            if(b.kind==DescriptorKind::UniformBuffer||b.kind==DescriptorKind::StorageBuffer)write.pBufferInfo=&result->resources_[index].buffer;
            else if(b.kind==DescriptorKind::UniformTexelBuffer||b.kind==DescriptorKind::StorageTexelBuffer)write.pTexelBufferView=&result->resources_[index].texelView;
            else write.pImageInfo=&result->resources_[index].image;writes.push_back(write);
        }
        VkDescriptorBufferInfo guards[]={{result->log_->handle(),0,result->log_->size()},{result->metadata_->handle(),result->metadataOffset_,result->metadataBytes_}};
        for(uint32_t i=0;i<2;i++){VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=result->sets_.back();write.dstBinding=i;write.descriptorCount=1;write.descriptorType=i&&layouts_->uniformMetadataWords?VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;write.pBufferInfo=&guards[i];writes.push_back(write);}
        layouts_->dispatch.vkUpdateDescriptorSets(layouts_->device,uint32_t(writes.size()),writes.data(),0,nullptr);
        auto& count=*layouts_->counters;count.poolCreates++;count.metadataUploads++;count.updates++;count.liveSnapshots++;result->counted_=true;cache_=result;return result;
    }
private:
    mavk::DeviceContext& context_;std::shared_ptr<descriptor_detail::Layouts> layouts_;GuardMetadataAbi abi_;std::weak_ptr<NativeDescriptorSnapshot> cache_;
};
}
