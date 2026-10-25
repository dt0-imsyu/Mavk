// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <spirv-tools/libspirv.hpp>
#include <spirv/unified1/spirv.h>
#include <unordered_map>
#include <set>
#include <stdexcept>
#include <mavk/compat/shader_guards.hpp>

namespace mavk::compat {
inline std::vector<uint32_t> uniformGuardMetadata(const std::vector<uint32_t>& code,uint32_t set,uint32_t words,GuardTarget target,bool standardLayout){
    if(!words||words>1024||code.size()<5||code[3]>UINT32_MAX-8-code.size()/3)throw std::invalid_argument("uniform guard metadata budget");
    std::unordered_map<uint32_t,size_t> definitions;
    std::set<uint32_t> guardSets,metadataBindings,zeroes;
    size_t firstType=0;
    for(size_t p=5;p<code.size();p+=code[p]>>16){
        const auto n=code[p]>>16,op=code[p]&65535;
        if(!n||p+n>code.size())throw std::invalid_argument("uniform metadata SPIR-V shape");
        if(op>=SpvOpTypeVoid&&op<=SpvOpTypeForwardPointer){definitions[code[p+1]]=p;if(!firstType)firstType=p;}
        if(op==SpvOpConstant&&n==4){definitions[code[p+2]]=p;if(!code[p+3])zeroes.insert(code[p+2]);}
        if(op==SpvOpConstantNull&&n==3)zeroes.insert(code[p+2]);
        if(op==SpvOpVariable)definitions[code[p+2]]=p;
        if(op==SpvOpDecorate&&n==4){
            if(code[p+2]==SpvDecorationDescriptorSet&&code[p+3]==set)guardSets.insert(code[p+1]);
            if(code[p+2]==SpvDecorationBinding&&code[p+3]==1)metadataBindings.insert(code[p+1]);
        }
    }
    uint32_t variable=0;
    for(auto id:guardSets)if(metadataBindings.count(id)){if(variable)throw std::invalid_argument("multiple metadata inputs");variable=id;}
    if(!variable)return code;
    const auto at=definitions.at(variable);
    const auto pointer=definitions.at(code[at+1]);
    if((code[at]>>16)!=4||code[at+3]!=SpvStorageClassStorageBuffer||(code[pointer]&65535)!=SpvOpTypePointer)throw std::invalid_argument("metadata variable storage");
    const auto block=definitions.at(code[pointer+3]);
    if((code[block]&65535)!=SpvOpTypeStruct||(code[block]>>16)!=3)throw std::invalid_argument("metadata block shape");
    const auto array=definitions.at(code[block+2]);
    if((code[array]&65535)!=SpvOpTypeRuntimeArray)throw std::invalid_argument("metadata array shape");
    const uint32_t uintType=code[array+2];const auto integer=definitions.at(uintType);
    if((code[integer]&65535)!=SpvOpTypeInt||code[integer+2]!=32||code[integer+3])throw std::invalid_argument("metadata scalar type");
    // std140 uvec4 array packs four ABI words into each 16-byte slot.
    // Preserve the existing word-index ABI by splitting index into vector/lane.
    uint32_t vectorType=0;
    for(const auto& entry:definitions){const auto p=entry.second;if((code[p]&65535)==SpvOpTypeVector&&(code[p]>>16)==4&&code[p+2]==uintType&&code[p+3]==4)vectorType=entry.first;}
    const bool newVector=!vectorType;
    uint32_t next=code[3];const auto length=next++,arrayType=next++,blockType=next++,blockPointer=next++,elementPointer=next++,shift=next++,mask=next++;
    if(newVector)vectorType=next++;
    std::vector<uint32_t> out(code.begin(),code.begin()+5);out[3]=next;
    const auto emit=[&](SpvOp op,std::initializer_list<uint32_t> operands){out.push_back((uint32_t(operands.size()+1)<<16)|uint32_t(op));out.insert(out.end(),operands.begin(),operands.end());};
    for(size_t p=5;p<code.size();p+=code[p]>>16){
        if(p==firstType){emit(SpvOpDecorate,{arrayType,SpvDecorationArrayStride,16});emit(SpvOpDecorate,{blockType,SpvDecorationBlock});emit(SpvOpMemberDecorate,{blockType,0,SpvDecorationOffset,0});}
        if(p==at){emit(SpvOpConstant,{uintType,length,(words+3)/4});emit(SpvOpConstant,{uintType,shift,2});emit(SpvOpConstant,{uintType,mask,3});if(newVector)emit(SpvOpTypeVector,{vectorType,uintType,4});emit(SpvOpTypeArray,{arrayType,vectorType,length});emit(SpvOpTypeStruct,{blockType,arrayType});emit(SpvOpTypePointer,{blockPointer,SpvStorageClassUniform,blockType});emit(SpvOpTypePointer,{elementPointer,SpvStorageClassUniform,uintType});}
        const auto n=code[p]>>16,op=code[p]&65535;const auto start=out.size();out.insert(out.end(),code.begin()+p,code.begin()+p+n);
        if(p==at){out[start+1]=blockPointer;out[start+3]=SpvStorageClassUniform;}
        if((op==SpvOpAccessChain||op==SpvOpInBoundsAccessChain)&&n>=4&&code[p+3]==variable){
            const auto type=definitions.at(code[p+1]);
            if(n!=6||!zeroes.count(code[p+4])||(code[type]&65535)!=SpvOpTypePointer||code[type+3]!=uintType)throw std::invalid_argument("unsupported metadata access path");
            out.resize(start);const auto vectorIndex=next++,laneIndex=next++;
            emit(SpvOpShiftRightLogical,{uintType,vectorIndex,code[p+5],shift});
            emit(SpvOpBitwiseAnd,{uintType,laneIndex,code[p+5],mask});
            emit(SpvOp(op),{elementPointer,code[p+2],variable,code[p+4],vectorIndex,laneIndex});
        }
    }
    out[3]=next;
    const auto environment=target==GuardTarget::Vulkan13?SPV_ENV_VULKAN_1_3:target==GuardTarget::Vulkan12?SPV_ENV_VULKAN_1_2:SPV_ENV_VULKAN_1_1;
    std::string error;spvtools::SpirvTools validator(environment);validator.SetMessageConsumer([&](spv_message_level_t,const char*,const spv_position_t&,const char* text){if(error.size()<2048)error+=text;});
    spvtools::ValidatorOptions options;options.SetUniformBufferStandardLayout(standardLayout);
    if(!validator.Validate(out.data(),out.size(),options))throw std::invalid_argument("uniform metadata validation: "+error);
    return out;
}
}
