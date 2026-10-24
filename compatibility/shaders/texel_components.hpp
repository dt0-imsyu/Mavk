#pragma once
#include "source/opt/pass.h"
#include "source/opt/ir_builder.h"
#include <map>
#include <set>
#include <stdexcept>

namespace mavk::compat::detail {
// IndexInitFormats adds per-view component counts without changing range units.
class TexelComponents final : public spvtools::opt::Pass {
    using Inst=spvtools::opt::Instruction;
    uint32_t privateSet_;
    uint32_t decoration(uint32_t id,SpvDecoration kind){
        for(auto& d:get_module()->annotations())if(d.opcode()==SpvOpDecorate&&d.GetSingleWordInOperand(0)==id&&d.GetSingleWordInOperand(1)==uint32_t(kind))return d.GetSingleWordInOperand(2);
        throw std::invalid_argument("texel format metadata descriptor decoration missing");
    }
public:
    explicit TexelComponents(uint32_t privateSet):privateSet_(privateSet){}
    const char* name() const override{return "mavk-texel-component-metadata";}
    Status Process() override{
        std::map<uint32_t,Inst*> references;
        get_module()->ForEachInst([&](Inst* inst){
            if(inst->opcode()!=SpvOpImageFetch&&inst->opcode()!=SpvOpImageRead)return;
            auto* image=get_def_use_mgr()->GetDef(inst->GetSingleWordInOperand(0));
            auto* type=get_def_use_mgr()->GetDef(image->type_id());
            if(type->opcode()==SpvOpTypeImage&&type->GetSingleWordInOperand(1)==SpvDimBuffer&&type->GetSingleWordInOperand(6)==SpvImageFormatUnknown)references.emplace(inst->result_id(),inst);
        });
        if(references.empty())return Status::SuccessWithoutChange;
        uint32_t input=0;
        for(auto& d:get_module()->annotations())if(d.opcode()==SpvOpDecorate&&d.GetSingleWordInOperand(1)==SpvDecorationDescriptorSet&&d.GetSingleWordInOperand(2)==privateSet_){auto id=d.GetSingleWordInOperand(0);if(decoration(id,SpvDecorationBinding)==1)input=id;}
        if(!input)throw std::invalid_argument("texel format metadata input missing");
        std::vector<Inst*> phis;get_module()->ForEachInst([&](Inst* inst){if(inst->opcode()==SpvOpPhi)phis.push_back(inst);});
        std::set<uint32_t> patched;
        for(auto* phi:phis)for(uint32_t operand=0;operand<phi->NumInOperands();operand+=2){
            auto found=references.find(phi->GetSingleWordInOperand(operand));if(found==references.end())continue;
            if(phi->NumInOperands()!=4)throw std::invalid_argument("texel format bounds merge shape");
            auto* ref=found->second;const auto valid=phi->GetSingleWordInOperand(operand+1),other=operand?0u:2u,invalid=phi->GetSingleWordInOperand(other+1);
            bool bounds=false;
            for(auto predecessor:context()->cfg()->preds(valid)){
                auto* branch=context()->cfg()->block(predecessor)->terminator();
                if(branch->opcode()!=SpvOpBranchConditional||branch->GetSingleWordInOperand(1)!=valid||branch->GetSingleWordInOperand(2)!=invalid)continue;
                auto* compare=get_def_use_mgr()->GetDef(branch->GetSingleWordInOperand(0));
                bounds=compare->opcode()==SpvOpULessThan&&get_def_use_mgr()->GetDef(compare->GetSingleWordInOperand(1))->opcode()==SpvOpImageQuerySize;
            }
            if(!bounds)throw std::invalid_argument("texel format substitution requires proven OOB branch");
            auto* object=get_def_use_mgr()->GetDef(ref->GetSingleWordInOperand(0));
            for(unsigned depth=0;depth<8&&(object->opcode()==SpvOpImage||object->opcode()==SpvOpSampledImage||object->opcode()==SpvOpCopyObject);depth++)object=get_def_use_mgr()->GetDef(object->GetSingleWordInOperand(0));
            if(object->opcode()!=SpvOpLoad)throw std::invalid_argument("texel format descriptor provenance");
            auto* pointer=get_def_use_mgr()->GetDef(object->GetSingleWordInOperand(0));uint32_t index=0;
            if(pointer->opcode()==SpvOpAccessChain&&pointer->NumInOperands()==2){index=pointer->GetSingleWordInOperand(1);pointer=get_def_use_mgr()->GetDef(pointer->GetSingleWordInOperand(0));}
            if(pointer->opcode()!=SpvOpVariable)throw std::invalid_argument("texel format descriptor pointer");
            auto set=decoration(pointer->result_id(),SpvDecorationDescriptorSet),binding=decoration(pointer->result_id(),SpvDecorationBinding);
            if(set>=privateSet_)throw std::invalid_argument("texel format private descriptor collision");
            auto* types=context()->get_type_mgr();auto* vector=types->GetType(ref->type_id())->AsVector();
            if(!vector||vector->element_count()!=4)throw std::invalid_argument("texel format result shape");
            const auto scalar=get_def_use_mgr()->GetDef(ref->type_id())->GetSingleWordInOperand(0);auto* scalarDecl=get_def_use_mgr()->GetDef(scalar);
            if(scalarDecl->GetSingleWordInOperand(0)!=32)throw std::invalid_argument("texel format component width");
            spvtools::opt::InstructionBuilder builder(context(),context()->cfg()->block(invalid)->terminator(),spvtools::opt::IRContext::kAnalysisDefUse|spvtools::opt::IRContext::kAnalysisInstrToBlockMapping);
            const auto uint=types->GetUIntTypeId(),zero=builder.GetUintConstantId(0);
            auto* inputType=get_def_use_mgr()->GetDef(get_def_use_mgr()->GetDef(input)->type_id());
            spvtools::opt::analysis::Pointer address(types->GetUIntType(),static_cast<SpvStorageClass>(inputType->GetSingleWordInOperand(0)));auto pointerType=types->GetTypeInstruction(&address);
            auto read=[&](uint32_t at){auto p=builder.AddAccessChain(pointerType,input,{zero,at})->result_id();return builder.AddLoad(uint,p)->result_id();};
            if(!index)index=zero;else if(get_def_use_mgr()->GetDef(index)->type_id()!=uint)index=builder.AddUnaryOp(uint,SpvOpBitcast,index)->result_id();
            auto base=read(zero);auto root=builder.AddBinaryOp(uint,SpvOpIAdd,base,base)->result_id();
            root=builder.AddBinaryOp(uint,SpvOpISub,root,builder.GetUintConstantId(1))->result_id();
            auto table=read(builder.AddBinaryOp(uint,SpvOpIAdd,root,builder.GetUintConstantId(set))->result_id());
            auto entries=read(builder.AddBinaryOp(uint,SpvOpIAdd,table,builder.GetUintConstantId(binding))->result_id());
            auto components=read(builder.AddBinaryOp(uint,SpvOpIAdd,entries,index)->result_id());
            auto* constants=context()->get_constant_mgr();auto zeroScalar=constants->GetDefiningInstruction(constants->GetConstant(vector->element_type(),{0u}))->result_id();
#ifdef MAVK_TEST_TEXEL_COMPONENT_PROBE
            const auto alphaBits=components;
#else
            // Bound views have 1..4 components; only four-component views carry alpha.
            auto present=builder.AddBinaryOp(uint,SpvOpShiftRightLogical,components,builder.GetUintConstantId(2))->result_id();
            const auto alphaBits=builder.AddBinaryOp(uint,SpvOpISub,builder.GetUintConstantId(1),present)->result_id();
#endif
            auto alpha=scalar==uint?alphaBits:builder.AddUnaryOp(scalar,scalarDecl->opcode()==SpvOpTypeFloat?SpvOpConvertUToF:SpvOpBitcast,alphaBits)->result_id();
            auto value=builder.AddCompositeConstruct(ref->type_id(),{zeroScalar,zeroScalar,zeroScalar,alpha})->result_id();
            phi->SetInOperand(other,{value});context()->UpdateDefUse(phi);patched.insert(ref->result_id());
        }
        if(patched.size()!=references.size())throw std::invalid_argument("texel format reference lacks bounds instrumentation");
        return Status::SuccessWithChange;
    }
};
}
