// Experimental CLI consuming the same guard compiler as native adapters.
#include <mavk/compat/shader_guards.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc,char** argv){
    try{
        if(argc<3||argc>6)throw std::runtime_error("usage: mavk-shader-tool input.spv output.spv [--vulkan12|--vulkan13] [--no-reports] [--texel-formats] (public set0,metadata set1)");
        mavk::compat::GuardCompilerOptions options;
        for(int i=3;i<argc;i++){const std::string flag=argv[i];if(flag=="--vulkan13")options.target=mavk::compat::GuardTarget::Vulkan13;else if(flag=="--vulkan12")options.target=mavk::compat::GuardTarget::Vulkan12;else if(flag=="--no-reports")options.reportErrors=false;else if(flag=="--texel-formats")options.texelFormatMetadata=true;else throw std::runtime_error("unknown compiler option");}
        std::ifstream input(argv[1],std::ios::binary|std::ios::ate);auto bytes=input.tellg();
        if(bytes<20||bytes>1024*1024||bytes%4)throw std::runtime_error("bounded aligned module required");
        std::vector<uint32_t> code(size_t(bytes)/4);input.seekg(0);input.read(reinterpret_cast<char*>(code.data()),bytes);if(!input)throw std::runtime_error("input read failed");
        mavk::compat::GuardCompiler compiler(16,8*1024*1024,options);auto guarded=compiler.compile(code);
        if(std::ifstream(argv[2],std::ios::binary))throw std::runtime_error("preserve prior output");
        std::ofstream output(argv[2],std::ios::binary);output.write(reinterpret_cast<const char*>(guarded->data()),std::streamsize(guarded->size()*4));if(!output)throw std::runtime_error("output write failed");
        std::cout<<"Guarded module validated, "<<guarded->size()*4<<" bytes; NOT full nullDescriptor support\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
