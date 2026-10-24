#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <mavk/compat/shader_guards.hpp>

namespace mavk::compat {
enum class CullPrimitive { Points, Lines, Triangles };
enum class CullProducer { Vertex, TessellationEvaluation };
struct CullDistanceProgram {
    std::vector<uint32_t> vertex,geometry;
    uint32_t distances=0,inputComponents=0,outputComponents=0,vertices=0;
    uint32_t locations=0;
};
// Explicit pipeline transformation, not Vulkan feature admission. Requires a
// native geometry stage after VS or TES; vertex holds the patched producer.
// Rejects existing GS/XFB/clip-distance contracts; adapters must
// check physical geometry/component limits before creating the native pipeline.
CullDistanceProgram compileCullDistance(const std::vector<uint32_t>& vertex,
    const std::vector<uint32_t>& fragment,CullPrimitive primitive,
    GuardTarget target=GuardTarget::Vulkan11,
    CullProducer producer=CullProducer::Vertex);
}
