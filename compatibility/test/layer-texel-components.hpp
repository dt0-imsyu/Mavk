void texelViewComponents(Root& root,mavk::DeviceContext& context,const char* integerShader,const char* floatShader){
    const auto flag=std::getenv("MAVK_LAYER_GUARDS");if(!std::getenv("MAVK_EXPECT_LAYER_PATH")||!flag||std::strcmp(flag,"1"))throw std::runtime_error("texel component gate requires mapped guarded layer");
    struct Handles {
        VkDevice device;VkBufferView view{};VkDescriptorSetLayout schema{};VkDescriptorPool pool{};VkPipelineLayout layout{};
        ~Handles(){if(view)vkDestroyBufferView(device,view,nullptr);if(layout)vkDestroyPipelineLayout(device,layout,nullptr);if(pool)vkDestroyDescriptorPool(device,pool,nullptr);if(schema)vkDestroyDescriptorSetLayout(device,schema,nullptr);}
    } handles{root.device};
    VkDescriptorSetLayoutBinding bindings[]={{0,VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}};
    VkDescriptorSetLayoutCreateInfo schema{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};schema.bindingCount=2;schema.pBindings=bindings;ok(vkCreateDescriptorSetLayout(root.device,&schema,nullptr,&handles.schema));
    VkDescriptorPoolSize sizes[]={{VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,1},{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1}};
    VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=1;pool.poolSizeCount=2;pool.pPoolSizes=sizes;ok(vkCreateDescriptorPool(root.device,&pool,nullptr,&handles.pool));
    VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};allocate.descriptorPool=handles.pool;allocate.descriptorSetCount=1;allocate.pSetLayouts=&handles.schema;VkDescriptorSet set{};ok(vkAllocateDescriptorSets(root.device,&allocate,&set));
    VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};layout.setLayoutCount=1;layout.pSetLayouts=&handles.schema;ok(vkCreatePipelineLayout(root.device,&layout,nullptr,&handles.layout));
    const VkFormat formats[2][3]={{VK_FORMAT_R32_UINT,VK_FORMAT_R32G32_UINT,VK_FORMAT_R32G32B32A32_UINT},{VK_FORMAT_R32_SFLOAT,VK_FORMAT_R32G32_SFLOAT,VK_FORMAT_R32G32B32A32_SFLOAT}};
    const unsigned channels[]={1,2,4};const int indices[]={0,1,3,-1,4,0,1073741824,2};unsigned checked=0;
    mavk::Buffer input(context,256,VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),output(context,160,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    auto source=static_cast<uint32_t*>(input.map()),result=static_cast<uint32_t*>(output.map());
    for(unsigned floating=0;floating<2;floating++){
        for(unsigned i=0;i<64;i++){if(floating){float value=float(i+1)*0.125f;std::memcpy(source+i,&value,4);}else source[i]=101+i;}input.flush();
        const auto code=words(floating?floatShader:integerShader);mavk::Shader shader(context,code.data(),code.size()*4);VkComputePipelineCreateInfo info{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};info.layout=handles.layout;info.stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;info.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;info.stage.module=shader.handle();info.stage.pName="main";mavk::Pipeline pipeline(context,info);
        for(unsigned step=0;step<3;step++){
            const unsigned variant=std::getenv("MAVK_TEST_REVERSE_FORMATS")?2-step:step;
            auto format=formats[floating][variant];auto components=channels[variant];VkFormatProperties properties{};vkGetPhysicalDeviceFormatProperties(root.gpu,format,&properties);if(!(properties.bufferFeatures&VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT))throw std::runtime_error("NO-GO native texel view format="+std::to_string(format));
            VkBufferViewCreateInfo view{VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};view.buffer=input.handle();view.format=format;view.range=4*components*4;ok(vkCreateBufferView(root.device,&view,nullptr,&handles.view));
            for(unsigned phase=0;phase<3;phase++){
                VkBufferView selected=phase==1?VK_NULL_HANDLE:handles.view;VkDescriptorBufferInfo destination{output.handle(),0,160};VkWriteDescriptorSet writes[2]{};
                for(auto& w:writes){w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w.dstSet=set;w.descriptorCount=1;}
                writes[0].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;writes[0].pTexelBufferView=&selected;writes[1].dstBinding=1;writes[1].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;writes[1].pBufferInfo=&destination;vkUpdateDescriptorSets(root.device,2,writes,0,nullptr);
                std::fill(result,result+40,0xCCCCCCCCu);output.flush();mavk::CommandBatch cmd(context);mavk::Fence fence(context);cmd.begin();vkCmdBindPipeline(cmd.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,pipeline.handle());vkCmdBindDescriptorSets(cmd.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,handles.layout,0,1,&set,0,nullptr);vkCmdDispatch(cmd.handle(),1,1,1);cmd.hostReadBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_WRITE_BIT);cmd.end();auto command=cmd.handle();VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;ok(context.submit(root.queue,submit,fence.handle()));ok(fence.wait(5000000000ull));output.invalidate();input.invalidate();
                for(unsigned i=0;i<40;i++){
                    uint32_t expected=0xCCCCCCCCu;
                    if(i<32){const auto component=i%4;const auto index=indices[i/4];const bool valid=index>=0&&index<4;expected=phase==1?0u:component==3&&components<4?(floating?0x3f800000u:1u):valid&&component<components?source[index*components+component]:0u;}
                    if(result[i]!=expected)throw std::runtime_error("texel view components mismatch format="+std::to_string(format)+" phase="+std::to_string(phase)+" word="+std::to_string(i)+" got="+std::to_string(result[i])+" expected="+std::to_string(expected));checked++;
                }
                for(unsigned i=0;i<64;i++){uint32_t expected=101+i;if(floating){float value=float(i+1)*0.125f;std::memcpy(&expected,&value,4);}if(source[i]!=expected)throw std::runtime_error("texel view source modified");checked++;}
            }
            vkDestroyBufferView(root.device,handles.view,nullptr);handles.view=VK_NULL_HANDLE;
        }
    }
    std::cout<<"TEXEL_VIEW_COMPONENTS_GPU_PASS formats=6 phases=18 checkedWords="<<checked<<" shaderPipelines=2 perViewMetadata=1 nullAllZero=1 restoredValid=1\n";
}
