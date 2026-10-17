#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace mavk::compat {
// Raw block payload only. Not DDS/KTX parsing, Vulkan compressed-image semantics,
// or permission to advertise textureCompressionBC (compressed-image API incomplete).
enum class BcFormat { BC1_RGB, BC1_RGBA, BC2_RGBA, BC3_RGBA };
struct DecodedBc {
    uint32_t width=0,height=0;
    // Encoded sRGB bytes remain encoded. Caller MUST upload to an sRGB image,
    // not an UNORM image, when srgb=true. No implicit color-space conversion.
    bool srgb=false;
    std::vector<uint8_t> rgba;
};
constexpr size_t MaxBcDecodedBytes=64u*1024u*1024u;
DecodedBc decodeBc(BcFormat format,uint32_t width,uint32_t height,
                   const uint8_t* blocks,size_t bytes,size_t blockRowPitch=0,
                   bool srgb=false);
enum class BcChannelFormat { BC4_UNORM, BC4_SNORM, BC5_UNORM, BC5_SNORM };
struct DecodedBcChannels {
    uint32_t width=0,height=0;
    // RGBA float reference/upload representation: unused G/B=0, A=1.
    // Unlike RGBA8, preserves negative SNORM and interpolated channel precision.
    std::vector<float> rgba;
};
DecodedBcChannels decodeBcChannels(BcChannelFormat,uint32_t width,uint32_t height,
    const uint8_t* blocks,size_t bytes,size_t blockRowPitch=0);
DecodedBc decodeBc7(uint32_t width,uint32_t height,const uint8_t* blocks,size_t bytes,
    size_t blockRowPitch=0,bool srgb=false);
// Full HDR RGB decoded to RGBA float (A=1), including signed values and >1.
DecodedBcChannels decodeBc6h(bool signedValues,uint32_t width,uint32_t height,
    const uint8_t* blocks,size_t bytes,size_t blockRowPitch=0);
}
