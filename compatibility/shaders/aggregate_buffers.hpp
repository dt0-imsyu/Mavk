#pragma once
#include "source/opt/pass.h"
#include "source/opt/ir_builder.h"
#include <stdexcept>

namespace mavk::compat::detail {
// Normalize buffer-pointer aliases and lower whole buffer composites so the
// stride/offset-aware pass checks references,rather than initialization alone.
class AggregateBuffers final : public spvtools::opt::Pass {
    using Inst=spvtools::opt::Instruction;
    using Builder=spvtools::opt::InstructionBuilder;
    size_t leaves_=0;
    static bool composite(SpvOp op){return op==SpvOpTypeStruct||op==SpvOpTypeArray||op==SpvOpTypeMatrix;}
    void memoryOperands(Inst* target,const Inst& original){
        const auto start=original.opcode()==SpvOpLoad?1u:2u;
        if(original.NumInOperands()==start)return;
        const auto mask=original.GetSingleWordInOperand(start);
        if(mask&~7u||original.NumInOperands()!=start+1+((mask&2u)?1u:0u))
            throw std::invalid_argument("aggregate buffer memory scope/operand form unsupported");
        for(uint32_t i=start;i<original.NumInOperands();i++){
            auto operand=original.GetInOperand(i);
            if(i==start+1&&(mask&2u))operand.words={1};
            target->AddOperand(std::move(operand));
        }
        context()->UpdateDefUse(target);
    }
    uint32_t lower(Builder& builder,const Inst& original,uint32_t pointer,uint32_t type,
                   uint32_t storage,uint32_t value,unsigned depth){
        if(depth>32)throw std::invalid_argument("aggregate buffer nesting budget");
        auto* declaration=get_def_use_mgr()->GetDef(type);
        const auto op=declaration->opcode();
        if(!composite(op)){
            if(op!=SpvOpTypeInt&&op!=SpvOpTypeFloat&&op!=SpvOpTypeVector)
                throw std::invalid_argument("aggregate buffer leaf type unsupported");
            if(++leaves_>1024)throw std::invalid_argument("aggregate buffer instruction budget");
            auto* access=value?builder.AddStore(pointer,value):builder.AddLoad(type,pointer);
            if(!value&&(get_decoration_mgr()->HasDecoration(pointer,SpvDecorationNonUniformEXT)||get_decoration_mgr()->HasDecoration(original.result_id(),SpvDecorationNonUniformEXT)))
                get_decoration_mgr()->AddDecoration(access->result_id(),SpvDecorationNonUniformEXT);
            memoryOperands(access,original);return access->result_id();
        }
        uint32_t count;
        if(op==SpvOpTypeStruct)count=declaration->NumInOperands();
        else if(op==SpvOpTypeMatrix)count=declaration->GetSingleWordInOperand(1);
        else{
            auto* length=get_def_use_mgr()->GetDef(declaration->GetSingleWordInOperand(1));
            if(length->opcode()!=SpvOpConstant||length->NumInOperands()!=1)
                throw std::invalid_argument("aggregate buffer array length unsupported");
            count=length->GetSingleWordInOperand(0);
        }
        if(count>1024)throw std::invalid_argument("aggregate buffer member budget");
        std::vector<uint32_t> members;
        for(uint32_t i=0;i<count;i++){
            const auto child=declaration->GetSingleWordInOperand(op==SpvOpTypeStruct?i:0);
            auto* types=context()->get_type_mgr();
            spvtools::opt::analysis::Pointer childPointer(types->GetType(child),static_cast<SpvStorageClass>(storage));
            auto address=builder.AddAccessChain(types->GetTypeInstruction(&childPointer),pointer,{builder.GetUintConstantId(i)})->result_id();
            if(get_decoration_mgr()->HasDecoration(pointer,SpvDecorationNonUniformEXT))
                get_decoration_mgr()->AddDecoration(address,SpvDecorationNonUniformEXT);
            auto element=value?builder.AddCompositeExtract(child,value,{i})->result_id():0;
            auto result=lower(builder,original,address,child,storage,element,depth+1);
            if(!value)members.push_back(result);
        }
        return value?0:builder.AddCompositeConstruct(type,members)->result_id();
    }
public:
    const char* name() const override{return "mavk-lower-buffer-aggregates";}
    Status Process() override{
        // The pinned bounds pass recognizes AccessChain roots,not pointer
        // aliases. Remove only identical buffer-pointer CopyObjects before
        // aggregate lowering/chain combination;leave application value math.
        std::vector<Inst*> aliases;
        get_module()->ForEachInst([&](Inst* inst){
            if(inst->opcode()!=SpvOpCopyObject)return;
            auto* type=get_def_use_mgr()->GetDef(inst->type_id());
            if(type->opcode()!=SpvOpTypePointer)return;
            const auto storage=type->GetSingleWordInOperand(0);
            if(storage==SpvStorageClassStorageBuffer||storage==SpvStorageClassUniform)aliases.push_back(inst);
        });
        for(auto* alias:aliases){
            const auto source=alias->GetSingleWordInOperand(0);
            if(get_decoration_mgr()->HasDecoration(alias->result_id(),SpvDecorationNonUniformEXT)&&
               !get_decoration_mgr()->HasDecoration(source,SpvDecorationNonUniformEXT))
                get_decoration_mgr()->AddDecoration(source,SpvDecorationNonUniformEXT);
            context()->KillNamesAndDecorates(alias->result_id());
            context()->ReplaceAllUsesWith(alias->result_id(),source);
            context()->KillInst(alias);
        }
        std::vector<Inst*> copies;
        get_module()->ForEachInst([&](Inst* inst){
            if(inst->opcode()!=SpvOpCopyMemory)return;
            for(uint32_t i=0;i<2;i++){
                auto* pointer=get_def_use_mgr()->GetDef(inst->GetSingleWordInOperand(i));
                auto* type=get_def_use_mgr()->GetDef(pointer->type_id());
                if(type->opcode()==SpvOpTypePointer&&(type->GetSingleWordInOperand(0)==SpvStorageClassStorageBuffer||type->GetSingleWordInOperand(0)==SpvStorageClassUniform)){
                    copies.push_back(inst);return;
                }
            }
        });
        for(auto* inst:copies){
            uint32_t cursor=2;
            auto access=[&](){
                Inst::OperandList operands;
                if(cursor==inst->NumInOperands())return operands;
                const auto mask=inst->GetSingleWordInOperand(cursor);
                const auto count=1u+((mask&2u)?1u:0u);
                if(mask&~7u||count>inst->NumInOperands()-cursor)
                    throw std::invalid_argument("buffer CopyMemory scoped/unknown memory operands unsupported");
                for(uint32_t i=0;i<count;i++)operands.push_back(inst->GetInOperand(cursor++));
                return operands;
            };
            const auto targetAccess=access();
            const auto sourceAccess=cursor==inst->NumInOperands()?targetAccess:access();
            if(cursor!=inst->NumInOperands())throw std::invalid_argument("buffer CopyMemory trailing memory operands");
            auto destination=inst->GetSingleWordInOperand(0),source=inst->GetSingleWordInOperand(1);
            auto* type=get_def_use_mgr()->GetDef(get_def_use_mgr()->GetDef(source)->type_id());
            Builder builder(context(),inst,spvtools::opt::IRContext::kAnalysisDefUse|spvtools::opt::IRContext::kAnalysisInstrToBlockMapping);
            auto* value=builder.AddLoad(type->GetSingleWordInOperand(1),source);
            for(auto operand:sourceAccess)value->AddOperand(std::move(operand));context()->UpdateDefUse(value);
            if(get_decoration_mgr()->HasDecoration(source,SpvDecorationNonUniformEXT))
                get_decoration_mgr()->AddDecoration(value->result_id(),SpvDecorationNonUniformEXT);
            auto* store=builder.AddStore(destination,value->result_id());
            for(auto operand:targetAccess)store->AddOperand(std::move(operand));context()->UpdateDefUse(store);
            context()->KillInst(inst);
        }
        std::vector<Inst*> work;
        get_module()->ForEachInst([&](Inst* inst){
            if(inst->opcode()!=SpvOpLoad&&inst->opcode()!=SpvOpStore)return;
            auto* pointer=get_def_use_mgr()->GetDef(inst->GetSingleWordInOperand(0));
            auto* pointerType=get_def_use_mgr()->GetDef(pointer->type_id());
            if(pointerType->opcode()!=SpvOpTypePointer)return;
            const auto storage=pointerType->GetSingleWordInOperand(0);
            if(storage!=SpvStorageClassStorageBuffer&&storage!=SpvStorageClassUniform)return;
            auto* type=get_def_use_mgr()->GetDef(pointerType->GetSingleWordInOperand(1));
            if(!composite(type->opcode()))return;
            auto* base=pointer;
            while(base->opcode()==SpvOpAccessChain||base->opcode()==SpvOpInBoundsAccessChain)
                base=get_def_use_mgr()->GetDef(base->GetSingleWordInOperand(0));
            if(base->opcode()==SpvOpVariable)work.push_back(inst);
        });
        for(auto* inst:work){
            leaves_=0;auto pointer=inst->GetSingleWordInOperand(0);
            auto* type=get_def_use_mgr()->GetDef(get_def_use_mgr()->GetDef(pointer)->type_id());
            Builder builder(context(),inst,spvtools::opt::IRContext::kAnalysisDefUse|spvtools::opt::IRContext::kAnalysisInstrToBlockMapping);
            auto result=lower(builder,*inst,pointer,type->GetSingleWordInOperand(1),type->GetSingleWordInOperand(0),inst->opcode()==SpvOpStore?inst->GetSingleWordInOperand(1):0,0);
            if(inst->opcode()==SpvOpLoad)context()->ReplaceAllUsesWith(inst->result_id(),result);
            context()->KillInst(inst);
        }
        return work.empty()&&copies.empty()&&aliases.empty()?Status::SuccessWithoutChange:Status::SuccessWithChange;
    }
};
}
