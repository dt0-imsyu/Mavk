#include <mavk/compat/bc.hpp>
#include <array>
#include <cstdio>
#include <stdexcept>
#include <string>
using namespace mavk::compat;
static unsigned checks=0,rejects=0;
static void require(bool b,const char* message){checks++;if(!b)throw std::runtime_error(message);}
static void pixel(const DecodedBc& d,unsigned i,std::array<uint8_t,4> expected){
    for(unsigned c=0;c<4;c++)require(d.rgba[i*4+c]==expected[c],"BC golden pixel");
}
template<class F>static void reject(F f){try{f();}catch(const std::invalid_argument&){rejects++;return;}throw std::runtime_error("Invalid BC input accepted");}
int main(){try{
    // All four color selectors, both endpoint orderings and equality.
    std::array<uint8_t,8> color{0,248,224,7,0xe4,0xe4,0xe4,0xe4}; // red,green
    auto d=decodeBc(BcFormat::BC1_RGBA,4,4,color.data(),8);
    for(unsigned i=0;i<16;i++)pixel(d,i,i%4==0?std::array<uint8_t,4>{255,0,0,255}:
        i%4==1?std::array<uint8_t,4>{0,255,0,255}:i%4==2?std::array<uint8_t,4>{170,85,0,255}:std::array<uint8_t,4>{85,170,0,255});
    std::array<uint8_t,8> blackWhite{0,0,255,255,0xe4,0xe4,0xe4,0xe4};
    for(auto f:{BcFormat::BC1_RGB,BcFormat::BC1_RGBA}){
        d=decodeBc(f,4,4,blackWhite.data(),8);
        pixel(d,0,{0,0,0,255});pixel(d,1,{255,255,255,255});pixel(d,2,{127,127,127,255});
        pixel(d,3,{0,0,0,uint8_t(f==BcFormat::BC1_RGBA?0:255)});
    }
    std::array<uint8_t,8> equal{0,0,0,0,255,255,255,255};
    pixel(decodeBc(BcFormat::BC1_RGBA,1,1,equal.data(),8),0,{0,0,0,0});
    std::array<uint8_t,16> block{};for(unsigned i=0;i<8;i++)block[i]=uint8_t((2*i)|((2*i+1)<<4));
    // BC2 and BC3 always have a four-color palette, even when color0<=color1.
    for(unsigned i=0;i<8;i++)block[8+i]=blackWhite[i];
    d=decodeBc(BcFormat::BC2_RGBA,4,4,block.data(),16);
    for(unsigned i=0;i<16;i++){
        unsigned rgb=i%4==0?0:i%4==1?255:i%4==2?85:170;
        pixel(d,i,{uint8_t(rgb),uint8_t(rgb),uint8_t(rgb),uint8_t(i*17)});
    }
    uint64_t alphaIndices=0;for(unsigned i=0;i<16;i++)alphaIndices|=uint64_t(i%8)<<(3*i);
    for(unsigned i=0;i<6;i++)block[2+i]=uint8_t(alphaIndices>>(8*i));
    for(bool reverse:{false,true}){
        block[0]=reverse?0:255;block[1]=reverse?255:0;
        const uint8_t a7[]={255,0,218,182,145,109,72,36},a5[]={0,255,51,102,153,204,0,255};
        d=decodeBc(BcFormat::BC3_RGBA,4,4,block.data(),16);
        for(unsigned i=0;i<16;i++)require(d.rgba[4*i+3]==(reverse?a5:a7)[i%8],"BC3 alpha golden");
    }
    // 5x5 edge blocks and padded input rows. Each block has a distinct solid color.
    std::array<uint8_t,40> padded{};unsigned offsets[]={0,8,24,32};uint16_t colors[]={0xf800,0x07e0,0x001f,0xffff};
    for(unsigned i=0;i<4;i++){padded[offsets[i]]=uint8_t(colors[i]);padded[offsets[i]+1]=uint8_t(colors[i]>>8);}
    d=decodeBc(BcFormat::BC1_RGB,5,5,padded.data(),40,24,true);
    require(d.width==5&&d.height==5&&d.srgb&&d.rgba.size()==100,"Extent/encoding metadata");
    pixel(d,0,{255,0,0,255});pixel(d,4,{0,255,0,255});pixel(d,20,{0,0,255,255});pixel(d,24,{255,255,255,255});
    auto bad=[&](uint32_t w,uint32_t h,const uint8_t* data,size_t bytes,size_t pitch=0){reject([&]{decodeBc(BcFormat::BC1_RGB,w,h,data,bytes,pitch);});};
    bad(0,4,color.data(),8);bad(4,0,color.data(),8);bad(4,4,nullptr,8);bad(4,4,color.data(),7);bad(4,4,color.data(),9);
    bad(4,4,color.data(),8,7);bad(4,4,color.data(),8,9);bad(5,5,padded.data(),39,24);bad(5,5,padded.data(),41,24);
    bad(0xffffffffu,0xffffffffu,color.data(),8);bad(16384,16384,color.data(),8);bad(4,8,color.data(),8,size_t(-1));
    reject([&]{decodeBc(static_cast<BcFormat>(999),4,4,color.data(),8);});
    // Deterministic random payloads for every small extent and all four formats.
    uint32_t rng=1;for(unsigned w=1;w<=9;w++)for(unsigned h=1;h<=9;h++)for(auto f:{BcFormat::BC1_RGB,BcFormat::BC1_RGBA,BcFormat::BC2_RGBA,BcFormat::BC3_RGBA}){
        size_t b=(w+3)/4*((h+3)/4)*(f==BcFormat::BC1_RGB||f==BcFormat::BC1_RGBA?8:16);
        std::vector<uint8_t> bytes(b);for(auto& v:bytes){rng=1664525*rng+1013904223;v=uint8_t(rng>>24);}
        auto decoded=decodeBc(f,w,h,bytes.data(),bytes.size());require(decoded.rgba.size()==w*h*4,"Small extent decode bounds");
        auto second=decodeBc(f,w,h,bytes.data(),bytes.size());require(second.rgba==decoded.rgba,"Deterministic decode");
    }
    require(rejects==13,"Negative coverage");
    std::printf("BC123_CPU_PASS checks=%u rejects=%u random_images=324 NOT_FULL_BC_FEATURE_OR_DXVK_PASS\n",checks,rejects);
    return 0;
}catch(const std::exception& e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
