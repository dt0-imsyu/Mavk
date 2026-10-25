void metadataArenaRetention(Root& root,mavk::DeviceContext& context,const char* shaderPath){
    constexpr uint32_t count=2048;
    VkPhysicalDeviceProperties properties{};vkGetPhysicalDeviceProperties(root.gpu,&properties);
    const auto stride=std::max(VkDeviceSize(128),properties.limits.minStorageBufferOffsetAlignment);
    if(stride>4096)throw std::runtime_error("metadata retention test allocation budget");
    struct Handles {
        VkDevice device;VkDescriptorSetLayout schema{};VkDescriptorPool pool{};VkPipelineLayout layout{};
        ~Handles(){if(layout)vkDestroyPipelineLayout(device,layout,nullptr);if(pool)vkDestroyDescriptorPool(device,pool,nullptr);if(schema)vkDestroyDescriptorSetLayout(device,schema,nullptr);}
    } handles{root.device};
    mavk::Buffer input(context,128,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    mavk::Buffer output(context,count*stride,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    auto source=static_cast<uint32_t*>(input.map()),result=static_cast<uint32_t*>(output.map());
    std::fill(source,source+32,0xA5A5A5A5u);input.flush();
    VkDescriptorSetLayoutBinding bindings[]={{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}};
    VkDescriptorSetLayoutCreateInfo schema{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};schema.bindingCount=2;schema.pBindings=bindings;ok(vkCreateDescriptorSetLayout(root.device,&schema,nullptr,&handles.schema));
    VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,2*count};
    VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=count;pool.poolSizeCount=1;pool.pPoolSizes=&size;ok(vkCreateDescriptorPool(root.device,&pool,nullptr,&handles.pool));
    std::vector<VkDescriptorSetLayout> layouts(count,handles.schema);std::vector<VkDescriptorSet> sets(count);
    VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};allocate.descriptorPool=handles.pool;allocate.descriptorSetCount=count;allocate.pSetLayouts=layouts.data();ok(vkAllocateDescriptorSets(root.device,&allocate,sets.data()));
    std::vector<VkDescriptorBufferInfo> infos(2*count);std::vector<VkWriteDescriptorSet> writes(2*count);
    for(uint32_t i=0;i<count;i++){
        infos[2*i]={input.handle(),0,i%2?128u:4u};infos[2*i+1]={output.handle(),i*stride,4};
        for(uint32_t j=0;j<2;j++){auto& w=writes[2*i+j];w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w.dstSet=sets[i];w.dstBinding=j;w.descriptorCount=1;w.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;w.pBufferInfo=&infos[2*i+j];}
    }
    vkUpdateDescriptorSets(root.device,uint32_t(writes.size()),writes.data(),0,nullptr);
    VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};layout.setLayoutCount=1;layout.pSetLayouts=&handles.schema;ok(vkCreatePipelineLayout(root.device,&layout,nullptr,&handles.layout));
    auto code=words(shaderPath);mavk::Shader shader(context,code.data(),code.size()*4);
    VkComputePipelineCreateInfo info{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};info.layout=handles.layout;info.stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;info.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;info.stage.module=shader.handle();info.stage.pName="main";mavk::Pipeline pipeline(context,info);
    mavk::CommandBatch cmd(context);mavk::Fence fence(context);
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};ok(vkBeginCommandBuffer(cmd.handle(),&begin));
    vkCmdBindPipeline(cmd.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,pipeline.handle());
    for(uint32_t i=0;i<count;i++){vkCmdBindDescriptorSets(cmd.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,handles.layout,0,1,&sets[i],0,nullptr);vkCmdDispatch(cmd.handle(),1,1,1);}
    cmd.hostReadBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_WRITE_BIT);cmd.end();
    auto command=cmd.handle();VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;
    for(unsigned replay=0;replay<2;replay++){
        std::fill(result,result+count*stride/4,0xCCCCCCCCu);output.flush();if(replay)fence.reset();
        ok(context.submit(root.queue,submit,fence.handle()));ok(fence.wait(5000000000ull));output.invalidate();input.invalidate();
        for(uint32_t i=0;i<count;i++)for(VkDeviceSize j=0;j<stride/4;j++)if(result[i*stride/4+j]!=(j?0xCCCCCCCCu:i%2?0xA5A5A5A5u:0u))throw std::runtime_error("metadata generation overwritten or descriptor range lost");
        for(unsigned i=0;i<32;i++)if(source[i]!=0xA5A5A5A5u)throw std::runtime_error("metadata retention input modified");
    }
    std::cout<<"METADATA_ARENA_GPU_PASS generations="<<count<<" checkedWords="<<2*count*stride/4<<" submissions=2 retainedAcrossCompletion=1 canaries=1\n";
}
