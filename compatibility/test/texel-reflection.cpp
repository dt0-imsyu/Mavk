#include <mavk/compat/shader_descriptors.hpp>
#include <fstream>
#include <iostream>
#include "grouped-fixture.hpp"
using namespace mavk::compat;
int main(int argc,char** argv){try{
    if(argc!=2&&argc!=3)throw std::runtime_error("compiled texel reflection shader and optional fresh grouped output required");
    std::ifstream file(argv[1],std::ios::binary|std::ios::ate);auto bytes=file.tellg();
    if(bytes<20||bytes>1024*1024||bytes%4)throw std::runtime_error("shader file length");
    std::vector<uint32_t> code(size_t(bytes)/4);file.seekg(0);file.read(reinterpret_cast<char*>(code.data()),bytes);if(!file)throw std::runtime_error("shader read");
    auto reflected=reflectTexelFormats(code);if(reflected.size()!=6)throw std::runtime_error("only six Buffer image descriptors expected");
    auto images=reflectColor2DFormats(code);if(images.size()!=1||images.at({2,0})!=VK_FORMAT_R8G8B8A8_UNORM)throw std::runtime_error("color 2D reflection/texel exclusion mismatch");
    auto gamma=reflectColor1DFormats(code);if(gamma.size()!=1||gamma.at({2,2})!=VK_FORMAT_R8G8B8A8_UNORM)throw std::runtime_error("color 1D reflection/dimension exclusion mismatch");
    ImageShaderTypes declarations;reflectImageFormats(code,true,1,&declarations);
    if(declarations.at({2,0})[0]!=1||declarations.at({2,2})[0]!=0||declarations.at({0,0})[0]!=5)throw std::runtime_error("image declaration dimensions lost");
    auto colorTypes=reflectColorImageTypes(code);
    if(colorTypes.size()!=5||colorTypes.at({2,3}).viewType!=VK_IMAGE_VIEW_TYPE_2D_ARRAY||colorTypes.at({2,3}).format!=VK_FORMAT_R8G8B8A8_UNORM)throw std::runtime_error("array shader shape not retained");
    if(colorTypes.at({2,4}).viewType!=VK_IMAGE_VIEW_TYPE_3D||colorTypes.at({2,4}).format!=VK_FORMAT_R8G8B8A8_UNORM||declarations.at({2,4})[0]!=2||images.count({2,4})||gamma.count({2,4}))throw std::runtime_error("3D shader shape aliased");
    if(colorTypes.at({2,5}).viewType!=VK_IMAGE_VIEW_TYPE_CUBE||declarations.at({2,5})[0]!=3||colorTypes.count({2,6})||declarations.at({2,6})[1]!=1||colorTypes.count({2,7})||declarations.at({2,7})[2]!=1)throw std::runtime_error("cube shape/depth/cube-array exclusion mismatch");
    auto array3D=code;for(size_t p=5;p<array3D.size();p+=array3D[p]>>16)if((array3D[p]&65535)==25&&array3D[p+3]==2)array3D[p+5]=1;
    if(reflectColorImageTypes(array3D).count({2,4}))throw std::runtime_error("nonexistent 3D-array view admitted");
    if(declarations.at({2,3})[2]!=1||images.count({2,3})||gamma.count({2,3}))throw std::runtime_error("array/non-array shapes aliased");
    size_t groupCount=0;auto grouped=groupDescriptorFixture(code,&groupCount);
    if(argc==3){if(std::ifstream(argv[2],std::ios::binary))throw std::runtime_error("preserve grouped fixture");std::ofstream out(argv[2],std::ios::binary);out.write(reinterpret_cast<const char*>(grouped.data()),std::streamsize(grouped.size()*4));if(!out)throw std::runtime_error("grouped fixture write");}
    if(reflectTexelFormats(grouped)!=reflected||reflectColor2DFormats(grouped)!=images||reflectColor1DFormats(grouped)!=gamma)throw std::runtime_error("group decorations lost image/texel metadata");
    ImageShaderTypes groupedDeclarations;reflectImageFormats(grouped,true,1,&groupedDeclarations);if(groupedDeclarations!=declarations)throw std::runtime_error("group declarations differ");
    auto groupedTypes=reflectColorImageTypes(grouped);if(groupedTypes.size()!=colorTypes.size())throw std::runtime_error("grouped image view count");
    for(const auto& [key,type]:colorTypes)if(groupedTypes.at(key).format!=type.format||groupedTypes.at(key).viewType!=type.viewType)throw std::runtime_error("grouped image view shape changed");
    const VkFormat formats[]={VK_FORMAT_R32_UINT,VK_FORMAT_R32_SINT,VK_FORMAT_R32_SFLOAT};
    unsigned checks=0,negative=0;
    for(unsigned set=0;set<2;set++)for(unsigned binding=0;binding<3;binding++){if(reflected.at({set,binding})!=formats[binding])throw std::runtime_error("numeric type mismatch");checks++;}
    auto reject=[&](auto invalid){try{reflectTexelFormats(invalid);}catch(const std::exception&){negative++;return;}throw std::runtime_error("malformed reflection input accepted");};
    reject(std::vector<uint32_t>{});auto invalid=code;invalid[0]=0;reject(invalid);
    invalid=code;invalid[5]&=65535;reject(invalid);
    invalid=code;invalid[5]=(uint32_t(code.size())<<16)|(invalid[5]&65535);reject(invalid);
    size_t groupAt=0,applyAt=0;
    for(size_t p=5;p<grouped.size();p+=grouped[p]>>16){if(!groupAt&&(grouped[p]&65535)==73)groupAt=p;if(!applyAt&&(grouped[p]&65535)==74)applyAt=p;}
    if(!groupAt||!applyAt)throw std::runtime_error("group fixture opcodes missing");
    invalid=grouped;invalid[applyAt+1]=grouped[3]+1;reject(invalid);
    invalid=grouped;invalid[applyAt+2]=grouped[applyAt+1];reject(invalid);
    invalid=grouped;invalid.insert(invalid.begin()+groupAt,{(2u<<16)|73,grouped[groupAt+1]});reject(invalid);
    invalid=grouped;auto words=invalid[applyAt]>>16;invalid.erase(invalid.begin()+applyAt+2,invalid.begin()+applyAt+words);invalid[applyAt]=(2u<<16)|74;reject(invalid);
    invalid=grouped;invalid.insert(invalid.begin()+applyAt,{(4u<<16)|71,grouped[applyAt+2],33,31});reject(invalid);
    invalid=grouped;invalid.insert(invalid.begin()+applyAt,{(4u<<16)|75,grouped[applyAt+1],grouped[applyAt+2],0});reject(invalid);
    std::cout<<"TEXEL_REFLECTION_PASS formats="<<checks<<" negatives="<<negative<<" otherImageExcluded=1 decorationGroups="<<groupCount<<" equivalentGroupedViews=1\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
