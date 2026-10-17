#include "../src/shader-filter.hpp"
#include <cassert>
#include <cstdio>
int main(){
    using namespace mavk::bridge;
    unsigned rejected=0;
    auto words=std::vector<uint32_t>{0x07230203,0x00010000,0,8,0};
    assert(shaderInstructionsAllowed(words));
    for(uint32_t decoration:{33u,34u}){
        auto decorated=words;decorated.insert(decorated.end(),{(4u<<16)|71u,1,decoration,0});
        assert(!shaderInstructionsAllowed(decorated));rejected++;
        assert(shaderInstructionsAllowed(decorated,true));
        decorated.back()=1;assert(!shaderInstructionsAllowed(decorated,true));rejected++;
        decorated.pop_back();decorated[5]=(3u<<16)|71u;
        assert(!shaderInstructionsAllowed(decorated,true));rejected++;
    }
    words.push_back(0);assert(!shaderInstructionsAllowed(words,true));rejected++;
    words.back()=4u<<16;assert(!shaderInstructionsAllowed(words,true));rejected++;
    auto sampled=std::vector<uint32_t>{0x07230203,0x00010000,0,8,0,(4u<<16)|71u,1,34,0,(4u<<16)|71u,1,33,1};
    assert(shaderInstructionsAllowed(sampled,false,true));
    assert(!shaderInstructionsAllowed(sampled));rejected++;
    assert(!shaderInstructionsAllowed(sampled,true));rejected++;
    sampled.back()=0;assert(!shaderInstructionsAllowed(sampled,false,true));rejected++;
    sampled.back()=1;sampled[8]=1;assert(!shaderInstructionsAllowed(sampled,false,true));rejected++;
    std::printf("SHADER_DESCRIPTOR_FILTER_CPU_PASS rejected=%u (not GPU PASS)\n",rejected);
}
