// Ordered experimental indexed scene recording. No singleton last-state fallback.
namespace {
struct UiPacket {std::vector<uint8_t> bytes;std::vector<std::pair<size_t,DescriptorSet*>> textures;};
const uint8_t* uiBufferBytes(const Buffer* b,Device* device,uint64_t offset,uint64_t size){
    if(!b||!b->memory||b->memory->device!=device||b->offset>b->memory->bytes.size()||b->bytes>b->memory->bytes.size()-b->offset||offset>b->bytes||size>b->bytes-offset)return nullptr;
    return b->memory->bytes.data()+size_t(b->offset+offset);
}
bool sameUiGroup(const DrawSnapshot& a,const DrawSnapshot& b){
    return a.pipeline==b.pipeline&&a.descriptor==b.descriptor&&a.vertices==b.vertices&&a.indices==b.indices&&a.vertexBufferOffset==b.vertexBufferOffset&&a.indexBufferOffset==b.indexBufferOffset&&a.indexType==b.indexType;
}
bool buildUiPacket(Command* c,Device* device,UiPacket& output){
    if(!UiLayersEnabled||!c->frame||!c->frame->image||c->drawSnapshots.empty()||c->drawSnapshots.size()>64||c->directDraw)return false;
    auto* target=c->frame->image;uint64_t bytes=uint64_t(target->w)*target->h*4;
    if(target->w<16||target->w>1024||target->h<16||target->h>1024||!target->memory||target->memory->device!=device||bytes>target->memory->bytes.size()||(c->frame->depth&&!c->frame->depth->memory))return false;
    UiPacket packet;put32(packet.bytes,1);put32(packet.bytes,0);unsigned groups=0;size_t at=0;
    auto number=[](std::vector<uint8_t>& b,float f){uint32_t n;std::memcpy(&n,&f,4);put32(b,n);};
    for(float f:c->clear)if(!std::isfinite(f))return false;
    while(at<c->drawSnapshots.size()){
        const auto& first=c->drawSnapshots[at];auto* pipeline=first.pipeline;
        if(++groups>8||!pipeline||!pipeline->vs||!pipeline->fs||pipeline->cs||!pipeline->stride||pipeline->stride>64||pipeline->stride%4||pipeline->attributes.empty()||pipeline->attributes.size()>4||pipeline->blendMode>2||pipeline->pushLayout.sampledBinding>1||pipeline->pushLayout.storage)return false;
        if(pipeline->depth&&(!c->frame->depth||groups!=1))return false; // later depth LOAD not represented; never silently clear/reuse it
        size_t end=at+1;while(end<c->drawSnapshots.size()&&sameUiGroup(first,c->drawSnapshots[end]))end++;
        const auto& layout=pipeline->pushLayout;
        if(layout.bytes>64||layout.bytes%4||bool(layout.bytes)!=bool(layout.stages)||(layout.stages&~uint32_t(VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT)))return false;
        if(pipeline->vs->code.size()<20||pipeline->vs->code.size()>65536||pipeline->vs->code.size()%4||pipeline->fs->code.size()<20||pipeline->fs->code.size()>65536||pipeline->fs->code.size()%4)return false;
        uint32_t width=first.indexType==VK_INDEX_TYPE_UINT16?2:first.indexType==VK_INDEX_TYPE_UINT32?4:0;
        if(!width||!first.indices||!(first.indices->usage&VK_BUFFER_USAGE_INDEX_BUFFER_BIT)||!first.vertices||!(first.vertices->usage&VK_BUFFER_USAGE_VERTEX_BUFFER_BIT))return false;
        std::vector<uint8_t> indexData;std::vector<mavk::bridge::SceneDraw> draws;uint32_t vertices=0,totalIndices=0;
        for(size_t j=at;j<end;j++){
            const auto& d=c->drawSnapshots[j];
            if(!d.indexCount||d.indexCount>12288-totalIndices||d.pushBytes!=layout.bytes||d.pushStages!=layout.stages||d.push.size()!=layout.bytes||d.clip.offset.x<0||d.clip.offset.y<0||!d.clip.extent.width||!d.clip.extent.height||uint64_t(d.clip.offset.x)+d.clip.extent.width>target->w||uint64_t(d.clip.offset.y)+d.clip.extent.height>target->h)return false;
            if(uint64_t(d.firstIndex)*width>UINT64_MAX-first.indexBufferOffset)return false;
            const auto* data=uiBufferBytes(first.indices,device,first.indexBufferOffset+uint64_t(d.firstIndex)*width,uint64_t(d.indexCount)*width);if(!data)return false;
            for(uint32_t i=0;i<d.indexCount;i++){
                uint32_t raw=width==4?get32(data+i*4):uint32_t(data[i*2])|(uint32_t(data[i*2+1])<<8);int64_t effective=int64_t(raw)+d.baseVertex;
                if(effective<0||effective>=4096)return false;vertices=std::max(vertices,uint32_t(effective)+1);
            }
            mavk::bridge::SceneDraw draw;draw.firstIndex=totalIndices;draw.indexCount=d.indexCount;draw.vertexOffset=d.baseVertex;draw.push=d.push;draw.clipped=true;draw.clip=d.clip;draws.push_back(std::move(draw));
            totalIndices+=d.indexCount;indexData.insert(indexData.end(),data,data+size_t(d.indexCount)*width);
        }
        uint64_t vertexBytes=uint64_t(vertices)*pipeline->stride;const auto* vertexData=uiBufferBytes(first.vertices,device,first.vertexBufferOffset,vertexBytes);if(!vertexData)return false;
        std::vector<uint8_t> geometry;put32(geometry,uint32_t(16+pipeline->attributes.size()*12+vertexBytes));put32(geometry,uint32_t(first.indexType));put32(geometry,totalIndices);put32(geometry,uint32_t(indexData.size()));
        put32(geometry,pipeline->stride);put32(geometry,uint32_t(pipeline->attributes.size()));put32(geometry,vertices);put32(geometry,uint32_t(vertexBytes));
        uint32_t locations=0;for(const auto& a:pipeline->attributes){uint32_t size=mavk::bridge::vertexFormatBytes(a.format,true);if(a.location>=4||a.binding||(locations&(1u<<a.location))||!size||a.offset%4||a.offset>pipeline->stride||size>pipeline->stride-a.offset)return false;locations|=1u<<a.location;put32(geometry,a.location);put32(geometry,uint32_t(a.format));put32(geometry,a.offset);}
        geometry.insert(geometry.end(),vertexData,vertexData+vertexBytes);geometry.insert(geometry.end(),indexData.begin(),indexData.end());
        Command captured{};captured.device=device;captured.descriptor=first.descriptor;std::vector<uint8_t> uniform;
        if(layout.uniform&&(!uniformSnapshot(&captured,uniform)||uniform.size()>4096||uniform.size()%4||layout.sampledBinding==0))return false;
        if(layout.sampled&&(!first.descriptor||first.descriptor->bad||!first.descriptor->pool->sampled||first.descriptor->pool->device!=device||!first.descriptor->imageView||!first.descriptor->sampler))return false;
        std::vector<uint8_t> scene;put32(scene,target->w);put32(scene,target->h);put32(scene,uint32_t(pipeline->vs->code.size()));put32(scene,uint32_t(pipeline->fs->code.size()));put32(scene,layout.stages);put32(scene,layout.bytes);put32(scene,pipeline->cull);put32(scene,uint32_t(pipeline->front));put32(scene,pipeline->depth?VK_FORMAT_D32_SFLOAT:VK_FORMAT_UNDEFINED);put32(scene,uint32_t(draws.size()));put32(scene,uint32_t(geometry.size()));for(float f:c->clear)number(scene,f);number(scene,c->clearDepth);
        scene.insert(scene.end(),pipeline->vs->code.begin(),pipeline->vs->code.end());scene.insert(scene.end(),pipeline->fs->code.begin(),pipeline->fs->code.end());scene.insert(scene.end(),geometry.begin(),geometry.end());
        for(const auto& d:draws){put32(scene,d.firstIndex);put32(scene,d.indexCount);put32(scene,uint32_t(d.vertexOffset));put32(scene,1);put32(scene,uint32_t(d.clip.offset.x));put32(scene,uint32_t(d.clip.offset.y));put32(scene,d.clip.extent.width);put32(scene,d.clip.extent.height);scene.insert(scene.end(),d.push.begin(),d.push.end());}
        size_t length=20+uniform.size()+scene.size();if(length>1048576||packet.bytes.size()+4+length>1048576)return false;
        put32(packet.bytes,uint32_t(length));put32(packet.bytes,pipeline->blendMode);put32(packet.bytes,layout.sampledBinding);size_t textureAt=packet.bytes.size();put32(packet.bytes,0);put32(packet.bytes,0);if(layout.sampled)packet.textures.push_back({textureAt,first.descriptor});
        put32(packet.bytes,uint32_t(uniform.size()));packet.bytes.insert(packet.bytes.end(),uniform.begin(),uniform.end());packet.bytes.insert(packet.bytes.end(),scene.begin(),scene.end());at=end;
    }
    for(unsigned i=0;i<4;i++)packet.bytes[4+i]=uint8_t(groups>>(i*8));output=std::move(packet);return true;
}
VkResult executeUiLayers(Command* c,Device* device){
    UiPacket packet;if(!buildUiPacket(c,device,packet))return Unsupported;
    for(const auto& texture:packet.textures){
#ifdef MAVK_EXPERIMENTAL_SAMPLED
        Command captured{};captured.device=device;captured.descriptor=texture.second;uint64_t id=0;auto status=sampledRemote(&captured,device,id);if(status!=VK_SUCCESS)return status;
        for(unsigned i=0;i<8;i++)packet.bytes[texture.first+i]=uint8_t(id>>(i*8));
#else
        (void)texture;return Unsupported;
#endif
    }
    uint64_t bytes=uint64_t(c->frame->image->w)*c->frame->image->h*4;
    if(c->output&&!uiBufferBytes(c->output,device,c->offset,bytes))return Unsupported;
    auto* wire=device->physical->instance->wire.get();auto descriptor=wire->exchange(24,packet.bytes);
    if(descriptor.size()!=16||get32(descriptor.data()+8)!=bytes||get32(descriptor.data()+12)!=c->frame->image->w*4)return VK_ERROR_DEVICE_LOST;
    std::vector<uint8_t> id(descriptor.begin(),descriptor.begin()+8),pixels(size_t(bytes),uint8_t{});
    try{for(uint32_t at=0;at<bytes;){uint32_t amount=uint32_t(std::min<uint64_t>(1048576,bytes-at));auto request=id;put32(request,at);put32(request,amount);auto chunk=wire->exchange(15,request);if(chunk.size()!=amount)throw std::runtime_error("UI layer readback size");std::memcpy(pixels.data()+at,chunk.data(),amount);at+=amount;}}
    catch(...){try{wire->exchange(16,id);}catch(...){}throw;}
    wire->exchange(16,id);auto* image=c->frame->image;std::memcpy(image->memory->bytes.data(),pixels.data(),pixels.size());image->initialized=true;image->layout=c->renderFinal;
    if(c->output)std::memcpy(c->output->memory->bytes.data()+c->output->offset+c->offset,pixels.data(),pixels.size());return VK_SUCCESS;
}
}
