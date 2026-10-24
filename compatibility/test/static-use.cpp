#include <mavk/compat/shader_guards.hpp>
#include <spirv-tools/libspirv.hpp>
#include <iostream>
#include <stdexcept>
using namespace mavk::compat;
unsigned checks=0,rejections=0;
void check(bool yes){++checks;if(!yes)throw std::runtime_error("static-use assertion");}
template<class F>void reject(F f){try{f();}catch(const std::exception&){++rejections;return;}throw std::runtime_error("expected static-use rejection");}
const char* fixture=R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %first "first"
OpEntryPoint GLCompute %second "second"
OpEntryPoint Fragment %fragment "first"
OpExecutionMode %first LocalSize 1 1 1
OpExecutionMode %second LocalSize 1 1 1
OpExecutionMode %fragment OriginUpperLeft
OpDecorate %Block Block
OpMemberDecorate %Block 0 Offset 0
OpDecorate %a DescriptorSet 0
OpDecorate %a Binding 2
%group = OpDecorationGroup
OpDecorate %group DescriptorSet 3
OpGroupDecorate %group %b
OpDecorate %b Binding 7
OpDecorate %unused DescriptorSet 6
OpDecorate %unused Binding 11
%void = OpTypeVoid
%fn = OpTypeFunction %void
%uint = OpTypeInt 32 0
%Block = OpTypeStruct %uint
%ptr = OpTypePointer StorageBuffer %Block
%field = OpTypePointer StorageBuffer %uint
%zero = OpConstant %uint 0
%a = OpVariable %ptr StorageBuffer
%b = OpVariable %ptr StorageBuffer
%unused = OpVariable %ptr StorageBuffer
%first = OpFunction %void None %fn
%l1 = OpLabel
%call = OpFunctionCall %void %helper
OpReturn
OpFunctionEnd
%second = OpFunction %void None %fn
%l2 = OpLabel
%p2 = OpAccessChain %field %b %zero
%v2 = OpLoad %uint %p2
OpReturn
OpFunctionEnd
%helper = OpFunction %void None %fn
%lh = OpLabel
%p1 = OpAccessChain %field %a %zero
%v1 = OpLoad %uint %p1
OpSelectionMerge %merge None
OpSwitch %zero %merge 999 %merge
%merge = OpLabel
OpReturn
OpFunctionEnd
%fragment = OpFunction %void None %fn
%lf = OpLabel
%pf = OpAccessChain %field %b %zero
%vf = OpLoad %uint %pf
OpReturn
OpFunctionEnd
%unreachable = OpFunction %void None %fn
%lu = OpLabel
%pu = OpAccessChain %field %unused %zero
%vu = OpLoad %uint %pu
OpReturn
OpFunctionEnd
)";
int main(){try{
    spvtools::SpirvTools tools(SPV_ENV_VULKAN_1_1);std::vector<uint32_t> code;
    check(tools.Assemble(fixture,&code));check(tools.Validate(code));
    auto a=staticDescriptorUses(code,"first",5),b=staticDescriptorUses(code,"second",5);
    check(a.size()==1&&a[0].set==0&&a[0].binding==2);
    check(b.size()==1&&b[0].set==3&&b[0].binding==7);
    check(a[0].variable!=b[0].variable);
    auto fragment=staticDescriptorUses(code,"first",4);check(fragment.size()==1&&fragment[0].variable==b[0].variable);
    reject([&]{staticDescriptorUses(code,"first",0);});
    reject([&]{staticDescriptorUses(code,"missing",5);});
    reject([&]{staticDescriptorUses(code,"",5);});
    reject([&]{staticDescriptorUses(code,std::string("first\0suffix",12),5);});
    reject([&]{staticDescriptorUses({},"first",5);});
    auto bad=code;bad.push_back(0);reject([&]{staticDescriptorUses(bad,"first",5);});
    reject([&]{staticDescriptorUses(code,"first",5,static_cast<GuardTarget>(99));});
    // A literal matching a descriptor ID is not an ID operand and cannot add use.
    auto literal=code;bool replaced=false;
    for(size_t p=5;p<literal.size();p+=literal[p]>>16)if((literal[p]&65535)==251){literal[p+3]=b[0].variable;replaced=true;break;}
    check(replaced);auto onlyA=staticDescriptorUses(literal,"first",5);check(onlyA.size()==1&&onlyA[0].set==0);
    std::string modern=fixture;
    auto replace=[&](const std::string& from,const std::string& to){auto at=modern.find(from);if(at==std::string::npos)throw std::runtime_error("fixture replacement");modern.replace(at,from.size(),to);};
    replace("%first \"first\"","%first \"first\" %a %b %unused");
    replace("%second \"second\"","%second \"second\" %a %b %unused");
    replace("%fragment \"first\"","%fragment \"first\" %a %b %unused");
    spvtools::SpirvTools modernTools(SPV_ENV_VULKAN_1_3);std::vector<uint32_t> interfaceCode;
    check(modernTools.Assemble(modern,&interfaceCode));check(modernTools.Validate(interfaceCode));
    auto interfaceUse=staticDescriptorUses(interfaceCode,"first",5,GuardTarget::Vulkan13);
    check(interfaceUse.size()==1&&interfaceUse[0].set==0&&interfaceUse[0].binding==2);
    replace("%call = OpFunctionCall %void %helper","%call = OpFunctionCall %void %noop");
    modern+="\n%noop = OpFunction %void None %fn\n%ln = OpLabel\nOpReturn\nOpFunctionEnd\n";
    check(modernTools.Assemble(modern,&interfaceCode));check(modernTools.Validate(interfaceCode));
    check(staticDescriptorUses(interfaceCode,"first",5,GuardTarget::Vulkan13).empty());
    const char* packed=R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main" %ubo %out
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %array ArrayStride 4
OpDecorate %Block Block
OpMemberDecorate %Block 0 Offset 0
OpDecorate %ubo DescriptorSet 0
OpDecorate %ubo Binding 0
OpDecorate %Result Block
OpMemberDecorate %Result 0 Offset 0
OpDecorate %out DescriptorSet 0
OpDecorate %out Binding 1
%void = OpTypeVoid
%fn = OpTypeFunction %void
%uint = OpTypeInt 32 0
%zero = OpConstant %uint 0
%one = OpConstant %uint 1
%eight = OpConstant %uint 8
%array = OpTypeArray %uint %eight
%Block = OpTypeStruct %array
%Result = OpTypeStruct %uint
%up = OpTypePointer Uniform %Block
%uf = OpTypePointer Uniform %uint
%sp = OpTypePointer StorageBuffer %Result
%sf = OpTypePointer StorageBuffer %uint
%ubo = OpVariable %up Uniform
%out = OpVariable %sp StorageBuffer
%main = OpFunction %void None %fn
%label = OpLabel
%p = OpAccessChain %uf %ubo %zero %one
%v = OpLoad %uint %p
%r = OpAccessChain %sf %out %zero
OpStore %r %v
OpReturn
OpFunctionEnd
)";
    spvtools::SpirvTools packedTools(SPV_ENV_VULKAN_1_2);std::vector<uint32_t> packedCode;
    check(packedTools.Assemble(packed,&packedCode));
    check(!packedTools.Validate(packedCode));
    reject([&]{staticDescriptorUses(packedCode,"main",5,GuardTarget::Vulkan12);});
    check(staticDescriptorUses(packedCode,"main",5,GuardTarget::Vulkan12,true).size()==2);
    GuardCompilerOptions packedOptions;packedOptions.target=GuardTarget::Vulkan12;
    GuardCompiler strict(4,1024*1024,packedOptions);reject([&]{strict.compile(packedCode);});
    packedOptions.uniformBufferStandardLayout=true;packedOptions.reportErrors=false;
    GuardCompiler admitted(4,1024*1024,packedOptions);auto guarded=admitted.compile(packedCode);
    spvtools::ValidatorOptions packedValidation;packedValidation.SetUniformBufferStandardLayout(true);
    check(packedTools.Validate(guarded->data(),guarded->size(),packedValidation));
    check(admitted.compile(packedCode)==guarded&&admitted.stats().hits==1);
    packedOptions.metadataReadOnly=true;GuardCompiler readOnly(4,1024*1024,packedOptions);
    auto readOnlyCode=readOnly.compile(packedCode);
    check(packedTools.Validate(readOnlyCode->data(),readOnlyCode->size(),packedValidation));
    unsigned readOnlyDecorations=0;
    for(size_t p=5;p<readOnlyCode->size();p+=(*readOnlyCode)[p]>>16)
        if(((*readOnlyCode)[p]&65535)==71&&((*readOnlyCode)[p]>>16)==3&&(*readOnlyCode)[p+2]==24)++readOnlyDecorations;
    check(readOnlyDecorations==1);
    packedOptions.optimizeMetadataReads=true;
    GuardCompiler optimized(4,1024*1024,packedOptions);
    auto optimizedCode=optimized.compile(packedCode);
    check(packedTools.Validate(optimizedCode->data(),optimizedCode->size(),packedValidation));
    unsigned functionCalls=0;
    for(size_t p=5;p<optimizedCode->size();p+=(*optimizedCode)[p]>>16)
        functionCalls+=((*optimizedCode)[p]&65535)==57;
    check(functionCalls==0);
    check(optimized.compile(packedCode)==optimizedCode&&optimized.stats().hits==1);
    std::cout<<"GUARD_CSE_STRUCTURE baselineWords="<<readOnlyCode->size()<<" optimizedWords="<<optimizedCode->size()<<" functionCalls="<<functionCalls<<"\n";
    std::cout<<"STATIC_USE_PASS checks="<<checks<<" rejections="<<rejections<<"\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
