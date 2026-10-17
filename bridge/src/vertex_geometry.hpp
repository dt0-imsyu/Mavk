#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <utility>

namespace mavk::bridge {
struct VertexGeometry {
    uint32_t stride{},count{};
    std::vector<VkVertexInputAttributeDescription> attributes;
    std::vector<uint8_t> bytes;
    VkIndexType indexType{VK_INDEX_TYPE_UINT32};
    uint32_t indexCount{};
    std::vector<uint8_t> indices;
};
inline uint32_t vertexFormatBytes(VkFormat format,bool packedColor=false){
    switch(format){
    case VK_FORMAT_R32_SFLOAT:return 4;
    case VK_FORMAT_R32G32_SFLOAT:return 8;
    case VK_FORMAT_R32G32B32_SFLOAT:return 12;
    case VK_FORMAT_R32G32B32A32_SFLOAT:return 16;
    case VK_FORMAT_R8G8B8A8_UNORM:return packedColor?4:0;
    default:return 0;
    }
}
// Explicit wire fields, no casts to native Vulkan structures. No mutation on fail.
inline bool decodeVertexGeometry(const std::vector<uint8_t>& bytes,size_t start,VertexGeometry& output,bool packedColor=false){
    if(start>bytes.size()||bytes.size()-start<16)return false;
    auto word=[&](size_t p){return uint32_t(bytes[p])|(uint32_t(bytes[p+1])<<8)|(uint32_t(bytes[p+2])<<16)|(uint32_t(bytes[p+3])<<24);};
    uint32_t stride=word(start),attributes=word(start+4),count=word(start+8),size=word(start+12);
    if(!stride||stride>64||stride%4||!attributes||attributes>4||!count||count>4096||size!=uint64_t(stride)*count)return false;
    if(bytes.size()-start!=16+size_t(attributes)*12+size)return false;
    VertexGeometry result;result.stride=stride;result.count=count;uint32_t locations=0;
    for(uint32_t i=0;i<attributes;i++){
        size_t p=start+16+i*12;uint32_t location=word(p),format=word(p+4),offset=word(p+8);
        uint32_t width=vertexFormatBytes(VkFormat(format),packedColor);
        if(location>=4||(locations&(1u<<location))||!width||offset%4||offset>stride||width>stride-offset)return false;
        locations|=1u<<location;result.attributes.push_back({location,0,VkFormat(format),offset});
    }
    result.bytes.assign(bytes.begin()+start+16+attributes*12,bytes.end());
    output=std::move(result);return true;
}
inline bool decodeIndexedGeometry(const std::vector<uint8_t>& bytes,size_t start,VertexGeometry& output,bool packedColor=false,bool baseVertex=false){
    if(start>bytes.size()||bytes.size()-start<16)return false;
    auto word=[&](size_t p){return uint32_t(bytes[p])|(uint32_t(bytes[p+1])<<8)|(uint32_t(bytes[p+2])<<16)|(uint32_t(bytes[p+3])<<24);};
    uint32_t geometryBytes=word(start),type=word(start+4),count=word(start+8),size=word(start+12);
    uint32_t width=type==VK_INDEX_TYPE_UINT16?2:type==VK_INDEX_TYPE_UINT32?4:0;
    if(!width||!count||count>12288||uint64_t(width)*count!=size||geometryBytes>262208||bytes.size()-start!=16+size_t(geometryBytes)+size)return false;
    std::vector<uint8_t> geometry(bytes.begin()+start+16,bytes.begin()+start+16+geometryBytes);
    VertexGeometry result;if(!decodeVertexGeometry(geometry,0,result,packedColor))return false;
    size_t at=start+16+geometryBytes;
    for(uint32_t i=0;i<count;i++){
        uint32_t index=uint32_t(bytes[at+i*width])|(uint32_t(bytes[at+i*width+1])<<8);
        if(width==4)index|=(uint32_t(bytes[at+i*width+2])<<16)|(uint32_t(bytes[at+i*width+3])<<24);
        if(!baseVertex&&index>=result.count)return false;
    }
    result.indexType=VkIndexType(type);result.indexCount=count;result.indices.assign(bytes.begin()+at,bytes.end());
    output=std::move(result);return true;
}
}
