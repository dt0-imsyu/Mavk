void pollGpuSamples(Device& s){
    if(s.gpuSamples.empty())return;
    for(auto& sample:s.gpuSamples){
        if(!sample->submitted||sample->reported)continue;
        uint64_t values[4]{};
        const auto result=s.fn<PFN_vkGetQueryPoolResults>("vkGetQueryPoolResults")(s.handle,sample->queries,0,2,sizeof(values),values,2*sizeof(uint64_t),VK_QUERY_RESULT_64_BIT|VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);
        if(result==VK_NOT_READY)continue;
        if(result!=VK_SUCCESS){std::fprintf(stderr,"MAVK_GPU_PROBE_ERROR operation=query result=%d\n",result);sample->reported=true;continue;}
        if(!values[1]||!values[3])continue;
        const uint64_t mask=sample->bits==64?~uint64_t(0):(uint64_t(1)<<sample->bits)-1;
        const auto ticks=(values[2]-values[0])&mask;
        const auto ms=double(ticks)*double(s.core->properties().limits.timestampPeriod)/1e6;
        std::fprintf(stderr,"MAVK_GPU_BATCH api=%s queue=%p sequence=%llu commandBuffers=%u validBits=%u periodNs=%.6f start=%llu end=%llu spanMs=%.6f scope=no-semaphore-wait-batch notGpuBusyPercent=1\n",sample->api,(void*)sample->queue,(unsigned long long)sample->sequence,sample->commands,sample->bits,double(s.core->properties().limits.timestampPeriod),(unsigned long long)values[0],(unsigned long long)values[2],ms);
        sample->reported=true;
    }
}

GpuSample* prepareGpuSample(Device& s,VkQueue queue,uint32_t commands,bool eligible,const char* api){
    pollGpuSamples(s);
    if(!s.gpuProfile||!eligible||s.gpuSamples.size()>=64||std::chrono::steady_clock::now()<s.gpuNext)return nullptr;
    auto at=s.queueInfos.find(queue);if(at==s.queueInfos.end()||at->second.flags||at->second.family>=s.gpuFamilies.size())return nullptr;
    const auto& family=s.gpuFamilies[at->second.family];
    if(!family.timestampValidBits||family.timestampValidBits>64||!(family.queueFlags&(VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT))||!(s.core->properties().limits.timestampPeriod>0))return nullptr;
    s.gpuNext=std::chrono::steady_clock::now()+std::chrono::milliseconds(100);
    try{
        auto sample=std::make_unique<GpuSample>();sample->device=s.handle;sample->queue=queue;sample->bits=family.timestampValidBits;sample->commands=commands;sample->sequence=s.submissions;sample->api=api;
        sample->destroyPool=s.fn<PFN_vkDestroyCommandPool>("vkDestroyCommandPool");sample->destroyQueries=s.fn<PFN_vkDestroyQueryPool>("vkDestroyQueryPool");
        auto check=[](VkResult r){if(r!=VK_SUCCESS)throw std::runtime_error("GPU timestamp resource/recording failure");};
        VkQueryPoolCreateInfo query{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};query.queryType=VK_QUERY_TYPE_TIMESTAMP;query.queryCount=2;
        check(s.fn<PFN_vkCreateQueryPool>("vkCreateQueryPool")(s.handle,&query,nullptr,&sample->queries));
        // A queued reset has not necessarily executed when the host first polls.
        auto reset=s.fn<PFN_vkResetQueryPool>("vkResetQueryPool");
        if(!reset)reset=s.fn<PFN_vkResetQueryPool>("vkResetQueryPoolEXT");
        if(!reset)throw std::runtime_error("enabled host query reset entry point unavailable");
        reset(s.handle,sample->queries,0,2);
        VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pool.queueFamilyIndex=at->second.family;
        check(s.fn<PFN_vkCreateCommandPool>("vkCreateCommandPool")(s.handle,&pool,nullptr,&sample->pool));
        VkCommandBufferAllocateInfo allocation{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};allocation.commandPool=sample->pool;allocation.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;allocation.commandBufferCount=2;
        check(s.fn<PFN_vkAllocateCommandBuffers>("vkAllocateCommandBuffers")(s.handle,&allocation,sample->marks));
        for(uint32_t i=0;i<2;i++){
            VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            check(s.fn<PFN_vkBeginCommandBuffer>("vkBeginCommandBuffer")(sample->marks[i],&begin));
            if(i==0)s.fn<PFN_vkCmdResetQueryPool>("vkCmdResetQueryPool")(sample->marks[i],sample->queries,0,2);
            s.fn<PFN_vkCmdWriteTimestamp>("vkCmdWriteTimestamp")(sample->marks[i],i?VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT:VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,sample->queries,i);
            check(s.fn<PFN_vkEndCommandBuffer>("vkEndCommandBuffer")(sample->marks[i]));
        }
        auto result=sample.get();s.gpuSamples.push_back(std::move(sample));return result;
    }catch(const std::exception& e){s.gpuProfile=false;std::fprintf(stderr,"MAVK_GPU_PROBE_ERROR operation=prepare reason=%s originalSubmission=preserved\n",e.what());return nullptr;}
}
