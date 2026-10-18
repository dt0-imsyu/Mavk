std::string descriptorGenerationGate(const std::vector<uint32_t>& code,bool validated,ValidationCapture& validation,bool templates=false){
    Root root(validation);root.init(validated);mavk::DeviceContext context(root.physical,root.device,root.family);
    {
        using K=mavk::compat::DescriptorKind;using A=mavk::compat::GuardMetadataAbi;
        std::vector<mavk::compat::DescriptorBinding> layout{{0,0,1,K::StorageBuffer},{0,1,1,K::StorageBuffer}};
        mavk::compat::NativeDescriptorAdapter adapter(context,layout,A::IndexAndInit);mavk::compat::VulkanDescriptorResources resources;mavk::compat::VulkanDescriptorUpdates updates(resources);
        // Logical keys are consumed only by our frontend,never by the driver.
        auto logical0=reinterpret_cast<VkDescriptorSet>(uintptr_t(0x7001)),logical1=reinterpret_cast<VkDescriptorSet>(uintptr_t(0x7002));
        VkDescriptorSetLayoutBinding bindings[]={{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}};
        VkDescriptorSetLayoutCreateInfo create{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};create.bindingCount=2;create.pBindings=bindings;updates.registerSet(logical0,create);updates.registerSet(logical1,create);
        auto host=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,coherent=VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        auto input0=std::make_shared<mavk::Buffer>(context,4,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,host,coherent),input1=std::make_shared<mavk::Buffer>(context,4,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,host,coherent);
        auto output0=std::make_shared<mavk::Buffer>(context,256,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,host,coherent),output1=std::make_shared<mavk::Buffer>(context,256,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,host,coherent);
        output0->map();output1->map(); // Core invalidate requires a mapped range.
        resources.registerBuffer(input0->handle(),4,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,input0);resources.registerBuffer(input1->handle(),4,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,input1);
        resources.registerBuffer(output0->handle(),256,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,output0);resources.registerBuffer(output1->handle(),256,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,output1);
        *static_cast<uint32_t*>(input0->map())=101;*static_cast<uint32_t*>(input1->map())=202;input0->flush();input1->flush();
        mavk::Shader shader(context,code.data(),code.size()*4);VkComputePipelineCreateInfo pc{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};pc.layout=adapter.pipelineLayout();pc.stage={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};pc.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;pc.stage.module=shader.handle();pc.stage.pName="main";mavk::Pipeline pipeline(context,pc);mavk::CommandBatch first(context),second(context);mavk::Fence fence(context);
        auto resolve=mavk::compat::VulkanDescriptorResources::resolve;
        VkDescriptorBufferInfo buffer0[]={{input0->handle(),0,VK_WHOLE_SIZE},{output0->handle(),0,256}};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=logical0;write.dstBinding=0;write.descriptorCount=2;write.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;write.pBufferInfo=buffer0;
        VkDescriptorUpdateTemplateEntry entry{0,0,2,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,32};
        VkDescriptorUpdateTemplateCreateInfo templateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO};templateInfo.templateType=VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET;templateInfo.descriptorSetLayout=reinterpret_cast<VkDescriptorSetLayout>(uintptr_t(0x7003));templateInfo.descriptorUpdateEntryCount=1;templateInfo.pDescriptorUpdateEntries=&entry;
        auto descriptorTemplate=updates.compileTemplate(logical0,templateInfo);
        auto templateUpdate=[&](VkDescriptorSet set,const VkDescriptorBufferInfo* info){std::vector<unsigned char> data(descriptorTemplate.requiredBytes());for(size_t i=0;i<2;i++)std::memcpy(data.data()+1+32*i,&info[i],sizeof(info[i]));updates.updateWithTemplate(set,descriptorTemplate,data.data(),data.size());};
        if(templates)templateUpdate(logical0,buffer0);else updates.update(1,&write,0,nullptr);
        auto cpu0=updates.snapshot(logical0,A::IndexAndInit);auto native0=adapter.materialize(cpu0,resolve);
        auto allocated=context.counters().totalAllocations;if(adapter.materialize(cpu0,resolve)!=native0||context.counters().totalAllocations!=allocated)throw std::runtime_error("native generation cache allocated");
        auto record=[&](mavk::CommandBatch& commands,const std::shared_ptr<mavk::compat::NativeDescriptorSnapshot>& descriptors){commands.begin();vkCmdBindPipeline(commands.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,pipeline.handle());vkCmdBindDescriptorSets(commands.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,descriptors->pipelineLayout(),0,2,descriptors->sets().data(),0,nullptr);vkCmdDispatch(commands.handle(),1,1,1);commands.hostReadBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_WRITE_BIT);commands.end();};
        record(first,native0);
        // Mutate state and create another native generation BEFORE either
        // recorded dispatch executes. No set referenced by first is updated.
        VkDescriptorBufferInfo buffer1[]={{input1->handle(),0,VK_WHOLE_SIZE},{output1->handle(),0,256}};write.dstSet=logical1;write.pBufferInfo=buffer1;
        VkCopyDescriptorSet copy{VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET};copy.srcSet=logical1;copy.srcBinding=0;copy.dstSet=logical0;copy.dstBinding=0;copy.descriptorCount=2;
        if(templates){templateUpdate(logical1,buffer1);updates.update(0,nullptr,1,&copy);}else updates.update(1,&write,1,&copy);
        auto cpu1=updates.snapshot(logical0,A::IndexAndInit);auto native1=adapter.materialize(cpu1,resolve);record(second,native1);
        if(native0->sets()[0]==native1->sets()[0]||native0->sets()[1]==native1->sets()[1])throw std::runtime_error("native generations alias descriptor sets");
        unsigned negatives=0;auto reject=[&](auto f){auto before=context.counters().totalAllocations;try{f();}catch(const std::invalid_argument&){if(context.counters().totalAllocations!=before)throw std::runtime_error("rejected materialization allocated");negatives++;return;}throw std::runtime_error("bad native descriptor accepted");};
        reject([&]{adapter.materialize(updates.snapshot(logical0,A::BufferRanges),resolve);});adapter.invalidateResolverCache();
        reject([&]{adapter.materialize(cpu1,[](const auto&,uint32_t,const auto&){return mavk::compat::NativeDescriptorResource{};});});
        reject([&]{adapter.materialize(cpu1,[&](const auto& b,uint32_t i,const auto& v){auto resource=resolve(b,i,v);resource.buffer.range++;return resource;});});
        std::weak_ptr<mavk::Buffer> weakInput0=input0,weakInput1=input1,weakOutput0=output0,weakOutput1=output1;
        resources.unregisterBuffer(input0->handle());resources.unregisterBuffer(input1->handle());resources.unregisterBuffer(output0->handle());resources.unregisterBuffer(output1->handle());
        updates.unregisterSet(logical0);updates.unregisterSet(logical1);input0.reset();input1.reset();output0.reset();output1.reset();cpu0.reset();cpu1.reset();
        if(weakInput0.expired()||weakInput1.expired()||weakOutput0.expired()||weakOutput1.expired())throw std::runtime_error("native generation lost resource lease before submit");
        VkCommandBuffer commands[]={first.handle(),second.handle()};VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=2;submit.pCommandBuffers=commands;ok(context.submit(root.queue,submit,fence.handle()),"two retained generations submit");auto waited=fence.wait(5'000'000'000ull);if(waited!=VK_SUCCESS)vkDeviceWaitIdle(root.device);ok(waited,"two generations fence");
        {
            auto a=weakOutput0.lock(),b=weakOutput1.lock();a->invalidate();b->invalidate();auto x=static_cast<uint32_t*>(a->map()),y=static_cast<uint32_t*>(b->map());
            for(uint32_t i=0;i<64;i++)if(x[i]!=101+i||y[i]!=202+i)throw std::runtime_error("recorded generation changed under later descriptor update");
        }
        native0.reset();native1.reset();
        if(!weakInput0.expired()||!weakInput1.expired()||!weakOutput0.expired()||!weakOutput1.expired()||adapter.counters().liveSnapshots)throw std::runtime_error("retired generation leaked native resources");
        if(adapter.counters().poolCreates!=2||adapter.counters().metadataUploads!=2||adapter.counters().updates!=2||negatives!=3)throw std::runtime_error("native generation gate coverage");
    }
    if(!context.counters().empty())throw std::runtime_error("native generation Core leaks");
    std::string templateFields=templates?",\"vulkan_descriptor_templates\":true,\"api_template_updates\":2,\"api_template_entries\":1,\"template_required_bytes\":57":"";
    return "{\"result\":\"NATIVE_DESCRIPTOR_GENERATIONS_PASS\",\"device\":"+quote(root.properties.deviceName)+",\"verified_words\":128,\"generations\":2,\"native_pool_creates\":2,\"native_metadata_uploads\":2,\"native_updates\":2,\"negative_tests\":3,\"record_before_submit\":true,\"resource_lease_retirement\":true,\"snapshot_cache_reused\":true,\"native_descriptor_adapter\":true,\"vulkan_descriptor_updates\":true,\"api_write_structures\":"+(templates?"0":"2")+",\"api_copy_structures\":1,\"core_live_zero\":true"+templateFields+",\"scope\":\"Standard Vulkan write/copy or template structs drive two native generations via shared frontend+registry;resource release retained through GPU completion. Private frontend,NOT loader interception,public nullDescriptor/full Vulkan API,DXVK or performance PASS.\"}";
}
