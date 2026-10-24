struct CullPipelineStages {
    CullDistanceProgram program;
    std::array<VkShaderModuleCreateInfo,2> modules{};
    std::vector<VkPipelineShaderStageCreateInfo> stages;
};
bool prepareCullPipeline(Device& state,const VkGraphicsPipelineCreateInfo& ci,CullPipelineStages& output){
    if(!state.cullPipelines)return false;
    const VkPipelineShaderStageCreateInfo* vs=nullptr;const VkPipelineShaderStageCreateInfo* fs=nullptr;
    const VkPipelineShaderStageCreateInfo* control=nullptr;const VkPipelineShaderStageCreateInfo* evaluation=nullptr;
    for(uint32_t i=0;i<ci.stageCount;i++){
        const auto& s=ci.pStages[i];const VkPipelineShaderStageCreateInfo** slot=nullptr;
        if(s.stage==VK_SHADER_STAGE_VERTEX_BIT)slot=&vs;
        if(s.stage==VK_SHADER_STAGE_FRAGMENT_BIT)slot=&fs;
        if(s.stage==VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)slot=&control;
        if(s.stage==VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)slot=&evaluation;
        if(slot){if(*slot)throw std::invalid_argument("ambiguous cull pipeline stage");*slot=&s;}
    }
    const auto producer=evaluation?evaluation:vs;
    auto hasCull=[&](const VkPipelineShaderStageCreateInfo* stage){if(!stage)return false;bool found=false;auto code=pipelineStageWords(state,*stage);
        for(size_t p=5;p<code->size();){const auto n=(*code)[p]>>16;if(!n||n>code->size()-p)throw std::invalid_argument("malformed cull stage");if(((*code)[p]&65535)==17&&n==2&&(*code)[p+1]==33)found=true;p+=n;}return found;
    };
    const bool cull=hasCull(producer);
    if(evaluation&&(hasCull(vs)||hasCull(control)))throw std::invalid_argument("upstream tessellation cull interface not admitted");
    if(!cull)return false;
    if(!state.nativeGeometry||state.nativeMultiview||!ci.pInputAssemblyState||ci.pInputAssemblyState->pNext||ci.pInputAssemblyState->flags)
        throw std::invalid_argument("cull requires native geometry and ordinary VS/FS pipeline");
    if(!fs||!vs)throw std::invalid_argument("cull vertex/fragment stages missing");
    for(uint32_t i=0;i<ci.stageCount;i++)if(ci.pStages[i].pSpecializationInfo||ci.pStages[i].flags||shaderStageNativeChain(ci.pStages[i]))throw std::invalid_argument("specialized/extended cull stage not admitted");
    if(evaluation){
        if(!control||ci.stageCount!=4||!state.nativeTessellation||!ci.pTessellationState||ci.pTessellationState->pNext||ci.pTessellationState->flags||ci.pInputAssemblyState->topology!=VK_PRIMITIVE_TOPOLOGY_PATCH_LIST)
            throw std::invalid_argument("cull requires native static TCS/TES patch pipeline");
    }else if(control||ci.stageCount!=2||ci.pTessellationState)throw std::invalid_argument("cull requires native geometry and ordinary VS/FS pipeline");
    for(auto p=static_cast<const VkBaseInStructure*>(ci.pNext);p;p=p->pNext){
        if(p->sType!=VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO||reinterpret_cast<const VkPipelineRenderingCreateInfo*>(p)->viewMask)throw std::invalid_argument("extended/multiview cull pipeline not admitted");
    }
    if(ci.pDynamicState)for(uint32_t i=0;i<ci.pDynamicState->dynamicStateCount;i++)if(ci.pDynamicState->pDynamicStates[i]==VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY)throw std::invalid_argument("dynamic cull topology not admitted");
    CullPrimitive primitive;
    if(evaluation){
        bool triangles=false;
        for(const auto stage:{control,evaluation}){const auto code=pipelineStageWords(state,*stage);
            for(size_t p=5;p<code->size();){const auto n=(*code)[p]>>16;if(!n||n>code->size()-p)throw std::invalid_argument("malformed tessellation stage");
                if(((*code)[p]&65535)==16&&n>=3){const auto mode=(*code)[p+2];
                    if(mode==10||mode==25)throw std::invalid_argument("tessellation point/isoline cull not admitted");
                    if(mode==22||mode==24){if(stage==control)throw std::invalid_argument("tessellation control topology declaration not admitted");triangles=true;}
                }p+=n;
            }
        }
        if(!triangles)throw std::invalid_argument("TES triangle/quad mode required");primitive=CullPrimitive::Triangles;
    }else switch(ci.pInputAssemblyState->topology){
    case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:primitive=CullPrimitive::Points;break;
    case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:primitive=CullPrimitive::Lines;break;
    case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:primitive=CullPrimitive::Triangles;break;
    default:throw std::invalid_argument("cull topology not admitted");
    }
    const auto target=state.instance->apiVersion>=VK_API_VERSION_1_3?GuardTarget::Vulkan13:state.instance->apiVersion>=VK_API_VERSION_1_2?GuardTarget::Vulkan12:GuardTarget::Vulkan11;
    output.program=compileCullDistance(*pipelineStageWords(state,*producer),*pipelineStageWords(state,*fs),primitive,target,evaluation?CullProducer::TessellationEvaluation:CullProducer::Vertex);
    const auto& p=output.program;const auto& limits=state.core->properties().limits;
    if(p.inputComponents>limits.maxGeometryInputComponents||p.outputComponents>limits.maxGeometryOutputComponents||p.vertices>limits.maxGeometryOutputVertices||uint64_t(p.vertices)*p.outputComponents>limits.maxGeometryTotalOutputComponents||uint64_t(p.locations)*4>(evaluation?limits.maxTessellationEvaluationOutputComponents:limits.maxVertexOutputComponents)||uint64_t(p.locations)*4>limits.maxGeometryInputComponents)
        throw std::invalid_argument("native cull geometry limits exceeded");
    for(auto& module:output.modules)module.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    output.modules[0].codeSize=p.vertex.size()*4;output.modules[0].pCode=p.vertex.data();
    output.modules[1].codeSize=p.geometry.size()*4;output.modules[1].pCode=p.geometry.data();
    output.stages.assign(ci.pStages,ci.pStages+ci.stageCount);
    const auto producerIndex=size_t(producer-ci.pStages);output.stages[producerIndex].module=VK_NULL_HANDLE;output.stages[producerIndex].pNext=&output.modules[0];
    output.stages.push_back({VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO});
    auto& geometry=output.stages.back();geometry.stage=VK_SHADER_STAGE_GEOMETRY_BIT;geometry.pName="main";geometry.pNext=&output.modules[1];
    std::fprintf(stderr,"MAVK_LAYER_CULL_LINK originalStages=%u nativeStages=%zu planes=%u vertices=%u nativeLimits=checked publicFeature=0\n",ci.stageCount,output.stages.size(),p.distances,p.vertices);
    return true;
}
