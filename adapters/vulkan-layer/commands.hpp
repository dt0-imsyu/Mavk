// SPDX-License-Identifier: Apache-2.0
// Private include inside layer.cpp. Record generations until reset/free,not
// just until a fence completes:executable command buffers may be resubmitted.
VKAPI_ATTR void VKAPI_CALL getQueue(VkDevice h,uint32_t family,uint32_t index,VkQueue* out){
    auto s=device(h);if(!s)return;s->fn<PFN_vkGetDeviceQueue>("vkGetDeviceQueue")(h,family,index,out);if(*out){std::lock_guard local(s->mutex);s->queueInfos[*out]={family,0};std::lock_guard lock(registryMutex);queueDevices[*out]=s;}
}
VKAPI_ATTR void VKAPI_CALL getQueue2(VkDevice h,const VkDeviceQueueInfo2* info,VkQueue* out){
    auto s=device(h);if(!s)return;s->fn<PFN_vkGetDeviceQueue2>("vkGetDeviceQueue2")(h,info,out);if(*out){std::lock_guard local(s->mutex);s->queueInfos[*out]={info->queueFamilyIndex,info->flags};std::lock_guard lock(registryMutex);queueDevices[*out]=s;}
}
VKAPI_ATTR VkResult VKAPI_CALL allocateCommands(VkDevice h,const VkCommandBufferAllocateInfo* ci,VkCommandBuffer* out){
    auto s=device(h);if(!s)return VK_ERROR_INITIALIZATION_FAILED;
    auto r=s->fn<PFN_vkAllocateCommandBuffers>("vkAllocateCommandBuffers")(h,ci,out);if(r!=VK_SUCCESS)return r;
    try{std::lock_guard lock(s->mutex);std::lock_guard global(registryMutex);for(uint32_t i=0;i<ci->commandBufferCount;i++){Command c;c.pool=ci->commandPool;c.secondary=ci->level==VK_COMMAND_BUFFER_LEVEL_SECONDARY;s->commands.emplace(out[i],std::move(c));commandDevices[out[i]]=s;}}
    catch(...){s->fn<PFN_vkFreeCommandBuffers>("vkFreeCommandBuffers")(h,ci->commandPool,ci->commandBufferCount,out);std::lock_guard lock(s->mutex);std::lock_guard global(registryMutex);for(uint32_t i=0;i<ci->commandBufferCount;i++){s->commands.erase(out[i]);commandDevices.erase(out[i]);out[i]=VK_NULL_HANDLE;}return VK_ERROR_OUT_OF_HOST_MEMORY;}return r;
}
VKAPI_ATTR VkResult VKAPI_CALL beginCommand(VkCommandBuffer h,const VkCommandBufferBeginInfo* ci){
    auto s=commandDevice(h);if(!s)return VK_ERROR_INITIALIZATION_FAILED;auto r=s->fn<PFN_vkBeginCommandBuffer>("vkBeginCommandBuffer")(h,ci);
    if(r==VK_SUCCESS){std::lock_guard lock(s->mutex);auto& c=s->commands.at(h);s->retireCommand(c);c.recording=true;}return r;
}
VKAPI_ATTR VkResult VKAPI_CALL endCommand(VkCommandBuffer h){
    auto s=commandDevice(h);if(!s)return VK_ERROR_INITIALIZATION_FAILED;std::lock_guard lock(s->mutex);auto& c=s->commands.at(h);
    // Read-only guards emit no diagnostic writes for the host to consume.
    if(s->guardReports&&s->guarded&&!c.secondary&&!c.failed&&!c.generations.empty()){
        VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};barrier.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
        s->fn<PFN_vkCmdPipelineBarrier>("vkCmdPipelineBarrier")(h,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&barrier,0,nullptr,0,nullptr);
    }
    auto r=s->fn<PFN_vkEndCommandBuffer>("vkEndCommandBuffer")(h);c.recording=false;return c.failed?VK_ERROR_VALIDATION_FAILED_EXT:r;
}
VKAPI_ATTR VkResult VKAPI_CALL resetCommand(VkCommandBuffer h,VkCommandBufferResetFlags flags){
    auto s=commandDevice(h);if(!s)return VK_ERROR_INITIALIZATION_FAILED;auto r=s->fn<PFN_vkResetCommandBuffer>("vkResetCommandBuffer")(h,flags);
    if(r==VK_SUCCESS){std::lock_guard lock(s->mutex);s->retireCommand(s->commands.at(h));}return r;
}
VKAPI_ATTR void VKAPI_CALL freeCommands(VkDevice h,VkCommandPool pool,uint32_t n,const VkCommandBuffer* commands){
    auto s=device(h);if(!s)return;s->fn<PFN_vkFreeCommandBuffers>("vkFreeCommandBuffers")(h,pool,n,commands);
    std::lock_guard lock(s->mutex);std::lock_guard global(registryMutex);for(uint32_t i=0;i<n;i++){auto at=s->commands.find(commands[i]);if(at!=s->commands.end()){s->retireCommand(at->second);s->commands.erase(at);}commandDevices.erase(commands[i]);}
}
VKAPI_ATTR VkResult VKAPI_CALL resetCommandPool(VkDevice h,VkCommandPool pool,VkCommandPoolResetFlags flags){
    auto s=device(h);if(!s)return VK_ERROR_INITIALIZATION_FAILED;auto r=s->fn<PFN_vkResetCommandPool>("vkResetCommandPool")(h,pool,flags);
    if(r==VK_SUCCESS){std::lock_guard lock(s->mutex);for(auto& c:s->commands)if(c.second.pool==pool)s->retireCommand(c.second);}return r;
}
VKAPI_ATTR void VKAPI_CALL destroyCommandPool(VkDevice h,VkCommandPool pool,const VkAllocationCallbacks* alloc){
    auto s=device(h);if(!s)return;s->fn<PFN_vkDestroyCommandPool>("vkDestroyCommandPool")(h,pool,alloc);
    std::lock_guard lock(s->mutex);std::lock_guard global(registryMutex);for(auto at=s->commands.begin();at!=s->commands.end();)if(at->second.pool==pool){s->retireCommand(at->second);commandDevices.erase(at->first);at=s->commands.erase(at);}else ++at;
}
bool compatibleSet(const PipelineSchema& a,const PipelineSchema& b,uint32_t set){
    if(a.sets.size()<=set||b.sets.size()<=set||a.push.size()!=b.push.size())return false;
    for(size_t i=0;i<a.push.size();i++)if(a.push[i].stageFlags!=b.push[i].stageFlags||a.push[i].offset!=b.push[i].offset||a.push[i].size!=b.push[i].size)return false;
    for(uint32_t i=0;i<=set;i++){if(a.sets[i].nativeSampler!=b.sets[i].nativeSampler||a.sets[i].deferred!=b.sets[i].deferred)return false;if(a.sets[i].deferred&&a.sets[i].owner!=b.sets[i].owner)return false;const auto& x=a.sets[i].bindings;const auto& y=b.sets[i].bindings;if(x.size()!=y.size())return false;
        if(bool(a.sets[i].immutable)!=bool(b.sets[i].immutable)||(a.sets[i].immutable&&a.sets[i].immutable->handles!=b.sets[i].immutable->handles))return false;
        for(size_t j=0;j<x.size();j++)if(x[j].binding!=y[j].binding||x[j].descriptorType!=y[j].descriptorType||x[j].descriptorCount!=y[j].descriptorCount||x[j].stageFlags!=y[j].stageFlags)return false;
    }return true;
}
size_t bindIndex(VkPipelineBindPoint p){if(p==VK_PIPELINE_BIND_POINT_GRAPHICS)return 0;if(p==VK_PIPELINE_BIND_POINT_COMPUTE)return 1;throw std::invalid_argument("guarded bind point unsupported");}
VKAPI_ATTR void VKAPI_CALL bindPipeline(VkCommandBuffer h,VkPipelineBindPoint point,VkPipeline pipeline){
    auto s=commandDevice(h);if(!s)return;std::lock_guard lock(s->mutex);auto& c=s->commands.at(h);
    if(s->guarded)try{auto at=s->pipelines.find(pipeline);if(at==s->pipelines.end()||!at->second->owner)throw std::invalid_argument("untracked guarded pipeline");
        c.bound[bindIndex(point)].pipeline=at->second;c.pipelines.push_back(at->second);}
    catch(const std::exception& e){s->gap("vkCmdBindPipeline",e.what());c.failed=true;return;}
    s->fn<PFN_vkCmdBindPipeline>("vkCmdBindPipeline")(h,point,pipeline);
}
// Real Sarek uses render-pass-continuation secondaries. Retain their immutable
// descriptor generations in the executing primary. The diagnostic host barrier
// is emitted at PRIMARY end, never inside the secondary's render pass.
VKAPI_ATTR void VKAPI_CALL executeCommands(VkCommandBuffer h,uint32_t n,const VkCommandBuffer* children){
    auto s=commandDevice(h);if(!s)return;
    if(!s->guarded){s->fn<PFN_vkCmdExecuteCommands>("vkCmdExecuteCommands")(h,n,children);return;}
    std::lock_guard lock(s->mutex);auto& primary=s->commands.at(h);
    try{
        if(primary.secondary||!n||n>4096||!children)throw std::invalid_argument("secondary execution shape");
        std::vector<std::shared_ptr<PipelineShadow>> keep;
        std::vector<std::shared_ptr<NativeDescriptorSnapshot>> generations;
        for(uint32_t i=0;i<n;i++){auto at=s->commands.find(children[i]);
            if(at==s->commands.end()||!at->second.secondary||at->second.failed||at->second.recording)throw std::invalid_argument("secondary execution state");
            keep.insert(keep.end(),at->second.pipelines.begin(),at->second.pipelines.end());
            for(const auto& g:at->second.generations)if(std::find(primary.generations.begin(),primary.generations.end(),g)==primary.generations.end()&&std::find(generations.begin(),generations.end(),g)==generations.end())generations.push_back(g);
        }
        if(keep.size()>65536||primary.pipelines.size()>65536-keep.size()||generations.size()>65536||primary.generations.size()>65536-generations.size())throw std::invalid_argument("secondary retention bound");
        primary.pipelines.reserve(primary.pipelines.size()+keep.size());primary.generations.reserve(primary.generations.size()+generations.size());
        primary.generations.insert(primary.generations.end(),generations.begin(),generations.end());
        primary.pipelines.insert(primary.pipelines.end(),keep.begin(),keep.end());primary.bound={};
        s->fn<PFN_vkCmdExecuteCommands>("vkCmdExecuteCommands")(h,n,children);
        if(s->trace)std::fprintf(stderr,"MAVK_LAYER_SECONDARY count=%u retainedGenerations=%zu\n",n,generations.size());
    }catch(const std::exception& e){s->gap("vkCmdExecuteCommands",e.what());primary.failed=true;}
}
void saveSets(Device& s,Command& c,VkPipelineBindPoint point,VkPipelineLayout layout,uint32_t first,uint32_t n,const VkDescriptorSet* sets,uint32_t dynamic,const uint32_t* offsets){
    auto at=s.pipelineLayouts.find(layout);if(at==s.pipelineLayouts.end()||first>at->second->sets.size()||n>at->second->sets.size()-first||(n&&!sets)||(dynamic&&!offsets))throw std::invalid_argument("guarded descriptor layout/range/dynamic offsets");
    auto& bound=c.bound[bindIndex(point)];auto staged=bound.sets;uint32_t used=0;
    for(uint32_t i=0;i<n;i++){auto set=s.sets.find(sets[i]);const auto& schema=at->second->sets[first+i];
        if(set==s.sets.end()||(schema.deferred?!set->second.deferred:schema.nativeSampler?!set->second.nativeSampler:!set->second.tracked))throw std::invalid_argument("untracked/incompatible bound set");
        std::vector<uint32_t> values;for(const auto& b:schema.bindings)if(descriptor_api::dynamic(b.descriptorType)){
            if(b.descriptorCount>dynamic-used)throw std::invalid_argument("missing dynamic offsets");
            for(uint32_t j=0;j<b.descriptorCount;j++)values.push_back(offsets[used++]);
        }
        staged[first+i]={sets[i],at->second,std::move(values)};
    }
    if(used!=dynamic)throw std::invalid_argument("extra dynamic offsets");bound.sets=std::move(staged);
}
VKAPI_ATTR void VKAPI_CALL bindSets(VkCommandBuffer h,VkPipelineBindPoint point,VkPipelineLayout layout,uint32_t first,uint32_t n,const VkDescriptorSet* sets,uint32_t dynamic,const uint32_t* offsets){
    auto s=commandDevice(h);if(!s)return;if(!s->guarded){s->fn<PFN_vkCmdBindDescriptorSets>("vkCmdBindDescriptorSets")(h,point,layout,first,n,sets,dynamic,offsets);return;}
    std::lock_guard lock(s->mutex);auto& c=s->commands.at(h);try{saveSets(*s,c,point,layout,first,n,sets,dynamic,offsets);}catch(const std::exception& e){s->gap("vkCmdBindDescriptorSets",e.what());c.failed=true;}
}
void bindSets2Named(VkCommandBuffer h,const VkBindDescriptorSetsInfo* info,const char* name){
    auto s=commandDevice(h);if(!s)return;if(!s->guarded){s->fn<PFN_vkCmdBindDescriptorSets2KHR>(name)(h,info);return;}
    std::lock_guard lock(s->mutex);auto& c=s->commands.at(h);try{
        if(info->pNext||!info->stageFlags||(info->stageFlags&~(VK_SHADER_STAGE_ALL_GRAPHICS|VK_SHADER_STAGE_COMPUTE_BIT)))throw std::invalid_argument("guarded binding stages unsupported");
        if(info->stageFlags&VK_SHADER_STAGE_ALL_GRAPHICS)saveSets(*s,c,VK_PIPELINE_BIND_POINT_GRAPHICS,info->layout,info->firstSet,info->descriptorSetCount,info->pDescriptorSets,info->dynamicOffsetCount,info->pDynamicOffsets);
        if(info->stageFlags&VK_SHADER_STAGE_COMPUTE_BIT)saveSets(*s,c,VK_PIPELINE_BIND_POINT_COMPUTE,info->layout,info->firstSet,info->descriptorSetCount,info->pDescriptorSets,info->dynamicOffsetCount,info->pDynamicOffsets);
    }catch(const std::exception& e){s->gap(name,e.what());c.failed=true;}
}
VKAPI_ATTR void VKAPI_CALL bindSets2(VkCommandBuffer h,const VkBindDescriptorSetsInfo* info){bindSets2Named(h,info,"vkCmdBindDescriptorSets2");}
VKAPI_ATTR void VKAPI_CALL bindSets2KHR(VkCommandBuffer h,const VkBindDescriptorSetsInfo* info){bindSets2Named(h,info,"vkCmdBindDescriptorSets2KHR");}
void reportProfileLocked(Device& s){
    if(!s.profile)return;
    const auto now=std::chrono::steady_clock::now();
    const auto seconds=std::chrono::duration<double>(now-s.profileSince).count();
    if(seconds<1)return;
    // All observed operations can flush: an upper WSI layer may consume present.
    std::fprintf(stderr,"MAVK_LAYER_PROFILE seconds=%.3f presentCalls=%llu guardCalls=%llu guardWallMs=%.3f fenceWaitCalls=%llu fenceWaitWallMs=%.3f semaphoreWaitCalls=%llu semaphoreWaitWallMs=%.3f submitCalls=%llu submitWallMs=%.3f presentWallMs=%.3f uniqueFrames=unknown\n",seconds,(unsigned long long)s.profilePresents,(unsigned long long)s.profileGuardCalls,s.profileGuardNs/1e6,(unsigned long long)s.profileWaitCalls,s.profileWaitNs/1e6,(unsigned long long)s.profileSemaphoreCalls,s.profileSemaphoreNs/1e6,(unsigned long long)s.profileSubmitCalls,s.profileSubmitNs/1e6,s.profilePresentNs/1e6);
    s.profileSince=now;
    s.profileGuardNs=s.profileWaitNs=s.profilePresentNs=s.profileGuardCalls=s.profileWaitCalls=s.profilePresents=0;
    s.profileSemaphoreNs=s.profileSemaphoreCalls=s.profileSubmitNs=s.profileSubmitCalls=0;
}
bool prepareExecution(Device& s,VkCommandBuffer h,VkPipelineBindPoint point){
    if(!s.guarded)return true;auto& c=s.commands.at(h);if(c.failed)return false;
    struct Timing {
        Device& s;std::chrono::steady_clock::time_point start;
        explicit Timing(Device& state):s(state),start(s.profile?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{}){}
        ~Timing(){if(s.profile){const auto now=std::chrono::steady_clock::now();s.profileGuardNs+=uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(now-start).count());s.profileGuardCalls++;
            // WSI may consume present before this layer. Attribute guarded
            // recording independently; never infer visible FPS from draws.
            reportProfileLocked(s);
        }}
    } timing(s); // Caller holds the existing device mutex; no new lock/submit.
    try{auto& bound=c.bound[bindIndex(point)];if(!bound.pipeline)throw std::invalid_argument("guarded pipeline not bound");
        if(std::none_of(bound.pipeline->requiredSets.begin(),bound.pipeline->requiredSets.end(),[](bool used){return used;}))return true;
        if(!bound.pipeline->descriptors)throw std::invalid_argument("guarded descriptor adapter missing");
        std::vector<VkDescriptorSet> logical;std::vector<uint32_t> dynamicOffsets;for(uint32_t i=0;i<bound.pipeline->schema->sets.size();i++){const auto& set=bound.sets[i];const auto& schema=bound.pipeline->schema->sets[i];if(!bound.pipeline->requiredSets[i]){logical.push_back(VK_NULL_HANDLE);continue;}if(!set.set||!set.schema||!compatibleSet(*set.schema,*bound.pipeline->schema,i))throw std::invalid_argument("guarded descriptor compatibility");logical.push_back(schema.nativeSampler?VK_NULL_HANDLE:set.set);dynamicOffsets.insert(dynamicOffsets.end(),set.dynamicOffsets.begin(),set.dynamicOffsets.end());}
        auto cpu=s.updates->snapshotUsedBindings(logical,GuardMetadataAbi::IndexInitFormats,bound.pipeline->resourceBindings);
        if(s.testTexelMetadata&&s.testTexelMetadataRecords++<32){
            std::fprintf(stderr,"MAVK_TEST_TEXEL_METADATA generation=%llu words=",(unsigned long long)cpu->generation);
            for(size_t i=0;i<std::min(size_t(64),cpu->metadata.size());i++)std::fprintf(stderr,"%u,",cpu->metadata[i]);
            for(size_t row=0;row<cpu->bindings.size();row++)if(cpu->bindings[row].kind==DescriptorKind::UniformTexelBuffer||cpu->bindings[row].kind==DescriptorKind::StorageTexelBuffer)for(size_t i=0;i<std::min(size_t(4),cpu->values[row].size());i++)std::fprintf(stderr," components[%u:%u:%zu]=%u",cpu->bindings[row].set,cpu->bindings[row].binding,i,cpu->values[row][i].texelComponents);
            std::fprintf(stderr,"\n");
        }
        auto resolve=[&](const DescriptorBinding& binding,uint32_t index,const DescriptorValue& value){
            if(value.status==DescriptorStatus::Null&&(binding.kind==DescriptorKind::SampledImage||binding.kind==DescriptorKind::CombinedImageSampler||binding.kind==DescriptorKind::StorageImage)){
                auto at=bound.pipeline->imageFormats.find({binding.set,binding.binding});
                if(!s.nullImages||at==bound.pipeline->imageFormats.end()){
                    std::fprintf(stderr,"MAVK_LAYER_NULL_IMAGE_UNRESOLVED set=%u binding=%u index=%u kind=%u reflected=%zu\n",binding.set,binding.binding,index,unsigned(binding.kind),bound.pipeline->imageFormats.size());
                    auto type=bound.pipeline->imageTypes.find({binding.set,binding.binding});
                    if(type!=bound.pipeline->imageTypes.end()){const auto& t=type->second;std::fprintf(stderr,"MAVK_LAYER_NULL_IMAGE_TYPE dim=%u depth=%u arrayed=%u ms=%u sampled=%u format=%u\n",t[0],t[1],t[2],t[3],t[4],t[5]);}
                    else std::fprintf(stderr,"MAVK_LAYER_NULL_IMAGE_TYPE declaration=absent staticUse=unknown\n");
                    throw std::invalid_argument("null color image shader type not admitted");
                }
                NativeDescriptorResource sampler;
                if(binding.kind==DescriptorKind::CombinedImageSampler){
                    auto tuple=std::static_pointer_cast<const VulkanDescriptorResources::Resource>(value.owner);
                    if(!tuple||!tuple->sampling)throw std::invalid_argument("null combined sampler lease missing");
                    sampler.owner=tuple->sampling;sampler.image.sampler=tuple->sampler;
                }
                const auto& type=at->second;const bool one=type.viewType==VK_IMAGE_VIEW_TYPE_1D||type.viewType==VK_IMAGE_VIEW_TYPE_1D_ARRAY;
                return s.nullImages->resolve(binding,value,type.format,sampler,one?VK_IMAGE_TYPE_1D:type.viewType==VK_IMAGE_VIEW_TYPE_3D?VK_IMAGE_TYPE_3D:VK_IMAGE_TYPE_2D,type.viewType);
            }
            if(value.status==DescriptorStatus::Null&&(binding.kind==DescriptorKind::UniformTexelBuffer||binding.kind==DescriptorKind::StorageTexelBuffer)){
                auto at=bound.pipeline->texelFormats.find({binding.set,binding.binding});
                if(!s.nullTexels||at==bound.pipeline->texelFormats.end())throw std::invalid_argument("null texel shader format not admitted");
                return s.nullTexels->resolve(binding,value,at->second);
            }
            if(value.status==DescriptorStatus::Null&&s.nullBuffers)return s.nullBuffers->resolve(binding,value);
            return VulkanDescriptorResources::resolve(binding,index,value);
        };
        auto generation=bound.pipeline->descriptors->materialize(cpu,resolve);
        generation->validateDynamicOffsets(dynamicOffsets);
        if(std::find(c.generations.begin(),c.generations.end(),generation)==c.generations.end())c.generations.push_back(generation);
        auto sets=generation->sets();for(uint32_t i=0;i<bound.pipeline->schema->sets.size();i++)if(bound.pipeline->requiredSets[i]&&bound.pipeline->schema->sets[i].nativeSampler){if(sets[i])throw std::invalid_argument("native sampler heap unexpectedly snapshotted");sets[i]=bound.sets[i].set;}
        // Leave unused slots unbound; preserve their original indices and omit
        // only their dynamic offsets. The private guard set remains last.
        for(uint32_t first=0;first<sets.size();){
            if(!sets[first]){first++;continue;}
            uint32_t end=first;std::vector<uint32_t> offsets;
            while(end<sets.size()&&sets[end]){if(end<bound.pipeline->schema->sets.size()){const auto& values=bound.sets[end].dynamicOffsets;offsets.insert(offsets.end(),values.begin(),values.end());}end++;}
            s.fn<PFN_vkCmdBindDescriptorSets>("vkCmdBindDescriptorSets")(h,point,generation->pipelineLayout(),first,end-first,sets.data()+first,uint32_t(offsets.size()),offsets.data());first=end;
        }
        s.guardedDraws++;if(s.trace)std::fprintf(stderr,"MAVK_LAYER_EXECUTE guarded=1 sets=%zu generations=%zu\n",sets.size(),c.generations.size());return true;
    }catch(const std::exception& e){s.gap("guarded execution",e.what());c.failed=true;return false;}
}
VKAPI_ATTR void VKAPI_CALL draw(VkCommandBuffer h,uint32_t v,uint32_t i,uint32_t fv,uint32_t fi){auto s=commandDevice(h);if(!s)return;std::lock_guard lock(s->mutex);if(prepareExecution(*s,h,VK_PIPELINE_BIND_POINT_GRAPHICS))s->fn<PFN_vkCmdDraw>("vkCmdDraw")(h,v,i,fv,fi);}
VKAPI_ATTR void VKAPI_CALL drawIndexed(VkCommandBuffer h,uint32_t i,uint32_t n,uint32_t first,int32_t offset,uint32_t instance){auto s=commandDevice(h);if(!s)return;std::lock_guard lock(s->mutex);if(prepareExecution(*s,h,VK_PIPELINE_BIND_POINT_GRAPHICS))s->fn<PFN_vkCmdDrawIndexed>("vkCmdDrawIndexed")(h,i,n,first,offset,instance);}
VKAPI_ATTR void VKAPI_CALL dispatch(VkCommandBuffer h,uint32_t x,uint32_t y,uint32_t z){auto s=commandDevice(h);if(!s)return;std::lock_guard lock(s->mutex);if(prepareExecution(*s,h,VK_PIPELINE_BIND_POINT_COMPUTE))s->fn<PFN_vkCmdDispatch>("vkCmdDispatch")(h,x,y,z);}
// Sarek also records indirect work. It must bind the same guarded generation,
// not fall through to a driver command with stale public/private descriptors.
VKAPI_ATTR void VKAPI_CALL drawIndirect(VkCommandBuffer h,VkBuffer b,VkDeviceSize o,uint32_t n,uint32_t stride){auto s=commandDevice(h);if(!s)return;std::lock_guard lock(s->mutex);if(prepareExecution(*s,h,VK_PIPELINE_BIND_POINT_GRAPHICS))s->fn<PFN_vkCmdDrawIndirect>("vkCmdDrawIndirect")(h,b,o,n,stride);}
VKAPI_ATTR void VKAPI_CALL drawIndexedIndirect(VkCommandBuffer h,VkBuffer b,VkDeviceSize o,uint32_t n,uint32_t stride){auto s=commandDevice(h);if(!s)return;std::lock_guard lock(s->mutex);if(prepareExecution(*s,h,VK_PIPELINE_BIND_POINT_GRAPHICS))s->fn<PFN_vkCmdDrawIndexedIndirect>("vkCmdDrawIndexedIndirect")(h,b,o,n,stride);}
VKAPI_ATTR void VKAPI_CALL dispatchIndirect(VkCommandBuffer h,VkBuffer b,VkDeviceSize o){auto s=commandDevice(h);if(!s)return;std::lock_guard lock(s->mutex);if(prepareExecution(*s,h,VK_PIPELINE_BIND_POINT_COMPUTE))s->fn<PFN_vkCmdDispatchIndirect>("vkCmdDispatchIndirect")(h,b,o);}
// Resolve the exact advertised spelling downstream. A promoted alias must not
// silently require a different extension entry point, nor bypass preparation.
void dispatchBaseNamed(VkCommandBuffer h,uint32_t bx,uint32_t by,uint32_t bz,uint32_t x,uint32_t y,uint32_t z,const char* name){auto s=commandDevice(h);if(!s)return;std::lock_guard lock(s->mutex);if(prepareExecution(*s,h,VK_PIPELINE_BIND_POINT_COMPUTE))s->fn<PFN_vkCmdDispatchBase>(name)(h,bx,by,bz,x,y,z);}
VKAPI_ATTR void VKAPI_CALL dispatchBase(VkCommandBuffer h,uint32_t bx,uint32_t by,uint32_t bz,uint32_t x,uint32_t y,uint32_t z){dispatchBaseNamed(h,bx,by,bz,x,y,z,"vkCmdDispatchBase");}
VKAPI_ATTR void VKAPI_CALL dispatchBaseKHR(VkCommandBuffer h,uint32_t bx,uint32_t by,uint32_t bz,uint32_t x,uint32_t y,uint32_t z){dispatchBaseNamed(h,bx,by,bz,x,y,z,"vkCmdDispatchBaseKHR");}
void indirectCountNamed(VkCommandBuffer h,VkBuffer b,VkDeviceSize offset,VkBuffer count,VkDeviceSize countOffset,uint32_t maximum,uint32_t stride,const char* name){auto s=commandDevice(h);if(!s)return;std::lock_guard lock(s->mutex);if(prepareExecution(*s,h,VK_PIPELINE_BIND_POINT_GRAPHICS))s->fn<PFN_vkCmdDrawIndirectCount>(name)(h,b,offset,count,countOffset,maximum,stride);}
#define MAVK_COUNT_COMMAND(function,spelling) \
VKAPI_ATTR void VKAPI_CALL function(VkCommandBuffer h,VkBuffer b,VkDeviceSize o,VkBuffer c,VkDeviceSize co,uint32_t n,uint32_t stride){indirectCountNamed(h,b,o,c,co,n,stride,spelling);}
MAVK_COUNT_COMMAND(drawIndirectCount,"vkCmdDrawIndirectCount")
MAVK_COUNT_COMMAND(drawIndirectCountKHR,"vkCmdDrawIndirectCountKHR")
MAVK_COUNT_COMMAND(drawIndirectCountAMD,"vkCmdDrawIndirectCountAMD")
MAVK_COUNT_COMMAND(drawIndexedIndirectCount,"vkCmdDrawIndexedIndirectCount")
MAVK_COUNT_COMMAND(drawIndexedIndirectCountKHR,"vkCmdDrawIndexedIndirectCountKHR")
MAVK_COUNT_COMMAND(drawIndexedIndirectCountAMD,"vkCmdDrawIndexedIndirectCountAMD")
#undef MAVK_COUNT_COMMAND
// These limited guarded resources currently have ONE queue/family owner. Do
// not pretend queue order transfers between independent queues. Native/default
// mode is unaffected. No CPU wait or external queue submission is performed.
std::unique_ptr<ImagePrelude> prepareImagePrelude(Device& s,VkQueue q){
    if(!s.guarded||!s.nullImages||s.nullImages->empty())return {};
    auto info=s.queueInfos.find(q);
    if(info==s.queueInfos.end()||info->second.flags||info->second.family!=s.core->queueFamily()||(s.imageQueue&&s.imageQueue!=q))throw std::invalid_argument("guarded null images require one unprotected queue/family");
    auto initialization=s.nullImages->pending();if(!initialization.size())return {};
    if(s.imagePreludes.size()>=14)throw std::invalid_argument("null image initialization prelude bound");
    // Reserve BEFORE submitting:never allocate/throw while retaining a command
    // already queued on the GPU. Seven formats in each admitted dimension.
    s.imagePreludes.reserve(s.imagePreludes.size()+1);
    auto out=std::make_unique<ImagePrelude>(std::move(initialization));out->commands=std::make_unique<mavk::CommandBatch>(*s.core);
    out->commands->begin();out->initialization.record(out->commands->handle());out->commands->end();return out;
}
void retainImagePrelude(Device& s,VkQueue q,std::unique_ptr<ImagePrelude> prelude){
    if(s.nullImages&&!s.nullImages->empty())s.imageQueue=q;
    if(prelude){
        prelude->initialization.submitted();std::fprintf(stderr,"MAVK_LAYER_IMAGE_INIT images=%zu prelude=1 cpuWaits=0\n",prelude->initialization.size());
        s.imagePreludes.push_back(std::move(prelude));
    }
}
bool rejectPreludeForTest(Device& s,const ImagePrelude* prelude){
    const auto test=std::getenv("MAVK_TEST_SUBMIT_RETRY");
    if(prelude&&!s.testSubmitRejected&&test&&std::strcmp(test,"1")==0){
        s.testSubmitRejected=true;std::fprintf(stderr,"MAVK_LAYER_IMAGE_TEST_REJECT beforeDriver=1\n");return true;
    }
    return false;
}
VKAPI_ATTR VkResult VKAPI_CALL queueSubmit(VkQueue q,uint32_t n,const VkSubmitInfo* submits,VkFence f){
    auto s=queueDevice(q);if(!s)return VK_ERROR_INITIALIZATION_FAILED;std::lock_guard lock(s->mutex);
    if(s->guarded)for(uint32_t i=0;i<n;i++)for(uint32_t j=0;j<submits[i].commandBufferCount;j++){auto c=s->commands.find(submits[i].pCommandBuffers[j]);if(c==s->commands.end()||c->second.failed||c->second.recording)return VK_ERROR_VALIDATION_FAILED_EXT;}
    try{
        if(s->guarded&&s->nullImages&&!s->nullImages->empty())for(uint32_t i=0;i<n;i++)for(auto p=static_cast<const VkBaseInStructure*>(submits[i].pNext);p;p=p->pNext)
            if(p->sType==VK_STRUCTURE_TYPE_DEVICE_GROUP_SUBMIT_INFO||(p->sType==VK_STRUCTURE_TYPE_PROTECTED_SUBMIT_INFO&&reinterpret_cast<const VkProtectedSubmitInfo*>(p)->protectedSubmit))throw std::invalid_argument("null image device-group/protected submit unsupported");
        auto prelude=prepareImagePrelude(*s,q);std::vector<VkSubmitInfo> combined;VkCommandBuffer cmd{};
        if(prelude){cmd=prelude->commands->handle();VkSubmitInfo init{VK_STRUCTURE_TYPE_SUBMIT_INFO};init.commandBufferCount=1;init.pCommandBuffers=&cmd;combined.reserve(size_t(n)+1);combined.push_back(init);for(uint32_t i=0;i<n;i++)combined.push_back(submits[i]);}
        if(rejectPreludeForTest(*s,prelude.get()))return VK_ERROR_OUT_OF_HOST_MEMORY;
        const bool eligible=gpuSubmitEligible(n,submits,bool(prelude));
        auto sample=prepareGpuSample(*s,q,eligible?submits[0].commandBufferCount:0,eligible,"submit1");
        std::vector<VkCommandBuffer> measuredCommands;VkSubmitInfo measured{};
        if(sample&&!gpuDecorateSubmit(submits[0],*sample,measuredCommands,measured)){s->gpuProfile=false;sample=nullptr;}
        const auto start=s->profile?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};
        auto r=s->fn<PFN_vkQueueSubmit>("vkQueueSubmit")(q,prelude?n+1:n,sample?&measured:prelude?combined.data():submits,f);
        if(sample)sample->submitted=r==VK_SUCCESS;
        if(s->profile){s->profileSubmitNs+=uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-start).count());s->profileSubmitCalls++;reportProfileLocked(*s);}
        if(r==VK_SUCCESS){retainImagePrelude(*s,q,std::move(prelude));s->submissions++;}return r;
    }catch(const std::exception& e){s->gap("vkQueueSubmit image prelude",e.what());return VK_ERROR_FEATURE_NOT_PRESENT;}
}
VkResult queueSubmit2Named(VkQueue q,uint32_t n,const VkSubmitInfo2* submits,VkFence f,const char* name){
    auto s=queueDevice(q);if(!s)return VK_ERROR_INITIALIZATION_FAILED;std::lock_guard lock(s->mutex);
    if(s->guarded)for(uint32_t i=0;i<n;i++)for(uint32_t j=0;j<submits[i].commandBufferInfoCount;j++){auto c=s->commands.find(submits[i].pCommandBufferInfos[j].commandBuffer);if(c==s->commands.end()||c->second.failed||c->second.recording)return VK_ERROR_VALIDATION_FAILED_EXT;}
    try{
        if(s->guarded&&s->nullImages&&!s->nullImages->empty())for(uint32_t i=0;i<n;i++){
            if(submits[i].flags&VK_SUBMIT_PROTECTED_BIT)throw std::invalid_argument("null image protected submit2 unsupported");
            for(uint32_t j=0;j<submits[i].commandBufferInfoCount;j++)if(submits[i].pCommandBufferInfos[j].deviceMask>1)throw std::invalid_argument("null image device group submit2 unsupported");
        }
        auto prelude=prepareImagePrelude(*s,q);std::vector<VkSubmitInfo2> combined;VkCommandBufferSubmitInfo cmd{VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
        if(prelude){cmd.commandBuffer=prelude->commands->handle();cmd.deviceMask=1;VkSubmitInfo2 init{VK_STRUCTURE_TYPE_SUBMIT_INFO_2};init.commandBufferInfoCount=1;init.pCommandBufferInfos=&cmd;combined.reserve(size_t(n)+1);combined.push_back(init);for(uint32_t i=0;i<n;i++)combined.push_back(submits[i]);}
        if(rejectPreludeForTest(*s,prelude.get()))return VK_ERROR_OUT_OF_HOST_MEMORY;
        const bool eligible=gpuSubmitEligible(n,submits,bool(prelude));
        auto sample=prepareGpuSample(*s,q,eligible?submits[0].commandBufferInfoCount:0,eligible,name);
        std::vector<VkCommandBufferSubmitInfo> measuredCommands;VkSubmitInfo2 measured{};
        if(sample&&!gpuDecorateSubmit(submits[0],*sample,measuredCommands,measured)){s->gpuProfile=false;sample=nullptr;}
        const auto start=s->profile?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};
        auto r=s->fn<PFN_vkQueueSubmit2>(name)(q,prelude?n+1:n,sample?&measured:prelude?combined.data():submits,f);
        if(sample)sample->submitted=r==VK_SUCCESS;
        if(s->profile){s->profileSubmitNs+=uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-start).count());s->profileSubmitCalls++;reportProfileLocked(*s);}
        if(r==VK_SUCCESS){retainImagePrelude(*s,q,std::move(prelude));s->submissions++;}return r;
    }catch(const std::exception& e){s->gap(name,e.what());return VK_ERROR_FEATURE_NOT_PRESENT;}
}
VKAPI_ATTR VkResult VKAPI_CALL queueSubmit2(VkQueue q,uint32_t n,const VkSubmitInfo2* s,VkFence f){return queueSubmit2Named(q,n,s,f,"vkQueueSubmit2");}
VKAPI_ATTR VkResult VKAPI_CALL queueSubmit2KHR(VkQueue q,uint32_t n,const VkSubmitInfo2* s,VkFence f){return queueSubmit2Named(q,n,s,f,"vkQueueSubmit2KHR");}

// Opt-in wall-clock attribution. Native waits/present execute WITHOUT the Mavk
// mutex; these counters do not measure GPU timestamps or unique visible frames.
VKAPI_ATTR VkResult VKAPI_CALL profileWaitForFences(VkDevice h,uint32_t n,const VkFence* fences,VkBool32 all,uint64_t timeout){
    auto s=device(h);if(!s)return VK_ERROR_INITIALIZATION_FAILED;
    auto fn=s->fn<PFN_vkWaitForFences>("vkWaitForFences");if(!s->profile)return fn(h,n,fences,all,timeout);
    const auto start=std::chrono::steady_clock::now();const auto result=fn(h,n,fences,all,timeout);
    const auto elapsed=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-start).count();
    {std::lock_guard lock(s->mutex);s->profileWaitNs+=uint64_t(elapsed);s->profileWaitCalls++;reportProfileLocked(*s);}return result;
}
VkResult profileWaitSemaphoresNamed(VkDevice h,const VkSemaphoreWaitInfo* info,uint64_t timeout,const char* name){
    auto s=device(h);if(!s)return VK_ERROR_INITIALIZATION_FAILED;
    auto fn=s->fn<PFN_vkWaitSemaphores>(name);if(!s->profile)return fn(h,info,timeout);
    const auto start=std::chrono::steady_clock::now();const auto result=fn(h,info,timeout);
    const auto elapsed=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-start).count();
    {std::lock_guard lock(s->mutex);s->profileSemaphoreNs+=uint64_t(elapsed);s->profileSemaphoreCalls++;reportProfileLocked(*s);}return result;
}
VKAPI_ATTR VkResult VKAPI_CALL profileWaitSemaphores(VkDevice h,const VkSemaphoreWaitInfo* info,uint64_t timeout){return profileWaitSemaphoresNamed(h,info,timeout,"vkWaitSemaphores");}
VKAPI_ATTR VkResult VKAPI_CALL profileWaitSemaphoresKHR(VkDevice h,const VkSemaphoreWaitInfo* info,uint64_t timeout){return profileWaitSemaphoresNamed(h,info,timeout,"vkWaitSemaphoresKHR");}
VKAPI_ATTR VkResult VKAPI_CALL profilePresent(VkQueue q,const VkPresentInfoKHR* info){
    auto s=queueDevice(q);if(!s)return VK_ERROR_INITIALIZATION_FAILED;
    auto fn=s->fn<PFN_vkQueuePresentKHR>("vkQueuePresentKHR");if(!s->profile)return fn(q,info);
    const auto start=std::chrono::steady_clock::now();const auto result=fn(q,info);const auto now=std::chrono::steady_clock::now();
    std::lock_guard lock(s->mutex);s->profilePresentNs+=uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(now-start).count());s->profilePresents++;
    reportProfileLocked(*s);return result;
}
