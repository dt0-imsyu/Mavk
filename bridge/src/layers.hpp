#pragma once
#include "graphics.hpp"
namespace mavk::bridge {
struct LayerRequest {
    RasterOptions raster;uint64_t texture=0;std::vector<uint8_t> uniform,sceneBytes;SceneRequest scene;
};
// V1 bounded native-state packet, not a serialized native Vulkan struct/handle.
// Every record and draw is checked before the caller creates a GPU pipeline.
inline bool decodeLayers(const std::vector<uint8_t>& bytes,size_t start,std::vector<LayerRequest>& output){
    if(start>bytes.size()||bytes.size()-start<8||bytes.size()-start>1048576)return false;
    auto word=[&](size_t p){return uint32_t(bytes[p])|(uint32_t(bytes[p+1])<<8)|(uint32_t(bytes[p+2])<<16)|(uint32_t(bytes[p+3])<<24);};
    uint32_t count=word(start+4);if(word(start)!=1||!count||count>8)return false;size_t at=start+8;
    std::vector<LayerRequest> layers;layers.reserve(count);
    for(uint32_t i=0;i<count;i++){
        if(bytes.size()-at<4)return false;uint32_t length=word(at);at+=4;
        if(length<20||length>bytes.size()-at)return false;size_t end=at+length;
        LayerRequest layer;uint32_t blend=word(at),binding=word(at+4),uniformBytes=word(at+16);
        if(blend>2||binding>1||uniformBytes>4096||uniformBytes%4||uniformBytes>length-20||(uniformBytes&&binding==0))return false;
        layer.raster.sourceOver=blend==1;layer.raster.uiOverlayAlpha=blend==2;layer.raster.sampledBinding=binding;
        layer.texture=word(at+8)|(uint64_t(word(at+12))<<32);
        layer.uniform.assign(bytes.begin()+at+20,bytes.begin()+at+20+uniformBytes);
        layer.sceneBytes.assign(bytes.begin()+at+20+uniformBytes,bytes.begin()+end);
        if(!decodeScene(layer.sceneBytes,0,layer.scene,1024,false,true))return false;
        if(i&&(layer.scene.width!=layers[0].scene.width||layer.scene.height!=layers[0].scene.height||std::memcmp(layer.scene.clear,layers[0].scene.clear,16)))return false;
        layers.push_back(std::move(layer));at=end;
    }
    if(at!=bytes.size())return false;output=std::move(layers);return true;
}
}
