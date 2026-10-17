// Root and validation shared with existing native gates. No VM or Bridge.
std::string timelineGate(bool validated,ValidationCapture& validation){
    Root root(validation);root.init(validated,true);
    mavk::DeviceContext context(root.physical,root.device,root.family,true);
    uint64_t verified=0;unsigned rejected=0;double elapsed=0;
    {
        // Reject unenabled context before issuing any unsupported driver call.
        mavk::DeviceContext disabled(root.physical,root.device,root.family);
        try{mavk::TimelineSemaphore invalid(disabled);throw std::runtime_error("Unenabled timeline accepted");}catch(const std::invalid_argument&){rejected++;}
        mavk::TimelineSemaphore timeline(context);
        mavk::Buffer source(context,16384,VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        mavk::Buffer destination(context,16384,VK_BUFFER_USAGE_TRANSFER_DST_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        mavk::CommandBatch fill(context),copy(context);
        // Declared last: drain before referenced objects even on error.
        struct Drain{VkDevice d;~Drain(){vkDeviceWaitIdle(d);}} drain{root.device};
        if(timeline.counter()!=0||timeline.wait(1,0)!=VK_TIMEOUT)throw std::runtime_error("Timeline initial/timeout semantics");
        VkSemaphore semaphore=timeline.handle();auto begin=Clock::now();
        for(unsigned round=0;round<2;round++){
            uint32_t pattern=round?0xa5731f02u:0x173be945u;uint64_t ready=round*2+1,done=ready+1;
            fill.begin();vkCmdFillBuffer(fill.handle(),source.handle(),0,16384,pattern);fill.end();
            copy.begin();VkBufferCopy region{0,0,16384};vkCmdCopyBuffer(copy.handle(),source.handle(),destination.handle(),1,&region);copy.hostReadBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_TRANSFER_WRITE_BIT);copy.end();
            VkCommandBuffer first=fill.handle(),second=copy.handle();
            VkTimelineSemaphoreSubmitInfo signal{VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO};signal.signalSemaphoreValueCount=1;signal.pSignalSemaphoreValues=&ready;
            VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.pNext=&signal;submit.commandBufferCount=1;submit.pCommandBuffers=&first;submit.signalSemaphoreCount=1;submit.pSignalSemaphores=&semaphore;
            ok(context.submit(root.queue,submit,VK_NULL_HANDLE),"timeline fill submit");
            VkPipelineStageFlags stage=VK_PIPELINE_STAGE_TRANSFER_BIT;
            VkTimelineSemaphoreSubmitInfo dependency{VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO};dependency.waitSemaphoreValueCount=1;dependency.pWaitSemaphoreValues=&ready;dependency.signalSemaphoreValueCount=1;dependency.pSignalSemaphoreValues=&done;
            submit.pNext=&dependency;submit.pCommandBuffers=&second;submit.waitSemaphoreCount=1;submit.pWaitSemaphores=&semaphore;submit.pWaitDstStageMask=&stage;
            ok(context.submit(root.queue,submit,VK_NULL_HANDLE),"timeline dependent copy submit");
            ok(timeline.wait(done,5000000000ull),"timeline final CPU wait");
            if(timeline.counter()!=done)throw std::runtime_error("Timeline GPU counter mismatch");
            const auto* words=static_cast<const uint32_t*>(destination.map());destination.invalidate();
            for(unsigned i=0;i<4096;i++){if(words[i]!=pattern)throw std::runtime_error("Timeline ordered GPU data mismatch");verified++;}
        }
        timeline.signal(5);if(timeline.counter()!=5||timeline.wait(5,0)!=VK_SUCCESS)throw std::runtime_error("Timeline host signal");
        for(uint64_t value:{0ull,5ull})try{timeline.signal(value);throw std::runtime_error("Nonadvancing host signal accepted");}catch(const std::invalid_argument&){rejected++;}
        elapsed=ms(begin,Clock::now());
    }
    if(!context.counters().empty()||context.counters().totalSubmissions!=4||verified!=8192||rejected!=3)throw std::runtime_error("Timeline gate coverage/teardown");
    std::ostringstream out;out<<"{\"result\":\"NATIVE_CORE_TIMELINE_PASS\",\"device\":"<<quote(root.properties.deviceName)<<",\"native_timeline_enabled\":true,\"verified_words\":"<<verified<<",\"gpu_submissions\":4,\"negative_tests\":3,\"timeout_tests\":1,\"host_waits_for_gpu\":2,\"final_counter\":5,\"core_live_zero\":true,\"total_ms\":"<<elapsed<<",\"scope\":\"Native Vulkan1.2 timeline; GPU fill->dependent copy,2 rounds,no intermediate CPU fence. NOT adapter/CTS/performance proof.\"}";return out.str();
}
