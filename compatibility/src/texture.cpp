#include <mavk/compat/texture.hpp>
#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace mavk::compat {
BcTexture::BcTexture(TextureBcFormat f,uint32_t w,uint32_t h,uint32_t levels,size_t budget):format_(f){
    if(unsigned(f)>unsigned(TextureBcFormat::BC7_SRGB))throw std::invalid_argument("Unknown BC texture format");
    bool fp=f==TextureBcFormat::BC4_UNORM||f==TextureBcFormat::BC4_SNORM||f==TextureBcFormat::BC5_UNORM||f==TextureBcFormat::BC5_SNORM||f==TextureBcFormat::BC6H_UFLOAT||f==TextureBcFormat::BC6H_SFLOAT;
    bool srgb=f==TextureBcFormat::BC1_RGB_SRGB||f==TextureBcFormat::BC1_RGBA_SRGB||f==TextureBcFormat::BC2_SRGB||f==TextureBcFormat::BC3_SRGB||f==TextureBcFormat::BC7_SRGB;
    encoding_=fp?TextureEncoding::RGBA32_FLOAT:srgb?TextureEncoding::RGBA8_SRGB:TextureEncoding::RGBA8_UNORM;
    blockBytes_=(unsigned(f)<=3||f==TextureBcFormat::BC4_UNORM||f==TextureBcFormat::BC4_SNORM)?8:16;pixelBytes_=fp?16:4;
    if(!w||!h||w>16384||h>16384||!levels||!budget||budget>512u*1024u*1024u)throw std::invalid_argument("BC texture extent/budget");
    uint32_t maximum=1;for(uint32_t n=std::max(w,h);n>1;n>>=1)maximum++;
    if(levels>maximum)throw std::invalid_argument("BC texture excessive mip count");
    uint64_t total=0;
    for(uint32_t level=0;level<levels;level++){
        size_t columns=(size_t(w)+3)/4,rows=(size_t(h)+3)/4,blocks=columns*rows;
        uint64_t decoded=uint64_t(w)*h*pixelBytes_;total+=decoded+blocks*(blockBytes_+1);
        if(decoded>MaxBcDecodedBytes||total>budget)throw std::invalid_argument("BC texture memory budget");
        Mip m{};m.width=w;m.height=h;m.columns=columns;m.rows=rows;m.remaining=blocks;mips_.push_back(std::move(m));
        w=std::max(1u,w/2);h=std::max(1u,h/2);
    }
    // Allocate only after validating the entire chain, not one giant level first.
    for(auto& m:mips_){m.compressed.resize(m.columns*m.rows*blockBytes_);m.decoded.resize(size_t(m.width)*m.height*pixelBytes_);m.initialized.resize(m.columns*m.rows);}
}
const BcTexture::Mip& BcTexture::mipAt(uint32_t i) const{if(i>=mips_.size())throw std::invalid_argument("BC mip index");return mips_[i];}
BcTexture::Mip& BcTexture::mipAt(uint32_t i){return const_cast<Mip&>(static_cast<const BcTexture*>(this)->mipAt(i));}
uint32_t BcTexture::width(uint32_t i) const{return mipAt(i).width;}
uint32_t BcTexture::height(uint32_t i) const{return mipAt(i).height;}
bool BcTexture::initialized(uint32_t i) const{return mipAt(i).remaining==0;}
const std::vector<uint8_t>& BcTexture::decodedMip(uint32_t i) const{const auto& m=mipAt(i);if(m.remaining)throw std::logic_error("BC mip has uninitialized blocks");return m.decoded;}
const std::vector<uint8_t>& BcTexture::compressedMip(uint32_t i) const{const auto& m=mipAt(i);if(m.remaining)throw std::logic_error("BC mip has uninitialized blocks");return m.compressed;}
TextureRegion BcTexture::dirtyRegion(uint32_t i) const{return mipAt(i).dirty;}
uint64_t BcTexture::revision(uint32_t i) const{return mipAt(i).revision;}
bool BcTexture::acknowledge(uint32_t i,uint64_t revision){auto& m=mipAt(i);if(!revision||revision!=m.revision)return false;m.dirty={};return true;}
void BcTexture::validateRegion(uint32_t i,TextureRegion r) const{
    const auto& m=mipAt(i);
    if(!r.width||!r.height||r.x>=m.width||r.y>=m.height||r.width>m.width-r.x||r.height>m.height-r.y||r.x%4||r.y%4||
       (r.width%4&&r.x+r.width!=m.width)||(r.height%4&&r.y+r.height!=m.height))throw std::invalid_argument("BC block region alignment/extent");
}
void BcTexture::update(uint32_t i,TextureRegion r,const uint8_t* data,size_t bytes,size_t pitch){
    validateRegion(i,r);auto& m=mipAt(i);size_t columns=(size_t(r.width)+3)/4,rows=(size_t(r.height)+3)/4,rowBytes=columns*blockBytes_;
    if(!pitch)pitch=rowBytes;
    if(!data||pitch<rowBytes||pitch%blockBytes_||pitch>MaxBcDecodedBytes||bytes>MaxBcDecodedBytes||uint64_t(rows-1)*pitch+rowBytes!=bytes)throw std::invalid_argument("BC upload pitch/byte count");
    if(sequence_==std::numeric_limits<uint64_t>::max()||decodedBlocks_>std::numeric_limits<uint64_t>::max()-columns*rows)throw std::overflow_error("BC upload revision overflow");
    // Snapshot input before committing; safe even for overlapping/self source.
    std::vector<uint8_t> compressed(rowBytes*rows);for(size_t y=0;y<rows;y++)std::memcpy(compressed.data()+y*rowBytes,data+y*pitch,rowBytes);
    std::vector<uint8_t> decoded;
    bool srgb=encoding_==TextureEncoding::RGBA8_SRGB;
    if(unsigned(format_)<=7){
        BcFormat f=unsigned(format_)<=1?BcFormat::BC1_RGB:unsigned(format_)<=3?BcFormat::BC1_RGBA:unsigned(format_)<=5?BcFormat::BC2_RGBA:BcFormat::BC3_RGBA;
        decoded=decodeBc(f,r.width,r.height,compressed.data(),compressed.size(),0,srgb).rgba;
    }else if(format_==TextureBcFormat::BC7_UNORM||format_==TextureBcFormat::BC7_SRGB){
        decoded=decodeBc7(r.width,r.height,compressed.data(),compressed.size(),0,srgb).rgba;
    }else{
        DecodedBcChannels f;
        if(format_==TextureBcFormat::BC6H_UFLOAT||format_==TextureBcFormat::BC6H_SFLOAT)f=decodeBc6h(format_==TextureBcFormat::BC6H_SFLOAT,r.width,r.height,compressed.data(),compressed.size());
        else f=decodeBcChannels(static_cast<BcChannelFormat>(unsigned(format_)-unsigned(TextureBcFormat::BC4_UNORM)),r.width,r.height,compressed.data(),compressed.size());
        decoded.resize(f.rgba.size()*sizeof(float));std::memcpy(decoded.data(),f.rgba.data(),decoded.size());
    }
    // No remaining fallible allocations: reject leaves authoritative state intact.
    for(size_t y=0;y<rows;y++){
        size_t block=(size_t(r.y)/4+y)*m.columns+r.x/4;
        std::memcpy(m.compressed.data()+block*blockBytes_,compressed.data()+y*rowBytes,rowBytes);
        for(size_t x=0;x<columns;x++)if(!m.initialized[block+x]){m.initialized[block+x]=1;m.remaining--;}
    }
    for(uint32_t y=0;y<r.height;y++)std::memcpy(m.decoded.data()+((size_t(r.y)+y)*m.width+r.x)*pixelBytes_,decoded.data()+size_t(y)*r.width*pixelBytes_,size_t(r.width)*pixelBytes_);
    if(m.dirty.width){uint32_t x=std::min(m.dirty.x,r.x),y=std::min(m.dirty.y,r.y);m.dirty={x,y,std::max(m.dirty.x+m.dirty.width,r.x+r.width)-x,std::max(m.dirty.y+m.dirty.height,r.y+r.height)-y};}
    else m.dirty=r;
    m.revision=++sequence_;decodedBlocks_+=columns*rows;
}
void BcTexture::copyFrom(const BcTexture& src,uint32_t sourceMip,TextureRegion r,uint32_t destinationMip,uint32_t x,uint32_t y){
    if(src.format_!=format_)throw std::invalid_argument("BC cross-format copy not implemented");
    src.validateRegion(sourceMip,r);validateRegion(destinationMip,{x,y,r.width,r.height});const auto& m=src.mipAt(sourceMip);
    size_t columns=(size_t(r.width)+3)/4,rows=(size_t(r.height)+3)/4,rowBytes=columns*blockBytes_;
    for(size_t by=0;by<rows;by++)for(size_t bx=0;bx<columns;bx++)if(!m.initialized[(r.y/4+by)*m.columns+r.x/4+bx])throw std::logic_error("BC copy source uninitialized");
    std::vector<uint8_t> bytes(rowBytes*rows);
    for(size_t by=0;by<rows;by++)std::memcpy(bytes.data()+by*rowBytes,m.compressed.data()+((r.y/4+by)*m.columns+r.x/4)*blockBytes_,rowBytes);
    update(destinationMip,{x,y,r.width,r.height},bytes.data(),bytes.size());
}
}
