#pragma once
#include "vertex_geometry.hpp"
#include <cmath>
#include <cstring>

namespace mavk::bridge {
struct SceneDraw {
    uint32_t firstIndex{},indexCount{};std::vector<uint8_t> push;
    // Native/extended opcode24 fields; legacy scene commands leave defaults.
    int32_t vertexOffset=0;bool clipped=false;VkRect2D clip{};
};
struct SceneState {
    uint32_t pushBytes{16};VkShaderStageFlags pushStages{VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT};
    VkCullModeFlags cull{VK_CULL_MODE_NONE};VkFrontFace front{VK_FRONT_FACE_COUNTER_CLOCKWISE};
    VkFormat depthFormat{VK_FORMAT_UNDEFINED};float clearDepth{1};std::vector<SceneDraw> draws;
};
struct SceneRequest {
    uint32_t width{},height{},vertexBytes{},fragmentBytes{};size_t vertexStart{},fragmentStart{};
    float clear[4]{};VertexGeometry geometry;SceneState state;
};
// One bounded pipeline/geometry, up to16 indexed draws. Output unchanged on reject.
inline bool decodeScene(const std::vector<uint8_t>& bytes,size_t start,SceneRequest& output,uint32_t maxDimension=512,bool uniformOnly=false,bool extended=false){
    if(start>bytes.size()||bytes.size()-start<64||bytes.size()-start>1048576)return false;
    auto word=[&](size_t p){return uint32_t(bytes[p])|(uint32_t(bytes[p+1])<<8)|(uint32_t(bytes[p+2])<<16)|(uint32_t(bytes[p+3])<<24);};
    SceneRequest r;r.width=word(start);r.height=word(start+4);r.vertexBytes=word(start+8);r.fragmentBytes=word(start+12);
    auto& s=r.state;s.pushStages=word(start+16);s.pushBytes=word(start+20);s.cull=word(start+24);s.front=VkFrontFace(word(start+28));s.depthFormat=VkFormat(word(start+32));
    uint32_t count=word(start+36),geometryBytes=word(start+40);
    if(maxDimension>1024||r.width<16||r.width>maxDimension||r.height<16||r.height>maxDimension||r.vertexBytes<20||r.vertexBytes>65536||r.vertexBytes%4||r.fragmentBytes<20||r.fragmentBytes>65536||r.fragmentBytes%4)return false;
    if(extended){if(s.pushBytes>64||s.pushBytes%4||(s.pushStages&~uint32_t(VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT))||bool(s.pushStages)!=bool(s.pushBytes))return false;}
    else if(uniformOnly?(s.pushStages||s.pushBytes):(!s.pushStages||(s.pushStages&~uint32_t(VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT))||!s.pushBytes||s.pushBytes>64||s.pushBytes%4))return false;
    if(s.cull>3||uint32_t(s.front)>1||!count||count>(extended?64u:16u))return false;
    if(s.depthFormat!=VK_FORMAT_UNDEFINED&&s.depthFormat!=VK_FORMAT_D32_SFLOAT)return false;
    for(uint32_t i=0;i<5;i++){uint32_t bits=word(start+44+i*4);float f;std::memcpy(&f,&bits,4);if(!std::isfinite(f))return false;if(i<4)r.clear[i]=f;else s.clearDepth=f;}
    if(s.clearDepth<0||s.clearDepth>1)return false;
    uint64_t end=uint64_t(start)+64+r.vertexBytes+r.fragmentBytes+geometryBytes;
    uint32_t drawHeader=extended?32:8;
    if(end>bytes.size()||bytes.size()-end!=uint64_t(count)*(drawHeader+s.pushBytes))return false;
    r.vertexStart=start+64;r.fragmentStart=r.vertexStart+r.vertexBytes;
    size_t geometryStart=r.fragmentStart+r.fragmentBytes;
    std::vector<uint8_t> geometry(bytes.begin()+geometryStart,bytes.begin()+size_t(end));
    if(!decodeIndexedGeometry(geometry,0,r.geometry,extended,extended))return false;
    for(uint32_t i=0;i<count;i++){
        size_t at=size_t(end)+i*(drawHeader+s.pushBytes);SceneDraw draw;draw.firstIndex=word(at);draw.indexCount=word(at+4);
        if(!draw.indexCount||draw.firstIndex>r.geometry.indexCount||draw.indexCount>r.geometry.indexCount-draw.firstIndex)return false;
        if(extended){
            uint32_t base=word(at+8),flags=word(at+12);std::memcpy(&draw.vertexOffset,&base,4);if(flags>1)return false;draw.clipped=flags!=0;
            uint32_t x=word(at+16),y=word(at+20),w=word(at+24),h=word(at+28);
            if(draw.clipped){if(x>r.width||y>r.height||!w||!h||w>r.width-x||h>r.height-y)return false;draw.clip={{int32_t(x),int32_t(y)},{w,h}};}
            else if(x||y||w||h)return false;
            for(uint32_t k=draw.firstIndex;k<draw.firstIndex+draw.indexCount;k++){
                size_t p=size_t(k)*(r.geometry.indexType==VK_INDEX_TYPE_UINT16?2:4);uint32_t value=r.geometry.indices[p]|(uint32_t(r.geometry.indices[p+1])<<8);
                if(r.geometry.indexType==VK_INDEX_TYPE_UINT32)value|=(uint32_t(r.geometry.indices[p+2])<<16)|(uint32_t(r.geometry.indices[p+3])<<24);
                int64_t vertex=int64_t(value)+draw.vertexOffset;if(vertex<0||vertex>=r.geometry.count)return false;
            }
        }
        draw.push.assign(bytes.begin()+at+drawHeader,bytes.begin()+at+drawHeader+s.pushBytes);s.draws.push_back(std::move(draw));
    }
    output=std::move(r);return true;
}
}
