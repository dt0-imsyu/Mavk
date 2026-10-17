// Known failing reference investigation, NOT an acceptance gate. detex disagrees
// with bcdec and Microsoft WARP; keep repro, do not relax comparisons to hide it.
#include <mavk/compat/bc.hpp>
extern "C" {
#include <detex.h>
}
#include <array>
#include <bc7decomp.h>
#include <cmath>
#include <cstdio>
#include <stdexcept>
using namespace mavk::compat;
static uint64_t checks=0;static unsigned negatives=0;
static void require(bool b,const char* text){checks++;if(!b)throw std::runtime_error(text);}
static float half(uint16_t bits){
    int e=(bits>>10)&31,m=bits&1023;float f=e?std::ldexp(1.0f+float(m)/1024,e-15):std::ldexp(float(m),-24);
    return (bits&0x8000)?-f:f;
}
template<class F>static void reject(F f){try{f();}catch(const std::invalid_argument&){negatives++;return;}throw std::runtime_error("Invalid BC67 accepted");}
int main(){try{
    uint32_t rng=0x67426742;auto random=[&](uint8_t* p){for(unsigned i=0;i<16;i++){rng=1664525*rng+1013904223;p[i]=uint8_t(rng>>24);}};
    std::array<uint8_t,16> block{};
    // Independent golden: mode6 endpoints all255, all interpolation selectors0.
    block[0]=0xc0;for(unsigned i=1;i<8;i++)block[i]=255;block[8]=1;
    auto white=decodeBc7(4,4,block.data(),16);
    for(auto v:white.rgba)require(v==255,"BC7 white golden");
    block.fill(0);auto transparent=decodeBc7(4,4,block.data(),16);
    for(auto v:transparent.rgba)require(v==0,"BC7 invalid mode transparent golden");
    unsigned bc7Blocks=0,bc6Blocks=0,detexBc7Differences=0;
    for(unsigned mode=0;mode<8;mode++)for(unsigned trial=0;trial<256;trial++){
        random(block.data());block[0]=uint8_t((block[0]&~((1u<<(mode+1))-1u))|(1u<<mode));
        alignas(8) std::array<uint8_t,64> reference{};
        require(detexDecompressBlockBPTC(block.data(),0xff,0,reference.data()),"detex BC7 admission");
        auto out=decodeBc7(4,4,block.data(),16);
        std::array<bc7decomp::color_rgba,16> third{};
        require(bc7decomp::unpack_bc7(block.data(),third.data()),"Third BC7 reference admission");
        for(unsigned i=0;i<64;i++){
            if(out.rgba[i]!=reference[i])detexBc7Differences++;
            require(out.rgba[i]==third[i/4][i%4],"BC7 independent third oracle");
        }
        bc7Blocks++;
    }
    const unsigned codes[]={0,1,2,6,10,14,18,22,26,30,3,7,11,15};
    for(bool sign:{false,true})for(unsigned code:codes)for(unsigned trial=0;trial<256;trial++){
        random(block.data());block[0]=uint8_t((block[0]&~(code<2?3u:31u))|code);
        alignas(8) std::array<uint16_t,64> reference{};
        bool valid=sign?detexDecompressBlockBPTC_SIGNED_FLOAT(block.data(),0x3fff,0,reinterpret_cast<uint8_t*>(reference.data())):
            detexDecompressBlockBPTC_FLOAT(block.data(),0x3fff,0,reinterpret_cast<uint8_t*>(reference.data()));
        require(valid,"detex BC6H admission");auto out=decodeBc6h(sign,4,4,block.data(),16);
        for(unsigned p=0;p<16;p++){
            for(unsigned c=0;c<3;c++){
                if(out.rgba[p*4+c]!=half(reference[p*4+c])){std::fprintf(stderr,"BC6H sign=%u code=%u trial=%u pixel=%u ch=%u actual=%.9g reference=%.9g half=%04x block=",sign,code,trial,p,c,out.rgba[p*4+c],half(reference[p*4+c]),reference[p*4+c]);for(auto b:block)std::fprintf(stderr,"%02x",b);std::fputc('\n',stderr);}
                require(out.rgba[p*4+c]==half(reference[p*4+c]),"BC6H independent half oracle");
            }
            require(out.rgba[p*4+3]==1,"BC6H alpha");
        }
        bc6Blocks++;
    }
    // Reserved BC6H patterns produce black RGB, alpha remains1 in RGBA wrapper.
    for(unsigned code:{19u,23u,27u,31u})for(bool sign:{false,true}){
        block.fill(255);block[0]=uint8_t((block[0]&~31u)|code);auto out=decodeBc6h(sign,4,4,block.data(),16);
        for(unsigned p=0;p<16;p++)for(unsigned c=0;c<4;c++)require(out.rgba[p*4+c]==(c==3?1:0),"BC6H reserved mode black");
    }
    // Edge clipping/padded rows exercised against independently decoded tiles.
    for(bool hdr:{false,true})for(unsigned w=1;w<=9;w++)for(unsigned h=1;h<=9;h++){
        size_t cols=(w+3)/4,rows=(h+3)/4,pitch=(cols+1)*16;std::vector<uint8_t> bytes((rows-1)*pitch+cols*16);
        for(size_t y=0;y<rows;y++)for(size_t x=0;x<cols;x++){
            auto* p=bytes.data()+y*pitch+x*16;random(p);p[0]=hdr?uint8_t((p[0]&~31u)|3u):uint8_t((p[0]&~127u)|64u);
        }
        auto rgb=hdr?DecodedBc{}:decodeBc7(w,h,bytes.data(),bytes.size(),pitch,true);
        auto fp=hdr?decodeBc6h(true,w,h,bytes.data(),bytes.size(),pitch):DecodedBcChannels{};
        if(!hdr)require(rgb.srgb,"BC7 sRGB metadata preserved");
        for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){
            alignas(8) std::array<uint16_t,64> f{};alignas(8) std::array<uint8_t,64> b{};const auto* p=bytes.data()+(y/4)*pitch+(x/4)*16;
            if(hdr){require(detexDecompressBlockBPTC_SIGNED_FLOAT(p,0x3fff,0,reinterpret_cast<uint8_t*>(f.data())),"edge oracle HDR");for(unsigned c=0;c<3;c++)require(fp.rgba[(y*w+x)*4+c]==half(f[((y%4)*4+x%4)*4+c]),"HDR edge layout");}
            else{std::array<bc7decomp::color_rgba,16> third{};require(bc7decomp::unpack_bc7(p,third.data()),"edge oracle BC7");for(unsigned c=0;c<4;c++)require(rgb.rgba[(y*w+x)*4+c]==third[(y%4)*4+x%4][c],"BC7 edge layout");}
        }
    }
    for(bool hdr:{false,true}){
        auto call=[&](uint32_t w,uint32_t h,const uint8_t* p,size_t n,size_t pitch=0){if(hdr)(void)decodeBc6h(false,w,h,p,n,pitch);else(void)decodeBc7(w,h,p,n,pitch);};
        reject([&]{call(0,4,block.data(),16);});reject([&]{call(4,0,block.data(),16);});reject([&]{call(4,4,nullptr,16);});
        reject([&]{call(4,4,block.data(),15);});reject([&]{call(4,4,block.data(),17);});reject([&]{call(4,4,block.data(),16,15);});
        reject([&]{call(0xffffffffu,4,block.data(),16);});reject([&]{call(16384,16384,block.data(),16);});reject([&]{call(4,8,block.data(),16,size_t(-1));});
    }
    std::printf("BC67_CPU_ORACLE_PASS checks=%llu BC7_blocks=%u BC6H_blocks=%u rejects=%u all_modes=YES detex_BC7_disagreements=%u NOT_GPU_OR_FULL_BC_API_PASS\n",static_cast<unsigned long long>(checks),bc7Blocks,bc6Blocks,negatives,detexBc7Differences);return 0;
}catch(const std::exception& e){std::fprintf(stderr,"FAIL %s checks=%llu\n",e.what(),static_cast<unsigned long long>(checks));return 1;}}
