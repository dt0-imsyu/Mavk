#pragma once
#include <mavk/core.hpp>
#include <mavk/compat/descriptors.hpp>
#include <array>
#include <cstring>
#include <functional>
#include <string>

namespace mavk::compat {
struct NativeDescriptorResource {
    // Owns the REAL native resource/view/sampler dependencies,including typed
    // fallbacks for logical null. Caller must not use a non-owning placeholder.
    std::shared_ptr<const void> owner;
    VkDescriptorBufferInfo buffer{};
    uint64_t bufferBytes=0;
    VkDescriptorImageInfo image{};
    VkBufferView texelView{};uint64_t texelElements=0;
};
struct NativeDescriptorCounters {uint64_t poolCreates=0,metadataUploads=0,updates=0,liveSnapshots=0;};
namespace descriptor_detail {
inline void check(VkResult r,const char* message){if(r!=VK_SUCCESS)throw std::runtime_error(std::string(message)+": "+std::to_string(r));}
inline VkDescriptorType type(DescriptorKind k){switch(k){
    case DescriptorKind::UniformBuffer:return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case DescriptorKind::StorageBuffer:return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case DescriptorKind::CombinedImageSampler:return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    case DescriptorKind::SampledImage:return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    case DescriptorKind::StorageImage:return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    case DescriptorKind::UniformTexelBuffer:return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    case DescriptorKind::StorageTexelBuffer:return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
}throw std::invalid_argument("native descriptor kind");}
struct Layouts {
    VkDevice device{};std::vector<DescriptorBinding> bindings;std::vector<VkDescriptorSetLayout> sets;VkPipelineLayout pipeline{};
    VkPhysicalDeviceLimits limits{};std::shared_ptr<NativeDescriptorCounters> counters=std::make_shared<NativeDescriptorCounters>();
    ~Layouts(){if(pipeline)vkDestroyPipelineLayout(device,pipeline,nullptr);for(auto s:sets)if(s)vkDestroyDescriptorSetLayout(device,s,nullptr);}
};
}
// Immutable native generation. Do not release until every GPU user completes.
// No destructor wait/queue idle: the submission owner explicitly controls life.
class NativeDescriptorSnapshot {
public:
    NativeDescriptorSnapshot(const NativeDescriptorSnapshot&)=delete;
    ~NativeDescriptorSnapshot(){if(pool_)vkDestroyDescriptorPool(layouts_->device,pool_,nullptr);if(counted_)layouts_->counters->liveSnapshots--;}
    const std::vector<VkDescriptorSet>& sets() const{return sets_;}
    VkPipelineLayout pipelineLayout() const{return layouts_->pipeline;}
    const DescriptorSnapshot& cpu() const{return *cpu_;}
    // Caller must have completed GPU use before host read/reset of diagnostics.
    const uint32_t* readDiagnostics(){log_->invalidate();return static_cast<const uint32_t*>(log_->map());}
private:
    friend class NativeDescriptorAdapter;
    NativeDescriptorSnapshot(std::shared_ptr<descriptor_detail::Layouts> layouts,std::shared_ptr<const DescriptorSnapshot> cpu):layouts_(std::move(layouts)),cpu_(std::move(cpu)){}
    std::shared_ptr<descriptor_detail::Layouts> layouts_;std::shared_ptr<const DescriptorSnapshot> cpu_;
    std::vector<NativeDescriptorResource> resources_;std::unique_ptr<mavk::Buffer> metadata_,log_;
    VkDescriptorPool pool_{};std::vector<VkDescriptorSet> sets_;bool counted_=false;
};
// Bounded native backend API,not a loader layer or complete VkDescriptor API.
// Public sets followed by one private guard set; caller supplies real shader/layout
// admission and typed fallbacks. Serialized/external queue synchronization.
class NativeDescriptorAdapter {
public:
    using Resolver=std::function<NativeDescriptorResource(const DescriptorBinding&,uint32_t,const DescriptorValue&)>;
    NativeDescriptorAdapter(mavk::DeviceContext& context,std::vector<DescriptorBinding> bindings,GuardMetadataAbi abi,VkShaderStageFlags stages=VK_SHADER_STAGE_COMPUTE_BIT,uint32_t publicSetCount=0):context_(context),layouts_(std::make_shared<descriptor_detail::Layouts>()),abi_(abi){
        // Reuse common layout validation before any native resource creation.
        DescriptorState validated(bindings);(void)validated;
        if(abi!=GuardMetadataAbi::BufferRanges&&abi!=GuardMetadataAbi::IndexAndInit)throw std::invalid_argument("native shader metadata ABI");
        std::sort(bindings.begin(),bindings.end(),[](const auto& a,const auto& b){return a.set<b.set||(a.set==b.set&&a.binding<b.binding);});
        if(!publicSetCount)publicSetCount=bindings.back().set+1;
        if(publicSetCount>7||bindings.back().set>=publicSetCount)throw std::invalid_argument("native public descriptor set count");
        if(!stages)throw std::invalid_argument("native descriptor stage visibility");
        layouts_->device=context.vkDevice();layouts_->bindings=std::move(bindings);
        VkPhysicalDeviceProperties properties;vkGetPhysicalDeviceProperties(context.physicalDevice(),&properties);layouts_->limits=properties.limits;
        if(properties.limits.maxBoundDescriptorSets<publicSetCount+1)throw std::runtime_error("guard descriptor set unavailable");
        layouts_->sets.resize(publicSetCount+1,VK_NULL_HANDLE);
        VkDescriptorSetLayoutCreateInfo create{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        for(uint32_t set=0;set<publicSetCount;set++){
            std::vector<VkDescriptorSetLayoutBinding> native;for(const auto& b:layouts_->bindings)if(b.set==set)native.push_back({b.binding,descriptor_detail::type(b.kind),b.count,stages,nullptr});
            create.bindingCount=uint32_t(native.size());create.pBindings=native.data();descriptor_detail::check(vkCreateDescriptorSetLayout(layouts_->device,&create,nullptr,&layouts_->sets[set]),"native public descriptor layout");
        }
        VkDescriptorSetLayoutBinding guards[]={{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,stages,nullptr},{1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,stages,nullptr}};
        create.bindingCount=2;create.pBindings=guards;descriptor_detail::check(vkCreateDescriptorSetLayout(layouts_->device,&create,nullptr,&layouts_->sets.back()),"native guard descriptor layout");
        VkPipelineLayoutCreateInfo pipeline{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};pipeline.setLayoutCount=uint32_t(layouts_->sets.size());pipeline.pSetLayouts=layouts_->sets.data();descriptor_detail::check(vkCreatePipelineLayout(layouts_->device,&pipeline,nullptr,&layouts_->pipeline),"native descriptor pipeline layout");
    }
    VkPipelineLayout pipelineLayout() const{return layouts_->pipeline;}
    uint32_t guardSetIndex() const{return uint32_t(layouts_->sets.size()-1);}
    NativeDescriptorCounters counters() const{return *layouts_->counters;}
    // Resolver/resource mapping must remain stable for a CPU generation. If a
    // caller replaces its typed fallback mapping,explicitly drop this weak key.
    void invalidateResolverCache(){cache_.reset();}
    std::shared_ptr<NativeDescriptorSnapshot> materialize(std::shared_ptr<const DescriptorSnapshot> cpu,const Resolver& resolve){
        if(!cpu||cpu->abi!=abi_||!resolve||cpu->bindings.size()!=layouts_->bindings.size()||cpu->values.size()!=cpu->bindings.size()||cpu->metadata.empty()||cpu->metadata.size()>131072)
            throw std::invalid_argument("native descriptor snapshot shape");
        for(size_t i=0;i<cpu->bindings.size();i++){
            const auto& a=cpu->bindings[i];const auto& b=layouts_->bindings[i];if(a.set!=b.set||a.binding!=b.binding||a.count!=b.count||a.kind!=b.kind||cpu->values[i].size()!=b.count)throw std::invalid_argument("native snapshot/layout mismatch");
        }
        if(auto cached=cache_.lock())if(cached->cpu_.get()==cpu.get())return cached;
        auto result=std::shared_ptr<NativeDescriptorSnapshot>(new NativeDescriptorSnapshot(layouts_,std::move(cpu)));
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
            }else if(binding.kind==DescriptorKind::UniformTexelBuffer||binding.kind==DescriptorKind::StorageTexelBuffer){
                if(!resource.texelView||!resource.texelElements||resource.texelElements>layouts_->limits.maxTexelBufferElements)throw std::invalid_argument("native texel buffer view/size");
                if(value.status==DescriptorStatus::Bound&&(value.offset||value.range!=resource.texelElements))throw std::invalid_argument("native texel/guard mismatch");
            }else{
                const auto& image=resource.image;
                if(!image.imageView||(binding.kind==DescriptorKind::CombinedImageSampler&&!image.sampler))throw std::invalid_argument("native image/view/sampler");
                if(image.imageLayout!=VK_IMAGE_LAYOUT_GENERAL&&image.imageLayout!=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)throw std::invalid_argument("native image layout not supported");
                if(binding.kind==DescriptorKind::StorageImage&&image.imageLayout!=VK_IMAGE_LAYOUT_GENERAL)throw std::invalid_argument("native storage image layout");
            }
            result->resources_.push_back(std::move(resource));
        }
        auto host=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,coherent=VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        result->metadata_=std::make_unique<mavk::Buffer>(context_,result->cpu_->metadata.size()*4,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,host,coherent);
        std::memcpy(result->metadata_->map(),result->cpu_->metadata.data(),result->cpu_->metadata.size()*4);result->metadata_->flush();
        result->log_=std::make_unique<mavk::Buffer>(context_,32768,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,host,coherent);std::memset(result->log_->map(),0,32768);result->log_->flush();
        std::vector<VkDescriptorPoolSize> sizes{{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,2}};
        for(const auto& b:layouts_->bindings){auto type=descriptor_detail::type(b.kind);auto at=std::find_if(sizes.begin(),sizes.end(),[&](const auto& s){return s.type==type;});if(at==sizes.end())sizes.push_back({type,b.count});else at->descriptorCount+=b.count;}
        result->sets_.resize(layouts_->sets.size(),VK_NULL_HANDLE);
        VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=uint32_t(result->sets_.size());pool.poolSizeCount=uint32_t(sizes.size());pool.pPoolSizes=sizes.data();descriptor_detail::check(vkCreateDescriptorPool(layouts_->device,&pool,nullptr,&result->pool_),"native descriptor pool");
        VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};allocate.descriptorPool=result->pool_;allocate.descriptorSetCount=uint32_t(result->sets_.size());allocate.pSetLayouts=layouts_->sets.data();descriptor_detail::check(vkAllocateDescriptorSets(layouts_->device,&allocate,result->sets_.data()),"native descriptor sets");
        std::vector<VkWriteDescriptorSet> writes;size_t index=0;
        for(const auto& b:layouts_->bindings)for(uint32_t i=0;i<b.count;i++,index++){
            VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=result->sets_[b.set];write.dstBinding=b.binding;write.dstArrayElement=i;write.descriptorCount=1;write.descriptorType=descriptor_detail::type(b.kind);
            if(b.kind==DescriptorKind::UniformBuffer||b.kind==DescriptorKind::StorageBuffer)write.pBufferInfo=&result->resources_[index].buffer;
            else if(b.kind==DescriptorKind::UniformTexelBuffer||b.kind==DescriptorKind::StorageTexelBuffer)write.pTexelBufferView=&result->resources_[index].texelView;
            else write.pImageInfo=&result->resources_[index].image;writes.push_back(write);
        }
        VkDescriptorBufferInfo guards[]={{result->log_->handle(),0,result->log_->size()},{result->metadata_->handle(),0,result->metadata_->size()}};
        for(uint32_t i=0;i<2;i++){VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=result->sets_.back();write.dstBinding=i;write.descriptorCount=1;write.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;write.pBufferInfo=&guards[i];writes.push_back(write);}
        vkUpdateDescriptorSets(layouts_->device,uint32_t(writes.size()),writes.data(),0,nullptr);
        auto& count=*layouts_->counters;count.poolCreates++;count.metadataUploads++;count.updates++;count.liveSnapshots++;result->counted_=true;cache_=result;return result;
    }
private:
    mavk::DeviceContext& context_;std::shared_ptr<descriptor_detail::Layouts> layouts_;GuardMetadataAbi abi_;std::weak_ptr<NativeDescriptorSnapshot> cache_;
};
}
