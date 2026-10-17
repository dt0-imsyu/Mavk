#include "../src/vertex_geometry.hpp"
#include <cassert>
#include <cstdio>
#include <algorithm>
#include <iterator>
using namespace mavk::bridge;
static void word(std::vector<uint8_t>& b,size_t p,uint32_t v){for(unsigned i=0;i<4;i++)b[p+i]=uint8_t(v>>(i*8));}
int main(){
    std::vector<uint8_t> good(16+24+3*24);
    word(good,0,24);word(good,4,2);word(good,8,3);word(good,12,72);
    word(good,16,0);word(good,20,VK_FORMAT_R32G32B32_SFLOAT);word(good,24,0);
    word(good,28,1);word(good,32,VK_FORMAT_R32G32B32_SFLOAT);word(good,36,12);
    VertexGeometry output;assert(decodeVertexGeometry(good,0,output));assert(output.bytes.size()==72&&output.attributes.size()==2&&output.count==3);
    unsigned rejected=0;
    for(size_t n=0;n<good.size();n++){auto b=good;b.resize(n);assert(!decodeVertexGeometry(b,0,output));rejected++;}
    for(auto change:std::vector<std::pair<size_t,uint32_t>>{{0,0},{0,68},{0,22},{4,0},{4,5},{8,0},{8,4097},{12,71},{12,UINT32_MAX},{16,4},{28,0},{20,VK_FORMAT_R8G8B8A8_UNORM},{24,16},{24,UINT32_MAX},{36,13}}){
        auto b=good;word(b,change.first,change.second);assert(!decodeVertexGeometry(b,0,output));assert(output.count==3&&output.bytes.size()==72);rejected++;
    }
    auto trailing=good;trailing.push_back(0);assert(!decodeVertexGeometry(trailing,0,output));rejected++;
    assert(!decodeVertexGeometry(good,SIZE_MAX,output));rejected++;
    std::vector<uint8_t> indexed(16+good.size()+12);word(indexed,0,uint32_t(good.size()));word(indexed,4,VK_INDEX_TYPE_UINT32);word(indexed,8,3);word(indexed,12,12);
    std::copy(good.begin(),good.end(),indexed.begin()+16);word(indexed,16+good.size(),2);word(indexed,20+good.size(),0);word(indexed,24+good.size(),1);
    assert(decodeIndexedGeometry(indexed,0,output)&&output.indexCount==3&&output.indices.size()==12);
    for(size_t n=0;n<indexed.size();n++){auto b=indexed;b.resize(n);assert(!decodeIndexedGeometry(b,0,output));rejected++;}
    for(auto change:std::vector<std::pair<size_t,uint32_t>>{{0,UINT32_MAX},{4,VK_INDEX_TYPE_UINT8_EXT},{8,12289},{12,11},{16+good.size(),3},{20+good.size(),UINT32_MAX}}){auto b=indexed;word(b,change.first,change.second);assert(!decodeIndexedGeometry(b,0,output));rejected++;}
    indexed.resize(16+good.size()+6);word(indexed,4,VK_INDEX_TYPE_UINT16);word(indexed,12,6);
    const uint8_t indices16[]={2,0,0,0,1,0};std::copy(std::begin(indices16),std::end(indices16),indexed.end()-6);
    assert(decodeIndexedGeometry(indexed,0,output)&&output.indexType==VK_INDEX_TYPE_UINT16&&output.indexCount==3);
    std::printf("VERTEX_INDEX_GEOMETRY_WIRE_PASS rejected=%u (CPU parser only, not GPU PASS)\n",rejected);
}
