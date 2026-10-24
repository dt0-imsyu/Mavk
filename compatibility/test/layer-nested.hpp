void nestedBuffers(Root& root,mavk::DeviceContext& context,const char* shaderPath,uint32_t expectedRead=0,bool recordBenchmark=false){
    struct Handles {
        VkDevice device;VkDescriptorSetLayout schema{};VkDescriptorPool pool{};VkPipelineLayout layout{};
        ~Handles(){if(layout)vkDestroyPipelineLayout(device,layout,nullptr);if(pool)vkDestroyDescriptorPool(device,pool,nullptr);if(schema)vkDestroyDescriptorSetLayout(device,schema,nullptr);}
    } handles{root.device};
    const bool standardUbo=std::getenv("MAVK_TEST_STD430_UBO")!=nullptr;
    const auto inputType=standardUbo?VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    mavk::Buffer input(context,128,standardUbo?VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT:VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    mavk::Buffer output(context,128,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    auto source=static_cast<uint32_t*>(input.map()),result=static_cast<uint32_t*>(output.map());
    std::fill(source,source+32,0xA5A5A5A5u);std::fill(result,result+32,0xCCCCCCCCu);input.flush();output.flush();
    VkDescriptorSetLayoutBinding bindings[]={{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}};
    bindings[0].descriptorType=inputType;
    VkDescriptorSetLayoutCreateInfo schema{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};schema.bindingCount=2;schema.pBindings=bindings;ok(vkCreateDescriptorSetLayout(root.device,&schema,nullptr,&handles.schema));
    VkDescriptorPoolSize sizes[]={{inputType,standardUbo?1u:2u},{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1}};VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=1;pool.poolSizeCount=standardUbo?2:1;pool.pPoolSizes=sizes;if(std::getenv("MAVK_TEST_DEFERRED_USED"))pool.flags=VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;ok(vkCreateDescriptorPool(root.device,&pool,nullptr,&handles.pool));
    VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};allocate.descriptorPool=handles.pool;allocate.descriptorSetCount=1;allocate.pSetLayouts=&handles.schema;VkDescriptorSet set{};ok(vkAllocateDescriptorSets(root.device,&allocate,&set));
    VkDescriptorBufferInfo infos[]={{input.handle(),0,4},{output.handle(),0,4}};VkWriteDescriptorSet writes[2]{};
    for(unsigned i=0;i<2;i++){auto& w=writes[i];w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w.dstSet=set;w.dstBinding=i;w.descriptorCount=1;w.descriptorType=i?VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:inputType;w.pBufferInfo=infos+i;}if(!std::getenv("MAVK_TEST_DEFERRED_USED"))vkUpdateDescriptorSets(root.device,2,writes,0,nullptr);
    VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};layout.setLayoutCount=1;layout.pSetLayouts=&handles.schema;ok(vkCreatePipelineLayout(root.device,&layout,nullptr,&handles.layout));
    auto code=words(shaderPath);mavk::Shader shader(context,code.data(),code.size()*4);VkComputePipelineCreateInfo info{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};info.layout=handles.layout;info.stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;info.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;info.stage.module=shader.handle();info.stage.pName="main";
    VkPipelineShaderStageRequiredSubgroupSizeCreateInfo subgroup{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO};subgroup.requiredSubgroupSize=16;
    if(std::getenv("MAVK_TEST_SUBGROUP16"))info.stage.pNext=&subgroup;
    mavk::Pipeline pipeline(context,info);
    if(std::getenv("MAVK_TEST_DEFERRED_USED"))throw std::runtime_error("FAIL: statically used deferred layout admitted; no dispatch performed");
    if(recordBenchmark)for(unsigned sample=0;sample<5;sample++){
        mavk::CommandBatch recording(context);recording.begin();
        vkCmdBindPipeline(recording.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,pipeline.handle());
        vkCmdBindDescriptorSets(recording.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,handles.layout,0,1,&set,0,nullptr);
        const auto start=std::chrono::steady_clock::now();
        for(unsigned i=0;i<8192;i++)vkCmdDispatch(recording.handle(),1,1,1);
        const auto elapsed=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-start).count();
        recording.end();
        std::cout<<"RECORD_OVERHEAD_SAMPLE index="<<sample<<" calls=8192 wallNs="<<elapsed<<" submitted=0\n";
    }
    mavk::CommandBatch cmd(context);mavk::Fence fence(context);cmd.begin();vkCmdBindPipeline(cmd.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,pipeline.handle());vkCmdBindDescriptorSets(cmd.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,handles.layout,0,1,&set,0,nullptr);vkCmdDispatch(cmd.handle(),1,1,1);cmd.hostReadBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_WRITE_BIT);cmd.end();auto command=cmd.handle();VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;ok(context.submit(root.queue,submit,fence.handle()));ok(fence.wait(5000000000ull));output.invalidate();input.invalidate();
    for(unsigned i=0;i<32;i++){if(result[i]!=(i?0xCCCCCCCCu:expectedRead))throw std::runtime_error("nested buffer read/suppressed-write mismatch word="+std::to_string(i));if(source[i]!=0xA5A5A5A5u)throw std::runtime_error("nested input modified");}
    if(expectedRead)std::cout<<"WIDE_BUFFER_VALID_GPU_PASS readValue="<<expectedRead<<" writeSuppressed=1 sentinelWords=31 inputWords=32 descriptorRange=4 allocationBytes=128\n";
    else std::cout<<"NESTED_BUFFER_GPU_PASS readZero=1 writeSuppressed=1 sentinelWords=31 inputWords=32 descriptorRange=4 allocationBytes=128\n";
}
