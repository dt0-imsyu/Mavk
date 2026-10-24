#include <mavk/compat/cull_distance.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>
static std::vector<uint32_t> load(const char* path){std::ifstream in(path,std::ios::binary|std::ios::ate);auto bytes=in.tellg();if(bytes<20||bytes>1048576||bytes%4)throw std::runtime_error("test binary input");std::vector<uint32_t> result(size_t(bytes)/4);in.seekg(0);in.read(reinterpret_cast<char*>(result.data()),bytes);if(!in)throw std::runtime_error("test read");return result;}
int main(int argc,char** argv){try{
    if(argc!=6&&argc!=8&&argc!=10&&argc!=11&&argc!=12)throw std::runtime_error("sourceVS FS integerVS referenceVS clipVS [userBlockVS userBlockFS [blockLocationVS blockLocationFS [primitiveIdFS [TES]]]] required");
    const auto source=load(argv[1]),fragment=load(argv[2]),integer=load(argv[3]);unsigned valid=0,negative=0;
    for(auto mode:{mavk::compat::CullPrimitive::Points,mavk::compat::CullPrimitive::Lines,mavk::compat::CullPrimitive::Triangles})for(const auto* input:{&source,&integer}){
        auto program=mavk::compat::compileCullDistance(*input,fragment,mode);
        if(program.distances!=2||program.geometry.empty()||program.vertex.empty()||program.inputComponents!=(input==&source?10u:12u))throw std::runtime_error("cull program metadata");valid++;
    }
    auto reject=[&](auto call){try{call();}catch(const std::invalid_argument&){negative++;return;}throw std::runtime_error("unsupported cull contract admitted");};
    auto linked=mavk::compat::compileCullDistance(source,fragment,mavk::compat::CullPrimitive::Triangles,mavk::compat::GuardTarget::Vulkan12);
    if(linked.locations!=4||linked.vertices!=3||linked.geometry.size()<5)throw std::runtime_error("pipeline link target/limits metadata");valid++;
    for(int i=4;i<6;i++){auto input=load(argv[i]);reject([&]{mavk::compat::compileCullDistance(input,fragment,mavk::compat::CullPrimitive::Triangles);});}
    reject([&]{mavk::compat::compileCullDistance(fragment,fragment,mavk::compat::CullPrimitive::Triangles);});
    reject([&]{mavk::compat::compileCullDistance(source,fragment,mavk::compat::CullPrimitive(99));});
    auto malformed=source;malformed.pop_back();reject([&]{mavk::compat::compileCullDistance(malformed,fragment,mavk::compat::CullPrimitive::Triangles);});
    for(int i=6;i+1<argc&&i<10;i+=2){auto block=mavk::compat::compileCullDistance(load(argv[i]),load(argv[i+1]),mavk::compat::CullPrimitive::Triangles);if(block.inputComponents!=15||block.outputComponents!=13||block.locations!=(i==6?7u:11u))throw std::runtime_error("user block metadata");valid++;}
    if(argc>=11){auto ids=mavk::compat::compileCullDistance(source,load(argv[10]),mavk::compat::CullPrimitive::Triangles);if(ids.inputComponents!=11||ids.outputComponents!=9||ids.locations!=4)throw std::runtime_error("primitive-ID limit accounting");valid++;}
    if(argc==12){const auto tes=load(argv[11]);auto program=mavk::compat::compileCullDistance(tes,fragment,mavk::compat::CullPrimitive::Triangles,mavk::compat::GuardTarget::Vulkan12,mavk::compat::CullProducer::TessellationEvaluation);if(program.inputComponents!=10||program.outputComponents!=8||program.locations!=4)throw std::runtime_error("TES cull metadata");valid++;
        reject([&]{mavk::compat::compileCullDistance(tes,fragment,mavk::compat::CullPrimitive::Triangles);});
        reject([&]{mavk::compat::compileCullDistance(source,fragment,mavk::compat::CullPrimitive::Triangles,mavk::compat::GuardTarget::Vulkan12,mavk::compat::CullProducer::TessellationEvaluation);});
        reject([&]{mavk::compat::compileCullDistance(tes,fragment,mavk::compat::CullPrimitive::Lines,mavk::compat::GuardTarget::Vulkan12,mavk::compat::CullProducer::TessellationEvaluation);});
    }
    std::cout<<"CULL_COMPILER_HOST_PASS valid="<<valid<<" negatives="<<negative<<" gpuProofSeparate=1\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
