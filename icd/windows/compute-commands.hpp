// Restricted single storage-buffer compute pipeline, synchronous submissions.
API VkResult VKAPI_CALL vkCreateComputePipelines(VkDevice,VkPipelineCache,uint32_t n,const VkComputePipelineCreateInfo* ci,const VkAllocationCallbacks* a,VkPipeline* out){
    if(n!=1)return Unsupported;*out=VK_NULL_HANDLE;const auto& p=*ci;const auto& s=p.stage;
    if(a||p.pNext||p.flags||p.basePipelineHandle||p.basePipelineIndex!=-1||!p.layout||!as<Layout>(p.layout)->storage||s.sType!=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO||s.pNext||s.flags||s.stage!=VK_SHADER_STAGE_COMPUTE_BIT||s.pSpecializationInfo||!s.pName||std::strcmp(s.pName,"main")||!s.module)return Unsupported;
    try{auto* shader=as<Shader>(s.module);std::vector<uint32_t> words(shader->code.size()/4);std::memcpy(words.data(),shader->code.data(),shader->code.size());uint32_t local{};if(!mavk::bridge::computeShape(words,local))return Unsupported;
        auto pipeline=std::make_unique<Pipeline>();pipeline->pushLayout=*as<Layout>(p.layout);pipeline->cs=std::make_unique<Shader>(*shader);*out=handle<VkPipeline>(pipeline.release());return VK_SUCCESS;
    }catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}
}
API void VKAPI_CALL vkCmdDispatch(VkCommandBuffer h,uint32_t x,uint32_t y,uint32_t z){
    auto* c=as<Command>(h);
    if(!c->recording||c->computeGroups||c->drew||c->frame||c->inPass||!c->copies.empty()||!c->imageOps.empty()||!c->pipeline||!c->pipeline->cs||!c->descriptor||!x||x>256||y!=1||z!=1||c->pushBytes!=c->pipeline->pushLayout.bytes||c->pushStages!=c->pipeline->pushLayout.stages){c->bad=true;return;}
    c->computeGroups=x;
}
namespace {
VkResult executeCompute(Command* c,Device* d){
    if(!c->pipeline||!c->pipeline->cs||!c->copies.empty()||!c->imageOps.empty()||c->frame)return Unsupported;
    std::vector<uint8_t> input;if(!uniformSnapshot(c,input,true))return Unsupported;
    std::vector<uint8_t> payload;put32(payload,uint32_t(c->pipeline->cs->code.size()));put32(payload,uint32_t(input.size()));put32(payload,c->pushBytes);put32(payload,c->computeGroups);put32(payload,1);put32(payload,1);
    payload.insert(payload.end(),c->pipeline->cs->code.begin(),c->pipeline->cs->code.end());payload.insert(payload.end(),c->push,c->push+c->pushBytes);payload.insert(payload.end(),input.begin(),input.end());
    auto output=d->physical->instance->wire->exchange(18,payload);if(output.size()!=input.size())return VK_ERROR_DEVICE_LOST;
    auto* set=c->descriptor;std::memcpy(set->buffer->memory->bytes.data()+set->buffer->offset+set->offset,output.data(),output.size());return VK_SUCCESS;
}
}
