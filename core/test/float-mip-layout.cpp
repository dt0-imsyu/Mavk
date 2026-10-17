#include <mavk/sampled.hpp>
#include <cstdio>
#include <stdexcept>
int main(){
    auto bytes=mavk::sampledMipLayout(8,8,4,VK_FORMAT_R8G8B8A8_UNORM);
    auto floats=mavk::sampledMipLayout(8,8,4,VK_FORMAT_R32G32B32A32_SFLOAT);
    if(bytes.back().offset+bytes.back().bytes!=340||floats.back().offset+floats.back().bytes!=1360)return 1;
    for(unsigned i=0;i<4;i++)if(floats[i].offset!=bytes[i].offset*4||floats[i].bytes!=bytes[i].bytes*4)return 2;
    unsigned rejected=0;
    try{mavk::sampledMipLayout(8,8,4,VK_FORMAT_R16G16_SFLOAT);}catch(const std::invalid_argument&){rejected++;}
    try{mavk::sampledMipLayout(0,8,4,VK_FORMAT_R32G32B32A32_SFLOAT);}catch(const std::invalid_argument&){rejected++;}
    try{mavk::sampledMipLayout(8,8,5,VK_FORMAT_R32G32B32A32_SFLOAT);}catch(const std::invalid_argument&){rejected++;}
    if(rejected!=3)return 3;
    std::puts("SAMPLED_FLOAT_LAYOUT_CPU_PASS rejects=3 defaults_preserved=YES");return 0;
}
