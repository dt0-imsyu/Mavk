#include <mavk/compat/shader_guards.hpp>
#include "source/opt/build_module.h"
#include "source/opt/decoration_manager.h"
#include <spirv-tools/optimizer.hpp>
#include <cstring>
#include <unordered_map>
#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <tuple>

namespace mavk::compat {
std::vector<uint32_t> specializeDescriptorShader(const std::vector<uint32_t>& code,
    const std::vector<SpecializationValue>& values,GuardTarget target,bool uniformLayout,bool scalarLayout){
    if(code.size()<5||code.size()>262144||values.size()>1024)throw std::invalid_argument("specialization budget");
    const auto env=target==GuardTarget::Vulkan13?SPV_ENV_VULKAN_1_3:target==GuardTarget::Vulkan12?SPV_ENV_VULKAN_1_2:SPV_ENV_VULKAN_1_1;
    spvtools::ValidatorOptions validation;validation.SetUniformBufferStandardLayout(uniformLayout);validation.SetScalarBlockLayout(scalarLayout);
    std::string diagnostic;
    auto consumer=[&](spv_message_level_t,const char*,const spv_position_t&,const char* message){if(diagnostic.size()<4096)diagnostic+=message;};
    spvtools::SpirvTools validator(env);validator.SetMessageConsumer(consumer);
    if(!validator.Validate(code.data(),code.size(),validation))throw std::invalid_argument("specialization input: "+diagnostic);
    std::map<uint32_t,uint32_t> widths,types,ids;
    for(size_t p=5;p<code.size();p+=code[p]>>16){
        auto n=code[p]>>16,op=code[p]&65535;
        if(op==20)widths[code[p+1]]=32;
        if(op==21||op==22)widths[code[p+1]]=code[p+2];
        if(op==48||op==49||op==50)types[code[p+2]]=code[p+1];
        if(op==71&&n==4&&code[p+2]==1)ids[code[p+3]]=code[p+1];
        if(op==74||op==75)throw std::invalid_argument("grouped specialization not admitted");
    }
    std::set<uint32_t> seen;std::unordered_map<uint32_t,std::vector<uint32_t>> replacements;
    for(const auto& value:values){
        if(!seen.insert(value.id).second)throw std::invalid_argument("duplicate specialization id");
        auto id=ids.find(value.id);if(id==ids.end())continue; // Vulkan ignores absent SpecIds.
        auto type=types.find(id->second);if(type==types.end()||!widths.count(type->second))throw std::invalid_argument("specialization scalar type");
        auto width=widths.at(type->second);
        if((width!=8&&width!=16&&width!=32&&width!=64)||value.bytes.size()!=width/8)throw std::invalid_argument("specialization size/type mismatch");
        std::vector<uint32_t> words((value.bytes.size()+3)/4);std::memcpy(words.data(),value.bytes.data(),value.bytes.size());
        replacements.emplace(value.id,std::move(words));
    }
    spvtools::Optimizer optimizer(env);optimizer.SetMessageConsumer(consumer);
    optimizer.RegisterPass(spvtools::CreateSetSpecConstantDefaultValuePass(replacements));
    optimizer.RegisterPass(spvtools::CreateFreezeSpecConstantValuePass());
    optimizer.RegisterPass(spvtools::CreateFoldSpecConstantOpAndCompositePass());
    optimizer.RegisterPass(spvtools::CreateDeadBranchElimPass());
    optimizer.RegisterPass(spvtools::CreateAggressiveDCEPass(true));
    optimizer.RegisterPass(spvtools::CreateRemoveUnusedInterfaceVariablesPass());
    optimizer.RegisterPass(spvtools::CreateDeadVariableEliminationPass());
    spvtools::OptimizerOptions options;options.set_validator_options(validation);
    std::vector<uint32_t> out;
    if(!optimizer.Run(code.data(),code.size(),&out,options)||!validator.Validate(out.data(),out.size(),validation))throw std::invalid_argument("specialization failed: "+diagnostic);
    return out;
}
std::vector<DescriptorUse> staticDescriptorUses(const std::vector<uint32_t>& code,
    const std::string& name,uint32_t model,GuardTarget target){
    return staticDescriptorUses(code,name,model,target,false);
}
std::vector<DescriptorUse> staticDescriptorUses(const std::vector<uint32_t>& code,
    const std::string& name,uint32_t model,GuardTarget target,bool uniformBufferStandardLayout){
    return staticDescriptorUses(code,name,model,target,uniformBufferStandardLayout,false);
}
std::vector<DescriptorUse> staticDescriptorUses(const std::vector<uint32_t>& code,
    const std::string& name,uint32_t model,GuardTarget target,bool uniformBufferStandardLayout,bool scalarBlockLayout){
    if(code.size()<5||code.size()>262144||code[0]!=0x07230203||name.empty()||name.size()>1024||name.find('\0')!=std::string::npos)
        throw std::invalid_argument("bounded module and entry point required");
    if(target!=GuardTarget::Vulkan11&&target!=GuardTarget::Vulkan12&&target!=GuardTarget::Vulkan13)throw std::invalid_argument("unsupported static-use environment");
    const auto env=target==GuardTarget::Vulkan13?SPV_ENV_VULKAN_1_3:target==GuardTarget::Vulkan12?SPV_ENV_VULKAN_1_2:SPV_ENV_VULKAN_1_1;
    std::string diagnostic;
    auto consumer=[&](spv_message_level_t,const char*,const spv_position_t&,const char* text){if(diagnostic.size()<4096)diagnostic.append(text,0,4096-diagnostic.size());};
    spvtools::SpirvTools validator(env);validator.SetMessageConsumer(consumer);
    spvtools::ValidatorOptions options;options.SetUniformBufferStandardLayout(uniformBufferStandardLayout);options.SetScalarBlockLayout(scalarBlockLayout);
    if(!validator.Validate(code.data(),code.size(),options))throw std::invalid_argument("invalid static-use SPIR-V: "+diagnostic);
    auto context=spvtools::BuildModule(env,consumer,code.data(),code.size());
    if(!context)throw std::invalid_argument("cannot build static-use IR: "+diagnostic);
    auto* module=context->module();uint32_t root=0;
    for(auto& entry:module->entry_points())if(entry.GetSingleWordInOperand(0)==model&&entry.GetInOperand(2).AsString()==name){
        if(root)throw std::invalid_argument("ambiguous static-use entry point");
        root=entry.GetSingleWordInOperand(1);
    }
    if(!root)throw std::invalid_argument("static-use entry point/stage not found");
    std::map<uint32_t,spvtools::opt::Function*> functions;
    for(auto& function:*module)functions.emplace(function.result_id(),&function);
    auto* decorations=context->get_decoration_mgr();
    auto decoration=[&](uint32_t id,uint32_t kind){
        std::optional<uint32_t> value;
        decorations->ForEachDecoration(id,kind,[&](const spvtools::opt::Instruction& inst){
            if(inst.opcode()!=SpvOpDecorate||inst.NumInOperands()!=3)throw std::invalid_argument("unsupported descriptor decoration");
            auto next=inst.GetSingleWordInOperand(2);
            if(value&&*value!=next)throw std::invalid_argument("conflicting descriptor decoration");
            value=next;
        });return value;
    };
    std::map<uint32_t,DescriptorUse> descriptors;
    for(auto& inst:module->types_values())if(inst.opcode()==SpvOpVariable){
        const auto storage=inst.GetSingleWordInOperand(0);
        if(storage!=SpvStorageClassUniformConstant&&storage!=SpvStorageClassUniform&&storage!=SpvStorageClassStorageBuffer)continue;
        auto set=decoration(inst.result_id(),SpvDecorationDescriptorSet),binding=decoration(inst.result_id(),SpvDecorationBinding);
        if(!set||!binding)throw std::invalid_argument("descriptor lacks set/binding");
        descriptors.emplace(inst.result_id(),DescriptorUse{*set,*binding,inst.result_id()});
    }
    std::set<uint32_t> visited,used;std::vector<uint32_t> pending{root};
    while(!pending.empty()){
        auto id=pending.back();pending.pop_back();if(!visited.insert(id).second)continue;
        auto at=functions.find(id);if(at==functions.end())throw std::invalid_argument("unresolved static-use function");
        at->second->ForEachInst([&](const spvtools::opt::Instruction* inst){
            if(inst->opcode()==SpvOpFunctionCall)pending.push_back(inst->GetSingleWordInOperand(0));
            inst->ForEachInId([&](const uint32_t* operand){if(descriptors.count(*operand))used.insert(*operand);});
        });
    }
    std::vector<DescriptorUse> result;
    for(auto id:used)result.push_back(descriptors.at(id));
    std::sort(result.begin(),result.end(),[](const auto& a,const auto& b){return std::tie(a.set,a.binding,a.variable)<std::tie(b.set,b.binding,b.variable);});
    return result;
}
}
