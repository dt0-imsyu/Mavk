// Experimental configurable shader guard compiler, not a complete Vulkan layer.
// Link with pinned SPIRV-Tools + reviewed buffer-atomics.patch. Upstream Apache2
// license is retained in the external source/build distribution.
#include <spirv-tools/optimizer.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstdint>

int main(int argc,char** argv){
    try{
        if(argc!=3)throw std::runtime_error("usage: mavk-shader-tool input.spv output.spv (original set0 only,metadata set1)");
        std::ifstream input(argv[1],std::ios::binary|std::ios::ate);auto bytes=input.tellg();
        if(bytes<20||bytes>1024*1024||bytes%4)throw std::runtime_error("bounded aligned module required");
        std::vector<uint32_t> code(size_t(bytes)/4);input.seekg(0);input.read(reinterpret_cast<char*>(code.data()),bytes);if(!input)throw std::runtime_error("input read failed");
        if(code[0]!=0x07230203)throw std::runtime_error("SPIR-V header required");
        for(size_t p=5;p<code.size();){
            auto count=code[p]>>16,op=code[p]&65535;if(!count||p+count>code.size())throw std::runtime_error("malformed instruction");
            if(op==71&&count==4&&code[p+2]==34&&code[p+3]!=0)throw std::runtime_error("original set0 only; metadata set1 reserved");
            // Pointer aliases/physical addressing/image atomics/extension float
            // atomics require separate implementations. No silent admission.
            if(op==60||op==67||op==70||op==120||op==318||op==319||op==5614||op==5615||op==6035)throw std::runtime_error("unsupported pointer/image/flag/float-atomic access form");
            p+=count;
        }
        spvtools::Optimizer optimizer(SPV_ENV_VULKAN_1_1);
        optimizer.SetMessageConsumer([](spv_message_level_t,const char*,const spv_position_t& pos,const char* text){std::cerr<<pos.index<<": "<<text<<'\n';});
        optimizer.RegisterPass(spvtools::CreateInstBindlessCheckPass(1,23,true,true,true,true));
        optimizer.RegisterPass(spvtools::CreateSimplificationPass());
        optimizer.RegisterPass(spvtools::CreateDeadBranchElimPass());
        optimizer.RegisterPass(spvtools::CreateBlockMergePass());
        optimizer.RegisterPass(spvtools::CreateAggressiveDCEPass(true));
        std::vector<uint32_t> guarded;
        if(!optimizer.Run(code.data(),code.size(),&guarded))throw std::runtime_error("guard compilation failed");
        spvtools::SpirvTools validator(SPV_ENV_VULKAN_1_1);if(!validator.Validate(guarded))throw std::runtime_error("guarded module invalid");
        if(std::ifstream(argv[2],std::ios::binary))throw std::runtime_error("preserve prior output");
        std::ofstream output(argv[2],std::ios::binary);output.write(reinterpret_cast<const char*>(guarded.data()),std::streamsize(guarded.size()*4));if(!output)throw std::runtime_error("output write failed");
        std::cout<<"Guarded module validated, "<<guarded.size()*4<<" bytes; NOT full nullDescriptor support\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
