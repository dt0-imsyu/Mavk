void aggregateBuffers(Root& root,mavk::DeviceContext& context,const char* shaderPath){
    struct Handles {
        VkDevice device;VkDescriptorSetLayout schema{};VkDescriptorPool pool{};VkPipelineLayout layout{};
        ~Handles(){if(layout)vkDestroyPipelineLayout(device,layout,nullptr);if(pool)vkDestroyDescriptorPool(device,pool,nullptr);if(schema)vkDestroyDescriptorSetLayout(device,schema,nullptr);}
    } handles{root.device};
    mavk::Buffer input(context,128,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    mavk::Buffer output(context,128,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    auto source=static_cast<uint32_t*>(input.map()),result=static_cast<uint32_t*>(output.map());
    auto value=[](unsigned i){return 0x3F800000u+i*0x00100000u;};
    for(unsigned i=0;i<32;i++)source[i]=value(i);input.flush();
    VkDescriptorSetLayoutBinding bindings[]={{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}};
    VkDescriptorSetLayoutCreateInfo schema{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};schema.bindingCount=2;schema.pBindings=bindings;ok(vkCreateDescriptorSetLayout(root.device,&schema,nullptr,&handles.schema));
    VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,2};VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=1;pool.poolSizeCount=1;pool.pPoolSizes=&size;ok(vkCreateDescriptorPool(root.device,&pool,nullptr,&handles.pool));
    VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};allocate.descriptorPool=handles.pool;allocate.descriptorSetCount=1;allocate.pSetLayouts=&handles.schema;VkDescriptorSet set{};ok(vkAllocateDescriptorSets(root.device,&allocate,&set));
    VkDescriptorBufferInfo infos[]={{input.handle(),0,8},{output.handle(),0,8}};VkWriteDescriptorSet writes[2]{};
    for(unsigned i=0;i<2;i++){auto& w=writes[i];w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w.dstSet=set;w.dstBinding=i;w.descriptorCount=1;w.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;w.pBufferInfo=infos+i;}
    VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};layout.setLayoutCount=1;layout.pSetLayouts=&handles.schema;ok(vkCreatePipelineLayout(root.device,&layout,nullptr,&handles.layout));
    auto code=words(shaderPath);mavk::Shader shader(context,code.data(),code.size()*4);VkComputePipelineCreateInfo info{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};info.layout=handles.layout;info.stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;info.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;info.stage.module=shader.handle();info.stage.pName="main";mavk::Pipeline pipeline(context,info);
    for(unsigned phase=0;phase<3;phase++){
        const VkDeviceSize ranges[][2]={{8,16},{16,8},{16,16}};
        infos[0].range=ranges[phase][0];infos[1].range=ranges[phase][1];vkUpdateDescriptorSets(root.device,2,writes,0,nullptr);
        std::fill(result,result+32,0xCCCCCCCCu);output.flush();
        mavk::CommandBatch cmd(context);mavk::Fence fence(context);cmd.begin();vkCmdBindPipeline(cmd.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,pipeline.handle());vkCmdBindDescriptorSets(cmd.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,handles.layout,0,1,&set,0,nullptr);vkCmdDispatch(cmd.handle(),1,1,1);cmd.hostReadBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_WRITE_BIT);cmd.end();auto command=cmd.handle();VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;ok(context.submit(root.queue,submit,fence.handle()));ok(fence.wait(5000000000ull));output.invalidate();input.invalidate();
        for(unsigned i=0;i<32;i++){uint32_t expected=i<4?value(i):0xCCCCCCCCu;if(phase==0&&i>=2&&i<4)expected=0;if(phase==1&&i>=2)expected=0xCCCCCCCCu;if(result[i]!=expected)throw std::runtime_error("aggregate bounds mismatch phase="+std::to_string(phase)+" word="+std::to_string(i));if(source[i]!=value(i))throw std::runtime_error("aggregate input modified");}
    }
    std::cout<<"AGGREGATE_BUFFER_GPU_PASS phases=3 partialReadZero=1 partialWriteSuppressed=1 fullReadWrite=1 checkedWords=192\n";
}
