// Descriptor-init mode, not buffer-range mode. Same native Core resources.
std::string nullImagesGate(const std::vector<uint32_t>& code,bool validated,ValidationCapture& validation,bool liveArrays=false){
    const auto imageTypes=mavk::compat::reflectColorImageTypes(code);
    const bool arrayed=imageTypes.at({0,0}).viewType==VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    const bool volume=imageTypes.at({0,0}).viewType==VK_IMAGE_VIEW_TYPE_3D;
    const bool cube=imageTypes.at({0,0}).viewType==VK_IMAGE_VIEW_TYPE_CUBE;
    const bool storageVolume=imageTypes.at({0,1}).viewType==VK_IMAGE_VIEW_TYPE_3D;
    if(liveArrays&&!arrayed)throw std::runtime_error("live array oracle requires array shader");
    const uint32_t expectedGuards=(arrayed&&!liveArrays)||volume||cube?256:192;
    bool filtered=false;for(size_t p=5;p<code.size();p+=code[p]>>16)if((code[p]&65535)==88)filtered=true;
    std::map<uint32_t,uint32_t> descriptorSets,descriptorBindings;
    for(size_t p=5;p<code.size();p+=code[p]>>16)if((code[p]&65535)==71&&(code[p]>>16)==4){
        if(code[p+2]==34)descriptorSets[code[p+1]]=code[p+3];
        if(code[p+2]==33)descriptorBindings[code[p+1]]=code[p+3];
    }
    bool reports=false;for(auto [id,set]:descriptorSets)if(set==1&&descriptorBindings.count(id)&&descriptorBindings.at(id)==0)reports=true;
    Root root(validation);root.init(validated);mavk::DeviceContext context(root.physical,root.device,root.family);
    uint64_t verified=0,guards=0,fallbackVerified=0;
    {
        mavk::SampledPixels pixels;pixels.width=8;pixels.height=8;pixels.levels=1;pixels.rgba.resize(256);
        for(unsigned i=0;i<64;i++)for(unsigned c=0;c<4;c++)pixels.rgba[4*i+c]=uint8_t(11*(c+1));
        auto sampled0Owner=std::make_shared<mavk::SampledImage>(context,pixels);auto& sampled0=*sampled0Owner;sampled0.upload(root.queue,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        for(unsigned i=0;i<64;i++)for(unsigned c=0;c<4;c++)pixels.rgba[4*i+c]=uint8_t(55+11*c);
        auto sampled1Owner=std::make_shared<mavk::SampledImage>(context,pixels);auto& sampled1=*sampled1Owner;sampled1.upload(root.queue,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        VkImageCreateInfo image{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};image.imageType=VK_IMAGE_TYPE_2D;image.format=VK_FORMAT_R32_UINT;image.extent={8,8,1};image.mipLevels=1;image.arrayLayers=1;image.samples=VK_SAMPLE_COUNT_1_BIT;image.tiling=VK_IMAGE_TILING_OPTIMAL;image.usage=VK_IMAGE_USAGE_STORAGE_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        if(storageVolume)image.imageType=VK_IMAGE_TYPE_3D;
        struct StorageLease {
            VkDevice device;std::shared_ptr<mavk::Image> image;VkImageView view{};
            StorageLease(mavk::DeviceContext& context,const VkImageCreateInfo& info):device(context.vkDevice()),image(std::make_shared<mavk::Image>(context,info)){
                VkImageViewCreateInfo v{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};v.image=image->handle();v.viewType=info.imageType==VK_IMAGE_TYPE_3D?VK_IMAGE_VIEW_TYPE_3D:VK_IMAGE_VIEW_TYPE_2D;v.format=VK_FORMAT_R32_UINT;v.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};ok(vkCreateImageView(device,&v,nullptr,&view),"null storage view");
            }
            ~StorageLease(){if(view)vkDestroyImageView(device,view,nullptr);}
        };
        auto storage0Owner=std::make_shared<StorageLease>(context,image),storage1Owner=std::make_shared<StorageLease>(context,image);VkImage handles[]={storage0Owner->image->handle(),storage1Owner->image->handle()};
        const auto host=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,coherent=VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        auto outputOwner=std::make_shared<mavk::Buffer>(context,3072,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,host,coherent);auto& output=*outputOwner;
        mavk::Buffer readback(context,544,VK_BUFFER_USAGE_TRANSFER_DST_BIT,host,coherent);
        using Kind=mavk::compat::DescriptorKind;using Abi=mavk::compat::GuardMetadataAbi;
        mavk::compat::VulkanDescriptorResources resources;mavk::compat::VulkanDescriptorUpdates updates(resources,{},true);
        auto logical=reinterpret_cast<VkDescriptorSet>(uintptr_t(0x7003));
        auto sampledInfo0=sampled0.descriptor(),sampledInfo1=sampled1.descriptor();
        // View layers0/1 select physical layers1/2, not sentinel layer0.
        struct ArrayLease {
            VkDevice device;std::shared_ptr<mavk::Image> image;VkImageView view{};
            ArrayLease(mavk::DeviceContext& context):device(context.vkDevice()){
                VkImageCreateInfo i{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};i.imageType=VK_IMAGE_TYPE_2D;i.format=VK_FORMAT_R8G8B8A8_UNORM;i.extent={8,8,1};i.mipLevels=1;i.arrayLayers=3;i.samples=VK_SAMPLE_COUNT_1_BIT;i.tiling=VK_IMAGE_TILING_OPTIMAL;i.usage=VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                image=std::make_shared<mavk::Image>(context,i);VkImageViewCreateInfo v{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};v.image=image->handle();v.viewType=VK_IMAGE_VIEW_TYPE_2D_ARRAY;v.format=i.format;v.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,1,2};ok(vkCreateImageView(device,&v,nullptr,&view),"live array view");
            }
            ~ArrayLease(){if(view)vkDestroyImageView(device,view,nullptr);}
        };
        std::shared_ptr<ArrayLease> array0,array1;
        if(liveArrays){array0=std::make_shared<ArrayLease>(context);array1=std::make_shared<ArrayLease>(context);sampledInfo0.imageView=array0->view;sampledInfo1.imageView=array1->view;}
        resources.registerImageView(sampledInfo0.imageView,VK_IMAGE_USAGE_SAMPLED_BIT,liveArrays?std::shared_ptr<void>(array0):std::shared_ptr<void>(sampled0Owner));resources.registerImageView(sampledInfo1.imageView,VK_IMAGE_USAGE_SAMPLED_BIT,liveArrays?std::shared_ptr<void>(array1):std::shared_ptr<void>(sampled1Owner));
        resources.registerSampler(sampledInfo0.sampler,sampled0Owner);resources.registerSampler(sampledInfo1.sampler,sampled1Owner);
        resources.registerImageView(storage0Owner->view,VK_IMAGE_USAGE_STORAGE_BIT,storage0Owner);resources.registerImageView(storage1Owner->view,VK_IMAGE_USAGE_STORAGE_BIT,storage1Owner);
        resources.registerBuffer(outputOwner->handle(),3072,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,outputOwner);
        VkDescriptorSetLayoutBinding bindings[]={{0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,2,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{1,VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,2,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{2,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}};
        VkDescriptorSetLayoutCreateInfo create{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};create.bindingCount=3;create.pBindings=bindings;updates.registerSet(logical,create);
        VkDescriptorBufferInfo outputInfo{outputOwner->handle(),0,3072};VkWriteDescriptorSet outputWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};outputWrite.dstSet=logical;outputWrite.dstBinding=2;outputWrite.descriptorCount=1;outputWrite.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;outputWrite.pBufferInfo=&outputInfo;updates.update(1,&outputWrite,0,nullptr);
        auto o=static_cast<uint32_t*>(output.map()),r=static_cast<uint32_t*>(readback.map());
        mavk::compat::NativeDescriptorAdapter adapter(context,{{0,0,2,Kind::CombinedImageSampler},{0,1,2,Kind::StorageImage},{0,2,1,Kind::StorageBuffer}},Abi::IndexAndInit);
        mavk::Shader shader(context,code.data(),code.size()*4);VkComputePipelineCreateInfo pc{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};pc.layout=adapter.pipelineLayout();pc.stage={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};pc.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;pc.stage.module=shader.handle();pc.stage.pName="main";mavk::Pipeline pipeline(context,pc);mavk::CommandBatch command(context);mavk::Fence fence(context);
        mavk::compat::NullImage2DResources nullImages(context);std::vector<std::pair<VkImage,uint32_t>> fallbackImages;
        auto resolver=[&](const mavk::compat::DescriptorBinding& binding,uint32_t index,const mavk::compat::DescriptorValue& value){
            if(value.status==mavk::compat::DescriptorStatus::Bound)return mavk::compat::VulkanDescriptorResources::resolve(binding,index,value);
            mavk::compat::NativeDescriptorResource sampler;
            if(binding.kind==Kind::CombinedImageSampler){
                auto tuple=std::static_pointer_cast<const mavk::compat::VulkanDescriptorResources::Resource>(value.owner);
                if(!tuple||!tuple->sampling)throw std::runtime_error("null combined sampler lease absent");
                sampler.owner=tuple->sampling;sampler.image.sampler=tuple->sampler;
            }
            const bool sampled=binding.kind!=Kind::StorageImage;
            const bool threeD=sampled?volume:storageVolume;
            const auto viewType=threeD?VK_IMAGE_VIEW_TYPE_3D:sampled&&cube?VK_IMAGE_VIEW_TYPE_CUBE:sampled&&arrayed?VK_IMAGE_VIEW_TYPE_2D_ARRAY:VK_IMAGE_VIEW_TYPE_2D;
            return nullImages.resolve(binding,value,binding.kind==Kind::StorageImage?VK_FORMAT_R32_UINT:VK_FORMAT_R8G8B8A8_UNORM,sampler,threeD?VK_IMAGE_TYPE_3D:VK_IMAGE_TYPE_2D,viewType);
        };
        for(unsigned phase=0;phase<2;phase++){
            std::memset(o,0xCC,3072);
            VkDescriptorImageInfo sampled[]={{sampledInfo0.sampler,phase?sampledInfo0.imageView:VK_NULL_HANDLE,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},{sampledInfo1.sampler,phase?VK_NULL_HANDLE:sampledInfo1.imageView,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
            if((arrayed&&!liveArrays)||volume||cube)for(auto& descriptor:sampled)descriptor.imageView=VK_NULL_HANDLE;
            VkDescriptorImageInfo storage[]={{VK_NULL_HANDLE,phase?storage0Owner->view:VK_NULL_HANDLE,VK_IMAGE_LAYOUT_GENERAL},{VK_NULL_HANDLE,phase?VK_NULL_HANDLE:storage1Owner->view,VK_IMAGE_LAYOUT_GENERAL}};
            VkWriteDescriptorSet writes[2]{};for(auto& w:writes){w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w.dstSet=logical;w.descriptorCount=2;}
            writes[0].dstBinding=0;writes[0].descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;writes[0].pImageInfo=sampled;
            writes[1].dstBinding=1;writes[1].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;writes[1].pImageInfo=storage;updates.update(2,writes,0,nullptr);
            auto snapshot=updates.snapshot(logical,Abi::IndexAndInit);if(snapshot!=updates.snapshot(logical,Abi::IndexAndInit))throw std::runtime_error("shared image descriptor cache");
            auto native=adapter.materialize(snapshot,resolver);auto allocations=context.counters().totalAllocations;
            if(native!=adapter.materialize(snapshot,resolver)||context.counters().totalAllocations!=allocations||adapter.counters().updates!=phase+1)throw std::runtime_error("native image generation reallocated/updated");
            output.flush();
            fence.reset();command.begin();auto cmd=command.handle();auto initialization=nullImages.pending();if(initialization.size()!=(phase?0u:2u))throw std::runtime_error("null initialization/cache count");for(size_t i=0;i<initialization.size();i++)fallbackImages.emplace_back(initialization.image(i),initialization.layers(i));initialization.record(cmd);VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
            auto transition=[&](VkImage h,VkImageLayout from,VkImageLayout to,VkPipelineStageFlags srcStage,VkPipelineStageFlags dstStage,VkAccessFlags src,VkAccessFlags dst){VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};b.oldLayout=from;b.newLayout=to;b.srcAccessMask=src;b.dstAccessMask=dst;b.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.image=h;b.subresourceRange=range;vkCmdPipelineBarrier(cmd,srcStage,dstStage,0,0,nullptr,0,nullptr,1,&b);};
            if(liveArrays&&!phase)for(unsigned which=0;which<2;which++){
                const auto handle=(which?array1:array0)->image->handle();
                VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};b.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;b.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;b.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.image=handle;b.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,3};
                vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&b);
                for(unsigned layer=0;layer<3;layer++){VkClearColorValue clear{};for(unsigned c=0;c<4;c++)clear.float32[c]=float(layer?(which?55+11*c:11*(c+1))+7*(layer-1):201)/255.f;VkImageSubresourceRange one{VK_IMAGE_ASPECT_COLOR_BIT,0,1,layer,1};vkCmdClearColorImage(cmd,handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&clear,1,&one);}
                b.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;b.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;b.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;b.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
                vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,0,nullptr,0,nullptr,1,&b);
            }
            for(unsigned i=0;i<2;i++){
                transition(handles[i],phase?VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,phase?VK_PIPELINE_STAGE_TRANSFER_BIT:VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,phase?VK_ACCESS_TRANSFER_READ_BIT:0,VK_ACCESS_TRANSFER_WRITE_BIT);
                VkClearColorValue clear{};clear.uint32[0]=i?0x5678:0x1234;vkCmdClearColorImage(cmd,handles[i],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&clear,1,&range);
                transition(handles[i],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_GENERAL,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT);
            }
            for(auto [image,layers]:fallbackImages){VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.oldLayout=barrier.newLayout=VK_IMAGE_LAYOUT_GENERAL;barrier.srcAccessMask=VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.image=image;barrier.subresourceRange=range;barrier.subresourceRange.layerCount=layers;vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,0,nullptr,0,nullptr,1,&barrier);}
            vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline.handle());vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,native->pipelineLayout(),0,2,native->sets().data(),0,nullptr);vkCmdDispatch(cmd,1,1,1);
            for(unsigned i=0;i<2;i++){
                transition(handles[i],VK_IMAGE_LAYOUT_GENERAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT);
                VkBufferImageCopy copy{};copy.bufferOffset=i*256;copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={8,8,1};vkCmdCopyImageToBuffer(cmd,handles[i],VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,readback.handle(),1,&copy);
            }
            uint32_t fallbackWords=0;
            for(auto [image,layers]:fallbackImages){
                if(layers>8-fallbackWords)throw std::runtime_error("fallback readback budget");
                VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.oldLayout=barrier.newLayout=VK_IMAGE_LAYOUT_GENERAL;barrier.srcAccessMask=VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.image=image;barrier.subresourceRange=range;barrier.subresourceRange.layerCount=layers;
                vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
                VkBufferImageCopy copy{};copy.bufferOffset=512+fallbackWords*4;copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,layers};copy.imageExtent={1,1,1};vkCmdCopyImageToBuffer(cmd,image,VK_IMAGE_LAYOUT_GENERAL,readback.handle(),1,&copy);fallbackWords+=layers;
            }
            command.hostReadBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_TRANSFER_WRITE_BIT);command.end();VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&cmd;ok(context.submit(root.queue,submit,fence.handle()),"null image submit");initialization.submitted();auto waited=fence.wait(5'000'000'000ull);if(waited!=VK_SUCCESS)vkDeviceWaitIdle(root.device);ok(waited,"null image fence");output.invalidate();readback.invalidate();for(uint32_t i=0;i<fallbackWords;i++){if(r[128+i])throw std::runtime_error("null image write leaked to owned backing");fallbackVerified++;}auto events=native->readDiagnostics();
            for(unsigned i=0;i<64;i++){
                uint32_t expected[12]{};for(unsigned c=0;c<4;c++){expected[c]=!cube&&!volume&&(!arrayed||liveArrays)&&phase?11*(c+1)+(liveArrays?7*(i%2):0):0;expected[4+c]=!cube&&!volume&&(!arrayed||liveArrays)&&!phase?55+11*c+(liveArrays?7*(i%2):0):0;}expected[8]=phase?0x1234:0;expected[9]=phase?0:0x5678;expected[10]=i;expected[11]=0xFACE;
                for(unsigned c=0;c<12;c++){if(o[12*i+c]!=expected[c])throw std::runtime_error("null image read mismatch phase="+std::to_string(phase)+" channel="+std::to_string(c)+" actual="+std::to_string(o[12*i+c]));verified++;}
                if(r[i]!=(phase?1000+i:0x1234)||r[64+i]!=(phase?0x5678:2000+i))throw std::runtime_error("null storage image write leaked or live write lost");verified+=2;
            }
            // Pinned SPIRV-Tools emits ten-word init-only reports or eleven
            // words when buffer-OOB checks share the writer. Count records,
            // not a guessed byte stride; require the actual null-init error.
            if(events[0]>8191)throw std::runtime_error("null image diagnostics truncated");
            uint32_t records=0;std::array<uint32_t,64> invocations{};
            for(uint32_t at=1;at<=events[0];){
                const auto size=events[at];
                if((size!=10&&size!=11)||size>events[0]-at+1||events[at+1]!=23||events[at+3]!=5||events[at+7]!=1||events[at+4]>=64||events[at+5]||events[at+6]||events[at+8]>1)throw std::runtime_error("null image guard record");
                invocations[events[at+4]]++;records++;at+=size;
            }
            const auto expectedRecords=reports?expectedGuards:0;
            if(records!=expectedRecords)throw std::runtime_error("null image guard access count actual="+std::to_string(records)+" expected="+std::to_string(expectedRecords));
            for(auto count:invocations)if(count!=expectedRecords/64)throw std::runtime_error("null image per-invocation coverage");
            guards+=records;
        }
        if(adapter.counters().poolCreates!=2||adapter.counters().metadataUploads!=2||adapter.counters().updates!=2||adapter.counters().liveSnapshots)throw std::runtime_error("native image descriptor generation lifecycle");
    }
    if(!context.counters().empty()||verified!=1792||guards!=(reports?2*expectedGuards:0))throw std::runtime_error("null image resource/coverage");
    std::ostringstream report;report<<"{\"result\":\"EMULATED_NULL_IMAGE_FEASIBILITY_PASS\",\"device\":"<<quote(root.properties.deviceName)
      <<",\"verified_words\":1792,\"guarded_accesses\":"<<guards<<",\"expected_guarded_accesses\":"<<2*expectedGuards
      <<",\"diagnostic_reports\":"<<(reports?"true":"false")<<",\"gpu_submissions\":4,\"core_live_zero\":true,\"shared_descriptor_state\":true,\"snapshot_cache_reused\":true,\"nullDescriptor_advertised\":false"
      <<",\"null_sampled_array_view\":"<<(arrayed?"true":"false")<<",\"null_sampled_3d_view\":"<<(volume?"true":"false")
      <<",\"null_sampled_cube_view\":"<<(cube?"true":"false")<<",\"cube_backing_faces\":"<<(cube?6:0)<<",\"fallback_verified_words\":"<<fallbackVerified
      <<",\"storage_image_dimension\":"<<(storageVolume?3:2)<<",\"live_array_layers\":"<<(liveArrays?2:0)<<",\"array_base_layer\":"<<(liveArrays?1:0)
      <<",\"sample_operation\":"<<quote(filtered?"explicit_lod":"fetch")
      <<",\"scope\":\"Bounded two-phase sampled/storage oracle. Volume/cube sampled slots both null. Cube initialization/readback covers six faces; no live cube,interpolation,mip,query,atomic or full feature claim.\"}";return report.str();
}
