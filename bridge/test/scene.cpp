#include "../src/scene.hpp"
#include <cassert>
#include <cstdio>
using namespace mavk::bridge;
static void word(std::vector<uint8_t>& b,size_t p,uint32_t v){for(unsigned i=0;i<4;i++)b[p+i]=uint8_t(v>>(8*i));}
int main(){
    // Shader header bodies deliberately dummy: this unit tests framing, not SPIR-V.
    constexpr size_t geometry=64+40,geometryBytes=16+16+12+24+12,draws=geometry+geometryBytes;
    std::vector<uint8_t> good(draws+3*72);
    for(auto entry:std::vector<std::pair<size_t,uint32_t>>{{0,64},{4,64},{8,20},{12,20},{16,1},{20,64},{24,2},{28,1},{32,126},{36,3},{40,geometryBytes},{56,0x3f800000},{60,0x3f800000},
        {geometry,16+12+24},{geometry+4,1},{geometry+8,3},{geometry+12,12},{geometry+16,8},{geometry+20,1},{geometry+24,3},{geometry+28,24},{geometry+32,0},{geometry+36,103},{geometry+40,0},{geometry+68,1},{geometry+72,2}})word(good,entry.first,entry.second);
    for(unsigned i=0;i<3;i++)word(good,draws+i*72+4,3);
    SceneRequest output;assert(decodeScene(good,0,output)&&output.state.draws.size()==3&&output.geometry.count==3);
    unsigned rejected=0;
    for(size_t n=0;n<good.size();n++){auto b=good;b.resize(n);assert(!decodeScene(b,0,output));assert(output.state.draws.size()==3);rejected++;}
    for(auto change:std::vector<std::pair<size_t,uint32_t>>{{0,0},{4,513},{8,UINT32_MAX},{16,32},{20,0},{20,68},{24,4},{28,2},{32,129},{36,0},{36,17},{40,UINT32_MAX},{60,0x7fc00000},{60,0xbf800000},{draws,3},{draws+4,4}}){auto b=good;word(b,change.first,change.second);assert(!decodeScene(b,0,output));rejected++;}
    auto trailing=good;trailing.push_back(0);assert(!decodeScene(trailing,0,output));rejected++;
    assert(!decodeScene(good,SIZE_MAX,output));rejected++;
    auto large=good;word(large,0,1024);word(large,4,1024);
    assert(!decodeScene(large,0,output));rejected++;
    assert(decodeScene(large,0,output,1024)&&output.width==1024&&output.height==1024);
    word(large,4,1025);assert(!decodeScene(large,0,output,1024));rejected++;
    auto uniform=good;uniform.resize(draws+3*8);word(uniform,16,0);word(uniform,20,0);
    for(unsigned i=0;i<3;i++){word(uniform,draws+i*8,0);word(uniform,draws+i*8+4,3);}
    assert(decodeScene(uniform,0,output,1024,true)&&output.state.pushBytes==0);
    assert(!decodeScene(uniform,0,output,1024));rejected++;
    for(size_t cut=0;cut<uniform.size();cut++){auto b=uniform;b.resize(cut);assert(!decodeScene(b,0,output,1024,true));rejected++;}
    word(uniform,16,1);assert(!decodeScene(uniform,0,output,1024,true));rejected++;
    std::printf("SCENE_WIRE_CPU_PASS rejected=%u (not GPU/Windows PASS)\n",rejected);
}
