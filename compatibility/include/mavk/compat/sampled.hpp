#pragma once
#include <mavk/compat/texture.hpp>
#include <mavk/sampled.hpp>
#include <cstring>

namespace mavk::compat {
// Explicit snapshot for existing native Core. Not performed each render frame.
// Current Core extent limits and initialization gates are retained.
inline SampledPixels sampledPixels(const BcTexture& source,const SamplerState& sampler={}){
    SampledPixels out;out.width=source.width();out.height=source.height();out.levels=source.levels();out.sampler=sampler;
    switch(source.encoding()){
        case TextureEncoding::RGBA8_UNORM:out.format=VK_FORMAT_R8G8B8A8_UNORM;break;
        case TextureEncoding::RGBA8_SRGB:out.format=VK_FORMAT_R8G8B8A8_SRGB;break;
        case TextureEncoding::RGBA32_FLOAT:out.format=VK_FORMAT_R32G32B32A32_SFLOAT;break;
    }
    auto layout=sampledMipLayout(out.width,out.height,out.levels,out.format);
    // Check all source levels before allocating/copying the upload snapshot.
    for(uint32_t i=0;i<out.levels;i++)if(source.decodedMip(i).size()!=layout[i].bytes)throw std::logic_error("BC native snapshot mip bytes");
    out.rgba.resize(size_t(layout.back().offset+layout.back().bytes));
    for(uint32_t i=0;i<out.levels;i++)std::memcpy(out.rgba.data()+size_t(layout[i].offset),source.decodedMip(i).data(),size_t(layout[i].bytes));
    return out;
}
}
