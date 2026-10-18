#pragma once
#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>
#include <vector>

namespace mavk::compat {
// Fixed experimental ABI: original set0,private set1,IndexAndInit metadata.
// Not exhaustive shader admission or a Vulkan robustness feature declaration.
// Serialized callers only. Returned immutable words survive cache eviction.
struct GuardCompilerStats {uint64_t compiled=0,hits=0;size_t entries=0,retainedBytes=0;};
class GuardCompiler {
public:
    using Words=std::vector<uint32_t>;
    explicit GuardCompiler(size_t maxEntries=16,size_t maxBytes=8*1024*1024);
    std::shared_ptr<const Words> compile(const Words&);
    GuardCompilerStats stats() const;
private:
    struct Entry {Words original;std::shared_ptr<const Words> guarded;size_t bytes;};
    size_t maxEntries_,maxBytes_,retained_=0;uint64_t compiled_=0,hits_=0;
    std::list<Entry> cache_;
};
}
