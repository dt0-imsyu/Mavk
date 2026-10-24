void nativeTexelFormats(Root& root,mavk::DeviceContext& context,const char* unormShader,const char* uintShader,bool bounds=false){
    struct Handles {
        VkDevice device;VkBufferView view{};VkDescriptorSetLayout setLayout{};VkDescriptorPool pool{};VkPipelineLayout layout{};
        ~Handles(){if(layout)vkDestroyPipelineLayout(device,layout,nullptr);if(pool)vkDestroyDescriptorPool(device,pool,nullptr);if(setLayout)vkDestroyDescriptorSetLayout(device,setLayout,nullptr);if(view)vkDestroyBufferView(device,view,nullptr);}
    };
    VkPhysicalDeviceProperties props{};vkGetPhysicalDeviceProperties(root.gpu,&props);
    const VkDeviceSize offset=std::max<VkDeviceSize>(4,props.limits.minTexelBufferOffsetAlignment);
    if(offset>1048576||offset%4)throw std::runtime_error("texel fixture alignment budget");
    uint32_t verified=0;
    for(unsigned variant=0;variant<2;variant++){
        const auto format=variant?VK_FORMAT_R8G8B8A8_UINT:VK_FORMAT_R8G8B8A8_UNORM;
        VkFormatProperties features{};vkGetPhysicalDeviceFormatProperties(root.gpu,format,&features);
        if(!(features.bufferFeatures&VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT))throw std::runtime_error("NO-GO: native RGBA8 uniform texel feature missing");
        mavk::Buffer input(context,offset+258,VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        mavk::Buffer output(context,1024,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        auto bytes=static_cast<uint8_t*>(input.map());auto values=static_cast<uint32_t*>(output.map());
        std::fill(bytes,bytes+input.size(),0xA5);std::fill(values,values+256,0xCCCCCCCCu);
        for(unsigned i=0;i<256;i++)bytes[offset+i]=uint8_t((i*37+11)%256);
        input.flush();output.flush();Handles handles{root.device};
        VkBufferViewCreateInfo view{VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};view.buffer=input.handle();view.format=format;view.offset=offset;view.range=VK_WHOLE_SIZE;ok(vkCreateBufferView(root.device,&view,nullptr,&handles.view));
        VkDescriptorSetLayoutBinding bindings[]={{0,VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}};
        VkDescriptorSetLayoutCreateInfo schema{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};schema.bindingCount=2;schema.pBindings=bindings;ok(vkCreateDescriptorSetLayout(root.device,&schema,nullptr,&handles.setLayout));
        VkDescriptorPoolSize sizes[]={{VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,1},{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1}};
        VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=1;pool.poolSizeCount=2;pool.pPoolSizes=sizes;ok(vkCreateDescriptorPool(root.device,&pool,nullptr,&handles.pool));
        VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};allocate.descriptorPool=handles.pool;allocate.descriptorSetCount=1;allocate.pSetLayouts=&handles.setLayout;VkDescriptorSet set{};ok(vkAllocateDescriptorSets(root.device,&allocate,&set));
        VkDescriptorBufferInfo destination{output.handle(),0,1024};VkWriteDescriptorSet writes[2]{};
        for(auto& w:writes){w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w.dstSet=set;w.descriptorCount=1;}
        writes[0].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;writes[0].pTexelBufferView=&handles.view;
        writes[1].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;writes[1].dstBinding=1;writes[1].pBufferInfo=&destination;vkUpdateDescriptorSets(root.device,2,writes,0,nullptr);
        VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};layout.setLayoutCount=1;layout.pSetLayouts=&handles.setLayout;ok(vkCreatePipelineLayout(root.device,&layout,nullptr,&handles.layout));
        auto code=words(variant?uintShader:unormShader);mavk::Shader shader(context,code.data(),code.size()*4);
        VkComputePipelineCreateInfo info{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};info.layout=handles.layout;info.stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;info.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;info.stage.module=shader.handle();info.stage.pName="main";
        mavk::Pipeline pipeline(context,info);mavk::CommandBatch cmd(context);mavk::Fence fence(context);cmd.begin();
        vkCmdBindPipeline(cmd.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,pipeline.handle());vkCmdBindDescriptorSets(cmd.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,handles.layout,0,1,&set,0,nullptr);vkCmdDispatch(cmd.handle(),1,1,1);cmd.hostReadBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_WRITE_BIT);cmd.end();
        auto command=cmd.handle();VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;ok(context.submit(root.queue,submit,fence.handle()));ok(fence.wait(5000000000ull));output.invalidate();input.invalidate();
        for(unsigned i=0;i<256;i++){
            const unsigned access=(i/4)%5;
            const auto expected=bounds?(access<2?uint32_t((((access==0?0u:63u)*4+i%4)*37+11)%256):0u):uint32_t((i*37+11)%256);
            if(values[i]!=expected)throw std::runtime_error("RGBA8 native texel mismatch component="+std::to_string(i)+" got="+std::to_string(values[i])+" expected="+std::to_string(expected));verified++;
        }
        for(VkDeviceSize i=0;i<input.size();i++){const auto expected=i>=offset&&i<offset+256?uint8_t(((i-offset)*37+11)%256):uint8_t(0xA5);if(bytes[i]!=expected)throw std::runtime_error("RGBA8 input/padding modified");}
    }
    if(bounds)std::cout<<"TEXEL_BOUNDS_GPU_PASS formats=2 components="<<verified<<" coordinates=0,63,-1,64,1073741824 imageOperands=None,ZeroExtend\n";
    else std::cout<<"TEXEL_FORMAT_GPU_PASS formats=2 components="<<verified<<" elements=64 wholeSize=1 trailingBytes=2 offsetNonzero=1\n";
}
