#include <mavk/compat/sampled.hpp>
#include <array>
#include <cstdio>
#include <stdexcept>
using namespace mavk::compat;
static unsigned checks=0,negatives=0;
static void require(bool b){checks++;if(!b)throw std::runtime_error("BC texture state mismatch");}
template<class F>static void invalid(F f){try{f();}catch(const std::invalid_argument&){negatives++;return;}throw std::runtime_error("BC invalid input accepted");}
template<class F>static void incomplete(F f){try{f();}catch(const std::logic_error&){negatives++;return;}throw std::runtime_error("BC uninitialized input accepted");}
int main(){try{
    std::array<uint8_t,8> red{0,248},green{224,7},blue{31,0};
    BcTexture texture(TextureBcFormat::BC1_RGB_UNORM,8,8,4);
    incomplete([&]{sampledPixels(texture);});incomplete([&]{texture.compressedMip(0);});
    texture.update(0,{0,0,4,4},red.data(),8);require(!texture.initialized(0));
    texture.update(0,{4,0,4,4},green.data(),8);texture.update(0,{0,4,4,4},blue.data(),8);texture.update(0,{4,4,4,4},red.data(),8);
    for(unsigned level=1;level<4;level++)texture.update(level,{0,0,texture.width(level),texture.height(level)},green.data(),8);
    require(texture.initialized(0)&&texture.decodedBlocks()==7);
    auto out=sampledPixels(texture);require(out.rgba.size()==340&&out.format==VK_FORMAT_R8G8B8A8_UNORM);
    require(out.rgba[0]==255&&out.rgba[4*4+1]==255&&out.rgba[(4*8)*4+2]==255);
    auto version=texture.revision(0);require(texture.acknowledge(0,version));require(texture.dirtyRegion(0).width==0);
    auto before=texture.decodedMip(0);texture.update(0,{4,4,4,4},blue.data(),8);
    require(texture.decodedBlocks()==8&&!texture.acknowledge(0,version));
    auto dirty=texture.dirtyRegion(0);require(dirty.x==4&&dirty.y==4&&dirty.width==4&&dirty.height==4);
    for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++)if(x<4||y<4)for(unsigned c=0;c<4;c++)require(texture.decodedMip(0)[(y*8+x)*4+c]==before[(y*8+x)*4+c]);
    // Snapshot semantics for overlapping self-copy, not memcpy of aliased rows.
    texture.copyFrom(texture,0,{0,0,4,8},0,4,0);require(texture.decodedBlocks()==10);
    require(texture.decodedMip(0)[(4*8+4)*4+2]==255&&texture.decodedMip(0)[4*4]==255);
    BcTexture empty(TextureBcFormat::BC1_RGB_UNORM,8,8,1);
    incomplete([&]{texture.copyFrom(empty,0,{0,0,4,4},0,0,0);});
    // Invalid updates/copies must preserve revision, bytes and decoded-block count.
    before=texture.decodedMip(0);version=texture.revision(0);auto blocks=texture.decodedBlocks();
    invalid([&]{texture.update(0,{1,0,4,4},red.data(),8);});invalid([&]{texture.update(0,{0,1,4,4},red.data(),8);});
    invalid([&]{texture.update(0,{0,0,3,4},red.data(),8);});invalid([&]{texture.update(0,{0,0,4,3},red.data(),8);});
    invalid([&]{texture.update(0,{0,0,9,4},red.data(),8);});invalid([&]{texture.update(0,{0,0,4,4},nullptr,8);});
    invalid([&]{texture.update(0,{0,0,4,4},red.data(),7);});invalid([&]{texture.update(0,{0,0,4,4},red.data(),9);});
    invalid([&]{texture.update(9,{0,0,4,4},red.data(),8);});invalid([&]{texture.update(0,{0,0,4,4},red.data(),8,9);});
    invalid([&]{texture.copyFrom(texture,0,{0,0,4,4},0,7,0);});
    require(texture.revision(0)==version&&texture.decodedBlocks()==blocks&&texture.decodedMip(0)==before);
    invalid([&]{BcTexture bad(TextureBcFormat::BC1_RGB_UNORM,0,8,1);});invalid([&]{BcTexture bad(TextureBcFormat::BC1_RGB_UNORM,8,8,5);});
    invalid([&]{BcTexture bad(TextureBcFormat::BC1_RGB_UNORM,8,8,4,100);});invalid([&]{BcTexture bad(static_cast<TextureBcFormat>(99),8,8,1);});
    // Tiny/odd mip edges accept whole partial blocks; all16 formats route correctly.
    for(unsigned f=0;f<=15;f++){
        BcTexture t(static_cast<TextureBcFormat>(f),7,5,3);size_t blockBytes=(f<=3||f==8||f==9)?8:16;
        for(unsigned level=0;level<3;level++){
            size_t cols=(t.width(level)+3)/4,rows=(t.height(level)+3)/4,pitch=(cols+1)*blockBytes;
            std::vector<uint8_t> bytes((rows-1)*pitch+cols*blockBytes);
            t.update(level,{0,0,t.width(level),t.height(level)},bytes.data(),bytes.size(),pitch);
            require(t.initialized(level));
        }
        auto snapshot=sampledPixels(t);require(snapshot.levels==3&&snapshot.width==7&&snapshot.height==5);
        require(t.decodedBlocks()==6); // 4 base blocks +1+1 mip blocks
        if(f==1||f==3||f==5||f==7||f==15)require(snapshot.format==VK_FORMAT_R8G8B8A8_SRGB);
        else if(f>=8&&f<=13)require(snapshot.format==VK_FORMAT_R32G32B32A32_SFLOAT);
        else require(snapshot.format==VK_FORMAT_R8G8B8A8_UNORM);
    }
    BcTexture srgb(TextureBcFormat::BC1_RGB_SRGB,8,8,1);
    invalid([&]{texture.copyFrom(srgb,0,{0,0,4,4},0,0,0);});
    require(negatives==19);
    std::printf("BC_TEXTURE_LIFECYCLE_CPU_PASS checks=%u negatives=%u formats=16 dirty_block_decode=YES overlap_snapshot=YES NOT_VULKAN_API_OR_DXVK_PASS\n",checks,negatives);return 0;
}catch(const std::exception& e){std::fprintf(stderr,"FAIL %s checks=%u negatives=%u\n",e.what(),checks,negatives);return 1;}}
