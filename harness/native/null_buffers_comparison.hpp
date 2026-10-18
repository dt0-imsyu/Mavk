// Feasibility only. All native descriptors are valid resources. SPIRV-Tools
// guards logical-null accesses using metadata; no robustness feature enabled.
std::string nullBuffersGate(const std::vector<uint32_t>& code,bool validated,ValidationCapture& validation,bool atomics=false,bool multiSet=false){
    Root root(validation);root.init(validated);mavk::DeviceContext context(root.physical,root.device,root.family);
    uint64_t verified=0,records=0;double totalGpu=0;
    {
        const auto host=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,coherent=VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        auto leftOwner=std::make_shared<mavk::Buffer>(context,512,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,host,coherent),rightOwner=std::make_shared<mavk::Buffer>(context,512,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,host,coherent);
        auto outputOwner=std::make_shared<mavk::Buffer>(context,2048,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,host,coherent);
        auto uniform0Owner=std::make_shared<mavk::Buffer>(context,16,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,host,coherent),uniform1Owner=std::make_shared<mavk::Buffer>(context,16,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,host,coherent);
        auto& left=*leftOwner;auto& right=*rightOwner;auto& output=*outputOwner;auto& uniform0=*uniform0Owner;auto& uniform1=*uniform1Owner;
        using Kind=mavk::compat::DescriptorKind;using Abi=mavk::compat::GuardMetadataAbi;
        uint32_t uniformSet=multiSet?1:0;
        std::vector<mavk::compat::DescriptorBinding> bindings{{0,0,2,Kind::StorageBuffer},{0,1,1,Kind::StorageBuffer},{uniformSet,2,2,Kind::UniformBuffer}};
        mavk::compat::DescriptorState state(bindings, {root.properties.limits.minUniformBufferOffsetAlignment,root.properties.limits.minStorageBufferOffsetAlignment});
        state.bindBuffer(0,1,0,outputOwner,2048,0,2048);
        mavk::compat::VulkanDescriptorResources resources;
        mavk::compat::VulkanDescriptorUpdates updates(resources,{root.properties.limits.minUniformBufferOffsetAlignment,root.properties.limits.minStorageBufferOffsetAlignment},true);
        auto logical0=reinterpret_cast<VkDescriptorSet>(uintptr_t(0x7101)),logical1=reinterpret_cast<VkDescriptorSet>(uintptr_t(0x7102));
        std::vector<VkDescriptorSet> bound{logical0,logical1,VK_NULL_HANDLE};
        if(multiSet){
            for(auto owner:{leftOwner,rightOwner,outputOwner})resources.registerBuffer(owner->handle(),owner->size(),VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,owner);
            for(auto owner:{uniform0Owner,uniform1Owner})resources.registerBuffer(owner->handle(),owner->size(),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,owner);
            VkDescriptorSetLayoutBinding public0[]={{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,2,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}};
            VkDescriptorSetLayoutBinding public1{2,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,2,VK_SHADER_STAGE_COMPUTE_BIT,nullptr};
            VkDescriptorSetLayoutCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};info.bindingCount=2;info.pBindings=public0;updates.registerSet(logical0,info);info.bindingCount=1;info.pBindings=&public1;updates.registerSet(logical1,info);
        }
        mavk::compat::NativeDescriptorAdapter adapter(context,bindings,atomics?Abi::IndexAndInit:Abi::BufferRanges,VK_SHADER_STAGE_COMPUTE_BIT,multiSet?3:1);
        if(adapter.guardSetIndex()!=(multiSet?3u:1u))throw std::runtime_error("native/compiler guard-set contract");
        auto a=static_cast<uint32_t*>(left.map()),b=static_cast<uint32_t*>(right.map()),o=static_cast<uint32_t*>(output.map());
        auto u0=static_cast<uint32_t*>(uniform0.map()),u1=static_cast<uint32_t*>(uniform1.map());
        mavk::Shader shader(context,code.data(),code.size()*4);
        VkComputePipelineCreateInfo pc{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};pc.layout=adapter.pipelineLayout();pc.stage={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};pc.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;pc.stage.module=shader.handle();pc.stage.pName="main";
        mavk::Pipeline pipeline(context,pc);mavk::CommandBatch command(context);mavk::Fence fence(context);
        auto resolver=[&](const mavk::compat::DescriptorBinding& binding,uint32_t index,const mavk::compat::DescriptorValue& v){
            std::shared_ptr<const mavk::Buffer> owner;
            if(v.status==mavk::compat::DescriptorStatus::Bound){if(multiSet)return mavk::compat::VulkanDescriptorResources::resolve(binding,index,v);owner=std::static_pointer_cast<const mavk::Buffer>(v.owner);}
            else owner=binding.binding==0?(index?rightOwner:leftOwner):binding.binding==1?outputOwner:(index?uniform1Owner:uniform0Owner);
            mavk::compat::NativeDescriptorResource r;r.owner=owner;r.bufferBytes=owner->size();r.buffer={owner->handle(),v.status==mavk::compat::DescriptorStatus::Bound?v.offset:0,v.status==mavk::compat::DescriptorStatus::Bound?v.range:binding.binding==0?256u:binding.binding==1?2048u:16u};return r;
        };
        // Same shader, same descriptors. Only metadata switches logical nulls.
        for(unsigned phase=0;phase<2;phase++){
            for(unsigned i=0;i<128;i++){a[i]=0xA000+i;b[i]=0xB000+i;}
            for(unsigned i=0;i<4;i++){u0[i]=0x1234;u1[i]=0x5678;}
            std::memset(o,0xCC,2048);
            if(multiSet){
                VkDescriptorBufferInfo data[]={{phase?left.handle():VK_NULL_HANDLE,0,phase?256u:VK_WHOLE_SIZE},{phase?VK_NULL_HANDLE:right.handle(),0,phase?VK_WHOLE_SIZE:256u}};
                VkDescriptorBufferInfo uniforms[]={{phase?uniform0.handle():VK_NULL_HANDLE,0,phase?16u:VK_WHOLE_SIZE},{phase?VK_NULL_HANDLE:uniform1.handle(),0,phase?VK_WHOLE_SIZE:16u}};
                VkDescriptorBufferInfo result{output.handle(),0,2048};VkWriteDescriptorSet writes[3]{};
                for(auto& w:writes){w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w.dstSet=logical0;w.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;}
                writes[0].dstBinding=0;writes[0].descriptorCount=2;writes[0].pBufferInfo=data;
                writes[1].dstBinding=1;writes[1].descriptorCount=1;writes[1].pBufferInfo=&result;
                writes[2].dstSet=logical1;writes[2].dstBinding=2;writes[2].descriptorCount=2;writes[2].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;writes[2].pBufferInfo=uniforms;updates.update(3,writes,0,nullptr);
            }else if(phase){state.bindBuffer(0,0,0,leftOwner,512,0,256);state.setNull(0,0,1);state.bindBuffer(uniformSet,2,0,uniform0Owner,16,0,16);state.setNull(uniformSet,2,1);}
            else{state.setNull(0,0,0);state.bindBuffer(0,0,1,rightOwner,512,0,256);state.setNull(uniformSet,2,0);state.bindBuffer(uniformSet,2,1,uniform1Owner,16,0,16);}
            auto abi=atomics?Abi::IndexAndInit:Abi::BufferRanges;auto snapshot=multiSet?updates.snapshotBindings(bound,abi):state.snapshot(abi);
            if(snapshot!=(multiSet?updates.snapshotBindings(bound,abi):state.snapshot(abi)))throw std::runtime_error("shared descriptor snapshot cache");
            auto native=adapter.materialize(snapshot,resolver);auto allocations=context.counters().totalAllocations;
            if(native!=adapter.materialize(snapshot,resolver)||context.counters().totalAllocations!=allocations||adapter.counters().updates!=phase+1)throw std::runtime_error("native unchanged descriptor generation reallocated/updated");
            left.flush();right.flush();output.flush();uniform0.flush();uniform1.flush();
            fence.reset();command.begin();auto cmd=command.handle();vkCmdResetQueryPool(cmd,root.query,0,2);vkCmdWriteTimestamp(cmd,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,root.query,0);
            vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline.handle());vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,native->pipelineLayout(),0,uint32_t(native->sets().size()),native->sets().data(),0,nullptr);vkCmdDispatch(cmd,1,1,1);
            command.hostReadBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_WRITE_BIT);vkCmdWriteTimestamp(cmd,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,root.query,1);command.end();
            VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};auto handle=command.handle();submit.commandBufferCount=1;submit.pCommandBuffers=&handle;ok(context.submit(root.queue,submit,fence.handle()),"null submit");auto waited=fence.wait(5'000'000'000ull);if(waited!=VK_SUCCESS)vkDeviceWaitIdle(root.device);ok(waited,"null bounded fence");
            left.invalidate();right.invalidate();output.invalidate();auto events=native->readDiagnostics();
            for(unsigned i=0;i<64;i++){
                uint32_t expected[]={phase?0xA000+i:0,phase?0:0xB000+i,phase?0x1234u:0,phase?0:0x5678u,0,0,i,0xFACE};
                if(atomics){expected[6]=phase?0xDEAD0000+i:0;expected[7]=phase?0:0xB003+i;}
                for(unsigned j=0;j<8;j++){if(o[8*i+j]!=expected[j])throw std::runtime_error("null/bounds mismatch phase="+std::to_string(phase)+" invocation="+std::to_string(i)+" channel="+std::to_string(j)+" actual="+std::to_string(o[8*i+j])+" expected="+std::to_string(expected[j])+" guard_words="+std::to_string(events[0]));verified++;}
                if(a[i]!=(phase?0xDEAD0000+i+(atomics?7:0):0xA000+i)||b[i]!=(phase?0xB000+i:0xB003+i+(atomics?9:0)))throw std::runtime_error("null write leaked or valid write discarded");verified+=2;
                if(a[i+64]!=0xA040+i||b[i+64]!=0xB040+i)throw std::runtime_error("out-of-range store changed protected sentinel");verified+=2;
            }
            if(events[0]!=(atomics?5632u:4928u))throw std::runtime_error("null/bounds guard coverage");
            for(unsigned i=1;i<=events[0];i+=11)if(events[i]!=11||events[i+1]!=23||events[i+3]!=5||(events[i+7]!=4&&events[i+7]!=5))throw std::runtime_error("null/bounds instrumentation unexpected record");records+=events[0]/11;
            uint64_t ticks[2];ok(vkGetQueryPoolResults(root.device,root.query,0,2,sizeof(ticks),ticks,8,VK_QUERY_RESULT_64_BIT),"null timestamps");auto mask=root.bits==64?~0ull:((1ull<<root.bits)-1);totalGpu+=double((ticks[1]-ticks[0])&mask)*root.properties.limits.timestampPeriod/1e6;
        }
        if(adapter.counters().poolCreates!=2||adapter.counters().metadataUploads!=2||adapter.counters().updates!=2||adapter.counters().liveSnapshots)throw std::runtime_error("native descriptor generation lifecycle");
    }
    if(!context.counters().empty()||verified!=1536||records!=(atomics?1024u:896u))throw std::runtime_error("null gate coverage/resources");
    std::ostringstream out;out<<"{\"result\":"<<quote(atomics?"EMULATED_NULL_BUFFER_ATOMIC_FEASIBILITY_PASS":"EMULATED_NULL_BUFFER_RANGE_FEASIBILITY_PASS")<<",\"device\":"<<quote(root.properties.deviceName)<<",\"verified_words\":"<<verified<<",\"guarded_accesses\":"<<records<<",\"gpu_submissions\":2,\"gpu_total_ms\":"<<totalGpu<<",\"core_live_zero\":true,\"shared_descriptor_state\":true,\"snapshot_cache_reused\":true,\"nullDescriptor_advertised\":false,\"robustBufferAccess2_advertised\":false,\"atomic_iadd_tested\":"<<(atomics?"true":"false")<<",\"scope\":\"Shared descriptor state owns resource leases and drives native descriptors+guard metadata. Scalar SSBO/UBO,optional IAdd32,two static slots. NOT full Vulkan descriptor API,all atomics/images/dynamic indexing/robustness2/DXVK.\"}";return out.str();
}
