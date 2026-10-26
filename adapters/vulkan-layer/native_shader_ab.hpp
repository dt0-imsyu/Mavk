// SPDX-License-Identifier: Apache-2.0
#pragma once
// Diagnostic eligibility only, NOT robustness2 emulation or feature advertising.
// Both native robustness1 features must already be enabled by the application.
// Exclude nullable descriptors at draw time, descriptor arrays (except unchanged
// native sampler heaps), resource writes/atomics and unclassified pointer stores.
bool nativeShaderEligible(const std::vector<uint32_t>& words,const PipelineSchema& schema,uint32_t& reason,bool resourceWrites=false){
    auto reject=[&](uint32_t code){reason=code;return false;};
    std::map<uint32_t,std::pair<uint32_t,uint32_t>> pointers;
    std::map<uint32_t,uint32_t> sets,results;std::set<uint32_t> arrays,zeroes;
    for(size_t p=5;p<words.size();p+=words[p]>>16){
        const auto n=words[p]>>16,op=words[p]&65535;
        if(!n||p+n>words.size())return reject(96000);
        if(op==SpvOpTypePointer)pointers[words[p+1]]={words[p+2],words[p+3]};
        if(op==SpvOpTypePointer&&(words[p+2]==SpvStorageClassPhysicalStorageBuffer||words[p+2]==SpvStorageClassGeneric))return reject(97000);
        if(op==SpvOpTypeImage&&words[p+3]==SpvDimBuffer)return reject(90005);
        if((op==SpvOpConstant&&n==4&&words[p+3]==0)||(op==SpvOpConstantNull&&n==3))zeroes.insert(words[p+2]);
        if(op==SpvOpTypeArray||op==SpvOpTypeRuntimeArray)arrays.insert(words[p+1]);
        if(op==SpvOpDecorate&&n==4&&words[p+2]==SpvDecorationDescriptorSet)sets[words[p+1]]=words[p+3];
        if((op==SpvOpImageWrite&&!resourceWrites)||op==SpvOpImageTexelPointer||op==SpvOpImageSparseFetch||op==SpvOpAtomicFlagTestAndSet||op==SpvOpAtomicFlagClear||(op>=SpvOpAtomicLoad&&op<=SpvOpAtomicXor)||(op>=5000&&op!=SpvOpDemoteToHelperInvocationEXT&&op!=SpvOpIsHelperInvocationEXT))return reject(op);
    }
    for(size_t p=5;p<words.size();p+=words[p]>>16){
        const auto n=words[p]>>16,op=words[p]&65535;
        if(n>=3&&pointers.count(words[p+1]))switch(op){
        case SpvOpVariable:case SpvOpAccessChain:case SpvOpInBoundsAccessChain:case SpvOpPtrAccessChain:case SpvOpInBoundsPtrAccessChain:case SpvOpFunctionParameter:case SpvOpCopyObject:case SpvOpPhi:case SpvOpSelect:results[words[p+2]]=words[p+1];break;
        default:break;
        }
    }
    for(auto [variable,set]:sets){
        if(set>=schema.sets.size())return reject(95000);
        if(schema.sets[set].nativeSampler)continue; // Already passed through without nullable metadata.
        auto type=results.find(variable);if(type==results.end())return reject(93000);
        if(arrays.count(pointers.at(type->second).second))return reject(94000);
    }
    for(size_t p=5;p<words.size();p+=words[p]>>16){
        const auto n=words[p]>>16,op=words[p]&65535;
        // Native image robustness1 leaves invalid LOD undefined. Only constant
        // zero LOD is admitted; every valid bound image view has mip level zero.
        if(op==SpvOpImageQuerySizeLod&&(n!=5||!zeroes.count(words[p+4])))return reject(op);
        if(op==SpvOpImageFetch||op==SpvOpImageRead){
            const auto mask=n>5?words[p+5]:0;
            if(mask&(SpvImageOperandsBiasMask|SpvImageOperandsGradMask|SpvImageOperandsSampleMask))return reject(op);
            if(op==SpvOpImageFetch&&!(mask&SpvImageOperandsLodMask))return reject(op);
            if((mask&SpvImageOperandsLodMask)&&(n<7||!zeroes.count(words[p+6])))return reject(op);
        }
        if(op==SpvOpImageWrite){
            const auto mask=n>4?words[p+4]:0;
            if(mask&(SpvImageOperandsBiasMask|SpvImageOperandsGradMask|SpvImageOperandsSampleMask))return reject(op);
            if((mask&SpvImageOperandsLodMask)&&(n<6||!zeroes.count(words[p+5])))return reject(op);
        }
        if(op==SpvOpStore||op==SpvOpCopyMemory||op==SpvOpCopyMemorySized){
            auto type=results.find(words[p+1]);if(type==results.end())return reject(92000);
            const auto storage=pointers.at(type->second).first;
            if(storage!=SpvStorageClassOutput&&storage!=SpvStorageClassFunction&&storage!=SpvStorageClassPrivate&&!(resourceWrites&&storage==SpvStorageClassStorageBuffer))return reject(91000+storage);
        }
    }
    return true;
}
void prepareNativeShaderAB(Device& state,const VkGraphicsPipelineCreateInfo& ci,const std::shared_ptr<PipelineShadow>& shadow){
    if(state.nativeShaderABPath.empty()||!shadow||!shadow->owner||state.nativeShaderVariants>=512)return;
    try{
        for(auto p=static_cast<const VkBaseInStructure*>(ci.pNext);p;p=p->pNext)if(p->sType!=VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO&&p->sType!=VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO){if(++state.nativeShaderRejections[uint32_t(p->sType)]==1)std::fprintf(stderr,"MAVK_NATIVE_SHADER_AB rejectedChain=%u\n",unsigned(p->sType));return;}
        PipelineShadow original;
        for(uint32_t i=0;i<ci.stageCount;i++){
            auto source=pipelineStageWords(state,ci.pStages[i]);
            uint32_t reason=0;if(!nativeShaderEligible(*source,*shadow->schema,reason)){if(++state.nativeShaderRejections[reason]==1)std::fprintf(stderr,"MAVK_NATIVE_SHADER_AB rejectedCode=%u stage=%u\n",reason,unsigned(ci.pStages[i].stage));return;}
            original.shaders.push_back(std::move(source));
        }
        std::vector<std::shared_ptr<ShaderOwner>> modules;
        auto stages=instrumentStages(state,original,ci.stageCount,ci.pStages,modules);
        auto plain=ci;plain.pStages=stages.data();
        auto owner=std::make_shared<PipelineOwner>();owner->device=state.handle;owner->destroy=state.fn<PFN_vkDestroyPipeline>("vkDestroyPipeline");
        const auto result=state.fn<PFN_vkCreateGraphicsPipelines>("vkCreateGraphicsPipelines")(state.handle,VK_NULL_HANDLE,1,&plain,nullptr,&owner->handle);
        owner->owns=owner->handle!=VK_NULL_HANDLE;
        if(result!=VK_SUCCESS){std::fprintf(stderr,"MAVK_NATIVE_SHADER_AB pipelineUnavailable=%d\n",result);return;}
        owner->owns=true;shadow->nativeDiagnostic=std::move(owner);
        if(++state.nativeShaderVariants<=4)std::fprintf(stderr,"MAVK_NATIVE_SHADER_AB variant=%llu readOnly=1 nativeRobustness1Only=1\n",(unsigned long long)state.nativeShaderVariants);
    }catch(const std::exception& e){std::fprintf(stderr,"MAVK_NATIVE_SHADER_AB rejected=%s\n",e.what());}
}
void prepareNativeComputeAB(Device& state,const VkComputePipelineCreateInfo& ci,const std::shared_ptr<PipelineShadow>& shadow){
    if(state.nativeShaderABPath.empty()||!shadow||!shadow->owner||state.nativeShaderVariants>=512)return;
    try{
        for(auto p=static_cast<const VkBaseInStructure*>(ci.pNext);p;p=p->pNext)if(p->sType!=VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO)return;
        auto source=pipelineStageWords(state,ci.stage);uint32_t reason=0;
        if(!nativeShaderEligible(*source,*shadow->schema,reason,true)){if(++state.nativeShaderRejections[reason]==1)std::fprintf(stderr,"MAVK_NATIVE_SHADER_AB rejectedCode=%u stage=%u\n",reason,unsigned(ci.stage.stage));return;}
        PipelineShadow original;original.shaders.push_back(std::move(source));std::vector<std::shared_ptr<ShaderOwner>> modules;
        auto stages=instrumentStages(state,original,1,&ci.stage,modules);auto plain=ci;plain.stage=stages[0];
        auto owner=std::make_shared<PipelineOwner>();owner->device=state.handle;owner->destroy=state.fn<PFN_vkDestroyPipeline>("vkDestroyPipeline");
        const auto result=state.fn<PFN_vkCreateComputePipelines>("vkCreateComputePipelines")(state.handle,VK_NULL_HANDLE,1,&plain,nullptr,&owner->handle);owner->owns=owner->handle!=VK_NULL_HANDLE;
        if(result!=VK_SUCCESS){std::fprintf(stderr,"MAVK_NATIVE_SHADER_AB computeUnavailable=%d\n",result);return;}
        shadow->nativeDiagnostic=std::move(owner);++state.nativeShaderVariants;
    }catch(const std::exception& e){std::fprintf(stderr,"MAVK_NATIVE_SHADER_AB computeRejected=%s\n",e.what());}
}
