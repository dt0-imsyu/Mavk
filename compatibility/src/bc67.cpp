#include <mavk/compat/bc.hpp>
#include <array>
#include <stdexcept>
#define BCDECDEF static inline
#define BCDEC_IMPLEMENTATION
// Unmodified third-party MIT implementation, attribution/license alongside it.
#include "../../third_party/bcdec/bcdec.h"

namespace mavk::compat {
namespace {
struct Layout {size_t columns,rows,pitch;};
Layout checked(uint32_t w,uint32_t h,const uint8_t* p,size_t bytes,size_t pitch,size_t pixelBytes){
    if(!w||!h||w>16384||h>16384||!p||uint64_t(w)*h*pixelBytes>MaxBcDecodedBytes)
        throw std::invalid_argument("BC67 extent/source/output budget");
    size_t columns=(size_t(w)+3)/4,rows=(size_t(h)+3)/4,rowBytes=columns*16;
    if(!pitch)pitch=rowBytes;
    if(pitch<rowBytes||pitch%16||pitch>MaxBcDecodedBytes||uint64_t(rows-1)*pitch+rowBytes!=bytes)
        throw std::invalid_argument("BC67 row/byte count");
    return {columns,rows,pitch};
}
}
DecodedBc decodeBc7(uint32_t w,uint32_t h,const uint8_t* data,size_t bytes,size_t pitch,bool srgb){
    auto l=checked(w,h,data,bytes,pitch,4);DecodedBc result;result.width=w;result.height=h;result.srgb=srgb;result.rgba.resize(size_t(w)*h*4);
    for(size_t by=0;by<l.rows;by++)for(size_t bx=0;bx<l.columns;bx++){
        std::array<uint8_t,64> tile{};bcdec_bc7(data+by*l.pitch+bx*16,tile.data(),16);
        for(unsigned y=0;y<4&&by*4+y<h;y++)for(unsigned x=0;x<4&&bx*4+x<w;x++)for(unsigned c=0;c<4;c++)
            result.rgba[((by*4+y)*w+bx*4+x)*4+c]=tile[(y*4+x)*4+c];
    }
    return result;
}
DecodedBcChannels decodeBc6h(bool sign,uint32_t w,uint32_t h,const uint8_t* data,size_t bytes,size_t pitch){
    static_assert(sizeof(float)==4,"BC6H float output representation");
    auto l=checked(w,h,data,bytes,pitch,16);DecodedBcChannels result;result.width=w;result.height=h;result.rgba.resize(size_t(w)*h*4);
    for(size_t by=0;by<l.rows;by++)for(size_t bx=0;bx<l.columns;bx++){
        std::array<float,48> tile{};bcdec_bc6h_float(data+by*l.pitch+bx*16,tile.data(),12,sign?1:0);
        for(unsigned y=0;y<4&&by*4+y<h;y++)for(unsigned x=0;x<4&&bx*4+x<w;x++){
            size_t dst=((by*4+y)*w+bx*4+x)*4,src=(y*4+x)*3;
            for(unsigned c=0;c<3;c++)result.rgba[dst+c]=tile[src+c];
            result.rgba[dst+3]=1;
        }
    }
    return result;
}
}
