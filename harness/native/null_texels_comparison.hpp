// R32_UINT texel buffer feasibility,not full robustness2/format support.
std::string nullTexelsGate(const std::vector<uint32_t>& original,bool validated,ValidationCapture& validation){
#ifdef MAVK_RUNTIME_GUARDS
    mavk::compat::GuardCompiler compiler;auto code=compiler.compile(original);
    Root root(validation);root.init(validated);mavk::DeviceContext context(root.physical,root.device,root.family);
    VkFormatProperties properties;vkGetPhysicalDeviceFormatProperties(root.physical,VK_FORMAT_R32_UINT,&properties);
    if((properties.bufferFeatures&(VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT|VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT))!=(VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT|VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT))throw std::runtime_error("R32_UINT texel features unavailable");
    uint64_t verified=0,guards=0,uninitialized=0,uniformOob=0,storageOob=0;
    {
        struct ViewLease {
            VkDevice device;std::shared_ptr<mavk::Buffer> buffer;VkBufferView view{};
            ViewLease(mavk::DeviceContext& context,VkBufferUsageFlags usage):device(context.vkDevice()),buffer(std::make_shared<mavk::Buffer>(context,512,usage,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){
                VkBufferViewCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};info.buffer=buffer->handle();info.format=VK_FORMAT_R32_UINT;info.range=256;ok(vkCreateBufferView(device,&info,nullptr,&view),"texel view");buffer->map();
            }
            ~ViewLease(){if(view)vkDestroyBufferView(device,view,nullptr);}
        };
        auto sampled0=std::make_shared<ViewLease>(context,VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT),sampled1=std::make_shared<ViewLease>(context,VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT);
        auto storage0=std::make_shared<ViewLease>(context,VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT),storage1=std::make_shared<ViewLease>(context,VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT);
        auto output=std::make_shared<mavk::Buffer>(context,3072,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);auto o=static_cast<uint32_t*>(output->map());
        using K=mavk::compat::DescriptorKind;using A=mavk::compat::GuardMetadataAbi;
        mavk::compat::VulkanDescriptorResources resources;mavk::compat::VulkanDescriptorUpdates updates(resources,{},true);
        for(auto pair:{std::make_pair(sampled0,VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT),std::make_pair(sampled1,VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT),std::make_pair(storage0,VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT),std::make_pair(storage1,VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)}){
            resources.registerBuffer(pair.first->buffer->handle(),512,pair.second,pair.first->buffer);resources.registerBufferView(pair.first->view,pair.first->buffer->handle(),VK_FORMAT_R32_UINT,0,256,pair.first);
        }
        resources.registerBuffer(output->handle(),3072,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,output);
        auto logical=reinterpret_cast<VkDescriptorSet>(uintptr_t(0x7201));
        VkDescriptorSetLayoutBinding bindings[]={{0,VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,2,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{1,VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,2,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{2,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}};
        VkDescriptorSetLayoutCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};info.bindingCount=3;info.pBindings=bindings;updates.registerSet(logical,info);
        mavk::compat::NativeDescriptorAdapter adapter(context,{{0,0,2,K::UniformTexelBuffer},{0,1,2,K::StorageTexelBuffer},{0,2,1,K::StorageBuffer}},A::IndexAndInit);
        mavk::Shader shader(context,code->data(),code->size()*4);VkComputePipelineCreateInfo pc{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};pc.layout=adapter.pipelineLayout();pc.stage={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};pc.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;pc.stage.module=shader.handle();pc.stage.pName="main";mavk::Pipeline pipeline(context,pc);mavk::CommandBatch commands(context);mavk::Fence fence(context);
        auto formats=mavk::compat::reflectTexelFormats(original);
        if(formats.size()!=2||formats.at({0,0})!=VK_FORMAT_R32_UINT||formats.at({0,1})!=VK_FORMAT_R32_UINT)throw std::runtime_error("actual texel shader reflection mismatch");
        mavk::compat::NullTexelResources nullTexels(context);std::shared_ptr<const void> nullLease;VkBufferView nullView{};
        auto resolve=[&](const auto& binding,uint32_t index,const auto& value){
            if(value.status==mavk::compat::DescriptorStatus::Bound)return mavk::compat::VulkanDescriptorResources::resolve(binding,index,value);
            auto r=nullTexels.resolve(binding,value,formats.at({binding.set,binding.binding}));
            if(nullLease&&(r.owner!=nullLease||r.texelView!=nullView))throw std::runtime_error("null texel typed cache/lifetime changed");
            nullLease=r.owner;nullView=r.texelView;return r;
        };
        for(unsigned phase=0;phase<2;phase++){
            for(auto owner:{sampled0,sampled1,storage0,storage1}){auto data=static_cast<uint32_t*>(owner->buffer->map());for(unsigned i=0;i<128;i++)data[i]=(owner==sampled0?0x1000:owner==sampled1?0x2000:owner==storage0?0x3000:0x4000)+i;owner->buffer->flush();}std::memset(o,0xCC,3072);output->flush();
            VkBufferView sampled[]={phase?sampled0->view:VK_NULL_HANDLE,phase?VK_NULL_HANDLE:sampled1->view},storage[]={phase?storage0->view:VK_NULL_HANDLE,phase?VK_NULL_HANDLE:storage1->view};VkDescriptorBufferInfo outputInfo{output->handle(),0,3072};VkWriteDescriptorSet writes[3]{};for(auto& w:writes){w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w.dstSet=logical;w.descriptorCount=2;}
            writes[0].dstBinding=0;writes[0].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;writes[0].pTexelBufferView=sampled;writes[1].dstBinding=1;writes[1].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;writes[1].pTexelBufferView=storage;writes[2].dstBinding=2;writes[2].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;writes[2].descriptorCount=1;writes[2].pBufferInfo=&outputInfo;updates.update(3,writes,0,nullptr);
            auto cpu=updates.snapshotBindings({logical},A::IndexAndInit);if(cpu!=updates.snapshotBindings({logical},A::IndexAndInit))throw std::runtime_error("texel bound cache");auto native=adapter.materialize(cpu,resolve);if(native!=adapter.materialize(cpu,resolve))throw std::runtime_error("texel native cache");
            fence.reset();commands.begin();vkCmdBindPipeline(commands.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,pipeline.handle());vkCmdBindDescriptorSets(commands.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,native->pipelineLayout(),0,uint32_t(native->sets().size()),native->sets().data(),0,nullptr);vkCmdDispatch(commands.handle(),1,1,1);commands.hostReadBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_WRITE_BIT);commands.end();VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};auto command=commands.handle();submit.commandBufferCount=1;submit.pCommandBuffers=&command;ok(context.submit(root.queue,submit,fence.handle()),"texel submit");auto waited=fence.wait(5'000'000'000ull);if(waited!=VK_SUCCESS)vkDeviceWaitIdle(root.device);ok(waited,"texel fence");output->invalidate();
            for(unsigned i=0;i<64;i++){uint32_t expected[]={phase?0x1000+i:0,phase?0:0x2000+i,phase?0x3000+i:0,phase?0:0x4000+i,0,0,0,0,i,0xFACE};for(unsigned c=0;c<10;c++){if(o[10*i+c]!=expected[c])throw std::runtime_error("texel result phase="+std::to_string(phase)+" i="+std::to_string(i)+" channel="+std::to_string(c)+" actual="+std::to_string(o[10*i+c]));verified++;}}
            for(auto owner:{sampled0,sampled1,storage0,storage1}){owner->buffer->invalidate();auto data=static_cast<uint32_t*>(owner->buffer->map());for(unsigned i=0;i<128;i++){uint32_t expected=(owner==sampled0?0x1000:owner==sampled1?0x2000:owner==storage0?0x3000:0x4000)+i;if(i<64&&owner==storage0&&phase)expected=1000+i;if(i<64&&owner==storage1&&!phase)expected=2000+i;if(data[i]!=expected)throw std::runtime_error("texel store/guard sentinel mismatch");verified++;}}
            auto events=native->readDiagnostics();if(events[0]>8190)throw std::runtime_error("texel diagnostic log overflow");uint64_t init=0,u=0,s=0;
            for(uint32_t p=1;p<=events[0];){auto length=events[p];if(length<10||length>11||p+length>events[0]+1||events[p+1]!=23||events[p+3]!=5)throw std::runtime_error("texel diagnostic record");auto error=events[p+7];if(error==1)init++;else if(error==6)u++;else if(error==7)s++;else throw std::runtime_error("unexpected texel diagnostic code");p+=length;guards++;}
            if(init!=384||u!=64||s!=128)throw std::runtime_error("texel guard coverage init="+std::to_string(init)+" uniform="+std::to_string(u)+" storage="+std::to_string(s));uninitialized+=init;uniformOob+=u;storageOob+=s;
        }
    }
    if(!context.counters().empty()||verified!=2304||guards!=1152)throw std::runtime_error("texel resources/coverage");
    return "{\"result\":\"EMULATED_NULL_TEXEL_FEASIBILITY_PASS\",\"device\":"+quote(root.properties.deviceName)+",\"verified_words\":2304,\"guarded_accesses\":1152,\"null_guards\":"+std::to_string(uninitialized)+",\"uniform_oob_guards\":"+std::to_string(uniformOob)+",\"storage_oob_guards\":"+std::to_string(storageOob)+",\"gpu_submissions\":2,\"runtime_guard_compiler\":true,\"vulkan_descriptor_updates\":true,\"bound_descriptor_snapshots\":true,\"snapshot_cache_reused\":true,\"core_live_zero\":true,\"nullDescriptor_advertised\":false,\"robustBufferAccess2_advertised\":false,\"scope\":\"R32_UINT uniform/storage texel fetch/load/store and positive out-of-range guards;static slots,NOT all formats/operations,full robustness2 or DXVK PASS.\"}";
#else
    (void)original;(void)validated;(void)validation;throw std::runtime_error("texel gate requires runtime compiler");
#endif
}
