// Descriptor-init mode, not buffer-range mode. Same native Core resources.
std::string nullImagesGate(const std::vector<uint32_t>& code,bool validated,ValidationCapture& validation){
    Root root(validation);root.init(validated);mavk::DeviceContext context(root.physical,root.device,root.family);
    struct Native {
        VkDevice device;VkDescriptorSetLayout sets[2]{};VkPipelineLayout layout{};VkDescriptorPool pool{};VkImageView views[2]{};
        ~Native(){vkDeviceWaitIdle(device);if(pool)vkDestroyDescriptorPool(device,pool,nullptr);if(layout)vkDestroyPipelineLayout(device,layout,nullptr);for(auto s:sets)if(s)vkDestroyDescriptorSetLayout(device,s,nullptr);for(auto v:views)if(v)vkDestroyImageView(device,v,nullptr);}
    } native{root.device};
    uint64_t verified=0,guards=0;
    {
        mavk::SampledPixels pixels;pixels.width=8;pixels.height=8;pixels.levels=1;pixels.rgba.resize(256);
        for(unsigned i=0;i<64;i++)for(unsigned c=0;c<4;c++)pixels.rgba[4*i+c]=uint8_t(11*(c+1));
        auto sampled0Owner=std::make_shared<mavk::SampledImage>(context,pixels);auto& sampled0=*sampled0Owner;sampled0.upload(root.queue,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        for(unsigned i=0;i<64;i++)for(unsigned c=0;c<4;c++)pixels.rgba[4*i+c]=uint8_t(55+11*c);
        auto sampled1Owner=std::make_shared<mavk::SampledImage>(context,pixels);auto& sampled1=*sampled1Owner;sampled1.upload(root.queue,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        VkImageCreateInfo image{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};image.imageType=VK_IMAGE_TYPE_2D;image.format=VK_FORMAT_R32_UINT;image.extent={8,8,1};image.mipLevels=1;image.arrayLayers=1;image.samples=VK_SAMPLE_COUNT_1_BIT;image.tiling=VK_IMAGE_TILING_OPTIMAL;image.usage=VK_IMAGE_USAGE_STORAGE_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        struct StorageLease {
            VkDevice device;std::shared_ptr<mavk::Image> image;VkImageView view{};
            StorageLease(mavk::DeviceContext& context,const VkImageCreateInfo& info):device(context.vkDevice()),image(std::make_shared<mavk::Image>(context,info)){
                VkImageViewCreateInfo v{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};v.image=image->handle();v.viewType=VK_IMAGE_VIEW_TYPE_2D;v.format=VK_FORMAT_R32_UINT;v.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};ok(vkCreateImageView(device,&v,nullptr,&view),"null storage view");
            }
            ~StorageLease(){if(view)vkDestroyImageView(device,view,nullptr);}
        };
        auto storage0Owner=std::make_shared<StorageLease>(context,image),storage1Owner=std::make_shared<StorageLease>(context,image);VkImage handles[]={storage0Owner->image->handle(),storage1Owner->image->handle()};
        const auto host=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,coherent=VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        auto outputOwner=std::make_shared<mavk::Buffer>(context,3072,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,host,coherent);auto& output=*outputOwner;
        mavk::Buffer readback(context,512,VK_BUFFER_USAGE_TRANSFER_DST_BIT,host,coherent),metadata(context,512,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,host,coherent),log(context,16384,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,host,coherent);
        using Kind=mavk::compat::DescriptorKind;using Abi=mavk::compat::GuardMetadataAbi;
        mavk::compat::DescriptorState state({{0,0,2,Kind::CombinedImageSampler},{0,1,2,Kind::StorageImage},{0,2,1,Kind::StorageBuffer}});
        state.bindBuffer(0,2,0,outputOwner,3072,0,3072);
        auto o=static_cast<uint32_t*>(output.map()),r=static_cast<uint32_t*>(readback.map()),meta=static_cast<uint32_t*>(metadata.map()),events=static_cast<uint32_t*>(log.map());
        VkDescriptorSetLayoutBinding bindings[]={{0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,2,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{1,VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,2,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{2,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}};
        VkDescriptorSetLayoutBinding debug[]={{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}};
        VkDescriptorSetLayoutCreateInfo sl{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};sl.bindingCount=3;sl.pBindings=bindings;ok(vkCreateDescriptorSetLayout(root.device,&sl,nullptr,&native.sets[0]),"null image set");sl.bindingCount=2;sl.pBindings=debug;ok(vkCreateDescriptorSetLayout(root.device,&sl,nullptr,&native.sets[1]),"null image metadata set");
        VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};pl.setLayoutCount=2;pl.pSetLayouts=native.sets;ok(vkCreatePipelineLayout(root.device,&pl,nullptr,&native.layout),"null image pipeline layout");
        mavk::Shader shader(context,code.data(),code.size()*4);VkComputePipelineCreateInfo pc{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};pc.layout=native.layout;pc.stage={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};pc.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;pc.stage.module=shader.handle();pc.stage.pName="main";mavk::Pipeline pipeline(context,pc);mavk::CommandBatch command(context);mavk::Fence fence(context);
        VkDescriptorPoolSize sizes[]={{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,3},{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,2},{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,2}};VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=2;pool.poolSizeCount=3;pool.pPoolSizes=sizes;ok(vkCreateDescriptorPool(root.device,&pool,nullptr,&native.pool),"null image pool");
        VkDescriptorSet sets[2];VkDescriptorSetAllocateInfo da{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};da.descriptorPool=native.pool;da.descriptorSetCount=2;da.pSetLayouts=native.sets;ok(vkAllocateDescriptorSets(root.device,&da,sets),"null image descriptor sets");
        VkDescriptorImageInfo sampled[]={sampled0.descriptor(),sampled1.descriptor()},storage[]={{VK_NULL_HANDLE,storage0Owner->view,VK_IMAGE_LAYOUT_GENERAL},{VK_NULL_HANDLE,storage1Owner->view,VK_IMAGE_LAYOUT_GENERAL}};
        VkDescriptorBufferInfo buffers[]={{output.handle(),0,3072},{log.handle(),0,16384},{metadata.handle(),0,512}};VkWriteDescriptorSet writes[5]{};for(auto& w:writes)w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet=sets[0];writes[0].dstBinding=0;writes[0].descriptorCount=2;writes[0].descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;writes[0].pImageInfo=sampled;
        writes[1].dstSet=sets[0];writes[1].dstBinding=1;writes[1].descriptorCount=2;writes[1].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;writes[1].pImageInfo=storage;
        for(unsigned i=0;i<3;i++){writes[i+2].dstSet=sets[i?1:0];writes[i+2].dstBinding=i?i-1:2;writes[i+2].descriptorCount=1;writes[i+2].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;writes[i+2].pBufferInfo=&buffers[i];}
        for(unsigned phase=0;phase<2;phase++){
            std::memset(o,0xCC,3072);std::memset(meta,0,512);std::memset(events,0,16384);
            if(phase){state.bindImage(0,0,0,sampled0Owner);state.setNull(0,0,1);state.bindImage(0,1,0,storage0Owner);state.setNull(0,1,1);}
            else{state.setNull(0,0,0);state.bindImage(0,0,1,sampled1Owner);state.setNull(0,1,0);state.bindImage(0,1,1,storage1Owner);}
            auto snapshot=state.snapshot(Abi::IndexAndInit);if(snapshot!=state.snapshot(Abi::IndexAndInit)||snapshot->metadata.size()*4>512)throw std::runtime_error("shared image descriptor cache/budget");
            std::memcpy(meta,snapshot->metadata.data(),snapshot->metadata.size()*4);
            for(unsigned i=0;i<2;i++){
                const auto& sample=snapshot->at(0,0,i);const auto& store=snapshot->at(0,1,i);
                sampled[i]=sample.status==mavk::compat::DescriptorStatus::Null?(i?sampled1:sampled0).descriptor():std::static_pointer_cast<const mavk::SampledImage>(sample.owner)->descriptor();
                auto lease=store.status==mavk::compat::DescriptorStatus::Null?(i?storage1Owner:storage0Owner):std::static_pointer_cast<const StorageLease>(store.owner);
                storage[i]={VK_NULL_HANDLE,lease->view,VK_IMAGE_LAYOUT_GENERAL};
            }
            vkUpdateDescriptorSets(root.device,5,writes,0,nullptr);output.flush();metadata.flush();log.flush();
            fence.reset();command.begin();auto cmd=command.handle();VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
            auto transition=[&](VkImage h,VkImageLayout from,VkImageLayout to,VkPipelineStageFlags srcStage,VkPipelineStageFlags dstStage,VkAccessFlags src,VkAccessFlags dst){VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};b.oldLayout=from;b.newLayout=to;b.srcAccessMask=src;b.dstAccessMask=dst;b.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.image=h;b.subresourceRange=range;vkCmdPipelineBarrier(cmd,srcStage,dstStage,0,0,nullptr,0,nullptr,1,&b);};
            for(unsigned i=0;i<2;i++){
                transition(handles[i],phase?VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,phase?VK_PIPELINE_STAGE_TRANSFER_BIT:VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,phase?VK_ACCESS_TRANSFER_READ_BIT:0,VK_ACCESS_TRANSFER_WRITE_BIT);
                VkClearColorValue clear{};clear.uint32[0]=i?0x5678:0x1234;vkCmdClearColorImage(cmd,handles[i],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&clear,1,&range);
                transition(handles[i],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_GENERAL,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT);
            }
            vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline.handle());vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,native.layout,0,2,sets,0,nullptr);vkCmdDispatch(cmd,1,1,1);
            for(unsigned i=0;i<2;i++){
                transition(handles[i],VK_IMAGE_LAYOUT_GENERAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT);
                VkBufferImageCopy copy{};copy.bufferOffset=i*256;copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={8,8,1};vkCmdCopyImageToBuffer(cmd,handles[i],VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,readback.handle(),1,&copy);
            }
            command.hostReadBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_TRANSFER_WRITE_BIT);command.end();VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&cmd;ok(context.submit(root.queue,submit,fence.handle()),"null image submit");ok(fence.wait(5'000'000'000ull),"null image fence");output.invalidate();readback.invalidate();log.invalidate();
            for(unsigned i=0;i<64;i++){
                uint32_t expected[12]{};for(unsigned c=0;c<4;c++){expected[c]=phase?11*(c+1):0;expected[4+c]=phase?0:55+11*c;}expected[8]=phase?0x1234:0;expected[9]=phase?0:0x5678;expected[10]=i;expected[11]=0xFACE;
                for(unsigned c=0;c<12;c++){if(o[12*i+c]!=expected[c])throw std::runtime_error("null image read mismatch phase="+std::to_string(phase)+" channel="+std::to_string(c)+" actual="+std::to_string(o[12*i+c]));verified++;}
                if(r[i]!=(phase?1000+i:0x1234)||r[64+i]!=(phase?0x5678:2000+i))throw std::runtime_error("null storage image write leaked or live write lost");verified+=2;
            }
            if(events[0]!=1920)throw std::runtime_error("null image guard coverage");for(unsigned i=1;i<=1920;i+=10)if(events[i]!=10||events[i+1]!=23||events[i+3]!=5||events[i+7]!=1)throw std::runtime_error("null image guard record");guards+=192;
        }
    }
    if(!context.counters().empty()||verified!=1792||guards!=384)throw std::runtime_error("null image resource/coverage");
    return "{\"result\":\"EMULATED_NULL_IMAGE_FEASIBILITY_PASS\",\"device\":"+quote(root.properties.deviceName)+",\"verified_words\":1792,\"guarded_accesses\":384,\"gpu_submissions\":4,\"core_live_zero\":true,\"shared_descriptor_state\":true,\"snapshot_cache_reused\":true,\"nullDescriptor_advertised\":false,\"scope\":\"Shared descriptor state drives native descriptors and metadata with full image/view/sampler leases. Combined texelFetch/R32_UINT load-store,static slots. NOT complete Vulkan descriptor API,all image operations,full nullDescriptor,DXVK or performance PASS.\"}";
}
