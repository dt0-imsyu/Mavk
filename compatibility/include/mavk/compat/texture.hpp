#pragma once
#include <mavk/compat/bc.hpp>

namespace mavk::compat {
enum class TextureBcFormat {
    BC1_RGB_UNORM,BC1_RGB_SRGB,BC1_RGBA_UNORM,BC1_RGBA_SRGB,
    BC2_UNORM,BC2_SRGB,BC3_UNORM,BC3_SRGB,BC4_UNORM,BC4_SNORM,
    BC5_UNORM,BC5_SNORM,BC6H_UFLOAT,BC6H_SFLOAT,BC7_UNORM,BC7_SRGB
};
enum class TextureEncoding { RGBA8_UNORM,RGBA8_SRGB,RGBA32_FLOAT };
struct TextureRegion {uint32_t x=0,y=0,width=0,height=0;};
// 2D, one array layer. Compressed authoritative bytes + decoded upload cache.
// Serial caller; NOT a VkImage/ICD, sparse image, arbitrary view or render target.
class BcTexture {
public:
    BcTexture(TextureBcFormat,uint32_t width,uint32_t height,uint32_t levels,
              size_t memoryBudget=MaxBcDecodedBytes);
    void update(uint32_t mip,TextureRegion,const uint8_t* data,size_t bytes,size_t blockRowPitch=0);
    void copyFrom(const BcTexture& source,uint32_t sourceMip,TextureRegion sourceRegion,
                  uint32_t destinationMip,uint32_t destinationX,uint32_t destinationY);
    TextureBcFormat format() const{return format_;}
    TextureEncoding encoding() const{return encoding_;}
    uint32_t levels() const{return uint32_t(mips_.size());}
    uint32_t width(uint32_t mip=0) const;
    uint32_t height(uint32_t mip=0) const;
    bool initialized(uint32_t mip) const;
    const std::vector<uint8_t>& decodedMip(uint32_t mip) const;
    const std::vector<uint8_t>& compressedMip(uint32_t mip) const;
    TextureRegion dirtyRegion(uint32_t mip) const;
    uint64_t revision(uint32_t mip) const;
    // Caller acknowledges only AFTER the corresponding GPU upload completed.
    // A stale revision cannot erase damage from a newer update.
    bool acknowledge(uint32_t mip,uint64_t revision);
    uint64_t decodedBlocks() const{return decodedBlocks_;}
private:
    struct Mip {uint32_t width,height;size_t columns,rows,remaining;std::vector<uint8_t> compressed,decoded,initialized;TextureRegion dirty;uint64_t revision=0;};
    const Mip& mipAt(uint32_t) const;Mip& mipAt(uint32_t);
    void validateRegion(uint32_t,TextureRegion) const;
    TextureBcFormat format_;TextureEncoding encoding_;size_t blockBytes_,pixelBytes_;
    uint64_t sequence_=0,decodedBlocks_=0;std::vector<Mip> mips_;
};
}
