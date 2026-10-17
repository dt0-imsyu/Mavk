// Bounded set0/binding0 vertex UBO OR compute SSBO; no dynamic offsets/textures.
API VkResult VKAPI_CALL vkCreateDescriptorPool(VkDevice h,const VkDescriptorPoolCreateInfo* ci,const VkAllocationCallbacks* a,VkDescriptorPool* out){
#ifdef MAVK_EXPERIMENTAL_SAMPLED
    *out=VK_NULL_HANDLE;if(!a&&!ci->pNext&&!(ci->flags&~VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)&&ci->maxSets&&ci->maxSets<=16&&(ci->poolSizeCount==2||(UiLayersEnabled&&ci->poolSizeCount==1&&ci->pPoolSizes&&ci->pPoolSizes[0].type==VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)))return sampledPool(as<Device>(h),*ci,out);
#endif
    *out=VK_NULL_HANDLE;if(a||ci->pNext||(ci->flags&~VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)||!ci->maxSets||ci->maxSets>16||ci->poolSizeCount!=1||!ci->pPoolSizes||(ci->pPoolSizes[0].type!=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER&&ci->pPoolSizes[0].type!=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)||ci->pPoolSizes[0].descriptorCount<ci->maxSets||ci->pPoolSizes[0].descriptorCount>16)return Unsupported;
    try{auto pool=std::make_unique<DescriptorPool>();pool->device=as<Device>(h);pool->limit=ci->maxSets;pool->storage=ci->pPoolSizes[0].type==VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;pool->freeable=(ci->flags&VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)!=0;pool->sets.reserve(ci->maxSets);*out=handle<VkDescriptorPool>(pool.release());return VK_SUCCESS;}catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}
}
API void VKAPI_CALL vkDestroyDescriptorPool(VkDevice,VkDescriptorPool h,const VkAllocationCallbacks*){delete as<DescriptorPool>(h);}
API VkResult VKAPI_CALL vkResetDescriptorPool(VkDevice h,VkDescriptorPool p,VkDescriptorPoolResetFlags flags){auto* pool=as<DescriptorPool>(p);if(flags||pool->device!=as<Device>(h))return Unsupported;pool->sets.clear();return VK_SUCCESS;}
API VkResult VKAPI_CALL vkAllocateDescriptorSets(VkDevice h,const VkDescriptorSetAllocateInfo* ci,VkDescriptorSet* out){
    if(ci->descriptorSetCount>16)return Unsupported;for(uint32_t i=0;i<ci->descriptorSetCount;i++)out[i]=VK_NULL_HANDLE;
    auto* pool=as<DescriptorPool>(ci->descriptorPool);if(ci->pNext||!ci->descriptorSetCount||pool->device!=as<Device>(h))return Unsupported;
    if(ci->descriptorSetCount>pool->limit-pool->sets.size())return VK_ERROR_OUT_OF_POOL_MEMORY;
    if(pool->fontOnly&&ci->descriptorSetCount>pool->samplerCapacity-pool->sets.size())return VK_ERROR_OUT_OF_POOL_MEMORY;
    for(uint32_t i=0;i<ci->descriptorSetCount;i++){auto* l=as<EmptyDescriptorLayout>(ci->pSetLayouts[i]);if((pool->fontOnly?(!l->sampled||l->uniform||l->storage||l->sampledBinding!=0):(pool->storage?!l->storage:!l->uniform))||pool->sampled!=l->sampled)return Unsupported;}
    try{
        std::vector<std::unique_ptr<DescriptorSet>> pending;pending.reserve(ci->descriptorSetCount);
        for(uint32_t i=0;i<ci->descriptorSetCount;i++)pending.push_back(std::make_unique<DescriptorSet>(DescriptorSet{pool}));
        for(uint32_t i=0;i<ci->descriptorSetCount;i++){out[i]=handle<VkDescriptorSet>(pending[i].get());pool->sets.push_back(std::move(pending[i]));}return VK_SUCCESS;
    }catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}
}
API VkResult VKAPI_CALL vkFreeDescriptorSets(VkDevice h,VkDescriptorPool p,uint32_t n,const VkDescriptorSet* sets){
    auto* pool=as<DescriptorPool>(p);if(!pool->freeable||pool->device!=as<Device>(h)||n>16)return Unsupported;
    for(uint32_t i=0;i<n;i++){
        if(!sets[i]||as<DescriptorSet>(sets[i])->pool!=pool)return Unsupported;
        for(uint32_t j=0;j<i;j++)if(sets[i]==sets[j])return Unsupported;
    }
    for(uint32_t i=0;i<n;i++)pool->sets.erase(std::remove_if(pool->sets.begin(),pool->sets.end(),[&](const auto& s){return s.get()==as<DescriptorSet>(sets[i]);}),pool->sets.end());return VK_SUCCESS;
}
API void VKAPI_CALL vkUpdateDescriptorSets(VkDevice h,uint32_t n,const VkWriteDescriptorSet* writes,uint32_t copies,const VkCopyDescriptorSet* copyInfo){
    if(n>16||copies){for(uint32_t i=0;i<std::min(n,16u);i++)if(writes[i].dstSet)as<DescriptorSet>(writes[i].dstSet)->bad=true;for(uint32_t i=0;i<std::min(copies,16u);i++)if(copyInfo&&copyInfo[i].dstSet)as<DescriptorSet>(copyInfo[i].dstSet)->bad=true;return;}
    for(uint32_t i=0;i<n;i++){
#ifdef MAVK_EXPERIMENTAL_SAMPLED
        auto* typed=as<DescriptorSet>(writes[i].dstSet);if(typed&&typed->pool->sampled){if(!sampledWriteValid(as<Device>(h),writes[i])){typed->bad=true;return;}continue;}
#endif
        const auto& w=writes[i];auto* set=as<DescriptorSet>(w.dstSet);bool storage=set&&set->pool->storage;bool bad=!set||set->pool->device!=as<Device>(h)||w.pNext||w.dstBinding||w.dstArrayElement||w.descriptorCount!=1||w.descriptorType!=(storage?VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)||!w.pBufferInfo;
        if(!bad){auto& info=*w.pBufferInfo;auto* b=as<Buffer>(info.buffer);bad=!b||!(b->usage&(storage?VK_BUFFER_USAGE_STORAGE_BUFFER_BIT:VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT))||info.offset%16||info.offset>=b->bytes;
            if(!bad){uint64_t range=info.range==VK_WHOLE_SIZE?b->bytes-info.offset:info.range;bad=!range||range>(storage?524288u:4096u)||range%(storage?4:16)||range>b->bytes-info.offset;}
        }
        if(bad){if(set)set->bad=true;return;}
    }
    for(uint32_t i=0;i<n;i++){auto* set=as<DescriptorSet>(writes[i].dstSet);
#ifdef MAVK_EXPERIMENTAL_SAMPLED
        if(set->pool->sampled){applySampledWrite(writes[i]);continue;}
#endif
        const auto& info=*writes[i].pBufferInfo;set->buffer=as<Buffer>(info.buffer);set->offset=info.offset;set->range=info.range==VK_WHOLE_SIZE?set->buffer->bytes-info.offset:info.range;set->bad=false;}
}
API void VKAPI_CALL vkCmdBindDescriptorSets(VkCommandBuffer h,VkPipelineBindPoint bind,VkPipelineLayout layout,uint32_t first,uint32_t n,const VkDescriptorSet* sets,uint32_t dynamicCount,const uint32_t*){
    auto* c=as<Command>(h);
    auto* l=as<Layout>(layout);bool storage=l->storage;
    if(n==1&&sets&&sets[0]&&l->sampled!=as<DescriptorSet>(sets[0])->pool->sampled){c->bad=true;return;}
    if(n==1&&sets&&sets[0]&&l->sampled&&l->sampledBinding!=(as<DescriptorSet>(sets[0])->pool->fontOnly?0u:1u)){c->bad=true;return;}
    if(!c->recording||drawStateFrozen(c)||c->computeGroups||bind!=(storage?VK_PIPELINE_BIND_POINT_COMPUTE:VK_PIPELINE_BIND_POINT_GRAPHICS)||(!storage&&!l->uniform&&!(UiLayersEnabled&&l->sampled&&l->sampledBinding==0))||first||n!=1||dynamicCount||!sets||!sets[0]||as<DescriptorSet>(sets[0])->pool->device!=c->device||as<DescriptorSet>(sets[0])->pool->storage!=storage){c->bad=true;return;}
    c->descriptor=as<DescriptorSet>(sets[0]);
}
namespace {
bool uniformSnapshot(Command* c,std::vector<uint8_t>& out,bool storage=false){
    auto* set=c->descriptor;if(!set||set->bad||set->pool->storage!=storage||!set->buffer||!set->buffer->memory)return false;
    auto* b=set->buffer;if(b->memory->device!=c->device||set->offset>b->bytes||set->range>b->bytes-set->offset||b->offset>b->memory->bytes.size()||b->bytes>b->memory->bytes.size()-b->offset)return false;
    auto begin=b->memory->bytes.begin()+size_t(b->offset+set->offset);out.assign(begin,begin+size_t(set->range));return true;
}
}
