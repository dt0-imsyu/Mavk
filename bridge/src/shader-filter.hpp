#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
namespace mavk::bridge {
// Structural framing/binding guard, NOT a SPIR-V semantic validator.
inline bool shaderInstructionsAllowed(const std::vector<uint32_t>& code,bool vertexUniform=false,bool sampledFragment=false,uint32_t sampledBinding=1){
    if(sampledBinding>1)return false;
    for(size_t i=5;i<code.size();){
        uint32_t n=code[i]>>16;if(!n||n>code.size()-i)return false;
        if((code[i]&65535)==71&&n>=3&&(code[i+2]==33||code[i+2]==34)){
            if(n!=4||(!vertexUniform&&!sampledFragment))return false;
            if(code[i+3]!=(code[i+2]==33&&sampledFragment?sampledBinding:0u))return false;
        }
        i+=n;
    }
    return true;
}
}
