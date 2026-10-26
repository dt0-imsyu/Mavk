#include <mavk/compat/shader_guards.hpp>
#include <spirv-tools/optimizer.hpp>
#include "source/opt/pass.h"
#include "source/opt/ir_builder.h"
#include "aggregate_buffers.hpp"
#include "texel_atomics.hpp"
#include "texel_components.hpp"
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <unordered_map>

namespace mavk::compat {
namespace {
class ReadOnlyGuardMetadata final : public spvtools::opt::Pass {
    uint32_t first_,set_;
public:
    ReadOnlyGuardMetadata(uint32_t first,uint32_t set):first_(first),set_(set){}
    const char* name()const override{return "mavk-readonly-guard-metadata";}
    Status Process()override{
        auto* decorations=get_decoration_mgr();bool changed=false;
        for(auto& variable:get_module()->types_values())if(variable.opcode()==SpvOpVariable&&variable.result_id()>=first_){
            bool set=false,binding=false;
            decorations->ForEachDecoration(variable.result_id(),SpvDecorationDescriptorSet,[&](const auto& d){set|=d.GetSingleWordInOperand(2)==set_;});
            decorations->ForEachDecoration(variable.result_id(),SpvDecorationBinding,[&](const auto& d){binding|=d.GetSingleWordInOperand(2)==1;});
            if(!set||!binding)continue;
            if(variable.GetSingleWordInOperand(0)!=SpvStorageClassStorageBuffer)throw std::runtime_error("guard metadata storage class");
            decorations->AddDecoration(variable.result_id(),SpvDecorationNonWritable);changed=true;
        }
        return changed?Status::SuccessWithChange:Status::SuccessWithoutChange;
    }
};
class SaturateGuardOffsets final : public spvtools::opt::Pass {
    uint32_t firstGeneratedId_;
    std::unordered_map<uint32_t,uint32_t> replacements_;
    uint32_t checked(uint32_t id){
        if(id<firstGeneratedId_)return id;
        if(auto found=replacements_.find(id);found!=replacements_.end())return found->second;
        auto* inst=get_def_use_mgr()->GetDef(id);
        if(!inst)return id;
        auto* types=context()->get_type_mgr();const auto type=types->GetUIntTypeId();
        if(inst->opcode()==SpvOpUConvert||inst->opcode()==SpvOpSConvert){
            auto* target=types->GetType(inst->type_id())->AsInteger();
            auto source=inst->GetSingleWordInOperand(0);auto* sourceInst=get_def_use_mgr()->GetDef(source);
            auto* sourceType=types->GetType(sourceInst->type_id())->AsInteger();
            if(!target||target->width()!=32||!sourceType||sourceType->width()<=32)return id;
            spvtools::opt::InstructionBuilder builder(context(),inst,
                spvtools::opt::IRContext::kAnalysisDefUse|spvtools::opt::IRContext::kAnalysisInstrToBlockMapping);
            spvtools::opt::analysis::Integer unsignedWide(sourceType->width(),false);
            const auto wideType=types->GetTypeInstruction(&unsignedWide);
            const auto bits=sourceType->IsSigned()?builder.AddUnaryOp(wideType,SpvOpBitcast,source)->result_id():source;
            const auto low=builder.AddUnaryOp(type,SpvOpUConvert,bits)->result_id();
            const auto restored=builder.AddUnaryOp(wideType,SpvOpUConvert,low)->result_id();
            const auto lost=builder.AddBinaryOp(types->GetBoolTypeId(),SpvOpINotEqual,bits,restored)->result_id();
            const auto result=builder.AddTernaryOp(type,SpvOpSelect,lost,builder.GetUintConstantId(UINT32_MAX),low)->result_id();
            replacements_[id]=result;return result;
        }
        if(inst->opcode()!=SpvOpIAdd&&inst->opcode()!=SpvOpIMul)return id;
        if(inst->type_id()!=type)return id;
        const auto originalLeft=checked(inst->GetSingleWordInOperand(0)),originalRight=checked(inst->GetSingleWordInOperand(1));
        spvtools::opt::InstructionBuilder builder(context(),inst,
            spvtools::opt::IRContext::kAnalysisDefUse|spvtools::opt::IRContext::kAnalysisInstrToBlockMapping);
        // Integer offset arithmetic may legally combine signedness,whereas
        // UMulExtended requires exact unsigned member types. Preserve bits.
        auto unsigned32=[&](uint32_t operand){
            auto* definition=get_def_use_mgr()->GetDef(operand);
            if(definition->type_id()==type)return operand;
            auto* integer=types->GetType(definition->type_id())->AsInteger();
            if(!integer||integer->width()!=32)throw std::invalid_argument("generated guard offset operand width unsupported");
            return builder.AddUnaryOp(type,SpvOpBitcast,operand)->result_id();
        };
        const auto left=unsigned32(originalLeft),right=unsigned32(originalRight);
        uint32_t low,overflow;
        if(inst->opcode()==SpvOpIMul){
            spvtools::opt::analysis::Struct pair({types->GetUIntType(),types->GetUIntType()});
            auto product=builder.AddBinaryOp(types->GetTypeInstruction(&pair),SpvOpUMulExtended,left,right)->result_id();
            low=builder.AddCompositeExtract(type,product,{0})->result_id();
            auto high=builder.AddCompositeExtract(type,product,{1})->result_id();
            overflow=builder.AddBinaryOp(types->GetBoolTypeId(),SpvOpINotEqual,high,builder.GetUintConstantId(0))->result_id();
        }else{
            low=builder.AddBinaryOp(type,SpvOpIAdd,left,right)->result_id();
            overflow=builder.AddBinaryOp(types->GetBoolTypeId(),SpvOpULessThan,low,left)->result_id();
        }
        auto result=builder.AddTernaryOp(type,SpvOpSelect,overflow,builder.GetUintConstantId(UINT32_MAX),low)->result_id();
        replacements_[id]=result;return result;
    }
public:
    explicit SaturateGuardOffsets(uint32_t bound):firstGeneratedId_(bound){}
    const char* name() const override{return "mavk-saturate-guard-offsets";}
    Status Process() override{
        std::unordered_set<uint32_t> readers;
        for(auto& inst:get_module()->debugs2())if(inst.opcode()==SpvOpName&&inst.GetSingleWordInOperand(0)>=firstGeneratedId_){
            const auto name=inst.GetInOperand(1).AsString();
            if(name=="inst_bindless_direct_read_4")readers.insert(inst.GetSingleWordInOperand(0));
        }
        std::vector<spvtools::opt::Instruction*> checks;
        get_module()->ForEachInst([&](spvtools::opt::Instruction* inst){
            if(inst->opcode()!=SpvOpULessThan||inst->result_id()<firstGeneratedId_)return;
            auto* limit=get_def_use_mgr()->GetDef(inst->GetSingleWordInOperand(1));
            if(limit&&limit->opcode()==SpvOpFunctionCall&&readers.count(limit->GetSingleWordInOperand(0)))checks.push_back(inst);
        });
        bool changed=false;
        for(auto* inst:checks){const auto old=inst->GetSingleWordInOperand(0),replacement=checked(old);if(old==replacement)continue;
            inst->SetInOperand(0,{replacement});context()->UpdateDefUse(inst);changed=true;}
        return changed?Status::SuccessWithChange:Status::SuccessWithoutChange;
    }
};
class OmitGuardReports final : public spvtools::opt::Pass {
    uint32_t firstGeneratedId_;
public:
    explicit OmitGuardReports(uint32_t bound):firstGeneratedId_(bound){}
    const char* name() const override{return "mavk-omit-guard-reports";}
    Status Process() override{
        std::unordered_set<uint32_t> reports;
        for(auto& inst:get_module()->debugs2())if(inst.opcode()==SpvOpName&&inst.GetSingleWordInOperand(0)>=firstGeneratedId_){
            const auto name=inst.GetInOperand(1).AsString();
            if(name.compare(0,27,"inst_bindless_stream_write_")==0)reports.insert(inst.GetSingleWordInOperand(0));
        }
        std::vector<spvtools::opt::Instruction*> calls;
        get_module()->ForEachInst([&](spvtools::opt::Instruction* inst){
            if(inst->opcode()==SpvOpFunctionCall&&reports.count(inst->GetSingleWordInOperand(0)))calls.push_back(inst);
        });
        for(auto* call:calls)context()->KillInst(call);
        return calls.empty()?Status::SuccessWithoutChange:Status::SuccessWithChange;
    }
};
}
GuardCompiler::GuardCompiler(size_t entries,size_t bytes,GuardCompilerOptions options):maxEntries_(entries),maxBytes_(bytes),publicSets_(options.publicSetCount),target_(options.target),reportErrors_(options.reportErrors),texelFormatMetadata_(options.texelFormatMetadata),uniformBufferStandardLayout_(options.uniformBufferStandardLayout),scalarBlockLayout_(options.scalarBlockLayout),metadataReadOnly_(options.metadataReadOnly),optimizeMetadataReads_(options.optimizeMetadataReads){
    if(!entries||entries>256||bytes<1024||bytes>64*1024*1024||!publicSets_||publicSets_>7)throw std::invalid_argument("guard cache/set limits");
    if(target_!=GuardTarget::Vulkan11&&target_!=GuardTarget::Vulkan12&&target_!=GuardTarget::Vulkan13)throw std::invalid_argument("unsupported guard target environment");
}
GuardCompilerStats GuardCompiler::stats() const{return {compiled_,hits_,cache_.size(),retained_};}
std::shared_ptr<const GuardCompiler::Words> GuardCompiler::compile(const Words& code){
    if(code.size()<5||code.size()>1024*1024/4||code[0]!=0x07230203)throw std::invalid_argument("bounded SPIR-V module required");
    for(auto at=cache_.begin();at!=cache_.end();++at)if(at->original==code){auto result=at->guarded;cache_.splice(cache_.begin(),cache_,at);hits_++;return result;}
    bool decorationGroups=false;
    for(size_t p=5;p<code.size();){
        auto count=code[p]>>16,op=code[p]&65535;if(!count||p+count>code.size())throw std::invalid_argument("malformed SPIR-V instruction");
        if(op==71&&count==4&&code[p+2]==34&&code[p+3]>=publicSets_)throw std::invalid_argument("descriptor set outside public range or private metadata collision");
        if(op==67||op==70||op==120||op==318||op==319||op==5614||op==5615||op==6035)throw std::invalid_argument("unsupported pointer/image/flag/float-atomic access form");
        decorationGroups|=op==73||op==74||op==75;
        p+=count;
    }
    std::string diagnostics;
    auto consumer=[&](spv_message_level_t,const char*,const spv_position_t&,const char* text){if(diagnostics.size()<4096)diagnostics.append(text,0,4096-diagnostics.size());};
    const auto environment=target_==GuardTarget::Vulkan13?SPV_ENV_VULKAN_1_3:target_==GuardTarget::Vulkan12?SPV_ENV_VULKAN_1_2:SPV_ENV_VULKAN_1_1;
    spvtools::SpirvTools validator(environment);validator.SetMessageConsumer(consumer);
    spvtools::ValidatorOptions validation;validation.SetUniformBufferStandardLayout(uniformBufferStandardLayout_);validation.SetScalarBlockLayout(scalarBlockLayout_);
    if(!validator.Validate(code.data(),code.size(),validation))throw std::invalid_argument("input SPIR-V invalid: "+diagnostics);
    spvtools::Optimizer optimizer(environment);optimizer.SetMessageConsumer(consumer);
    // The bindless pass reads direct binding decorations. Normalize valid
    // groups before instrumentation rather than silently guarding wrong slots.
    if(decorationGroups){
        // Pinned SPIRV-Tools v2022.4 flattening loops on original OpMemberName
        // debug records. Remove only source debug info before flattening;
        // instrumentation's generated names are added afterwards.
        optimizer.RegisterPass(spvtools::CreateStripDebugInfoPass());
        optimizer.RegisterPass(spvtools::CreateFlattenDecorationPass());
    }
    // The bounds pass recognizes buffer pointers rooted directly at a global
    // variable. Canonicalize valid nested chains before it inspects accesses.
    optimizer.RegisterPass(spvtools::Optimizer::PassToken(std::make_unique<detail::ValidateTexelAtomics>()));
    optimizer.RegisterPass(spvtools::Optimizer::PassToken(std::make_unique<detail::AggregateBuffers>()));
    optimizer.RegisterPass(spvtools::CreateCombineAccessChainsPass());
    optimizer.RegisterPass(spvtools::CreateInstBindlessCheckPass(publicSets_,23,true,true,true,true));
    if(texelFormatMetadata_)optimizer.RegisterPass(spvtools::Optimizer::PassToken(std::make_unique<detail::TexelComponents>(publicSets_)));
    if(metadataReadOnly_||optimizeMetadataReads_)optimizer.RegisterPass(spvtools::Optimizer::PassToken(std::make_unique<ReadOnlyGuardMetadata>(code[3],publicSets_)));
    // Only generated offset expressions feeding metadata bounds checks change;
    // application arithmetic and metadata-table indexing retain their semantics.
    optimizer.RegisterPass(spvtools::Optimizer::PassToken(std::make_unique<SaturateGuardOffsets>(code[3])));
    if(!reportErrors_){
        optimizer.RegisterPass(spvtools::Optimizer::PassToken(std::make_unique<OmitGuardReports>(code[3])));
        optimizer.RegisterPass(spvtools::CreateEliminateDeadFunctionsPass());
    }
    optimizer.RegisterPass(spvtools::CreateSimplificationPass());
    optimizer.RegisterPass(spvtools::CreateDeadBranchElimPass());
    optimizer.RegisterPass(spvtools::CreateBlockMergePass());
    if(!reportErrors_)optimizer.RegisterPass(spvtools::CreateRemoveUnusedInterfaceVariablesPass());
    optimizer.RegisterPass(spvtools::CreateAggressiveDCEPass(true));
    if(optimizeMetadataReads_){
        optimizer.RegisterPass(spvtools::CreateInlineExhaustivePass());
        optimizer.RegisterPass(spvtools::CreateLocalRedundancyEliminationPass());
        optimizer.RegisterPass(spvtools::CreateRedundancyEliminationPass());
        optimizer.RegisterPass(spvtools::CreateSimplificationPass());
        optimizer.RegisterPass(spvtools::CreateDeadBranchElimPass());
        optimizer.RegisterPass(spvtools::CreateAggressiveDCEPass(true));
        optimizer.RegisterPass(spvtools::CreateEliminateDeadFunctionsPass());
    }
    auto output=std::make_shared<Words>();
    spvtools::OptimizerOptions optimization;optimization.set_validator_options(validation);
    if(!optimizer.Run(code.data(),code.size(),output.get(),optimization)||output->size()>4*1024*1024/4||!validator.Validate(output->data(),output->size(),validation))throw std::runtime_error("guard compilation/validation failed: "+diagnostics);
    if(!reportErrors_){
        std::unordered_set<uint32_t> privateVariables;
        for(size_t p=5;p<output->size();p+=(*output)[p]>>16)if(((*output)[p]&65535)==71&&((*output)[p]>>16)==4&&(*output)[p+2]==34&&(*output)[p+3]==publicSets_)privateVariables.insert((*output)[p+1]);
        for(size_t p=5;p<output->size();p+=(*output)[p]>>16)if(((*output)[p]&65535)==71&&((*output)[p]>>16)==4&&(*output)[p+2]==33&&(*output)[p+3]==0&&privateVariables.count((*output)[p+1]))throw std::runtime_error("guard diagnostic output survived read-only compilation");
    }
    compiled_++;size_t bytes=(code.size()+output->size())*sizeof(uint32_t);
    if(bytes<=maxBytes_){
        while(cache_.size()>=maxEntries_||retained_>maxBytes_-bytes){retained_-=cache_.back().bytes;cache_.pop_back();}
        cache_.push_front({code,output,bytes});retained_+=bytes;
    }
    return output;
}
}
