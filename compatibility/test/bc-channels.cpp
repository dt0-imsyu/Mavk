#include <mavk/compat/bc.hpp>
#include <array>
#include <cmath>
#include <cstdio>
#include <stdexcept>
using namespace mavk::compat;
static uint64_t checks=0;static unsigned rejects=0;
static void eq(float a,double b){checks++;if(!std::isfinite(a)||std::abs(double(a)-b)>3e-7)throw std::runtime_error("BC channel oracle mismatch");}
static void indices(uint8_t* p){uint64_t bits=0;for(unsigned i=0;i<16;i++)bits|=uint64_t(i%8)<<(3*i);for(unsigned i=0;i<6;i++)p[i+2]=uint8_t(bits>>(8*i));}
static double expected(unsigned a,unsigned b,unsigned index,bool sign){
    auto value=[&](unsigned v){int n=int(v);if(sign&&v>=128)n-=256;return sign?std::fmax(-1.0,double(n)/127.0):double(n)/255.0;};
    double x=value(a),y=value(b);
    if(index==0)return x;if(index==1)return y;
    if(x>y)return (double(8-index)*x+double(index-1)*y)/7.0;
    if(index<6)return (double(6-index)*x+double(index-1)*y)/5.0;
    return index==6?(sign?-1.0:0.0):1.0;
}
template<class F>static void reject(F f){try{f();}catch(const std::invalid_argument&){rejects++;return;}throw std::runtime_error("Invalid BC channel payload accepted");}
int main(){try{
    std::array<uint8_t,16> block{};indices(block.data());indices(block.data()+8);
    // Independent double-precision oracle, all endpoint byte pairs and selectors.
    // Signed ambiguous -127/-128 deliberately follows documented canonical branch.
    for(bool sign:{false,true})for(unsigned a=0;a<256;a++)for(unsigned b=0;b<256;b++){
        block[0]=uint8_t(a);block[1]=uint8_t(b);
        auto d=decodeBcChannels(sign?BcChannelFormat::BC4_SNORM:BcChannelFormat::BC4_UNORM,4,4,block.data(),8);
        for(unsigned p=0;p<16;p++){eq(d.rgba[p*4],expected(a,b,p%8,sign));eq(d.rgba[p*4+1],0);eq(d.rgba[p*4+2],0);eq(d.rgba[p*4+3],1);}
    }
    // BC5 channels remain independent, including signed extrema/zero and reversal.
    for(bool sign:{false,true})for(auto pair:std::array<std::array<unsigned,2>,6>{{{255,0},{0,255},{128,127},{129,128},{0,0},{64,192}}}){
        block[0]=uint8_t(pair[0]);block[1]=uint8_t(pair[1]);block[8]=uint8_t(pair[1]);block[9]=uint8_t(pair[0]);
        auto d=decodeBcChannels(sign?BcChannelFormat::BC5_SNORM:BcChannelFormat::BC5_UNORM,4,4,block.data(),16);
        for(unsigned p=0;p<16;p++){eq(d.rgba[p*4],expected(pair[0],pair[1],p%8,sign));eq(d.rgba[p*4+1],expected(pair[1],pair[0],p%8,sign));eq(d.rgba[p*4+2],0);eq(d.rgba[p*4+3],1);}
    }
    // All small edges and padded rows; alpha/missing channels and output ranges.
    for(auto format:{BcChannelFormat::BC4_UNORM,BcChannelFormat::BC4_SNORM,BcChannelFormat::BC5_UNORM,BcChannelFormat::BC5_SNORM})for(unsigned w=1;w<=9;w++)for(unsigned h=1;h<=9;h++){
        bool two=format==BcChannelFormat::BC5_UNORM||format==BcChannelFormat::BC5_SNORM;
        size_t blockBytes=two?16:8,cols=(w+3)/4,rows=(h+3)/4,pitch=(cols+1)*blockBytes;
        std::vector<uint8_t> bytes((rows-1)*pitch+cols*blockBytes,0xee);
        for(size_t y=0;y<rows;y++)for(size_t x=0;x<cols;x++){
            auto* p=bytes.data()+y*pitch+x*blockBytes;p[0]=255;p[1]=0;indices(p);
            if(two){p[8]=0;p[9]=255;indices(p+8);}
        }
        auto d=decodeBcChannels(format,w,h,bytes.data(),bytes.size(),pitch);
        if(d.rgba.size()!=w*h*4)throw std::runtime_error("BC channel extent");
        bool sign=format==BcChannelFormat::BC4_SNORM||format==BcChannelFormat::BC5_SNORM;
        for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){
            unsigned at=(y*w+x)*4,index=((y%4)*4+x%4)%8;
            eq(d.rgba[at],expected(255,0,index,sign));eq(d.rgba[at+1],two?expected(0,255,index,sign):0);eq(d.rgba[at+2],0);eq(d.rgba[at+3],1);
        }
    }
    auto bad=[&](uint32_t w,uint32_t h,const uint8_t* p,size_t n,size_t pitch=0){reject([&]{decodeBcChannels(BcChannelFormat::BC4_UNORM,w,h,p,n,pitch);});};
    bad(0,4,block.data(),8);bad(4,0,block.data(),8);bad(4,4,nullptr,8);bad(4,4,block.data(),7);bad(4,4,block.data(),9);
    bad(4,4,block.data(),8,7);bad(4,4,block.data(),8,9);bad(0xffffffffu,0xffffffffu,block.data(),8);bad(4096,4096,block.data(),8);bad(4,8,block.data(),8,size_t(-1));
    reject([&]{decodeBcChannels(static_cast<BcChannelFormat>(999),4,4,block.data(),8);});
    if(rejects!=11)throw std::runtime_error("BC45 negative coverage");
    std::printf("BC45_CPU_PASS checks=%llu rejects=%u exhaustive_endpoint_pairs=131072 edge_images=324 NOT_GPU_OR_FULL_BC_PASS\n",static_cast<unsigned long long>(checks),rejects);return 0;
}catch(const std::exception& e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
