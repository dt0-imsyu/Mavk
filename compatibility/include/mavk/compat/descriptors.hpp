#pragma once
#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

namespace mavk::compat {
enum class DescriptorKind {UniformBuffer,StorageBuffer,CombinedImageSampler,SampledImage,StorageImage};
enum class DescriptorStatus {Unwritten,Null,Bound};
enum class GuardMetadataAbi {BufferRanges,IndexAndInit};
struct DescriptorBinding {uint32_t set,binding,count;DescriptorKind kind;};
struct DescriptorLimits {uint64_t uniformOffsetAlignment=1,storageOffsetAlignment=1;};
// Trusted adapter supplies a lease owning the real resource(s). The common
// state never manufactures a native handle or a false ownership reference.
struct DescriptorValue {
    DescriptorStatus status=DescriptorStatus::Unwritten;
    std::shared_ptr<const void> owner;
    uint64_t offset=0,range=0;
};
struct DescriptorSnapshot {
    uint64_t generation=0;
    GuardMetadataAbi abi{};
    std::vector<uint32_t> metadata;
    std::vector<DescriptorBinding> bindings;
    std::vector<std::vector<DescriptorValue>> values;
    const DescriptorValue& at(uint32_t set,uint32_t binding,uint32_t index) const {
        for(size_t i=0;i<bindings.size();i++)if(bindings[i].set==set&&bindings[i].binding==binding){
            if(index>=values[i].size())throw std::out_of_range("descriptor snapshot element");return values[i][index];
        }
        throw std::out_of_range("descriptor snapshot binding");
    }
};
// CPU-only shared descriptor state. No VkDevice, guest protocol, rendering or
// feature advertising. Serialized caller; immutable submitted snapshots retain
// strong resource leases. Caller releases those snapshots AFTER GPU completion.
class DescriptorState {
public:
    explicit DescriptorState(std::vector<DescriptorBinding> bindings,DescriptorLimits limits={}):bindings_(std::move(bindings)),limits_(limits){
        if(bindings_.empty()||bindings_.size()>1024||!limits.uniformOffsetAlignment||!limits.storageOffsetAlignment)
            throw std::invalid_argument("descriptor layout/limits");
        std::sort(bindings_.begin(),bindings_.end(),[](const auto& a,const auto& b){return a.set<b.set||(a.set==b.set&&a.binding<b.binding);});
        uint64_t total=0;
        for(size_t i=0;i<bindings_.size();i++){
            const auto& b=bindings_[i];
            if(b.set>=8||b.binding>=1024||!b.count||b.count>65536||(total+=b.count)>65536)
                throw std::invalid_argument("descriptor layout bound");
            switch(b.kind){case DescriptorKind::UniformBuffer:case DescriptorKind::StorageBuffer:case DescriptorKind::CombinedImageSampler:case DescriptorKind::SampledImage:case DescriptorKind::StorageImage:break;default:throw std::invalid_argument("descriptor kind not implemented");}
            if(i&&bindings_[i-1].set==b.set&&bindings_[i-1].binding==b.binding)throw std::invalid_argument("duplicate descriptor binding");
            values_.emplace_back(b.count);
        }
    }
    uint64_t generation() const{return generation_;}
    const DescriptorValue& value(uint32_t set,uint32_t binding,uint32_t element) const {auto row=find(set,binding);requireRange(row,element,1);return values_[row][element];}
    // Copying an undefined Vulkan descriptor propagates undefined state. This
    // does not make an unwritten descriptor usable by a GPU snapshot.
    void setUnwritten(uint32_t set,uint32_t binding,uint32_t element){auto row=find(set,binding);requireRange(row,element,1);assign(row,element,{});}
    void setNull(uint32_t set,uint32_t binding,uint32_t element,std::shared_ptr<const void> owner={}){
        auto row=find(set,binding);requireRange(row,element,1);DescriptorValue value;value.status=DescriptorStatus::Null;value.owner=std::move(owner);assign(row,element,value);
    }
    void bindBuffer(uint32_t set,uint32_t binding,uint32_t element,std::shared_ptr<const void> owner,uint64_t allocationBytes,uint64_t offset,uint64_t range){
        auto row=find(set,binding);requireRange(row,element,1);auto kind=bindings_[row].kind;
        if(kind!=DescriptorKind::UniformBuffer&&kind!=DescriptorKind::StorageBuffer)throw std::invalid_argument("buffer descriptor kind");
        uint64_t alignment=kind==DescriptorKind::UniformBuffer?limits_.uniformOffsetAlignment:limits_.storageOffsetAlignment;
        if(!owner||offset>=allocationBytes||offset%alignment)throw std::invalid_argument("buffer descriptor owner/offset");
        if(range==std::numeric_limits<uint64_t>::max())range=allocationBytes-offset;
        if(!range||range>allocationBytes-offset||range>UINT32_MAX)throw std::invalid_argument("buffer descriptor range/guard32 bound");
        assign(row,element,{DescriptorStatus::Bound,std::move(owner),offset,range});
    }
    void bindImage(uint32_t set,uint32_t binding,uint32_t element,std::shared_ptr<const void> owner){
        auto row=find(set,binding);requireRange(row,element,1);auto kind=bindings_[row].kind;
        if((kind!=DescriptorKind::CombinedImageSampler&&kind!=DescriptorKind::SampledImage&&kind!=DescriptorKind::StorageImage)||!owner)
            throw std::invalid_argument("image descriptor kind/owner");
        assign(row,element,{DescriptorStatus::Bound,std::move(owner),0,1});
    }
    // One explicitly compatible binding range, not Vulkan's cross-binding spill.
    // Snapshot before writing so same-array overlap behaves like memmove.
    void copyFrom(const DescriptorState& source,uint32_t sourceSet,uint32_t sourceBinding,uint32_t sourceFirst,uint32_t set,uint32_t binding,uint32_t first,uint32_t count){
        auto sr=source.find(sourceSet,sourceBinding),dr=find(set,binding);source.requireRange(sr,sourceFirst,count);requireRange(dr,first,count);
        if(source.bindings_[sr].kind!=bindings_[dr].kind)throw std::invalid_argument("descriptor copy kind mismatch");
        const auto& sv=source.values_[sr];std::vector<DescriptorValue> copy(sv.begin()+sourceFirst,sv.begin()+sourceFirst+count);
        for(const auto& value:copy){
            if(value.status==DescriptorStatus::Unwritten)throw std::invalid_argument("copy from unwritten descriptor");
            const auto kind=bindings_[dr].kind;
            if(value.status==DescriptorStatus::Bound&&(kind==DescriptorKind::UniformBuffer||kind==DescriptorKind::StorageBuffer)){
                uint64_t align=kind==DescriptorKind::UniformBuffer?limits_.uniformOffsetAlignment:limits_.storageOffsetAlignment;
                if(value.offset%align)throw std::invalid_argument("descriptor copy target alignment");
            }
        }
        bool changed=false;for(uint32_t i=0;i<count;i++)changed|=!equal(copy[i],values_[dr][first+i]);
        if(!changed)return;changedGeneration();for(uint32_t i=0;i<count;i++)values_[dr][first+i]=std::move(copy[i]);
    }
    std::shared_ptr<const DescriptorSnapshot> snapshot(GuardMetadataAbi abi) const {
        if(abi!=GuardMetadataAbi::BufferRanges&&abi!=GuardMetadataAbi::IndexAndInit)throw std::invalid_argument("guard metadata ABI");
        auto& cache=abi==GuardMetadataAbi::BufferRanges?rangeCache_:indexCache_;if(cache)return cache;
        for(const auto& row:values_)for(const auto& value:row)if(value.status==DescriptorStatus::Unwritten)throw std::invalid_argument("unwritten descriptor cannot be submitted; explicit null required");
        auto out=std::make_shared<DescriptorSnapshot>();out->generation=generation_;out->abi=abi;out->bindings=bindings_;out->values=values_;
        const uint32_t sets=bindings_.back().set+1;const bool indexed=abi==GuardMetadataAbi::IndexAndInit;
        out->metadata.resize(1+sets+(indexed?sets:0),0);if(indexed)out->metadata[0]=1+sets;
        auto append=[&](uint32_t count){uint32_t at=uint32_t(out->metadata.size());if(uint64_t(at)+count>131072)throw std::length_error("guard metadata word budget");out->metadata.resize(size_t(at)+count,0);return at;};
        for(uint32_t set=0;set<sets;set++){
            uint32_t count=0;for(const auto& b:bindings_)if(b.set==set)count=std::max(count,b.binding+1);
            // Empty set has one sentinel entry; shader/layout validation must
            // still reject undeclared bindings, this is not a sandbox boundary.
            count=std::max(1u,count);uint32_t table=append(count),lengths=indexed?append(count):0;
            out->metadata[indexed?out->metadata[0]+set:set+1]=table;if(indexed)out->metadata[set+1]=lengths;
            for(size_t row=0;row<bindings_.size();row++)if(bindings_[row].set==set){
                const auto& b=bindings_[row];uint32_t entries=append(b.count);out->metadata[table+b.binding]=entries;if(indexed)out->metadata[lengths+b.binding]=b.count;
                for(uint32_t i=0;i<b.count;i++)out->metadata[entries+i]=values_[row][i].status==DescriptorStatus::Bound?uint32_t(values_[row][i].range):0;
            }
        }
        cache=out;return cache;
    }
private:
    size_t find(uint32_t set,uint32_t binding) const {for(size_t i=0;i<bindings_.size();i++)if(bindings_[i].set==set&&bindings_[i].binding==binding)return i;throw std::out_of_range("descriptor binding");}
    void requireRange(size_t row,uint32_t first,uint32_t count) const {if(!count||first>=values_[row].size()||count>values_[row].size()-first)throw std::out_of_range("descriptor array range");}
    static bool equal(const DescriptorValue& a,const DescriptorValue& b){return a.status==b.status&&a.owner==b.owner&&!a.owner.owner_before(b.owner)&&!b.owner.owner_before(a.owner)&&a.offset==b.offset&&a.range==b.range;}
    void changedGeneration(){if(generation_==UINT64_MAX)throw std::overflow_error("descriptor generation exhausted");generation_++;rangeCache_.reset();indexCache_.reset();}
    void assign(size_t row,uint32_t index,const DescriptorValue& value){if(equal(values_[row][index],value))return;changedGeneration();values_[row][index]=value;}
    std::vector<DescriptorBinding> bindings_;std::vector<std::vector<DescriptorValue>> values_;DescriptorLimits limits_;uint64_t generation_=0;
    mutable std::shared_ptr<const DescriptorSnapshot> rangeCache_,indexCache_;
};
}
