std::string descriptorGenerationGate(const std::vector<uint32_t>& code,bool validated,ValidationCapture& validation){
    Root root(validation);root.init(validated);mavk::DeviceContext context(root.physical,root.device,root.family);
    {
        using K=mavk::compat::DescriptorKind;using A=mavk::compat::GuardMetadataAbi;
        std::vector<mavk::compat::DescriptorBinding> layout{{0,0,1,K::StorageBuffer},{0,1,1,K::StorageBuffer}};
        mavk::compat::DescriptorState state(layout);mavk::compat::NativeDescriptorAdapter adapter(context,layout,A::IndexAndInit);
        auto host=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,coherent=VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        auto input0=std::make_shared<mavk::Buffer>(context,4,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,host,coherent),input1=std::make_shared<mavk::Buffer>(context,4,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,host,coherent);
        auto output0=std::make_shared<mavk::Buffer>(context,256,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,host,coherent),output1=std::make_shared<mavk::Buffer>(context,256,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,host,coherent);
        output0->map();output1->map(); // Core invalidate requires a mapped range.
        *static_cast<uint32_t*>(input0->map())=101;*static_cast<uint32_t*>(input1->map())=202;input0->flush();input1->flush();
        mavk::Shader shader(context,code.data(),code.size()*4);VkComputePipelineCreateInfo pc{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};pc.layout=adapter.pipelineLayout();pc.stage={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};pc.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;pc.stage.module=shader.handle();pc.stage.pName="main";mavk::Pipeline pipeline(context,pc);mavk::CommandBatch first(context),second(context);mavk::Fence fence(context);
        auto resolve=[](const mavk::compat::DescriptorBinding&,uint32_t,const mavk::compat::DescriptorValue& v){auto owner=std::static_pointer_cast<const mavk::Buffer>(v.owner);mavk::compat::NativeDescriptorResource resource;resource.owner=owner;resource.bufferBytes=owner->size();resource.buffer={owner->handle(),v.offset,v.range};return resource;};
        state.bindBuffer(0,0,0,input0,4,0,4);state.bindBuffer(0,1,0,output0,256,0,256);auto cpu0=state.snapshot(A::IndexAndInit);auto native0=adapter.materialize(cpu0,resolve);
        auto allocated=context.counters().totalAllocations;if(adapter.materialize(cpu0,resolve)!=native0||context.counters().totalAllocations!=allocated)throw std::runtime_error("native generation cache allocated");
        auto record=[&](mavk::CommandBatch& commands,const std::shared_ptr<mavk::compat::NativeDescriptorSnapshot>& descriptors){commands.begin();vkCmdBindPipeline(commands.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,pipeline.handle());vkCmdBindDescriptorSets(commands.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,descriptors->pipelineLayout(),0,2,descriptors->sets().data(),0,nullptr);vkCmdDispatch(commands.handle(),1,1,1);commands.hostReadBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_WRITE_BIT);commands.end();};
        record(first,native0);
        // Mutate state and create another native generation BEFORE either
        // recorded dispatch executes. No set referenced by first is updated.
        state.bindBuffer(0,0,0,input1,4,0,4);state.bindBuffer(0,1,0,output1,256,0,256);auto cpu1=state.snapshot(A::IndexAndInit);auto native1=adapter.materialize(cpu1,resolve);record(second,native1);
        if(native0->sets()[0]==native1->sets()[0]||native0->sets()[1]==native1->sets()[1])throw std::runtime_error("native generations alias descriptor sets");
        unsigned negatives=0;auto reject=[&](auto f){auto before=context.counters().totalAllocations;try{f();}catch(const std::invalid_argument&){if(context.counters().totalAllocations!=before)throw std::runtime_error("rejected materialization allocated");negatives++;return;}throw std::runtime_error("bad native descriptor accepted");};
        reject([&]{adapter.materialize(state.snapshot(A::BufferRanges),resolve);});adapter.invalidateResolverCache();
        reject([&]{adapter.materialize(cpu1,[](const auto&,uint32_t,const auto&){return mavk::compat::NativeDescriptorResource{};});});
        reject([&]{adapter.materialize(cpu1,[&](const auto& b,uint32_t i,const auto& v){auto resource=resolve(b,i,v);resource.buffer.range++;return resource;});});
        std::weak_ptr<mavk::Buffer> weakInput0=input0,weakInput1=input1,weakOutput0=output0,weakOutput1=output1;
        input0.reset();input1.reset();output0.reset();output1.reset();cpu0.reset();cpu1.reset();state.setNull(0,0,0);state.setNull(0,1,0);
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
    return "{\"result\":\"NATIVE_DESCRIPTOR_GENERATIONS_PASS\",\"device\":"+quote(root.properties.deviceName)+",\"verified_words\":128,\"generations\":2,\"native_pool_creates\":2,\"native_metadata_uploads\":2,\"native_updates\":2,\"negative_tests\":3,\"record_before_submit\":true,\"resource_lease_retirement\":true,\"snapshot_cache_reused\":true,\"native_descriptor_adapter\":true,\"core_live_zero\":true,\"scope\":\"Two immutable native descriptor generations execute correctly after state mutation and caller resource release. NOT update-after-bind feature,all Vulkan descriptor API,DXVK or performance PASS.\"}";
}
