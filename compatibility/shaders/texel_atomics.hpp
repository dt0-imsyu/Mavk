#pragma once
#include "source/opt/pass.h"
#include <stdexcept>

namespace mavk::compat::detail {
// Admit only image-pointer forms understood by the extended bounds pass.
class ValidateTexelAtomics final : public spvtools::opt::Pass {
public:
    const char* name() const override{return "mavk-validate-texel-atomics";}
    Status Process() override{
        auto unsupported=[](){throw std::invalid_argument("unsupported image atomic pointer/dimension/format/provenance");};
        get_module()->ForEachInst([&](spvtools::opt::Instruction* inst){
            if(inst->opcode()!=SpvOpImageTexelPointer)return;
            auto* pointer=get_def_use_mgr()->GetDef(inst->GetSingleWordInOperand(0));
            auto* type=get_def_use_mgr()->GetDef(pointer->type_id());
            if(type->opcode()!=SpvOpTypePointer||type->GetSingleWordInOperand(0)!=SpvStorageClassUniformConstant)unsupported();
            auto* image=get_def_use_mgr()->GetDef(type->GetSingleWordInOperand(1));
            if(image->opcode()!=SpvOpTypeImage||image->GetSingleWordInOperand(1)!=SpvDimBuffer||
               image->GetSingleWordInOperand(2)||image->GetSingleWordInOperand(3)||image->GetSingleWordInOperand(4)||
               image->GetSingleWordInOperand(5)!=2||
               (image->GetSingleWordInOperand(6)!=SpvImageFormatR32ui&&image->GetSingleWordInOperand(6)!=SpvImageFormatR32i))unsupported();
            auto* scalar=get_def_use_mgr()->GetDef(image->GetSingleWordInOperand(0));
            auto* coordinate=get_def_use_mgr()->GetDef(inst->GetSingleWordInOperand(1));
            auto* coordType=get_def_use_mgr()->GetDef(coordinate->type_id());
            if(scalar->opcode()!=SpvOpTypeInt||scalar->GetSingleWordInOperand(0)!=32||
               coordType->opcode()!=SpvOpTypeInt||coordType->GetSingleWordInOperand(0)!=32)unsupported();
            auto* root=pointer;
            if(root->opcode()==SpvOpAccessChain&&root->NumInOperands()==2)
                root=get_def_use_mgr()->GetDef(root->GetSingleWordInOperand(0));
            if(root->opcode()!=SpvOpVariable||root->GetSingleWordInOperand(0)!=SpvStorageClassUniformConstant)unsupported();
            if(!get_decoration_mgr()->HasDecoration(root->result_id(),SpvDecorationBinding)||
               !get_decoration_mgr()->HasDecoration(root->result_id(),SpvDecorationDescriptorSet))unsupported();
            get_def_use_mgr()->ForEachUser(inst->result_id(),[&](spvtools::opt::Instruction* user){
                const auto op=uint32_t(user->opcode());
                if(op==SpvOpName||op==SpvOpDecorate||op==SpvOpDecorateId)return;
                if(op<227||op>242||!user->NumInOperands()||user->GetSingleWordInOperand(0)!=inst->result_id())unsupported();
            });
        });
        return Status::SuccessWithoutChange;
    }
};
}
