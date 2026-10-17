#include <mavk/compat/bc.hpp>
#include <array>
#include <algorithm>
#include <stdexcept>

namespace mavk::compat {
namespace {
uint32_t le16(const uint8_t* p){return uint32_t(p[0])|(uint32_t(p[1])<<8);}
uint32_t le32(const uint8_t* p){return le16(p)|(le16(p+2)<<16);}
using Pixel=std::array<uint8_t,4>;
Pixel endpoint(uint32_t v){
    uint32_t r=v>>11,g=(v>>5)&63,b=v&31;
    return {uint8_t((r<<3)|(r>>2)),uint8_t((g<<2)|(g>>4)),uint8_t((b<<3)|(b>>2)),255};
}
void colorBlock(const uint8_t* p,bool four,bool punch,std::array<Pixel,16>& out){
    auto a=le16(p),b=le16(p+2);std::array<Pixel,4> pal{endpoint(a),endpoint(b),Pixel{},Pixel{}};
    if(four||a>b){
        for(unsigned c=0;c<3;c++){
            pal[2][c]=uint8_t((2u*pal[0][c]+pal[1][c])/3u);
            pal[3][c]=uint8_t((pal[0][c]+2u*pal[1][c])/3u);
        }
        pal[2][3]=pal[3][3]=255;
    }else{
        for(unsigned c=0;c<3;c++)pal[2][c]=uint8_t((unsigned(pal[0][c])+pal[1][c])/2u);
        pal[2][3]=255;pal[3]={0,0,0,uint8_t(punch?0:255)};
    }
    auto indices=le32(p+4);
    for(unsigned i=0;i<16;i++)out[i]=pal[(indices>>(2*i))&3u];
}
void channelBlock(const uint8_t* p,bool sign,std::array<float,16>& out){
    auto endpointValue=[&](uint8_t v){int n=v;if(sign&&n>=128)n-=256;return sign?float(std::max(-127,n))/127.0f:float(n)/255.0f;};
    float a=endpointValue(p[0]),b=endpointValue(p[1]);
    std::array<float,8> palette{a,b,0,0,0,0,0,0};
    // Signed -127/-128 pair has implementation-dependent comparison in the
    // Khronos format spec. Canonicalize both to -1 and choose the equal branch.
    if(a>b)for(unsigned i=1;i<=6;i++)palette[i+1]=((7-i)*a+i*b)/7.0f;
    else{
        for(unsigned i=1;i<=4;i++)palette[i+1]=((5-i)*a+i*b)/5.0f;
        palette[6]=sign?-1.0f:0.0f;palette[7]=1.0f;
    }
    uint64_t indices=0;for(unsigned i=0;i<6;i++)indices|=uint64_t(p[i+2])<<(i*8);
    for(unsigned i=0;i<16;i++)out[i]=palette[(indices>>(3*i))&7u];
}
}
DecodedBcChannels decodeBcChannels(BcChannelFormat format,uint32_t width,uint32_t height,const uint8_t* data,size_t bytes,size_t pitch){
    bool sign,two;
    switch(format){
        case BcChannelFormat::BC4_UNORM:sign=false;two=false;break;
        case BcChannelFormat::BC4_SNORM:sign=true;two=false;break;
        case BcChannelFormat::BC5_UNORM:sign=false;two=true;break;
        case BcChannelFormat::BC5_SNORM:sign=true;two=true;break;
        default:throw std::invalid_argument("Unsupported BC channel format");
    }
    if(!width||!height||width>16384||height>16384||!data)throw std::invalid_argument("BC channel extent/source");
    uint64_t outputBytes=uint64_t(width)*height*4*sizeof(float);
    if(outputBytes>MaxBcDecodedBytes)throw std::invalid_argument("BC channel output budget");
    size_t blockBytes=two?16:8,columns=(size_t(width)+3)/4,rows=(size_t(height)+3)/4,rowBytes=columns*blockBytes;
    if(!pitch)pitch=rowBytes;
    if(pitch<rowBytes||pitch%blockBytes||pitch>MaxBcDecodedBytes||uint64_t(rows-1)*pitch+rowBytes!=bytes)throw std::invalid_argument("BC channel row/byte count");
    DecodedBcChannels result;result.width=width;result.height=height;result.rgba.resize(size_t(width)*height*4);
    for(size_t by=0;by<rows;by++)for(size_t bx=0;bx<columns;bx++){
        const auto* block=data+by*pitch+bx*blockBytes;std::array<float,16> r{},g{};
        channelBlock(block,sign,r);if(two)channelBlock(block+8,sign,g);
        for(unsigned y=0;y<4&&by*4+y<height;y++)for(unsigned x=0;x<4&&bx*4+x<width;x++){
            size_t dst=((by*4+y)*width+bx*4+x)*4,src=y*4+x;
            result.rgba[dst]=r[src];result.rgba[dst+1]=g[src];result.rgba[dst+2]=0;result.rgba[dst+3]=1;
        }
    }
    return result;
}
DecodedBc decodeBc(BcFormat format,uint32_t width,uint32_t height,const uint8_t* data,size_t bytes,size_t pitch,bool srgb){
    size_t blockBytes;
    switch(format){
        case BcFormat::BC1_RGB:case BcFormat::BC1_RGBA:blockBytes=8;break;
        case BcFormat::BC2_RGBA:case BcFormat::BC3_RGBA:blockBytes=16;break;
        default:throw std::invalid_argument("Unsupported BC format");
    }
    if(!width||!height||width>16384||height>16384||!data)throw std::invalid_argument("BC extent/source");
    const uint64_t outputBytes=uint64_t(width)*height*4;
    if(outputBytes>MaxBcDecodedBytes)throw std::invalid_argument("BC output budget");
    size_t columns=(size_t(width)+3)/4,rows=(size_t(height)+3)/4,rowBytes=columns*blockBytes;
    if(!pitch)pitch=rowBytes;
    // Bound before multiply. Last row contains rowBytes, not trailing padding.
    if(pitch<rowBytes||pitch%blockBytes||pitch>MaxBcDecodedBytes||
       uint64_t(rows-1)*pitch+rowBytes!=bytes)throw std::invalid_argument("BC block row/byte count");
    DecodedBc result;result.width=width;result.height=height;result.srgb=srgb;result.rgba.resize(size_t(outputBytes));
    for(size_t by=0;by<rows;by++)for(size_t bx=0;bx<columns;bx++){
        const auto* block=data+by*pitch+bx*blockBytes;std::array<Pixel,16> pixels;
        bool alpha=format==BcFormat::BC2_RGBA||format==BcFormat::BC3_RGBA;
        colorBlock(block+(alpha?8:0),alpha,format==BcFormat::BC1_RGBA,pixels);
        if(format==BcFormat::BC2_RGBA){
            for(unsigned i=0;i<16;i++)pixels[i][3]=uint8_t(((block[i/2]>>((i%2)*4))&15u)*17u);
        }else if(format==BcFormat::BC3_RGBA){
            uint8_t palette[8]={block[0],block[1],0,0,0,0,0,0};
            if(block[0]>block[1])for(unsigned i=1;i<=6;i++)palette[i+1]=uint8_t(((7-i)*unsigned(block[0])+i*unsigned(block[1]))/7);
            else{
                for(unsigned i=1;i<=4;i++)palette[i+1]=uint8_t(((5-i)*unsigned(block[0])+i*unsigned(block[1]))/5);
                palette[6]=0;palette[7]=255;
            }
            uint64_t indices=0;for(unsigned i=0;i<6;i++)indices|=uint64_t(block[i+2])<<(8*i);
            for(unsigned i=0;i<16;i++)pixels[i][3]=palette[(indices>>(3*i))&7u];
        }
        for(unsigned y=0;y<4&&by*4+y<height;y++)for(unsigned x=0;x<4&&bx*4+x<width;x++){
            size_t dst=((by*4+y)*width+bx*4+x)*4;
            for(unsigned c=0;c<4;c++)result.rgba[dst+c]=pixels[y*4+x][c];
        }
    }
    return result;
}
}
