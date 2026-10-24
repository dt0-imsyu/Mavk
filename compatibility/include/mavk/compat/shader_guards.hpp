#pragma once
#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <vector>

namespace mavk::compat {
// Experimental ABI: public sets [0,count),private set count. The default uses
// IndexAndInit; texelFormatMetadata requires the matching IndexInitFormats ABI.
// Not exhaustive shader admission or a Vulkan robustness feature declaration.
// Target selection is explicit and immutable; Vulkan11 remains the default.
// Vulkan12/13 admit supported SPIR-V1.5/1.6 forms,not every extension/instruction.
// Serialized callers only. Returned immutable words survive cache eviction.
struct GuardCompilerStats {uint64_t compiled=0,hits=0;size_t entries=0,retainedBytes=0;};
enum class GuardTarget {Vulkan11=0,Vulkan13=1,Vulkan12=2};
struct DescriptorUse {uint32_t set,binding,variable;};
// Static use is defined by ID operands in the selected entry point's call tree,
// not by declarations, interface lists, or specialization-dependent execution.
std::vector<DescriptorUse> staticDescriptorUses(const std::vector<uint32_t>&,
    const std::string& entryPoint,uint32_t executionModel,GuardTarget target=GuardTarget::Vulkan11);
std::vector<DescriptorUse> staticDescriptorUses(const std::vector<uint32_t>&,
    const std::string& entryPoint,uint32_t executionModel,GuardTarget target,bool uniformBufferStandardLayout);
struct GuardCompilerOptions {uint32_t publicSetCount=1;GuardTarget target=GuardTarget::Vulkan11;bool reportErrors=true;bool texelFormatMetadata=false;bool uniformBufferStandardLayout=false;bool metadataReadOnly=false;bool optimizeMetadataReads=false;};
class GuardCompiler {
public:
    using Words=std::vector<uint32_t>;
    explicit GuardCompiler(size_t maxEntries=16,size_t maxBytes=8*1024*1024,GuardCompilerOptions options={});
    std::shared_ptr<const Words> compile(const Words&);
    GuardCompilerStats stats() const;
    uint32_t guardSetIndex() const{return publicSets_;}
private:
    struct Entry {Words original;std::shared_ptr<const Words> guarded;size_t bytes;};
    size_t maxEntries_,maxBytes_,retained_=0;uint64_t compiled_=0,hits_=0;
    uint32_t publicSets_=1;
    GuardTarget target_=GuardTarget::Vulkan11;
    bool reportErrors_=true;
    bool texelFormatMetadata_=false;
    bool uniformBufferStandardLayout_=false;
    bool metadataReadOnly_=false;
    bool optimizeMetadataReads_=false;
    std::list<Entry> cache_;
};
}
