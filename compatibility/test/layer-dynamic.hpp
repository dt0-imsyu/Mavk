void dynamicBuffers(Root& root,mavk::DeviceContext& context,const char* shaderPath,unsigned gapMode=0,bool resubmit=false,bool inflight=false){
    struct Layouts {
        VkDevice device{};VkDescriptorSetLayout sets[3]{};VkDescriptorPool pool{};VkPipelineLayout pipeline{};
        ~Layouts(){if(pipeline)vkDestroyPipelineLayout(device,pipeline,nullptr);if(pool)vkDestroyDescriptorPool(device,pool,nullptr);for(auto set:sets)if(set)vkDestroyDescriptorSetLayout(device,set,nullptr);}
    } layouts;layouts.device=root.device;
    const bool emptyGap=gapMode!=0,selectedBindings=gapMode==4||gapMode==5;const uint32_t destinationSet=emptyGap?2:1;
    VkPhysicalDeviceProperties properties{};vkGetPhysicalDeviceProperties(root.gpu,&properties);
    const uint32_t stride=uint32_t(std::max<VkDeviceSize>(16,std::max(properties.limits.minUniformBufferOffsetAlignment,properties.limits.minStorageBufferOffsetAlignment)));
    if(stride>1048576)throw std::runtime_error("dynamic fixture alignment budget");
    const auto host=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,coherent=VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    mavk::Buffer input(context,16*stride,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,host,coherent),output(context,16*stride,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,host,coherent);
    auto data=static_cast<uint32_t*>(input.map()),result=static_cast<uint32_t*>(output.map());
    std::fill(data,data+4*stride,0xBAD0BAD0u);std::fill(result,result+4*stride,0xCCCCCCCCu);
    auto value=[](unsigned page){return 1000+13*page;};for(unsigned page=0;page<16;page++)data[page*stride/4]=value(page);input.flush();output.flush();
    VkDescriptorSetLayoutBinding first[]={{7,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,2,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{3,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,2,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}};
    VkDescriptorSetLayoutBinding second[]={{9,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,2,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{2,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}};
    std::vector<VkDescriptorSetLayoutBinding> firstLayout(first,first+2),secondLayout(second,second+2);
    if(selectedBindings){
        firstLayout.insert(firstLayout.end(),{{1,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{5,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{8,VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{11,VK_DESCRIPTOR_TYPE_SAMPLER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}});
        secondLayout.insert(secondLayout.end(),{{0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{5,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{11,VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}});
    }
    VkDescriptorSetLayoutCreateInfo l{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};l.bindingCount=uint32_t(firstLayout.size());l.pBindings=firstLayout.data();ok(vkCreateDescriptorSetLayout(root.device,&l,nullptr,&layouts.sets[0]));l.bindingCount=uint32_t(secondLayout.size());l.pBindings=secondLayout.data();ok(vkCreateDescriptorSetLayout(root.device,&l,nullptr,&layouts.sets[destinationSet]));
    if(emptyGap){VkDescriptorSetLayoutBinding unused{21,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,3,VK_SHADER_STAGE_COMPUTE_BIT,nullptr};l.bindingCount=gapMode==2?1:0;l.pBindings=l.bindingCount?&unused:nullptr;
        VkDescriptorBindingFlags flags=VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT|VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT|VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
        VkDescriptorSetLayoutBindingFlagsCreateInfo chain{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO};chain.bindingCount=1;chain.pBindingFlags=&flags;
        if(gapMode==2&&std::getenv("MAVK_TEST_DEFERRED_UNUSED")){unused.descriptorType=VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;l.flags=VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;l.pNext=&chain;}
        ok(vkCreateDescriptorSetLayout(root.device,&l,nullptr,&layouts.sets[1]));}
    VkDescriptorPoolSize sizes[]={{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,selectedBindings?7u:3u},{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,4},{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,1},{VK_DESCRIPTOR_TYPE_SAMPLER,1},{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,1}};
    VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=2;pool.poolSizeCount=selectedBindings?5:2;pool.pPoolSizes=sizes;ok(vkCreateDescriptorPool(root.device,&pool,nullptr,&layouts.pool));
    VkDescriptorSetLayout allocationLayouts[]={layouts.sets[0],layouts.sets[destinationSet]};VkDescriptorSet sets[2]{};VkDescriptorSetAllocateInfo a{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};a.descriptorPool=layouts.pool;a.descriptorSetCount=2;a.pSetLayouts=allocationLayouts;ok(vkAllocateDescriptorSets(root.device,&a,sets));
    VkDescriptorBufferInfo infos[]={{input.handle(),stride,4},{input.handle(),stride,4},{input.handle(),stride,16},{input.handle(),stride,16},{input.handle(),stride,16},{output.handle(),stride,4},{output.handle(),stride,4}};
    VkWriteDescriptorSet writes[4]{};const uint32_t counts[]={2,2,1,2},bindings[]={3,7,2,9},indices[]={0,2,4,5};
    for(unsigned i=0;i<4;i++){auto& w=writes[i];w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w.dstSet=sets[i/2];w.dstBinding=bindings[i];w.descriptorCount=gapMode==5&&i==0?1:counts[i];w.descriptorType=i==0||i==3?VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;w.pBufferInfo=infos+indices[i];}vkUpdateDescriptorSets(root.device,4,writes,0,nullptr);
    VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};pl.setLayoutCount=destinationSet+1;pl.pSetLayouts=layouts.sets;ok(vkCreatePipelineLayout(root.device,&pl,nullptr,&layouts.pipeline));
    auto code=words(shaderPath);mavk::Shader shader(context,code.data(),code.size()*4);VkComputePipelineCreateInfo p{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};p.layout=layouts.pipeline;p.stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;p.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;p.stage.module=shader.handle();p.stage.pName="main";mavk::Pipeline pipeline(context,p);
    mavk::CommandBatch cmd(context);mavk::Fence fence(context);cmd.begin();vkCmdBindPipeline(cmd.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,pipeline.handle());
    uint32_t offsets[]={stride,2*stride,3*stride,4*stride,5*stride,0,2*stride};
    auto bindLeft=[&](const uint32_t* values){uint32_t expanded[]={11*stride,values[0],values[1],13*stride,values[2],values[3]};vkCmdBindDescriptorSets(cmd.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,layouts.pipeline,0,1,sets,selectedBindings?6:4,selectedBindings?expanded:values);std::fill(expanded,expanded+6,0);};
    auto bindRight=[&](const uint32_t* values){uint32_t expanded[]={14*stride,values[0],12*stride,values[1],values[2]};vkCmdBindDescriptorSets(cmd.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,layouts.pipeline,destinationSet,1,sets+1,selectedBindings?5:3,selectedBindings?expanded:values);std::fill(expanded,expanded+5,0);};
    auto bindBoth=[&](const uint32_t* values){if(emptyGap){bindLeft(values);if(gapMode!=3)bindRight(values+4);}else vkCmdBindDescriptorSets(cmd.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,layouts.pipeline,0,2,sets,7,values);};
    bindBoth(offsets);
    std::fill(offsets,offsets+7,0);vkCmdDispatch(cmd.handle(),1,1,1);
    if(gapMode==3){if(vkEndCommandBuffer(cmd.handle())!=VK_ERROR_VALIDATION_FAILED_EXT)throw std::runtime_error("missing statically used set not rejected");std::cout<<"STATIC_USED_SET_REJECTION_PASS missingSet=2 submissions=0\n";return;}
    if(gapMode==5){if(vkEndCommandBuffer(cmd.handle())!=VK_ERROR_VALIDATION_FAILED_EXT)throw std::runtime_error("unwritten statically used element not rejected");std::cout<<"STATIC_USED_BINDING_REJECTION_PASS unwrittenArrayElement=1 submissions=0\n";return;}
    uint32_t next[]={6*stride,4*stride,6*stride};bindRight(next);std::fill(next,next+3,0);vkCmdDispatch(cmd.handle(),1,1,1);
    uint32_t left[]={2*stride,3*stride,4*stride,5*stride},right[]={7*stride,8*stride,10*stride};
    bindLeft(left);bindRight(right);vkCmdDispatch(cmd.handle(),1,1,1);
    cmd.hostReadBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_WRITE_BIT);cmd.end();auto h=cmd.handle();VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&h;ok(context.submit(root.queue,submit,fence.handle()));ok(fence.wait(5000000000ull));output.invalidate();
    const unsigned sourcePages[3][5]={{2,3,4,5,6},{2,3,4,5,7},{3,4,5,6,8}},outputPages[3][2]={{1,3},{5,7},{9,11}};std::map<unsigned,uint32_t> expected;
    for(unsigned c=0;c<3;c++){const auto* q=sourcePages[c];expected[outputPages[c][0]]=value(q[0])+3*value(q[1])+5*value(q[2])+7*value(q[3])+11*value(q[4]);expected[outputPages[c][1]]=value(q[0])^value(q[1])^value(q[2])^value(q[3])^value(q[4]);}
    for(unsigned i=0;i<4*stride;i++){auto at=expected.find(i*4/stride);const uint32_t want=i%(stride/4)==0&&at!=expected.end()?at->second:0xCCCCCCCCu;if(result[i]!=want)throw std::runtime_error("dynamic array/partial-bind offset mismatch word="+std::to_string(i));}
    fence.reset();
    if(resubmit){ok(vkResetCommandBuffer(cmd.handle(),0));VkCommandBufferBeginInfo reusable{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};reusable.flags=inflight?VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT:0;ok(vkBeginCommandBuffer(cmd.handle(),&reusable));}else cmd.begin();
    if(inflight){VkMemoryBarrier order{VK_STRUCTURE_TYPE_MEMORY_BARRIER};order.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;order.dstAccessMask=VK_ACCESS_SHADER_WRITE_BIT;vkCmdPipelineBarrier(cmd.handle(),VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&order,0,nullptr,0,nullptr);}
    std::fill(result,result+4*stride,0xCCCCCCCCu);output.flush();
    const unsigned pages[]={2,3,4,5,6,1,3};for(unsigned i=0;i<7;i++){infos[i].offset=pages[i]*stride;infos[i].range=VK_WHOLE_SIZE;}vkUpdateDescriptorSets(root.device,4,writes,0,nullptr);
    vkCmdBindPipeline(cmd.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,pipeline.handle());uint32_t zeros[7]{};bindBoth(zeros);vkCmdDispatch(cmd.handle(),1,1,1);cmd.hostReadBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_WRITE_BIT);cmd.end();h=cmd.handle();ok(context.submit(root.queue,submit,fence.handle()));ok(fence.wait(5000000000ull));output.invalidate();
    for(unsigned i=0;i<4*stride;i++){const uint32_t want=i==stride/4?expected.at(1):i==3*stride/4?expected.at(3):0xCCCCCCCCu;if(result[i]!=want)throw std::runtime_error("dynamic WHOLE_SIZE/zero-offset mismatch");}
    if(resubmit){
        if(inflight){
            mavk::TimelineSemaphore gate(context);mavk::Fence secondFence(context);
            auto semaphore=gate.handle();uint64_t value=1;VkPipelineStageFlags stage=VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            VkTimelineSemaphoreSubmitInfo timeline{VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO};timeline.waitSemaphoreValueCount=1;timeline.pWaitSemaphoreValues=&value;
            auto blocked=submit;blocked.pNext=&timeline;blocked.waitSemaphoreCount=1;blocked.pWaitSemaphores=&semaphore;blocked.pWaitDstStageMask=&stage;
            std::fill(result,result+4*stride,0xCCCCCCCCu);output.flush();fence.reset();
            ok(context.submit(root.queue,blocked,fence.handle()));
            const auto second=context.submit(root.queue,blocked,secondFence.handle());
            if(second!=VK_SUCCESS){gate.signal(1);ok(fence.wait(5000000000ull));ok(second);}
            const bool pending=vkGetFenceStatus(root.device,fence.handle())==VK_NOT_READY&&vkGetFenceStatus(root.device,secondFence.handle())==VK_NOT_READY;
            gate.signal(1);ok(fence.wait(5000000000ull));ok(secondFence.wait(5000000000ull));output.invalidate();
            if(!pending)throw std::runtime_error("timeline did not hold both submissions pending");
            for(unsigned i=0;i<4*stride;i++){const uint32_t want=i==stride/4?expected.at(1):i==3*stride/4?expected.at(3):0xCCCCCCCCu;if(result[i]!=want)throw std::runtime_error("inflight descriptor replay mismatch");}
            std::cout<<"INFLIGHT_COMMAND_GPU_PASS blockedSubmissions=2 fencesInitiallyUnsignaled=2 hostTimelineRelease=1 simultaneousUse=1\n";
        }else for(unsigned replay=0;replay<2;replay++){
            std::fill(result,result+4*stride,0xCCCCCCCCu);output.flush();fence.reset();
            ok(context.submit(root.queue,submit,fence.handle()));ok(fence.wait(5000000000ull));output.invalidate();
            for(unsigned i=0;i<4*stride;i++){const uint32_t want=i==stride/4?expected.at(1):i==3*stride/4?expected.at(3):0xCCCCCCCCu;if(result[i]!=want)throw std::runtime_error("retained descriptor replay mismatch");}
        }
        ok(vkResetCommandBuffer(cmd.handle(),0));
        std::cout<<"REUSABLE_COMMAND_GPU_PASS submissions=4 replays=2 checkedWords="<<8*stride<<" resetAfterCompletion=1 descriptorUpdatesAfterRecord=0\n";
    }
    std::cout<<"DYNAMIC_DESCRIPTOR_GPU_PASS dispatches=4 dynamicDescriptors=7 sets=2 checkedWords="<<8*stride<<" partialRebind=1 consumedImmediately=1 wholeSize=1"<<(selectedBindings?" unusedBindings=7 dynamicOffsets=11":gapMode==2?" unusedNonemptySlots=1":emptyGap?" emptySlots=1":"")<<'\n';
}
