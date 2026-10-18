#include <mavk/compat/shader_guards.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace mavk::compat;
unsigned assertions=0,rejections=0;
void check(bool yes){assertions++;if(!yes)throw std::runtime_error("guard cache assertion");}
template<class F>void reject(F action){try{action();}catch(const std::exception&){rejections++;return;}throw std::runtime_error("shader should reject");}
GuardCompiler::Words read(const char* file){std::ifstream stream(file,std::ios::binary|std::ios::ate);auto bytes=stream.tellg();if(bytes<20||bytes>1024*1024||bytes%4)throw std::runtime_error("bounded fixture");GuardCompiler::Words words(size_t(bytes)/4);stream.seekg(0);stream.read(reinterpret_cast<char*>(words.data()),bytes);if(!stream)throw std::runtime_error("fixture read");return words;}
int main(int argc,char** argv){try{
    if(argc!=3)throw std::runtime_error("two original shader fixtures required");auto a=read(argv[1]),b=read(argv[2]);
    GuardCompiler cache(1);auto first=cache.compile(a);check(first->size()>a.size());check(cache.compile(a)==first);check(cache.stats().compiled==1&&cache.stats().hits==1);
    auto second=cache.compile(b);check(second!=first);check(cache.stats().entries==1);check(cache.stats().retainedBytes==(b.size()+second->size())*4);
    auto again=cache.compile(a);check(*again==*first&&again!=first);check(cache.stats().compiled==3&&cache.stats().entries==1);
    auto bytes=cache.stats().retainedBytes;reject([&]{cache.compile({});});auto bad=a;bad[0]=0;reject([&]{cache.compile(bad);});bad=a;bad.push_back(0);reject([&]{cache.compile(bad);});reject([&]{cache.compile(*first);});check(cache.stats().retainedBytes==bytes&&cache.stats().compiled==3);
    GuardCompiler small(1,1024);auto uncached=small.compile(a);check(*uncached==*first);check(small.stats().entries==0&&small.stats().retainedBytes==0);reject([]{GuardCompiler invalid(0);});
    auto remapped=a;for(size_t p=5;p<remapped.size();p+=remapped[p]>>16)if((remapped[p]&65535)==71&&(remapped[p]>>16)==4&&remapped[p+2]==34)remapped[p+3]=2;
    GuardCompiler multi(16,8*1024*1024,{3});check(multi.guardSetIndex()==3);auto guardedMulti=multi.compile(remapped);check(multi.compile(remapped)==guardedMulti);check(*multi.compile(a)!=*first);reject([&]{cache.compile(remapped);});reject([&]{multi.compile(*guardedMulti);});
    reject([]{GuardCompiler invalid(16,8*1024*1024,{0});});reject([]{GuardCompiler invalid(16,8*1024*1024,{8});});
    std::cout<<"SHADER_GUARDS_PASS assertions="<<assertions<<" rejections="<<rejections<<"\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
