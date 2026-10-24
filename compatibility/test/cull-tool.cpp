#include <mavk/compat/cull_distance.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>
static std::vector<uint32_t> read(const char* path){std::ifstream in(path,std::ios::binary|std::ios::ate);const auto size=in.tellg();if(size<20||size>1048576||size%4)throw std::runtime_error("bounded binary input required");std::vector<uint32_t> words(size_t(size)/4);in.seekg(0);in.read(reinterpret_cast<char*>(words.data()),size);if(!in)throw std::runtime_error("read failed");return words;}
static void write(const char* path,const std::vector<uint32_t>& words){if(std::ifstream(path))throw std::runtime_error("preserve existing output");std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<const char*>(words.data()),words.size()*4);if(!out)throw std::runtime_error("write failed");}
int main(int argc,char** argv){try{
    if(argc!=6&&argc!=7)throw std::runtime_error("producer.spv fragment.spv points|lines|triangles producer-out.spv geometry-out.spv [tes]");
    if(argc==7&&std::string(argv[6])!="tes")throw std::runtime_error("unknown cull producer");
    const std::string mode=argv[3];auto primitive=mode=="points"?mavk::compat::CullPrimitive::Points:mode=="lines"?mavk::compat::CullPrimitive::Lines:mode=="triangles"?mavk::compat::CullPrimitive::Triangles:throw std::runtime_error("unknown topology");
    const auto program=mavk::compat::compileCullDistance(read(argv[1]),read(argv[2]),primitive,mavk::compat::GuardTarget::Vulkan11,argc==7?mavk::compat::CullProducer::TessellationEvaluation:mavk::compat::CullProducer::Vertex);
    if(std::ifstream(argv[4])||std::ifstream(argv[5]))throw std::runtime_error("preserve prior outputs");
    write(argv[4],program.vertex);write(argv[5],program.geometry);
    std::cout<<"CULL_PROGRAM_VALIDATED distances="<<program.distances<<" vertices="<<program.vertices<<" inputComponents="<<program.inputComponents<<" outputComponents="<<program.outputComponents<<" featureNotAdvertised=1\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
