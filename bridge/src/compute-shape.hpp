#pragma once
#include "shader-filter.hpp"
namespace mavk::bridge {
// Initial compute slice: literal local size, one dimensional dispatch, one SSBO.
// Not SPIR-V semantic validation. Trusted finite guest only.
inline bool computeShape(const std::vector<uint32_t>& words,uint32_t& localX){
    if(words.size()<5||words[0]!=0x07230203||words[1]>0x00010300||!words[3]||words[3]>65536||words[4]||!shaderInstructionsAllowed(words,true))return false;
    uint32_t entry=0,size=0;unsigned entries=0,modes=0;
    for(size_t i=5;i<words.size();i+=words[i]>>16){
        uint32_t n=words[i]>>16,op=words[i]&65535;
        if(op==15){if(n<5||words[i+1]!=5)return false;entry=words[i+2];entries++;}
    }
    for(size_t i=5;i<words.size();i+=words[i]>>16){
        uint32_t n=words[i]>>16,op=words[i]&65535;
        if(op==16&&n>=3&&words[i+2]==17){if(n!=6||words[i+1]!=entry||!words[i+3]||words[i+3]>256||words[i+4]!=1||words[i+5]!=1)return false;size=words[i+3];modes++;}
        if(op==331)return false; // LocalSizeId/specialized workgroup sizes not supported.
    }
    if(entries!=1||modes!=1)return false;localX=size;return true;
}
}
