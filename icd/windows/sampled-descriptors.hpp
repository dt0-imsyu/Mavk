// Opt-in set0: binding0 vertex UBO + binding1 fragment combined image sampler.
namespace {
VkResult sampledLayout(const VkDescriptorSetLayoutCreateInfo& ci,VkDescriptorSetLayout* out){
    if(!ci.pBindings)return Unsupported;bool buffer=false,image=false;
    if(UiLayersEnabled&&ci.bindingCount==1){const auto& b=ci.pBindings[0];if(b.binding||b.descriptorType!=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER||b.descriptorCount!=1||b.stageFlags!=VK_SHADER_STAGE_FRAGMENT_BIT||b.pImmutableSamplers)return Unsupported;try{*out=handle<VkDescriptorSetLayout>(new EmptyDescriptorLayout{false,false,true,0});return VK_SUCCESS;}catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}}
    for(uint32_t n=0;n<2;n++){
        const auto& b=ci.pBindings[n];if(b.descriptorCount!=1||b.pImmutableSamplers)return Unsupported;
        if(b.binding==0&&b.descriptorType==VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER&&b.stageFlags==VK_SHADER_STAGE_VERTEX_BIT&&!buffer)buffer=true;
        else if(b.binding==1&&b.descriptorType==VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER&&b.stageFlags==VK_SHADER_STAGE_FRAGMENT_BIT&&!image)image=true;
        else return Unsupported;
    }
    if(!buffer||!image)return Unsupported;
    try{*out=handle<VkDescriptorSetLayout>(new EmptyDescriptorLayout{true,false,true});return VK_SUCCESS;}catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}
}
VkResult sampledPool(Device* device,const VkDescriptorPoolCreateInfo& ci,VkDescriptorPool* out){
    if(!ci.pPoolSizes)return Unsupported;bool buffer=false,image=false;
    if(UiLayersEnabled&&ci.poolSizeCount==1){const auto& s=ci.pPoolSizes[0];if(s.type!=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER||!s.descriptorCount||s.descriptorCount>16)return Unsupported;try{auto p=std::make_unique<DescriptorPool>();p->device=device;p->limit=ci.maxSets;p->freeable=bool(ci.flags&VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);p->sampled=p->fontOnly=true;p->samplerCapacity=s.descriptorCount;p->sets.reserve(ci.maxSets);*out=handle<VkDescriptorPool>(p.release());return VK_SUCCESS;}catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}}
    for(uint32_t n=0;n<2;n++){
        const auto& s=ci.pPoolSizes[n];if(s.descriptorCount<ci.maxSets||s.descriptorCount>16)return Unsupported;
        if(s.type==VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER&&!buffer)buffer=true;
        else if(s.type==VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER&&!image)image=true;
        else return Unsupported;
    }
    if(!buffer||!image)return Unsupported;
    try{auto p=std::make_unique<DescriptorPool>();p->device=device;p->limit=ci.maxSets;p->freeable=bool(ci.flags&VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);p->sampled=true;p->sets.reserve(ci.maxSets);*out=handle<VkDescriptorPool>(p.release());return VK_SUCCESS;}catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}
}
bool sampledWriteValid(Device* d,const VkWriteDescriptorSet& w){
    auto* s=as<DescriptorSet>(w.dstSet);
    if(!s||!s->pool->sampled||s->pool->device!=d||w.pNext||w.dstArrayElement||w.descriptorCount!=1)return false;
    if(w.dstBinding==(s->pool->fontOnly?0u:1u)){
        if(w.descriptorType!=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER||!w.pImageInfo)return false;
        const auto& i=*w.pImageInfo;auto* view=as<View>(i.imageView);auto* sampler=as<Sampler>(i.sampler);
        return view&&view->image->sampled&&view->image->owner==d&&sampler&&sampler->owner==d&&i.imageLayout==VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    if(s->pool->fontOnly||w.dstBinding||w.descriptorType!=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER||!w.pBufferInfo)return false;
    const auto& i=*w.pBufferInfo;auto* b=as<Buffer>(i.buffer);
    if(!b||!(b->usage&VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)||i.offset%16||i.offset>=b->bytes)return false;
    uint64_t range=i.range==VK_WHOLE_SIZE?b->bytes-i.offset:i.range;
    return range&&range<=4096&&range%4==0&&range<=b->bytes-i.offset;
}
void applySampledWrite(const VkWriteDescriptorSet& w){
    auto* s=as<DescriptorSet>(w.dstSet);s->bad=false;
    if(w.dstBinding==(s->pool->fontOnly?0u:1u)){s->imageView=as<View>(w.pImageInfo->imageView);s->sampler=as<Sampler>(w.pImageInfo->sampler);}
    else {const auto& i=*w.pBufferInfo;s->buffer=as<Buffer>(i.buffer);s->offset=i.offset;s->range=i.range==VK_WHOLE_SIZE?s->buffer->bytes-i.offset:i.range;}
}
void putId(std::vector<uint8_t>& b,uint64_t id){put32(b,uint32_t(id));put32(b,uint32_t(id>>32));}
uint64_t readId(const std::vector<uint8_t>& b){if(b.size()!=8)throw std::runtime_error("Sampled resource ID size");return get32(b.data())|(uint64_t(get32(b.data()+4))<<32);}
VkResult sampledRemote(Command* c,Device* d,uint64_t& remote){
    auto* set=c->descriptor;
    if(!set||set->bad||!set->pool->sampled||!set->imageView||!set->sampler||set->pool->device!=d||set->sampler->owner!=d)return Unsupported;
    auto* i=set->imageView->image;
    if(!i->sampled||i->owner!=d||!i->memory||i->memory->device!=d||!i->initialized||i->layout!=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL||imageByteSize(i)>i->memory->bytes.size())return Unsupported;
    const auto& s=set->sampler->state;std::vector<uint8_t> key;
    for(uint32_t value:{uint32_t(s.mag),uint32_t(s.min),uint32_t(s.mip),uint32_t(s.u),uint32_t(s.v),uint32_t(s.w)})put32(key,value);
    for(float f:{s.bias,s.minLod,s.maxLod}){uint32_t value;std::memcpy(&value,&f,4);put32(key,value);}
    if(i->remoteTexture&&i->remoteRevision==i->revision&&i->remoteSampler==key){remote=i->remoteTexture;return VK_SUCCESS;}
    dropRemoteImage(i);auto* wire=d->physical->instance->wire.get();uint64_t id=0;
    try{
        std::vector<uint8_t> meta;put32(meta,i->w);put32(meta,i->h);put32(meta,i->levels);meta.insert(meta.end(),key.begin(),key.end());uint32_t total=uint32_t(imageByteSize(i));put32(meta,total);
        id=readId(wire->exchange(19,meta));if(!id)throw std::runtime_error("Zero sampled ID");
        for(uint32_t offset=0;offset<total;){uint32_t size=std::min(1048576u-16,total-offset);std::vector<uint8_t> chunk;putId(chunk,id);put32(chunk,offset);put32(chunk,size);const auto* at=i->memory->bytes.data()+offset;chunk.insert(chunk.end(),at,at+size);if(!wire->exchange(20,chunk).empty())throw std::runtime_error("Sampled upload response");offset+=size;}
        std::vector<uint8_t> request;putId(request,id);if(!wire->exchange(21,request).empty())throw std::runtime_error("Sampled commit response");
        i->remoteTexture=id;i->remoteRevision=i->revision;i->remoteSampler=std::move(key);remote=id;return VK_SUCCESS;
    }catch(...){if(id)try{std::vector<uint8_t> request;putId(request,id);wire->exchange(22,request);}catch(...){}throw;}
}
}
