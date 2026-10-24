#pragma once
#include <mavk/core.hpp>
#include <mavk/sampled.hpp>
#include <memory>
#include <vector>
#include "vertex_geometry.hpp"
#include "scene.hpp"

namespace mavk::bridge {
// Optional native profiling only; durations are CPU wall time, not GPU queries.
struct OffscreenTiming {double initializeMs{},pipelineCreateMs{},recordMs{},submitWaitMs{},readbackMs{};};
// Native straight-alpha source-over; not a claim of arbitrary blend equations.
struct RasterOptions {
    bool sourceOver=false;
    // Pinned upstream UI: identical color factors, alpha=(1-srcA)*srcA.
    // Mutually exclusive with sourceOver; never substitute either equation.
    bool uiOverlayAlpha=false;
    uint32_t sampledBinding=1;
    // Optional native GS; caller enables geometryShader on the borrowed device.
    std::vector<uint32_t> geometryShader;
    // Optional native TCS/TES pair; caller enables tessellationShader.
    std::vector<uint32_t> tessellationControlShader,tessellationEvaluationShader;
    uint32_t patchControlPoints=3;
    VkShaderStageFlags uniformStages=VK_SHADER_STAGE_VERTEX_BIT;
};
struct LayerFrame {
    const void* push=nullptr;
    const std::vector<uint8_t>* uniform=nullptr;
    const VkRect2D* clip=nullptr;
};
// Shared native renderer, no Bridge transport dependency. Immutable pipeline,
// geometry and target; push data and same-sized UBO may change between renders.
// Synchronous readback remains explicit; this is not asynchronous/zero-copy WSI.
// Caller serializes render/queue access and keeps DeviceContext, device and any
// external uploadedTexture alive until this session has been destroyed.
// render clip is target-bounded and nonempty; nullptr resets to the full target.
// RasterOptions are immutable per session, clipping is dynamic per frame.
class OffscreenSession {
public:
    OffscreenSession(DeviceContext&,VkQueue,uint32_t,uint32_t,
        const std::vector<uint32_t>&,const std::vector<uint32_t>&,
        const VertexGeometry* geometry=nullptr,uint32_t pushBytes=16,
        VkShaderStageFlags pushStages=VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
        const SceneState* scene=nullptr,const std::vector<uint8_t>* uniform=nullptr,
        const SampledPixels* texture=nullptr,SampledImage* uploadedTexture=nullptr,
        const RasterOptions* raster=nullptr,OffscreenSession* previousLayer=nullptr);
    ~OffscreenSession();OffscreenSession(const OffscreenSession&)=delete;
    OffscreenSession& operator=(const OffscreenSession&)=delete;
    std::vector<uint8_t> render(const void* push,const float* background,
        const std::vector<uint8_t>* uniform=nullptr,OffscreenTiming* timing=nullptr,
        const VkRect2D* clip=nullptr);
    // Previous layers are retained internally. Root clears, following passes
    // LOAD the same color attachment; depth is local/cleared per pass. One GPU
    // submit and one final readback. Frames ordered root -> this, max16 layers.
    std::vector<uint8_t> renderLayers(const std::vector<LayerFrame>&,const float* background);
private:struct Impl;std::shared_ptr<Impl> impl_;
};
// Bounded trusted graphics slice. Not an ICD command decoder or WSI path.
std::vector<uint8_t> drawTriangle(DeviceContext&,VkQueue,uint32_t,uint32_t);
std::vector<uint8_t> drawOffscreen(DeviceContext&,VkQueue,uint32_t,uint32_t,
    const std::vector<uint32_t>&,const std::vector<uint32_t>&,const void*,const float*,const VertexGeometry* = nullptr,
    uint32_t pushBytes=16,VkShaderStageFlags pushStages=VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
    const SceneState* scene=nullptr,const std::vector<uint8_t>* uniform=nullptr,
    const SampledPixels* texture=nullptr,SampledImage* uploadedTexture=nullptr,OffscreenTiming* timing=nullptr,
    const RasterOptions* raster=nullptr,const VkRect2D* clip=nullptr);
}
