#include "../src/layers.hpp"
#include "../src/shader-filter.hpp"
#include <cassert>
#include <cstdio>
using namespace mavk::bridge;
static void put(std::vector<uint8_t>& b,uint32_t n){for(unsigned i=0;i<4;i++)b.push_back(uint8_t(n>>(8*i)));}
static void set(std::vector<uint8_t>& b,size_t at,uint32_t n){for(unsigned i=0;i<4;i++)b[at+i]=uint8_t(n>>(8*i));}
int main(){
    std::vector<uint8_t> geo;for(uint32_t n:std::initializer_list<uint32_t>{12,2,3,36,0,uint32_t(VK_FORMAT_R32G32_SFLOAT),0,1,uint32_t(VK_FORMAT_R8G8B8A8_UNORM),8})put(geo,n);geo.resize(geo.size()+36);
    std::vector<uint8_t> indexed;for(uint32_t n:std::initializer_list<uint32_t>{uint32_t(geo.size()),uint32_t(VK_INDEX_TYPE_UINT16),3,6})put(indexed,n);indexed.insert(indexed.end(),geo.begin(),geo.end());indexed.insert(indexed.end(),{0,0,1,0,2,0});
    std::vector<uint8_t> scene;for(uint32_t n:std::initializer_list<uint32_t>{32,32,20,20,1,16,0,0,0,1,uint32_t(indexed.size()),0x3f800000,0,0x3f800000,0x3f800000,0x3f800000})put(scene,n);
    for(unsigned i=0;i<2;i++)for(uint32_t n:{0x07230203,0x00010000,0,8,0})put(scene,n);
    scene.insert(scene.end(),indexed.begin(),indexed.end());size_t draw=scene.size();
    for(uint32_t n:{0,3,0,1,0,0,32,32,0,0,0,0})put(scene,n);
    SceneRequest decoded;assert(!decodeScene(scene,0,decoded,1024,false));assert(decodeScene(scene,0,decoded,1024,false,true));
    std::vector<uint8_t> record;for(uint32_t n:{2,0,1,0,0})put(record,n);record.insert(record.end(),scene.begin(),scene.end());
    std::vector<uint8_t> packet;put(packet,1);put(packet,2);for(unsigned i=0;i<2;i++){put(packet,uint32_t(record.size()));packet.insert(packet.end(),record.begin(),record.end());}
    std::vector<LayerRequest> output;assert(decodeLayers(packet,0,output));assert(output.size()==2&&output[1].scene.geometry.attributes[1].format==VK_FORMAT_R8G8B8A8_UNORM);
    unsigned rejected=0;auto reject=[&](const std::vector<uint8_t>& bad){output[0].scene.width=77;assert(!decodeLayers(bad,0,output));assert(output.size()==2&&output[0].scene.width==77);rejected++;};
    for(size_t n=0;n<packet.size();n++){auto b=packet;b.resize(n);reject(b);}
    size_t first=12,sceneAt=first+20,second=first+record.size()+4;
    for(auto change:std::vector<std::pair<size_t,uint32_t>>{{0,0},{4,0},{4,9},{8,UINT32_MAX},{first,3},{first+4,2},{first+16,3},{first+16,4097},{sceneAt,0},{sceneAt+32,129},{sceneAt+44,0x7fc00000},{sceneAt+draw+4,4},{sceneAt+draw+8,UINT32_MAX},{sceneAt+draw+8,1},{sceneAt+draw+12,2},{sceneAt+draw+16,UINT32_MAX},{sceneAt+draw+24,0},{sceneAt+draw+24,33},{second+20,16}}){auto b=packet;set(b,change.first,change.second);reject(b);}
    auto trailing=packet;trailing.push_back(0);reject(trailing);assert(!decodeLayers(packet,SIZE_MAX,output));rejected++;
    std::vector<uint32_t> shader={0x07230203,0x00010000,0,8,0,(4u<<16)|71u,1,34,0,(4u<<16)|71u,1,33,0};
    assert(!shaderInstructionsAllowed(shader,false,true));assert(shaderInstructionsAllowed(shader,false,true,0));shader.back()=1;assert(!shaderInstructionsAllowed(shader,false,true,0));rejected++;
    std::printf("LAYER_PACKET_CPU_PASS rejected=%u (not GPU/guest PASS)\n",rejected);
}
