#include <mavk/compat/shader_guards.hpp>
#include <spirv-tools/optimizer.hpp>
#include <stdexcept>
#include <string>

namespace mavk::compat {
GuardCompiler::GuardCompiler(size_t entries,size_t bytes,GuardCompilerOptions options):maxEntries_(entries),maxBytes_(bytes),publicSets_(options.publicSetCount){
    if(!entries||entries>256||bytes<1024||bytes>64*1024*1024||!publicSets_||publicSets_>7)throw std::invalid_argument("guard cache/set limits");
}
GuardCompilerStats GuardCompiler::stats() const{return {compiled_,hits_,cache_.size(),retained_};}
std::shared_ptr<const GuardCompiler::Words> GuardCompiler::compile(const Words& code){
    if(code.size()<5||code.size()>1024*1024/4||code[0]!=0x07230203)throw std::invalid_argument("bounded SPIR-V module required");
    for(auto at=cache_.begin();at!=cache_.end();++at)if(at->original==code){auto result=at->guarded;cache_.splice(cache_.begin(),cache_,at);hits_++;return result;}
    for(size_t p=5;p<code.size();){
        auto count=code[p]>>16,op=code[p]&65535;if(!count||p+count>code.size())throw std::invalid_argument("malformed SPIR-V instruction");
        if(op==71&&count==4&&code[p+2]==34&&code[p+3]>=publicSets_)throw std::invalid_argument("descriptor set outside public range or private metadata collision");
        if(op==60||op==67||op==70||op==120||op==318||op==319||op==5614||op==5615||op==6035)throw std::invalid_argument("unsupported pointer/image/flag/float-atomic access form");
        p+=count;
    }
    std::string diagnostics;
    auto consumer=[&](spv_message_level_t,const char*,const spv_position_t&,const char* text){if(diagnostics.size()<4096)diagnostics.append(text,0,4096-diagnostics.size());};
    spvtools::SpirvTools validator(SPV_ENV_VULKAN_1_1);validator.SetMessageConsumer(consumer);
    if(!validator.Validate(code))throw std::invalid_argument("input SPIR-V invalid: "+diagnostics);
    spvtools::Optimizer optimizer(SPV_ENV_VULKAN_1_1);optimizer.SetMessageConsumer(consumer);
    optimizer.RegisterPass(spvtools::CreateInstBindlessCheckPass(publicSets_,23,true,true,true,true));
    optimizer.RegisterPass(spvtools::CreateSimplificationPass());
    optimizer.RegisterPass(spvtools::CreateDeadBranchElimPass());
    optimizer.RegisterPass(spvtools::CreateBlockMergePass());
    optimizer.RegisterPass(spvtools::CreateAggressiveDCEPass(true));
    auto output=std::make_shared<Words>();
    if(!optimizer.Run(code.data(),code.size(),output.get())||output->size()>4*1024*1024/4||!validator.Validate(*output))throw std::runtime_error("guard compilation/validation failed: "+diagnostics);
    compiled_++;size_t bytes=(code.size()+output->size())*sizeof(uint32_t);
    if(bytes<=maxBytes_){
        while(cache_.size()>=maxEntries_||retained_>maxBytes_-bytes){retained_-=cache_.back().bytes;cache_.pop_back();}
        cache_.push_front({code,output,bytes});retained_+=bytes;
    }
    return output;
}
}
