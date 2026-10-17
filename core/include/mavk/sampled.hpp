#pragma once
#include <mavk/core.hpp>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <vector>
#include <cmath>

namespace mavk {
struct RgbaMip { uint32_t width,height; VkDeviceSize offset,bytes; };
// Tight RGBA8 mip chain. Does not parse/rewrite KTX or generate missing levels.
inline std::vector<RgbaMip> rgbaMipLayout(uint32_t width,uint32_t height,uint32_t levels){
    if(!width||!height||width>1024||height>1024||!levels)
        throw std::invalid_argument("Mavk RGBA mip extent/level bound");
    uint32_t full=1;for(uint32_t n=std::max(width,height);n>1;n>>=1)full++;
    if(levels>full)throw std::invalid_argument("Mavk excess mip levels");
    std::vector<RgbaMip> out;VkDeviceSize offset=0;
    for(uint32_t i=0;i<levels;i++){
        VkDeviceSize bytes=VkDeviceSize(width)*height*4;
        out.push_back({width,height,offset,bytes});offset+=bytes;
        width=std::max(1u,width/2);height=std::max(1u,height/2);
    }
    return out;
}
struct SamplerState {
    VkFilter mag=VK_FILTER_NEAREST,min=VK_FILTER_NEAREST;
    VkSamplerMipmapMode mip=VK_SAMPLER_MIPMAP_MODE_NEAREST;
    VkSamplerAddressMode u=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                         v=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                         w=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    float bias=0,minLod=0,maxLod=VK_LOD_CLAMP_NONE;
};
inline bool samplerStateValid(const SamplerState& s,float maxBias){
    auto filter=[](VkFilter f){return f==VK_FILTER_NEAREST||f==VK_FILTER_LINEAR;};
    auto address=[](VkSamplerAddressMode a){return a==VK_SAMPLER_ADDRESS_MODE_REPEAT||a==VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT||a==VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;};
    return filter(s.mag)&&filter(s.min)&&(s.mip==VK_SAMPLER_MIPMAP_MODE_NEAREST||s.mip==VK_SAMPLER_MIPMAP_MODE_LINEAR)&&address(s.u)&&address(s.v)&&address(s.w)&&std::isfinite(s.bias)&&std::isfinite(s.minLod)&&std::isfinite(s.maxLod)&&s.minLod>=0&&s.maxLod>=s.minLod&&std::isfinite(maxBias)&&maxBias>=0&&std::abs(s.bias)<=maxBias;
}
struct SampledPixels {
    uint32_t width{},height{},levels{};
    SamplerState sampler{};
    std::vector<uint8_t> rgba;
    // Explicit native-only float upload for signed/high-precision compatibility.
    // Wire protocols/default callers remain RGBA8; no guest format advertising.
    VkFormat format=VK_FORMAT_R8G8B8A8_UNORM;
};
inline std::vector<RgbaMip> sampledMipLayout(uint32_t w,uint32_t h,uint32_t levels,VkFormat format){
    if(format!=VK_FORMAT_R8G8B8A8_UNORM&&format!=VK_FORMAT_R8G8B8A8_SRGB&&format!=VK_FORMAT_R32G32B32A32_SFLOAT)
        throw std::invalid_argument("Mavk sampled format not implemented");
    auto layout=rgbaMipLayout(w,h,levels);
    if(format==VK_FORMAT_R32G32B32A32_SFLOAT)for(auto& m:layout){m.offset*=4;m.bytes*=4;}
    return layout;
}
// Caller keeps this and descriptor users alive until GPU completion, just as
// for Core Image/Buffer. Initial upload is recorded once, not implicitly waited.
class SampledImage {
public:
    SampledImage(DeviceContext&,const SampledPixels&);
    ~SampledImage();SampledImage(const SampledImage&)=delete;
    SampledImage& operator=(const SampledImage&)=delete;
    VkDescriptorImageInfo descriptor() const {
        return {sampler_,view_,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    }
    void recordInitialUpload(VkCommandBuffer,VkPipelineStageFlags consumers);
    // Synchronous one-time upload; releases staging only after its fence.
    void upload(VkQueue,VkPipelineStageFlags consumers);
    // Explicit synchronous partial upload. Caller serializes this queue and all
    // readers; retains image/descriptor. Tight decoded region bytes, not BC data.
    void update(VkQueue,uint32_t mip,const VkRect2D&,const std::vector<uint8_t>&,
                VkPipelineStageFlags consumers=VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
private:
    void clear() noexcept;
    DeviceContext& context_;
    std::vector<RgbaMip> mips_;
    std::unique_ptr<Image> image_;
    std::unique_ptr<Buffer> staging_;
    VkImageView view_{};VkSampler sampler_{};bool recorded_=false;uint32_t pixelBytes_=4;
};
}
