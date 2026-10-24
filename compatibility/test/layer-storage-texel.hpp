void storageTexelBounds(Root& root,mavk::DeviceContext& context,const char* shaderPath,bool atomic=false,bool guarded=false,bool components=false){
    if(guarded){const auto flag=std::getenv("MAVK_LAYER_GUARDS");if(!std::getenv("MAVK_EXPECT_LAYER_PATH")||!flag||std::strcmp(flag,"1"))throw std::runtime_error("guarded atomic gate requires mapped candidate/guards");}
    struct Handles {
        VkDevice device;VkBufferView view{};VkDescriptorSetLayout schema{};VkDescriptorPool pool{};VkPipelineLayout layout{};
        ~Handles(){if(layout)vkDestroyPipelineLayout(device,layout,nullptr);if(pool)vkDestroyDescriptorPool(device,pool,nullptr);if(schema)vkDestroyDescriptorSetLayout(device,schema,nullptr);if(view)vkDestroyBufferView(device,view,nullptr);}
    } handles{root.device};
    VkFormatProperties formats{};vkGetPhysicalDeviceFormatProperties(root.gpu,VK_FORMAT_R32_UINT,&formats);
    if(!(formats.bufferFeatures&VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT))throw std::runtime_error("NO-GO: native R32_UINT storage texel unavailable");
    if(atomic&&!(formats.bufferFeatures&VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT))throw std::runtime_error("NO-GO: native R32_UINT storage texel atomic unavailable");
    const auto host=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,coherent=VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    mavk::Buffer target(context,128,VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,host,coherent),output(context,128,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,host,coherent);
    auto source=static_cast<uint32_t*>(target.map()),values=static_cast<uint32_t*>(output.map());
    for(unsigned i=0;i<32;i++)source[i]=0xA5000000u+i;std::fill(values,values+32,0xCCCCCCCCu);target.flush();output.flush();
    VkBufferViewCreateInfo view{VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};view.buffer=target.handle();view.format=VK_FORMAT_R32_UINT;view.range=16;ok(vkCreateBufferView(root.device,&view,nullptr,&handles.view));
    VkDescriptorSetLayoutBinding bindings[]={{0,VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}};
    VkDescriptorSetLayoutCreateInfo schema{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};schema.bindingCount=2;schema.pBindings=bindings;ok(vkCreateDescriptorSetLayout(root.device,&schema,nullptr,&handles.schema));
    VkDescriptorPoolSize sizes[]={{VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,1},{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1}};
    VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=1;pool.poolSizeCount=2;pool.pPoolSizes=sizes;ok(vkCreateDescriptorPool(root.device,&pool,nullptr,&handles.pool));
    VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};allocate.descriptorPool=handles.pool;allocate.descriptorSetCount=1;allocate.pSetLayouts=&handles.schema;VkDescriptorSet set{};ok(vkAllocateDescriptorSets(root.device,&allocate,&set));
    VkDescriptorBufferInfo result{output.handle(),0,128};VkWriteDescriptorSet writes[2]{};
    for(auto& w:writes){w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w.dstSet=set;w.descriptorCount=1;}
    writes[0].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;writes[0].pTexelBufferView=&handles.view;
    writes[1].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;writes[1].dstBinding=1;writes[1].pBufferInfo=&result;vkUpdateDescriptorSets(root.device,2,writes,0,nullptr);
    VkPushConstantRange range{VK_SHADER_STAGE_COMPUTE_BIT,0,4};
    VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};layout.setLayoutCount=1;layout.pSetLayouts=&handles.schema;if(guarded){layout.pushConstantRangeCount=1;layout.pPushConstantRanges=&range;}ok(vkCreatePipelineLayout(root.device,&layout,nullptr,&handles.layout));
    const auto code=words(shaderPath);mavk::Shader shader(context,code.data(),code.size()*4);VkComputePipelineCreateInfo info{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};info.layout=handles.layout;info.stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;info.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;info.stage.module=shader.handle();info.stage.pName="main";
    mavk::Pipeline pipeline(context,info);
    const uint32_t expected[]={0,0x5A17C0DEu,0xA5000001u,0};
    const uint32_t atomicExpected[]={0xA5000000u,0xA5000001u,0xA5000001u,0xA5000002u};
    for(unsigned phase=0;phase<(components?3u:guarded?5u:1u);phase++){
    for(unsigned i=0;i<32;i++)source[i]=0xA5000000u+i;std::fill(values,values+32,0xCCCCCCCCu);target.flush();output.flush();
    VkBufferView selected=(components?phase==1:phase==3)?VK_NULL_HANDLE:handles.view;writes[0].pTexelBufferView=&selected;vkUpdateDescriptorSets(root.device,2,writes,0,nullptr);
    mavk::CommandBatch cmd(context);mavk::Fence fence(context);cmd.begin();vkCmdBindPipeline(cmd.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,pipeline.handle());vkCmdBindDescriptorSets(cmd.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,handles.layout,0,1,&set,0,nullptr);if(guarded){uint32_t mode=phase<3?phase:0;vkCmdPushConstants(cmd.handle(),handles.layout,VK_SHADER_STAGE_COMPUTE_BIT,0,4,&mode);}vkCmdDispatch(cmd.handle(),1,1,1);cmd.hostReadBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_WRITE_BIT);cmd.end();auto command=cmd.handle();VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;ok(context.submit(root.queue,submit,fence.handle()));ok(fence.wait(5000000000ull));output.invalidate();target.invalidate();
    const bool inactive=components?phase==1:guarded&&phase>0&&phase<4;
    for(unsigned i=0;i<32;i++){
        const uint32_t targetExpected=inactive?0xA5000000u+i:atomic?(i==0?0xBEEFu:i==1?0x5A17C0DEu:0xA5000000u+i):(i?0xA5000000u+i:0x5A17C0DEu);
        if(source[i]!=targetExpected)throw std::runtime_error("storage texel target mismatch word="+std::to_string(i));
        const uint32_t expectedValue=components?(i<16?(inactive?0u:i%4==3?1u:i%4==0?expected[i/4]:0u):0xCCCCCCCCu):(i<4?(inactive?0u:atomic?atomicExpected[i]:expected[i]):0xCCCCCCCCu);
        if(values[i]!=expectedValue)throw std::runtime_error("storage texel result mismatch phase="+std::to_string(phase)+" word="+std::to_string(i)+" got="+std::to_string(values[i])+" expected="+std::to_string(expectedValue));
    }
    }
    if(components)std::cout<<"STORAGE_TEXEL_COMPONENTS_PASS phases=3 checkedWords=192 oobAlphaOne=1 nullAllZero=1 validAlphaOne=1 sentinelPreserved=1 restoredValid=1\n";
    else if(guarded)std::cout<<"GUARDED_STORAGE_TEXEL_ATOMIC_PASS phases=5 checkedWords=320 validReadModifyWrite=1 oobZeroNoEffects=1 negativeZero=1 diagnosticNull=1 restoredValid=1\n";
    else if(atomic)std::cout<<"NATIVE_STORAGE_TEXEL_ATOMIC_PASS checkedWords=64 viewElements=4 operations=4 addExchangeCompareSwap=1 allCoordinatesValid=1 noGuardClaim=1\n";
    else std::cout<<"STORAGE_TEXEL_BOUNDS_GPU_PASS checkedWords=64 viewElements=4 allocationElements=32 readZero=1 writeSuppressed=1 validReadWrite=1 negativeReadZero=1\n";
}
