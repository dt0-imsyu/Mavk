#include <mavk/compat/cull_distance.hpp>
#include <spirv-tools/libspirv.hpp>
#include <spirv/unified1/spirv.hpp>
#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace mavk::compat {
namespace {
struct Inst {size_t at;uint32_t op,n;};
struct Module {
    const std::vector<uint32_t>& code;std::vector<Inst> instructions;
    std::map<uint32_t,Inst> definitions;
    std::map<uint32_t,std::map<uint32_t,uint32_t>> decorations;
    std::map<std::pair<uint32_t,uint32_t>,std::map<uint32_t,uint32_t>> members;
    std::set<uint32_t> outputs;
    explicit Module(const std::vector<uint32_t>& words,uint32_t stage,spv_target_env env):code(words){
        if(code.size()<5||code.size()>262144)throw std::invalid_argument("bounded cull module required");
        spvtools::SpirvTools validator(env);std::string error;
        validator.SetMessageConsumer([&](auto,const char*,const auto&,const char* text){if(error.size()<4096)error+=text;});
        if(!validator.Validate(code))throw std::invalid_argument("invalid cull source: "+error);
        uint32_t entries=0;
        for(size_t at=5;at<code.size();){
            Inst i{at,code[at]&65535,code[at]>>16};instructions.push_back(i);
            if(i.op==spv::OpEntryPoint){if(code[at+1]!=stage)throw std::invalid_argument("cull stage mismatch");entries++;}
            if(i.op==spv::OpDecorationGroup||i.op==spv::OpGroupDecorate||i.op==spv::OpGroupMemberDecorate)
                throw std::invalid_argument("grouped cull interfaces not admitted");
            if(i.op==spv::OpDecorate){auto key=code[at+2];if(i.n>4)throw std::invalid_argument("extended interface decoration not admitted");decorations[code[at+1]][key]=i.n==4?code[at+3]:0;}
            if(i.op==spv::OpMemberDecorate){if(i.n>5)throw std::invalid_argument("extended member decoration not admitted");members[{code[at+1],code[at+2]}][code[at+3]]=i.n==5?code[at+4]:0;}
            if((i.op>=spv::OpTypeVoid&&i.op<=spv::OpTypeForwardPointer)||i.op==spv::OpConstant)definitions.emplace(code[at+1+(i.op==spv::OpConstant)],i);
            if(i.op==spv::OpVariable&&code[at+3]==spv::StorageClassOutput)outputs.insert(code[at+2]);
            if(i.op==spv::OpCapability&&(code[at+1]==spv::CapabilityTransformFeedback||code[at+1]==spv::CapabilityMultiView))throw std::invalid_argument("XFB/multiview cull not admitted");
            at+=i.n;
        }
        if(entries!=1)throw std::invalid_argument("single cull entry point required");
    }
    Inst type(uint32_t id) const {auto at=definitions.find(id);if(at==definitions.end())throw std::invalid_argument("missing cull interface type");return at->second;}
    uint32_t length(uint32_t id) const {auto i=type(id);if(i.op!=spv::OpConstant||i.n!=4||!code[i.at+3]||code[i.at+3]>32)throw std::invalid_argument("bounded fixed cull interface array required");return code[i.at+3];}
};
struct Field {uint32_t type,location,builtin=UINT32_MAX;std::map<uint32_t,uint32_t> decorations;int block=-1;uint32_t member=0;bool inheritedLocation=false;};
struct Types {
    const Module& m;std::ostringstream text;std::set<uint32_t> emitted;
    uint32_t components(uint32_t id,uint32_t depth=0){if(depth>8)throw std::invalid_argument("nested cull interface depth");auto i=m.type(id);auto p=i.at;
        if(i.op==spv::OpTypeFloat&&m.code[p+2]==32)return 1;
        if(i.op==spv::OpTypeInt&&m.code[p+2]==32)return 1;
        if(i.op==spv::OpTypeVector||i.op==spv::OpTypeArray){const auto count=components(m.code[p+2],depth+1)*(i.op==spv::OpTypeVector?m.code[p+3]:m.length(m.code[p+3]));if(count>128)throw std::invalid_argument("bounded cull component count");return count;}
        throw std::invalid_argument("cull interface requires 32bit numeric scalar/vector/array");
    }
    uint32_t locations(uint32_t id){auto i=m.type(id);return i.op==spv::OpTypeArray?locations(m.code[i.at+2])*m.length(m.code[i.at+3]):1;}
    std::string emit(uint32_t id){auto i=m.type(id);auto p=i.at;if(i.op==spv::OpTypeInt&&m.code[p+2]==32&&m.code[p+3]==0)return "%u";auto name="%t"+std::to_string(id);if(!emitted.insert(id).second)return name;
        (void)components(id);
        if(i.op==spv::OpTypeFloat)text<<name<<" = OpTypeFloat 32\n";
        else if(i.op==spv::OpTypeInt)text<<name<<" = OpTypeInt 32 "<<m.code[p+3]<<"\n";
        else if(i.op==spv::OpTypeVector){auto nested=emit(m.code[p+2]);text<<name<<" = OpTypeVector "<<nested<<' '<<m.code[p+3]<<"\n";}
        else if(i.op==spv::OpTypeArray){auto nested=emit(m.code[p+2]);text<<"%len"<<id<<" = OpConstant %u "<<m.length(m.code[p+3])<<"\n"<<name<<" = OpTypeArray "<<nested<<" %len"<<id<<"\n";}
        return name;
    }
};
}
CullDistanceProgram compileCullDistance(const std::vector<uint32_t>& vertex,const std::vector<uint32_t>& fragment,CullPrimitive primitive,GuardTarget target,CullProducer producer){
    if(target!=GuardTarget::Vulkan11&&target!=GuardTarget::Vulkan12&&target!=GuardTarget::Vulkan13)throw std::invalid_argument("unsupported cull target");
    const auto env=target==GuardTarget::Vulkan13?SPV_ENV_VULKAN_1_3:target==GuardTarget::Vulkan12?SPV_ENV_VULKAN_1_2:SPV_ENV_VULKAN_1_1;
    if(producer!=CullProducer::Vertex&&producer!=CullProducer::TessellationEvaluation)throw std::invalid_argument("unsupported cull producer");
    Module vs(vertex,producer==CullProducer::Vertex?spv::ExecutionModelVertex:spv::ExecutionModelTessellationEvaluation,env),fs(fragment,spv::ExecutionModelFragment,env);
    if(producer==CullProducer::TessellationEvaluation){
        bool domain=false;for(auto i:vs.instructions)if(i.op==spv::OpExecutionMode&&i.n>=3){const auto mode=vertex[i.at+2];
            if(mode==spv::ExecutionModePointMode||mode==spv::ExecutionModeIsolines)throw std::invalid_argument("tessellation point/isoline cull not admitted");
            if(mode==spv::ExecutionModeTriangles||mode==spv::ExecutionModeQuads)domain=true;
        }
        if(!domain||primitive!=CullPrimitive::Triangles)throw std::invalid_argument("TES triangle/quad cull producer required");
    }
    bool primitiveId=false;uint32_t primitiveIdSigned=0;
    auto recordPrimitiveId=[&](uint32_t type){auto t=fs.type(type);if(primitiveId||t.op!=spv::OpTypeInt||fragment[t.at+2]!=32)throw std::invalid_argument("single scalar32 fragment PrimitiveId required");primitiveId=true;primitiveIdSigned=fragment[t.at+3];};
    for(auto i:fs.instructions)if(i.op==spv::OpVariable&&fragment[i.at+3]==spv::StorageClassInput){
        auto pointer=fs.type(fragment[i.at+1]);auto value=fragment[pointer.at+3];auto type=fs.type(value);
        const auto& decorations=fs.decorations[fragment[i.at+2]];
        if(auto b=decorations.find(spv::DecorationBuiltIn);b!=decorations.end()&&b->second==spv::BuiltInPrimitiveId)recordPrimitiveId(value);
        if(type.op==spv::OpTypeStruct)for(uint32_t member=0;member<type.n-2;member++){
            const auto& decos=fs.members[{value,member}];if(auto b=decos.find(spv::DecorationBuiltIn);b!=decos.end()&&b->second==spv::BuiltInPrimitiveId)recordPrimitiveId(fragment[type.at+2+member]);
        }
    }
    for(auto& [id,decos]:fs.decorations)if(auto b=decos.find(spv::DecorationBuiltIn);b!=decos.end()&&b->second==spv::BuiltInCullDistance)throw std::invalid_argument("fragment cull-distance input not admitted");
    for(auto& [id,decos]:fs.members)if(auto b=decos.find(spv::DecorationBuiltIn);b!=decos.end()&&b->second==spv::BuiltInCullDistance)throw std::invalid_argument("fragment cull-distance member not admitted");
    uint32_t vertices=primitive==CullPrimitive::Points?1:primitive==CullPrimitive::Lines?2:primitive==CullPrimitive::Triangles?3:0;
    if(!vertices)throw std::invalid_argument("unsupported cull primitive");
    std::vector<Field> fields;std::vector<std::vector<uint32_t>> blocks;std::vector<uint32_t> blockLocations;std::map<size_t,uint32_t> patches;std::set<uint32_t> variableTypes;
    Types types{vs};uint32_t nextLocation=0,positionCount=0,cullIndex=UINT32_MAX;
    auto field=[&](uint32_t type,const std::map<uint32_t,uint32_t>& decorations){
        Field f{type,0,UINT32_MAX,decorations};(void)types.components(type);
        if(auto b=decorations.find(spv::DecorationBuiltIn);b!=decorations.end())f.builtin=b->second;
        else {auto l=decorations.find(spv::DecorationLocation);if(l==decorations.end())throw std::invalid_argument("cull output location absent");f.location=l->second;if(f.location>64||types.locations(type)>64-f.location)throw std::invalid_argument("bounded cull output location required");nextLocation=std::max(nextLocation,f.location+types.locations(type));}
        for(auto [key,value]:decorations)if(key!=spv::DecorationBuiltIn&&key!=spv::DecorationLocation&&key!=spv::DecorationFlat&&key!=spv::DecorationNoPerspective&&key!=spv::DecorationCentroid&&key!=spv::DecorationSample&&key!=spv::DecorationInvariant)throw std::invalid_argument("unsupported cull interface decoration");
        fields.push_back(f);
    };
    for(auto i:vs.instructions)if(i.op==spv::OpVariable&&vs.outputs.count(vertex[i.at+2])){
        auto pointer=vs.type(vertex[i.at+1]);auto value=vertex[pointer.at+3];auto t=vs.type(value);
        if(t.op==spv::OpTypeStruct){
            if(!vs.decorations[value].count(spv::DecorationBlock))throw std::invalid_argument("cull output structure must be Block");
            if(t.n<3)throw std::invalid_argument("empty cull interface block not admitted");
            blocks.emplace_back();
            const bool builtin=vs.members[{value,0}].count(spv::DecorationBuiltIn)!=0;
            const auto& variableDecorations=vs.decorations[vertex[i.at+2]];
            uint32_t cursor=UINT32_MAX;
            for(auto [key,location]:variableDecorations){if(key!=spv::DecorationLocation||builtin||location>64)throw std::invalid_argument("unsupported cull block variable decoration");cursor=location;}
            blockLocations.push_back(cursor);
            for(uint32_t member=0;member<t.n-2;member++){
                auto decorations=vs.members[{value,member}];
                if((decorations.count(spv::DecorationBuiltIn)!=0)!=builtin)throw std::invalid_argument("mixed builtin/user output block not admitted");
                if(!builtin&&decorations.count(spv::DecorationInvariant))throw std::invalid_argument("invariant user block requires separate input/output types");
                const bool inherited=!builtin&&!decorations.count(spv::DecorationLocation)&&cursor!=UINT32_MAX;
                if(inherited)decorations[spv::DecorationLocation]=cursor;
                field(vertex[t.at+2+member],decorations);fields.back().block=int(blocks.size()-1);fields.back().member=member;blocks.back().push_back(uint32_t(fields.size()-1));
                fields.back().inheritedLocation=inherited;
                if(!builtin)cursor=fields.back().location+types.locations(fields.back().type);
            }
            if(!variableTypes.insert(value).second)throw std::invalid_argument("shared output cull block type not admitted");
        }else {field(value,vs.decorations[vertex[i.at+2]]);variableTypes.insert(vertex[i.at+2]);}
    }
    if(fields.empty()||fields.size()>32||nextLocation>64)throw std::invalid_argument("bounded cull outputs required");
    CullDistanceProgram result;result.vertices=vertices;
    for(uint32_t n=0;n<fields.size();n++){
        auto& f=fields[n];result.inputComponents+=types.components(f.type);
        if(f.builtin!=UINT32_MAX){
            if(f.builtin!=spv::BuiltInPosition&&f.builtin!=spv::BuiltInPointSize&&f.builtin!=spv::BuiltInCullDistance)throw std::invalid_argument("unsupported pre-raster builtin in cull pipeline");
            f.location=nextLocation;nextLocation+=types.locations(f.type);
            if(f.builtin==spv::BuiltInPosition){auto t=vs.type(f.type);if(t.op!=spv::OpTypeVector||vertex[t.at+3]!=4||vs.type(vertex[t.at+2]).op!=spv::OpTypeFloat)throw std::invalid_argument("cull position must be vec4");positionCount++;}
            if(f.builtin==spv::BuiltInCullDistance){if(cullIndex!=UINT32_MAX)throw std::invalid_argument("ambiguous cull distance");cullIndex=n;auto a=vs.type(f.type);if(a.op!=spv::OpTypeArray||vs.type(vertex[a.at+2]).op!=spv::OpTypeFloat)throw std::invalid_argument("cull distance must be float array");result.distances=vs.length(vertex[a.at+3]);if(result.distances>8)throw std::invalid_argument("cull distance count exceeds8");}
        }
        if(f.builtin!=spv::BuiltInCullDistance)result.outputComponents+=types.components(f.type);
    }
    if(positionCount!=1||cullIndex==UINT32_MAX||nextLocation>64)throw std::invalid_argument("position and cull outputs required");
    result.locations=nextLocation;
    for(const auto& f:fields)if(f.builtin==spv::BuiltInPointSize)throw std::invalid_argument("geometry PointSize feature mediation not admitted");
    uint32_t fieldIndex=0;
    for(auto i:vs.instructions)if(i.op==spv::OpVariable&&vs.outputs.count(vertex[i.at+2])){
        auto pointer=vs.type(vertex[i.at+1]);auto t=vs.type(vertex[pointer.at+3]);
        if(t.op==spv::OpTypeStruct){for(uint32_t member=0;member<t.n-2;member++,fieldIndex++)if(fields[fieldIndex].builtin!=UINT32_MAX){for(auto d:vs.instructions)if(d.op==spv::OpMemberDecorate&&vertex[d.at+1]==vertex[pointer.at+3]&&vertex[d.at+2]==member&&vertex[d.at+3]==spv::DecorationBuiltIn)patches[d.at]=fields[fieldIndex].location;}}
        else {if(fields[fieldIndex].builtin!=UINT32_MAX)for(auto d:vs.instructions)if(d.op==spv::OpDecorate&&vertex[d.at+1]==vertex[i.at+2]&&vertex[d.at+2]==spv::DecorationBuiltIn)patches[d.at]=fields[fieldIndex].location;fieldIndex++;}
    }
    result.vertex.assign(vertex.begin(),vertex.begin()+5);
    for(auto i:vs.instructions){if(i.op==spv::OpCapability&&vertex[i.at+1]==spv::CapabilityCullDistance)continue;
        auto start=result.vertex.size();result.vertex.insert(result.vertex.end(),vertex.begin()+i.at,vertex.begin()+i.at+i.n);
        if(auto p=patches.find(i.at);p!=patches.end()){result.vertex[start+i.n-2]=spv::DecorationLocation;result.vertex[start+i.n-1]=p->second;}
    }
    std::ostringstream annotation,body,interface;
    std::string primitiveIdType=primitiveId&&!primitiveIdSigned?"%u":"";
    if(primitiveId){
        result.inputComponents++;result.outputComponents++;
        for(auto [id,t]:vs.definitions)if(t.op==spv::OpTypeInt&&vertex[t.at+2]==32&&vertex[t.at+3]==primitiveIdSigned){primitiveIdType=types.emit(id);break;}
        if(primitiveIdType.empty()){primitiveIdType="%pidType";types.text<<"%pidType = OpTypeInt 32 "<<primitiveIdSigned<<"\n";}
        interface<<" %primitiveIn %primitiveOut";annotation<<"OpDecorate %primitiveIn BuiltIn PrimitiveId\nOpDecorate %primitiveOut BuiltIn PrimitiveId\n";
        types.text<<"%primitiveInPtr = OpTypePointer Input "<<primitiveIdType<<"\n%primitiveOutPtr = OpTypePointer Output "<<primitiveIdType<<"\n%primitiveIn = OpVariable %primitiveInPtr Input\n%primitiveOut = OpVariable %primitiveOutPtr Output\n";
    }
    for(uint32_t n=0;n<fields.size();n++){
        const auto& f=fields[n];auto t=types.emit(f.type);auto suffix=std::to_string(n);
        if(f.block<0){interface<<" %in"<<n;annotation<<"OpDecorate %in"<<n<<" Location "<<f.location<<"\n";types.text<<"%arr"<<n<<" = OpTypeArray "<<t<<" %nv\n%pin"<<n<<" = OpTypePointer Input %arr"<<n<<"\n%in"<<n<<" = OpVariable %pin"<<n<<" Input\n";}
        types.text<<"%pelem"<<n<<" = OpTypePointer Input "<<t<<"\n";
        if(f.builtin==spv::BuiltInCullDistance)continue;
        if(f.block>=0&&f.builtin==UINT32_MAX){types.text<<"%pout"<<n<<" = OpTypePointer Output "<<t<<"\n";continue;}
        interface<<" %out"<<n;
        if(f.builtin==UINT32_MAX)annotation<<"OpDecorate %out"<<n<<" Location "<<f.location<<"\n";
        else annotation<<"OpDecorate %out"<<n<<" BuiltIn "<<(f.builtin==spv::BuiltInPosition?"Position":"PointSize")<<"\n";
        for(auto [key,value]:f.decorations)if(key==spv::DecorationFlat||key==spv::DecorationNoPerspective||key==spv::DecorationCentroid||key==spv::DecorationSample||key==spv::DecorationInvariant){const char* name=key==spv::DecorationFlat?"Flat":key==spv::DecorationNoPerspective?"NoPerspective":key==spv::DecorationCentroid?"Centroid":key==spv::DecorationSample?"Sample":"Invariant";annotation<<"OpDecorate %out"<<n<<' '<<name<<"\n";if(key!=spv::DecorationInvariant&&f.block<0)annotation<<"OpDecorate %in"<<n<<' '<<name<<"\n";}
        types.text<<"%pout"<<n<<" = OpTypePointer Output "<<t<<"\n%out"<<n<<" = OpVariable %pout"<<n<<" Output\n";
    }
    for(uint32_t b=0;b<blocks.size();b++){
        interface<<" %ib"<<b;annotation<<"OpDecorate %block"<<b<<" Block\n";
        if(blockLocations[b]!=UINT32_MAX)annotation<<"OpDecorate %ib"<<b<<" Location "<<blockLocations[b]<<"\nOpDecorate %ob"<<b<<" Location "<<blockLocations[b]<<"\n";
        types.text<<"%block"<<b<<" = OpTypeStruct";for(auto n:blocks[b])types.text<<' '<<types.emit(fields[n].type);types.text<<"\n";
        for(auto n:blocks[b]){const auto& f=fields[n];if(!f.inheritedLocation)annotation<<"OpMemberDecorate %block"<<b<<' '<<f.member<<" Location "<<f.location<<"\n";for(auto [key,value]:f.decorations)if(key==spv::DecorationFlat||key==spv::DecorationNoPerspective||key==spv::DecorationCentroid||key==spv::DecorationSample)annotation<<"OpMemberDecorate %block"<<b<<' '<<f.member<<' '<<(key==spv::DecorationFlat?"Flat":key==spv::DecorationNoPerspective?"NoPerspective":key==spv::DecorationCentroid?"Centroid":"Sample")<<"\n";}
        types.text<<"%barr"<<b<<" = OpTypeArray %block"<<b<<" %nv\n%bp"<<b<<" = OpTypePointer Input %barr"<<b<<"\n%ib"<<b<<" = OpVariable %bp"<<b<<" Input\n";
        if(fields[blocks[b].front()].builtin==UINT32_MAX){interface<<" %ob"<<b;types.text<<"%bop"<<b<<" = OpTypePointer Output %block"<<b<<"\n%ob"<<b<<" = OpVariable %bop"<<b<<" Output\n";}
    }
    auto input=[&](uint32_t n,uint32_t v){const auto& f=fields[n];return f.block<0?"%in"+std::to_string(n)+" %c"+std::to_string(v):"%ib"+std::to_string(f.block)+" %c"+std::to_string(v)+" %c"+std::to_string(f.member);};
    auto cullType=vs.type(fields[cullIndex].type);auto floatType=types.emit(vertex[cullType.at+2]);
    types.text<<"%pfloat = OpTypePointer Input "<<floatType<<"\n%zero = OpConstant "<<floatType<<" 0\n";
    for(uint32_t i=0;i<32;i++)types.text<<"%c"<<i<<" = OpConstant %u "<<i<<"\n";
    body<<"%main = OpFunction %void None %fn\n%entry = OpLabel\n";
    if(primitiveId)body<<"%primitive = OpLoad "<<primitiveIdType<<" %primitiveIn\nOpStore %primitiveOut %primitive\n";
    std::string reject="%false";
    for(uint32_t plane=0;plane<result.distances;plane++){
        std::string all="%true";
        for(uint32_t v=0;v<vertices;v++){
            const auto id=std::to_string(plane)+"_"+std::to_string(v);
            body<<"%ptr"<<id<<" = OpAccessChain %pfloat "<<input(cullIndex,v)<<" %c"<<plane<<"\n%value"<<id<<" = OpLoad "<<floatType<<" %ptr"<<id<<"\n%neg"<<id<<" = OpFOrdLessThan %bool %value"<<id<<" %zero\n%all"<<id<<" = OpLogicalAnd %bool "<<all<<" %neg"<<id<<"\n";all="%all"+id;
        }
        body<<"%reject"<<plane<<" = OpLogicalOr %bool "<<reject<<' '<<all<<"\n";reject="%reject"+std::to_string(plane);
    }
    body<<"OpSelectionMerge %end None\nOpBranchConditional "<<reject<<" %end %emit\n%emit = OpLabel\n";
    for(uint32_t v=0;v<vertices;v++){
        for(uint32_t n=0;n<fields.size();n++)if(n!=cullIndex){const auto& f=fields[n];auto id=std::to_string(v)+"_"+std::to_string(n);body<<"%src"<<id<<" = OpAccessChain %pelem"<<n<<' '<<input(n,v)<<"\n%loaded"<<id<<" = OpLoad "<<types.emit(f.type)<<" %src"<<id<<"\n";
            if(f.block>=0&&f.builtin==UINT32_MAX)body<<"%dst"<<id<<" = OpAccessChain %pout"<<n<<" %ob"<<f.block<<" %c"<<f.member<<"\nOpStore %dst"<<id<<" %loaded"<<id<<"\n";
            else body<<"OpStore %out"<<n<<" %loaded"<<id<<"\n";
        }
        if(primitiveId)body<<"OpStore %primitiveOut %primitive\n";
        body<<"OpEmitVertex\n";
    }
    body<<"OpEndPrimitive\nOpBranch %end\n%end = OpLabel\nOpReturn\nOpFunctionEnd\n";
    std::ostringstream text;text<<"OpCapability Shader\nOpCapability Geometry\nOpMemoryModel Logical GLSL450\nOpEntryPoint Geometry %main \"main\""<<interface.str()<<"\nOpExecutionMode %main "<<(vertices==1?"InputPoints":vertices==2?"InputLines":"Triangles")<<"\nOpExecutionMode %main "<<(vertices==1?"OutputPoints":vertices==2?"OutputLineStrip":"OutputTriangleStrip")<<"\nOpExecutionMode %main OutputVertices "<<vertices<<"\n"<<annotation.str()<<"%void = OpTypeVoid\n%fn = OpTypeFunction %void\n%u = OpTypeInt 32 0\n%nv = OpConstant %u "<<vertices<<"\n%bool = OpTypeBool\n%true = OpConstantTrue %bool\n%false = OpConstantFalse %bool\n"<<types.text.str()<<body.str();
    spvtools::SpirvTools tools(env);std::string errors;tools.SetMessageConsumer([&](auto,const char*,const auto&,const char* message){if(errors.size()<4096)errors+=message;});
    if(!tools.Validate(result.vertex)||!tools.Assemble(text.str(),&result.geometry)||!tools.Validate(result.geometry))throw std::invalid_argument("invalid generated cull program: "+errors);
    return result;
}
}
