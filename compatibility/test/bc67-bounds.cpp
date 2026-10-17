#include <mavk/compat/bc.hpp>
#include <array>
#include <cstdio>
#include <stdexcept>
using namespace mavk::compat;
static unsigned checks=0,rejects=0;
static void require(bool b){checks++;if(!b)throw std::runtime_error("BC67 golden/bounds");}
template<class F>static void reject(F f){try{f();}catch(const std::invalid_argument&){rejects++;return;}throw std::runtime_error("BC67 bad input accepted");}
static std::array<uint8_t,16> constant6(unsigned value){
    std::array<uint8_t,16> bytes{};unsigned bit=0;
    auto put=[&](unsigned v,unsigned n){for(unsigned j=0;j<n;j++,bit++)bytes[bit/8]|=uint8_t(((v>>j)&1u)<<(bit%8));};
    put(3,5);for(unsigned i=0;i<6;i++)put(value,10);return bytes;
}
int main(){try{
    auto positive=constant6(1023),negative=constant6(512);
    auto u=decodeBc6h(false,4,4,positive.data(),16),s=decodeBc6h(true,4,4,negative.data(),16);
    for(unsigned p=0;p<16;p++)for(unsigned c=0;c<4;c++){require(u.rgba[p*4+c]==(c==3?1.0f:65504.0f));require(s.rgba[p*4+c]==(c==3?1.0f:-65504.0f));}
    for(unsigned code:{19u,23u,27u,31u})for(bool sign:{false,true}){
        auto block=positive;block[0]=uint8_t((block[0]&~31u)|code);auto out=decodeBc6h(sign,4,4,block.data(),16);
        for(unsigned p=0;p<16;p++)for(unsigned c=0;c<4;c++)require(out.rgba[p*4+c]==(c==3?1.0f:0.0f));
    }
    // Row pitch48, two blocks per row plus16 padding (no trailing last-row pad).
    std::array<uint8_t,80> data{};unsigned offsets[]={0,16,48,64};
    std::array<uint8_t,16> white{};white[0]=0xc0;for(unsigned i=1;i<8;i++)white[i]=255;white[8]=1;
    for(unsigned j=0;j<4;j++)for(unsigned i=0;i<16;i++)data[offsets[j]+i]=white[i];
    for(unsigned w=5;w<=8;w++)for(unsigned h=5;h<=8;h++){
        auto out=decodeBc7(w,h,data.data(),80,48,true);require(out.rgba.size()==w*h*4&&out.srgb);
        for(auto v:out.rgba)require(v==255);
    }
    for(unsigned j=0;j<4;j++)for(unsigned i=0;i<16;i++)data[offsets[j]+i]=negative[i];
    auto fp=decodeBc6h(true,5,7,data.data(),80,48);require(fp.rgba.size()==5*7*4);
    for(unsigned p=0;p<35;p++)for(unsigned c=0;c<4;c++)require(fp.rgba[p*4+c]==(c==3?1.0f:-65504.0f));
    for(bool hdr:{false,true}){
        auto call=[&](uint32_t w,uint32_t h,const uint8_t* p,size_t n,size_t pitch=0){if(hdr)(void)decodeBc6h(false,w,h,p,n,pitch);else(void)decodeBc7(w,h,p,n,pitch);};
        reject([&]{call(0,4,data.data(),16);});reject([&]{call(4,0,data.data(),16);});reject([&]{call(4,4,nullptr,16);});
        reject([&]{call(4,4,data.data(),15);});reject([&]{call(4,4,data.data(),17);});reject([&]{call(4,4,data.data(),16,15);});
        reject([&]{call(0xffffffffu,4,data.data(),16);});reject([&]{call(16384,16384,data.data(),16);});reject([&]{call(4,8,data.data(),16,size_t(-1));});
    }
    require(rejects==18);
    std::printf("BC67_BOUNDS_GOLDEN_PASS checks=%u rejects=%u HDR_extrema=YES padded_edges=YES NOT_GPU_OR_FULL_BC_API_PASS\n",checks,rejects);return 0;
}catch(const std::exception& e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
