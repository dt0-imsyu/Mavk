// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <vulkan/vulkan.h>
#include <map>
#include <vector>
#include <stdexcept>
#include <array>
#include <set>

namespace mavk::compat {
using TexelShaderFormats=std::map<std::pair<uint32_t,uint32_t>,VkFormat>;
// Declaration metadata for precise unsupported-type diagnostics. Recording it
// does not admit the shape or establish static use by a selected entry point.
using ImageShaderTypes=std::map<std::pair<uint32_t,uint32_t>,std::array<uint32_t,6>>;
// Bounded reflection of validated ORIGINAL SPIR-V,not a SPIR-V validator.
// Only 32-bit scalar Buffer/color-1D/2D/3D images (1D/2D array views)
// and sampled cube are admitted for null selection. Depth/MS/cube-array remain gaps.
// Unknown dimensions/types are absent;sampled format selection preserves the
// declared scalar type. Explicit storage formats must match exactly.
inline TexelShaderFormats reflectImageFormats(const std::vector<uint32_t>& code,bool texel,uint32_t colorDimension=1,ImageShaderTypes* declarations=nullptr,uint32_t arrayed=0){
    if(code.size()<5||code.size()>262144||code[0]!=0x07230203)throw std::invalid_argument("bounded descriptor SPIR-V required");
    std::map<uint32_t,size_t> types;std::map<uint32_t,uint32_t> sets,bindings;
    std::vector<std::pair<uint32_t,uint32_t>> variables;
    std::set<uint32_t> groups;
    std::vector<std::pair<uint32_t,uint32_t>> groupTargets;
    std::vector<uint32_t> memberGroups;
    for(size_t p=5;p<code.size();){
        auto n=code[p]>>16,op=code[p]&65535;if(!n||n>code.size()-p)throw std::invalid_argument("malformed descriptor SPIR-V");
        if((op==21||op==22||op==25||op==27||op==28||op==29||op==32)&&n>=2){if(!types.emplace(code[p+1],p).second)throw std::invalid_argument("duplicate SPIR-V type");}
        if(op==59&&n>=4&&code[p+3]==0)variables.emplace_back(code[p+2],code[p+1]);
        if(op==71&&n==4&&(code[p+2]==33||code[p+2]==34)){
            auto& table=code[p+2]==33?bindings:sets;auto [at,added]=table.emplace(code[p+1],code[p+3]);
            if(!added&&at->second!=code[p+3])throw std::invalid_argument("conflicting descriptor decoration");
        }
        if(op==73){if(n!=2||!groups.insert(code[p+1]).second)throw std::invalid_argument("malformed decoration group");}
        if(op==74){if(n<3)throw std::invalid_argument("empty group decoration targets");for(uint32_t i=2;i<n;i++)groupTargets.emplace_back(code[p+1],code[p+i]);}
        if(op==75){if(n<4||(n-2)%2)throw std::invalid_argument("malformed member decoration group");memberGroups.push_back(code[p+1]);}
        p+=n;
    }
    auto apply=[&](auto& values,uint32_t group,uint32_t target){auto value=values.find(group);if(value==values.end())return;auto [at,added]=values.emplace(target,value->second);if(!added&&at->second!=value->second)throw std::invalid_argument("conflicting grouped descriptor decoration");};
    for(auto [group,target]:groupTargets){
        if(!groups.count(group)||groups.count(target))throw std::invalid_argument("unknown or nested decoration group");
        apply(sets,group,target);apply(bindings,group,target);
    }
    for(auto group:memberGroups)if(!groups.count(group)||sets.count(group)||bindings.count(group))throw std::invalid_argument("descriptor decoration applied to member");
    TexelShaderFormats out;
    for(auto [id,type]:variables){
        if(!sets.count(id)||!bindings.count(id))continue;
        size_t image=0;
        for(unsigned depth=0;depth<8;depth++){
            auto at=types.find(type);if(at==types.end())break;auto p=at->second;auto n=code[p]>>16,op=code[p]&65535;
            if(op==32&&n==4)type=code[p+3];
            else if((op==27&&n==3)||(op==28&&n==4)||(op==29&&n==3))type=code[p+2];
            else {if(op==25&&n>=9)image=p;break;}
        }
        if(!image)continue;
        if(declarations)declarations->emplace(std::make_pair(sets.at(id),bindings.at(id)),std::array<uint32_t,6>{code[image+3],code[image+4],code[image+5],code[image+6],code[image+7],code[image+8]});
        if(code[image+3]!=(texel?5u:colorDimension)||code[image+4]||code[image+5]!=arrayed||code[image+6])continue;
        if(!texel&&colorDimension==3&&code[image+7]!=1)continue;
        auto scalar=types.find(code[image+2]);if(scalar==types.end())continue;
        auto p=scalar->second;auto n=code[p]>>16,op=code[p]&65535;VkFormat format=VK_FORMAT_UNDEFINED;uint32_t explicitFormat=0;
        if(op==22&&n==3&&code[p+2]==32){format=VK_FORMAT_R32_SFLOAT;explicitFormat=3;}
        if(op==21&&n==4&&code[p+2]==32&&code[p+3]<=1){format=code[p+3]?VK_FORMAT_R32_SINT:VK_FORMAT_R32_UINT;explicitFormat=code[p+3]?24:33;}
        if(format==VK_FORMAT_UNDEFINED)continue;
        if(texel){if(code[image+8]&&code[image+8]!=explicitFormat)continue;}
        else {
            const auto scalarFormat=format;format=VK_FORMAT_UNDEFINED;
            switch(code[image+8]){
            case 0: // Unknown sampled format:preserve scalar type,not bit layout.
                if(code[image+7]==1)format=scalarFormat==VK_FORMAT_R32_SFLOAT?VK_FORMAT_R8G8B8A8_UNORM:scalarFormat==VK_FORMAT_R32_SINT?VK_FORMAT_R32G32B32A32_SINT:VK_FORMAT_R32G32B32A32_UINT;break;
            case 1:if(scalarFormat==VK_FORMAT_R32_SFLOAT)format=VK_FORMAT_R32G32B32A32_SFLOAT;break;
            case 3:if(scalarFormat==VK_FORMAT_R32_SFLOAT)format=VK_FORMAT_R32_SFLOAT;break;
            case 4:if(scalarFormat==VK_FORMAT_R32_SFLOAT)format=VK_FORMAT_R8G8B8A8_UNORM;break;
            case 21:if(scalarFormat==VK_FORMAT_R32_SINT)format=VK_FORMAT_R32G32B32A32_SINT;break;
            case 24:if(scalarFormat==VK_FORMAT_R32_SINT)format=VK_FORMAT_R32_SINT;break;
            case 30:if(scalarFormat==VK_FORMAT_R32_UINT)format=VK_FORMAT_R32G32B32A32_UINT;break;
            case 33:if(scalarFormat==VK_FORMAT_R32_UINT)format=VK_FORMAT_R32_UINT;break;
            default:break;
            }
            if(format==VK_FORMAT_UNDEFINED)continue;
        }
        auto [at,added]=out.emplace(std::make_pair(sets.at(id),bindings.at(id)),format);
        if(!added&&at->second!=format)throw std::invalid_argument("conflicting texel descriptor types");
    }
    return out;
}
inline TexelShaderFormats reflectTexelFormats(const std::vector<uint32_t>& code){return reflectImageFormats(code,true);}
inline TexelShaderFormats reflectColor2DFormats(const std::vector<uint32_t>& code){return reflectImageFormats(code,false);}
inline TexelShaderFormats reflectColor1DFormats(const std::vector<uint32_t>& code){return reflectImageFormats(code,false,0);}
struct ColorImageShaderType {VkFormat format;VkImageViewType viewType;};
using ColorImageShaderTypes=std::map<std::pair<uint32_t,uint32_t>,ColorImageShaderType>;
inline ColorImageShaderTypes reflectColorImageTypes(const std::vector<uint32_t>& code){
    ColorImageShaderTypes out;
    for(uint32_t dimension=0;dimension<4;dimension++){
      for(uint32_t arrayed=0;arrayed<2;arrayed++){
        if(dimension>=2&&arrayed)continue; // No 3D-array; cube-array not yet admitted.
        const auto view=dimension==3?VK_IMAGE_VIEW_TYPE_CUBE:dimension==2?VK_IMAGE_VIEW_TYPE_3D:dimension?(arrayed?VK_IMAGE_VIEW_TYPE_2D_ARRAY:VK_IMAGE_VIEW_TYPE_2D):(arrayed?VK_IMAGE_VIEW_TYPE_1D_ARRAY:VK_IMAGE_VIEW_TYPE_1D);
        for(auto [key,format]:reflectImageFormats(code,false,dimension,nullptr,arrayed))if(!out.emplace(key,ColorImageShaderType{format,view}).second)throw std::invalid_argument("conflicting color image shapes");
      }
    }
    return out;
}
}
