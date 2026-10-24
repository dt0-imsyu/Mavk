#include <mavk/compat/shader_guards.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <map>
#include <set>
#include "grouped-fixture.hpp"
using namespace mavk::compat;
unsigned assertions=0,rejections=0;
void check(bool yes){assertions++;if(!yes)throw std::runtime_error("guard cache assertion");}
template<class F>void reject(F action){try{action();}catch(const std::exception&){rejections++;return;}throw std::runtime_error("shader should reject");}
GuardCompiler::Words read(const char* file){std::ifstream stream(file,std::ios::binary|std::ios::ate);auto bytes=stream.tellg();if(bytes<20||bytes>1024*1024||bytes%4)throw std::runtime_error("bounded fixture");GuardCompiler::Words words(size_t(bytes)/4);stream.seekg(0);stream.read(reinterpret_cast<char*>(words.data()),bytes);if(!stream)throw std::runtime_error("fixture read");return words;}
bool privateBinding(const GuardCompiler::Words& words,uint32_t binding){
    for(size_t p=5;p<words.size();p+=words[p]>>16)if((words[p]&65535)==71&&(words[p]>>16)==4&&words[p+2]==34&&words[p+3]==1){
        auto id=words[p+1];for(size_t q=5;q<words.size();q+=words[q]>>16)if((words[q]&65535)==71&&(words[q]>>16)==4&&words[q+1]==id&&words[q+2]==33&&words[q+3]==binding)return true;
    }return false;
}
int main(int argc,char** argv){try{
    if(argc==3&&std::string(argv[1])=="--texel-format-metadata"){
        GuardCompilerOptions options;options.target=GuardTarget::Vulkan12;options.reportErrors=false;
        GuardCompiler legacy(16,8*1024*1024,options);auto source=read(argv[2]);auto old=legacy.compile(source);
        options.texelFormatMetadata=true;GuardCompiler compiler(16,8*1024*1024,options);auto current=compiler.compile(source);
        check(*old!=*current);check(compiler.compile(source)==current);check(privateBinding(*current,1));check(!privateBinding(*current,0));
        unsigned fetches=0,shifts=0,oldLoads=0,newLoads=0;
        for(size_t p=5;p<old->size();p+=(*old)[p]>>16)oldLoads+=((*old)[p]&65535)==61;
        for(size_t p=5;p<current->size();p+=(*current)[p]>>16){auto op=(*current)[p]&65535;fetches+=op==95;shifts+=op==194;newLoads+=op==61;}
        check(fetches==1);check(shifts>=1);check(newLoads>=oldLoads+4);
        std::cout<<"TEXEL_FORMAT_METADATA_COMPILER_PASS fetches="<<fetches<<" addedLoads="<<newLoads-oldLoads<<" assertions="<<assertions<<"\n";return 0;
    }
    if(argc==3&&std::string(argv[1])=="--texel-components"){
        GuardCompilerOptions options;options.target=GuardTarget::Vulkan12;options.reportErrors=false;
        GuardCompiler compiler(16,8*1024*1024,options);const auto result=compiler.compile(read(argv[2]));
        std::map<uint32_t,uint32_t> constants;std::set<uint32_t> zero,alpha;
        for(size_t p=5;p<result->size();p+=(*result)[p]>>16){auto op=(*result)[p]&65535,count=(*result)[p]>>16;if(op==43&&count==4)constants[(*result)[p+2]]=(*result)[p+3];if(op==46){zero.insert((*result)[p+2]);constants[(*result)[p+2]]=0;}}
        for(size_t p=5;p<result->size();p+=(*result)[p]>>16)if(((*result)[p]&65535)==44&&((*result)[p]>>16)==7){bool valid=true;for(unsigned c=0;c<4;c++){auto v=constants.find((*result)[p+3+c]);valid&=v!=constants.end()&&v->second==(c==3?1u:0u);}if(valid)alpha.insert((*result)[p+2]);}
        unsigned oob=0,nulls=0;
        for(size_t p=5;p<result->size();p+=(*result)[p]>>16)if(((*result)[p]&65535)==245){for(size_t q=p+3;q<p+((*result)[p]>>16);q+=2){oob+=alpha.count((*result)[q]);nulls+=zero.count((*result)[q]);}}
        check(!alpha.empty());check(oob>=3);check(nulls>=4);check(privateBinding(*result,1));check(!privateBinding(*result,0));
        std::cout<<"TEXEL_COMPONENTS_COMPILER_PASS oobAlphaPhi="<<oob<<" nullZeroPhi="<<nulls<<" assertions="<<assertions<<"\n";return 0;
    }
    if(argc==3&&std::string(argv[1])=="--texel-atomic"){
        GuardCompilerOptions options;options.target=GuardTarget::Vulkan12;options.reportErrors=false;
        GuardCompiler compiler(16,8*1024*1024,options);const auto original=read(argv[2]);const auto result=compiler.compile(original);
        using Tail=std::vector<uint32_t>;std::vector<Tail> before,after;
        std::map<uint32_t,size_t> definitions;std::map<uint32_t,uint32_t> blocks,trueCondition;
        uint32_t block=0;unsigned queries=0,pointers=0;
        for(size_t p=5;p<original.size();p+=original[p]>>16){auto op=original[p]&65535,count=original[p]>>16;if(op>=227&&op<=242){Tail tail{op};tail.insert(tail.end(),original.begin()+p+4,original.begin()+p+count);before.push_back(tail);}}
        for(size_t p=5;p<result->size();p+=(*result)[p]>>16){
            const auto op=(*result)[p]&65535,count=(*result)[p]>>16;
            if(op==248)block=(*result)[p+1];
            if(op==250)trueCondition[(*result)[p+2]]=(*result)[p+1];
            if(op==60||op==104||op==176){definitions[(*result)[p+2]]=p;blocks[(*result)[p+2]]=block;}
            queries+=op==104;pointers+=op==60;
            if(op>=227&&op<=242){
                Tail tail{op};tail.insert(tail.end(),result->begin()+p+4,result->begin()+p+count);after.push_back(tail);
                auto ptr=(*result)[p+3];check(definitions.count(ptr)&&blocks[ptr]==block);
                auto condition=trueCondition[block];check(definitions.count(condition));
                auto compare=definitions[condition];check(((*result)[compare]&65535)==176);
                auto size=(*result)[compare+4];check(definitions.count(size)&&(((*result)[definitions[size]]&65535)==104));
            }
        }
        check(before==after);check(after.size()==4);check(queries==4);check(pointers==4);
        check(privateBinding(*result,1));check(!privateBinding(*result,0));
        std::cout<<"TEXEL_ATOMIC_COMPILER_PASS atomics=4 queries=4 originalAtomicOperands=1 guardedPointerBranches=1 assertions="<<assertions<<"\n";return 0;
    }
    if(argc==3&&std::string(argv[1])=="--storage-texel"){
        GuardCompilerOptions options;options.target=GuardTarget::Vulkan12;options.reportErrors=false;
        GuardCompiler compiler(16,8*1024*1024,options);const auto result=compiler.compile(read(argv[2]));
        unsigned queries=0,reads=0,writes=0;
        for(size_t p=5;p<result->size();p+=(*result)[p]>>16){
            const auto op=(*result)[p]&65535,count=(*result)[p]>>16;
            queries+=op==104;writes+=op==99;
            reads+=op==98&&count==6&&(*result)[p+5]==8192;
        }
        // UINT_MAX cannot be below any uint range:the negative read folds to
        // zero before GPU execution;three reads and two writes remain guarded.
        check(queries==5);check(reads==3);check(writes==2);check(privateBinding(*result,1));check(!privateBinding(*result,0));
        std::cout<<"STORAGE_TEXEL_COMPILER_PASS queries="<<queries<<" reads="<<reads<<" writes="<<writes<<" assertions="<<assertions<<"\n";return 0;
    }
    if(argc==3&&std::string(argv[1])=="--texel-operands"){
        GuardCompilerOptions options;options.target=GuardTarget::Vulkan12;options.reportErrors=false;
        GuardCompiler compiler(16,8*1024*1024,options);const auto original=read(argv[2]);const auto result=compiler.compile(original);
        unsigned queries=0,fetches=0;uint32_t mask=UINT32_MAX;
        for(size_t p=5;p<original.size();p+=original[p]>>16)if((original[p]&65535)==95&&(original[p]>>16)==6)mask=original[p+5];
        check(mask!=UINT32_MAX);
        for(size_t p=5;p<result->size();p+=(*result)[p]>>16){
            queries+=((*result)[p]&65535)==104;
            if(((*result)[p]&65535)==95&&((*result)[p]>>16)==6&&(*result)[p+5]==mask)fetches++;
        }
        check(queries==1);check(fetches==1);check(privateBinding(*result,1));check(!privateBinding(*result,0));
        std::cout<<"TEXEL_OPERANDS_COMPILER_PASS sizeQueries="<<queries<<" mask="<<mask<<" assertions="<<assertions<<"\n";return 0;
    }
    if(argc==3&&std::string(argv[1])=="--copy-operands"){
        const auto original=read(argv[2]);
        GuardCompiler legacy;reject([&]{legacy.compile(original);});
        GuardCompilerOptions options;options.target=GuardTarget::Vulkan12;options.reportErrors=false;
        GuardCompiler compiler(16,8*1024*1024,options);const auto result=compiler.compile(original);
        check(privateBinding(*result,1));check(!privateBinding(*result,0));
        check(staticDescriptorUses(original,"main",5,GuardTarget::Vulkan12).size()==2);
        unsigned loads=0,stores=0,copies=0;
        for(size_t p=5;p<result->size();p+=(*result)[p]>>16){
            const auto op=(*result)[p]&65535,count=(*result)[p]>>16;
            copies+=op==63;
            if(op==61&&count==6&&(*result)[p+4]==6&&(*result)[p+5]==1)loads++;
            if(op==62&&count==5&&(*result)[p+3]==3&&(*result)[p+4]==1)stores++;
        }
        check(loads==2);check(stores==2);check(copies==0);
        std::cout<<"COPY_OPERANDS_VULKAN12_PASS loads="<<loads<<" stores="<<stores<<" assertions="<<assertions<<" rejections="<<rejections<<"\n";return 0;
    }
    if(argc==3&&std::string(argv[1])=="--aggregate"){
        GuardCompilerOptions options;options.reportErrors=false;GuardCompiler compiler(16,8*1024*1024,options);
        const auto result=compiler.compile(read(argv[2]));unsigned comparisons=0;
        for(size_t p=5;p<result->size();p+=(*result)[p]>>16)comparisons+=((*result)[p]&65535)==176;
        check(privateBinding(*result,1));check(!privateBinding(*result,0));check(comparisons>=4);
        std::cout<<"AGGREGATE_BUFFER_COMPILER_PASS rangeChecks="<<comparisons<<" assertions="<<assertions<<"\n";return 0;
    }
    if(argc==3&&std::string(argv[1])=="--wide"){
        GuardCompilerOptions options;options.reportErrors=false;GuardCompiler compiler(16,8*1024*1024,options);
        const auto result=compiler.compile(read(argv[2]));std::map<uint32_t,uint32_t> widths;
        for(size_t p=5;p<result->size();p+=(*result)[p]>>16)if(((*result)[p]&65535)==21)widths[(*result)[p+1]]=(*result)[p+2];
        unsigned widened=0;
        for(size_t p=5;p<result->size();p+=(*result)[p]>>16)if(((*result)[p]&65535)==113&&widths[(*result)[p+1]]==64)widened++;
        check(widened>=2);check(privateBinding(*result,1));check(!privateBinding(*result,0));
        std::cout<<"WIDE_BUFFER_COMPILER_PASS roundTrips="<<widened<<" assertions="<<assertions<<"\n";return 0;
    }
    if(argc==3&&std::string(argv[1])=="--overflow"){
        auto original=read(argv[2]);
        for(unsigned variant=0;variant<2;variant++){
            auto code=original;bool index=false,offset=false;
            for(size_t p=5;p<code.size();p+=code[p]>>16){
                if((code[p]&65535)==43&&(code[p]>>16)==4&&code[p+3]==17){code[p+3]=variant?0x3fffffffu:0x40000000u;index=true;}
                if(variant&&(code[p]&65535)==72&&(code[p]>>16)==5&&code[p+3]==35){code[p+4]=16;offset=true;}
            }
            check(index&&(!variant||offset));GuardCompilerOptions options;options.reportErrors=false;GuardCompiler compiler(16,8*1024*1024,options);
            const auto result=compiler.compile(code);std::map<uint32_t,uint32_t> constants;
            for(size_t p=5;p<result->size();p+=(*result)[p]>>16)if(((*result)[p]&65535)==43&&((*result)[p]>>16)==4)constants[(*result)[p+2]]=(*result)[p+3];
            unsigned saturated=0;
            for(size_t p=5;p<result->size();p+=(*result)[p]>>16)if(((*result)[p]&65535)==169&&constants.count((*result)[p+4])&&constants.at((*result)[p+4])==UINT32_MAX)saturated++;
            check(saturated>=2);
        }
        std::cout<<"BUFFER_OFFSET_OVERFLOW_PASS variants=2 assertions="<<assertions<<"\n";return 0;
    }
    if(argc==3&&std::string(argv[1])=="--nested"){
        const auto original=read(argv[2]);GuardCompiler compiler;
        const auto guarded=compiler.compile(original);
        check(privateBinding(*guarded,1));check(privateBinding(*guarded,0));
        GuardCompilerOptions options;options.reportErrors=false;GuardCompiler quiet(16,8*1024*1024,options);
        const auto readonly=quiet.compile(original);
        check(privateBinding(*readonly,1));check(!privateBinding(*readonly,0));
        std::cout<<"NESTED_BUFFER_GUARDS_PASS assertions="<<assertions<<"\n";return 0;
    }
    if(argc!=3&&argc!=4)throw std::runtime_error("two originals and optional SPIR-V1.6 vertex fixture required");auto a=read(argv[1]),b=read(argv[2]);
    GuardCompiler cache(1);auto first=cache.compile(a);check(first->size()>a.size());check(cache.compile(a)==first);check(cache.stats().compiled==1&&cache.stats().hits==1);
    auto second=cache.compile(b);check(second!=first);check(cache.stats().entries==1);check(cache.stats().retainedBytes==(b.size()+second->size())*4);
    auto again=cache.compile(a);check(*again==*first&&again!=first);check(cache.stats().compiled==3&&cache.stats().entries==1);
    auto bytes=cache.stats().retainedBytes;reject([&]{cache.compile({});});auto bad=a;bad[0]=0;reject([&]{cache.compile(bad);});bad=a;bad.push_back(0);reject([&]{cache.compile(bad);});reject([&]{cache.compile(*first);});check(cache.stats().retainedBytes==bytes&&cache.stats().compiled==3);
    GuardCompiler small(1,1024);auto uncached=small.compile(a);check(*uncached==*first);check(small.stats().entries==0&&small.stats().retainedBytes==0);reject([]{GuardCompiler invalid(0);});
    auto remapped=a;for(size_t p=5;p<remapped.size();p+=remapped[p]>>16)if((remapped[p]&65535)==71&&(remapped[p]>>16)==4&&remapped[p+2]==34)remapped[p+3]=2;
    GuardCompiler multi(16,8*1024*1024,{3});check(multi.guardSetIndex()==3);auto guardedMulti=multi.compile(remapped);check(multi.compile(remapped)==guardedMulti);check(*multi.compile(a)!=*first);reject([&]{cache.compile(remapped);});reject([&]{multi.compile(*guardedMulti);});
    reject([]{GuardCompiler invalid(16,8*1024*1024,{0});});reject([]{GuardCompiler invalid(16,8*1024*1024,{8});});
    GuardCompilerOptions quietOptions;quietOptions.reportErrors=false;GuardCompiler quiet(16,8*1024*1024,quietOptions);
    auto noReports=quiet.compile(a);check(privateBinding(*first,0));check(!privateBinding(*noReports,0));check(privateBinding(*noReports,1));check(*noReports!=*first);check(quiet.compile(a)==noReports);
    auto noReportsB=quiet.compile(b);check(!privateBinding(*noReportsB,0));check(privateBinding(*noReportsB,1));
    auto grouped=groupDescriptorFixture(a);auto normalized=quiet.compile(grouped);check(quiet.compile(grouped)==normalized);
    bool anyGroup=false;for(size_t p=5;p<normalized->size();p+=(*normalized)[p]>>16){const auto op=(*normalized)[p]&65535;anyGroup|=op==73||op==74||op==75;}
    check(!anyGroup);check(privateBinding(*normalized,1));check(!privateBinding(*normalized,0));
    if(argc==4){
        auto vertex=read(argv[3]);check(vertex[1]==0x00010600);
        GuardCompilerOptions modernOptions;modernOptions.target=GuardTarget::Vulkan13;modernOptions.reportErrors=false;
        GuardCompiler modern(16,8*1024*1024,modernOptions);auto guardedVertex=modern.compile(vertex);
        check(!privateBinding(*guardedVertex,0));check(privateBinding(*guardedVertex,1));check(modern.compile(vertex)==guardedVertex);
    }
    std::cout<<"SHADER_GUARDS_PASS assertions="<<assertions<<" rejections="<<rejections<<"\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
