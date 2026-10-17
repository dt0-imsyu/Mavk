// Compile-time opt-in candidate: standard sampled-image upload and sampler state.
namespace {
struct Sampler { Device* owner; mavk::SamplerState state; };
VkResult createSampledImage(Device* d,const VkImageCreateInfo& ci,const VkAllocationCallbacks* a,VkImage* out){
    if(a||ci.pNext||ci.flags||ci.imageType!=VK_IMAGE_TYPE_2D||ci.format!=VK_FORMAT_R8G8B8A8_UNORM||ci.extent.depth!=1||ci.arrayLayers!=1||ci.samples!=VK_SAMPLE_COUNT_1_BIT||ci.tiling!=VK_IMAGE_TILING_OPTIMAL||ci.sharingMode!=VK_SHARING_MODE_EXCLUSIVE||ci.initialLayout!=VK_IMAGE_LAYOUT_UNDEFINED)return Unsupported;
    try{
        (void)mavk::rgbaMipLayout(ci.extent.width,ci.extent.height,ci.mipLevels);
        auto image=std::make_unique<Image>();image->w=ci.extent.width;image->h=ci.extent.height;image->levels=ci.mipLevels;image->sampled=true;image->owner=d;*out=handle<VkImage>(image.release());return VK_SUCCESS;
    }catch(const std::invalid_argument&){return Unsupported;}catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}
}
void dropRemoteImage(Image* i){
    if(!i->remoteTexture)return;
    std::vector<uint8_t> id;put32(id,uint32_t(i->remoteTexture));put32(id,uint32_t(i->remoteTexture>>32));
    i->owner->physical->instance->wire->exchange(22,id);i->remoteTexture=0;i->remoteSampler.clear();
}
bool sampledCopyRegion(const Buffer* source,const Image* image,const VkBufferImageCopy& r){
    if(!source||!image||!image->sampled||!(source->usage&VK_BUFFER_USAGE_TRANSFER_SRC_BIT)||r.imageSubresource.aspectMask!=VK_IMAGE_ASPECT_COLOR_BIT||r.imageSubresource.mipLevel>=image->levels||r.imageSubresource.baseArrayLayer||r.imageSubresource.layerCount!=1||r.imageOffset.x||r.imageOffset.y||r.imageOffset.z||r.imageExtent.depth!=1||r.bufferOffset%4)return false;
    auto layout=mavk::rgbaMipLayout(image->w,image->h,image->levels);auto m=layout[r.imageSubresource.mipLevel];
    if(r.imageExtent.width!=m.width||r.imageExtent.height!=m.height||(r.bufferRowLength&&r.bufferRowLength!=m.width)||(r.bufferImageHeight&&r.bufferImageHeight!=m.height))return false;
    return r.bufferOffset<=source->bytes&&m.bytes<=source->bytes-r.bufferOffset;
}
VkResult executeSampledCopy(const Command::ImageOp& op){
    auto* i=op.src;auto* b=op.buffer;if(!sampledCopyRegion(b,i,op.region)||i->layout!=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL||!b->memory||!i->memory)return Unsupported;
    auto layout=mavk::rgbaMipLayout(i->w,i->h,i->levels);auto m=layout[op.region.imageSubresource.mipLevel];
    if(b->offset>b->memory->bytes.size()||b->bytes>b->memory->bytes.size()-b->offset||m.offset>i->memory->bytes.size()||m.bytes>i->memory->bytes.size()-m.offset)return Unsupported;
    std::memcpy(i->memory->bytes.data()+m.offset,b->memory->bytes.data()+b->offset+op.region.bufferOffset,size_t(m.bytes));
    i->uploadedMips|=1u<<op.region.imageSubresource.mipLevel;i->initialized=i->uploadedMips==((1u<<i->levels)-1);i->revision++;return VK_SUCCESS;
}
}
API VkResult VKAPI_CALL vkCreateSampler(VkDevice d,const VkSamplerCreateInfo* ci,const VkAllocationCallbacks* a,VkSampler* out){
    *out=VK_NULL_HANDLE;if(a||ci->pNext||ci->flags||ci->anisotropyEnable||ci->compareEnable||ci->unnormalizedCoordinates)return Unsupported;
    mavk::SamplerState s;s.mag=ci->magFilter;s.min=ci->minFilter;s.mip=ci->mipmapMode;s.u=ci->addressModeU;s.v=ci->addressModeV;s.w=ci->addressModeW;s.bias=ci->mipLodBias;s.minLod=ci->minLod;s.maxLod=ci->maxLod;
    if(!mavk::samplerStateValid(s,2))return Unsupported;
    try{*out=handle<VkSampler>(new Sampler{as<Device>(d),s});return VK_SUCCESS;}catch(...){return VK_ERROR_OUT_OF_HOST_MEMORY;}
}
API void VKAPI_CALL vkDestroySampler(VkDevice,VkSampler h,const VkAllocationCallbacks*){delete as<Sampler>(h);}
API void VKAPI_CALL vkCmdCopyBufferToImage(VkCommandBuffer h,VkBuffer src,VkImage dst,VkImageLayout layout,uint32_t n,const VkBufferImageCopy* regions){
    auto* c=as<Command>(h);auto* image=as<Image>(dst);auto* buffer=as<Buffer>(src);
    if(!c->recording||c->frame||c->drew||c->computeGroups||!c->copies.empty()||layout!=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL||!n||n>11||!regions||c->imageOps.size()+n>40){c->bad=true;return;}
    for(uint32_t j=0;j<n;j++)if(!sampledCopyRegion(buffer,image,regions[j])){c->bad=true;return;}
    try{for(uint32_t j=0;j<n;j++)c->imageOps.push_back({image,nullptr,layout,layout,buffer,regions[j]});}catch(...){c->bad=true;}
}
