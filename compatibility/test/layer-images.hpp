void imageSubresources(Root& root,mavk::DeviceContext& context,const char* shaderPath){
    constexpr VkFormat formats[]={VK_FORMAT_BC1_RGBA_UNORM_BLOCK,VK_FORMAT_BC1_RGBA_SRGB_BLOCK};
    for(auto format:formats){VkFormatProperties features{};vkGetPhysicalDeviceFormatProperties(root.gpu,format,&features);if(!(features.optimalTilingFeatures&VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT))throw std::runtime_error("NO-GO: BC1 RGBA sampled image unavailable");}
    VkImageFormatListCreateInfo allowed{VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO};allowed.viewFormatCount=2;allowed.pViewFormats=formats;
    VkImageCreateInfo create{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};create.pNext=&allowed;create.flags=VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;create.imageType=VK_IMAGE_TYPE_2D;create.format=formats[0];create.extent={8,8,1};create.mipLevels=3;create.arrayLayers=2;create.samples=VK_SAMPLE_COUNT_1_BIT;create.tiling=VK_IMAGE_TILING_OPTIMAL;create.usage=VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    mavk::Image source(context,create),copied(context,create);
    struct Objects {
        VkDevice device;std::vector<VkImageView> views;VkSampler sampler{};VkDescriptorSetLayout setLayout{};VkDescriptorPool pool{};VkPipelineLayout layout{};
        ~Objects(){if(layout)vkDestroyPipelineLayout(device,layout,nullptr);if(pool)vkDestroyDescriptorPool(device,pool,nullptr);if(setLayout)vkDestroyDescriptorSetLayout(device,setLayout,nullptr);if(sampler)vkDestroySampler(device,sampler,nullptr);for(auto view:views)vkDestroyImageView(device,view,nullptr);}
    } objects{root.device};
    mavk::Buffer upload(context,224,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    auto bytes=static_cast<uint8_t*>(upload.map());std::fill(bytes,bytes+224,0xCD);upload.flush();
    VkBufferImageCopy regions[3]{};const uint32_t offsets[]={0,144,208},rowPixels[]={12,8,4},heightPixels[]={12,8,4};
    for(uint32_t mip=0;mip<3;mip++){auto& region=regions[mip];region.bufferOffset=offsets[mip];region.bufferRowLength=rowPixels[mip];region.bufferImageHeight=heightPixels[mip];region.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,mip,0,2};region.imageExtent={8u>>mip,8u>>mip,1};}
    auto transition=[&](VkCommandBuffer cmd,VkImage image,VkImageLayout oldLayout,VkImageLayout newLayout,VkAccessFlags from,VkAccessFlags to,VkPipelineStageFlags before,VkPipelineStageFlags after){VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.oldLayout=oldLayout;barrier.newLayout=newLayout;barrier.srcAccessMask=from;barrier.dstAccessMask=to;barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.image=image;barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,3,0,2};vkCmdPipelineBarrier(cmd,before,after,0,0,nullptr,0,nullptr,1,&barrier);};
    mavk::CommandBatch command(context);mavk::Fence fence(context);command.begin();
    transition(command.handle(),source.handle(),VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,0,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    vkCmdCopyBufferToImage(command.handle(),upload.handle(),source.handle(),VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,3,regions);command.end();
    const uint16_t colors[3][2]={{0x8410,0xC618},{0x4208,0x7BEF},{0xF800,0x07E0}};
    // Host contents change AFTER recording: native transfer semantics consume
    // the final flushed contents at execution,not a CPU decode at record time.
    for(unsigned mip=0;mip<3;mip++)for(unsigned layer=0;layer<2;layer++){
        const unsigned blocks=(regions[mip].imageExtent.width+3)/4,rowBytes=rowPixels[mip]/4*8,layerBytes=rowBytes*(heightPixels[mip]/4);
        for(unsigned y=0;y<blocks;y++)for(unsigned x=0;x<blocks;x++){auto block=bytes+offsets[mip]+layer*layerBytes+y*rowBytes+x*8;std::memset(block,0,8);block[0]=uint8_t(colors[mip][layer]);block[1]=uint8_t(colors[mip][layer]>>8);}
    }upload.flush();
    auto handle=command.handle();VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&handle;ok(context.submit(root.queue,submit,fence.handle()));ok(fence.wait(5000000000ull));fence.reset();command.begin();
    transition(command.handle(),source.handle(),VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    transition(command.handle(),copied.handle(),VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,0,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    VkImageCopy copy{};copy.srcSubresource={VK_IMAGE_ASPECT_COLOR_BIT,1,0,2};copy.dstSubresource={VK_IMAGE_ASPECT_COLOR_BIT,1,0,2};copy.extent={4,4,1};vkCmdCopyImage(command.handle(),source.handle(),VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,copied.handle(),VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
    transition(command.handle(),source.handle(),VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_ACCESS_TRANSFER_READ_BIT,VK_ACCESS_SHADER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    transition(command.handle(),copied.handle(),VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    command.end();handle=command.handle();ok(context.submit(root.queue,submit,fence.handle()));ok(fence.wait(5000000000ull));
    VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};sampler.magFilter=sampler.minFilter=VK_FILTER_NEAREST;sampler.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST;sampler.addressModeU=sampler.addressModeV=sampler.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;sampler.maxAnisotropy=1;ok(vkCreateSampler(root.device,&sampler,nullptr,&objects.sampler));
    VkDescriptorSetLayoutBinding bindings[]={{0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}};
    VkDescriptorSetLayoutCreateInfo schema{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};schema.bindingCount=2;schema.pBindings=bindings;ok(vkCreateDescriptorSetLayout(root.device,&schema,nullptr,&objects.setLayout));
    VkDescriptorPoolSize sizes[]={{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1},{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1}};VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=1;pool.poolSizeCount=2;pool.pPoolSizes=sizes;ok(vkCreateDescriptorPool(root.device,&pool,nullptr,&objects.pool));
    VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};allocate.descriptorPool=objects.pool;allocate.descriptorSetCount=1;allocate.pSetLayouts=&objects.setLayout;VkDescriptorSet set{};ok(vkAllocateDescriptorSets(root.device,&allocate,&set));
    VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};layout.setLayoutCount=1;layout.pSetLayouts=&objects.setLayout;ok(vkCreatePipelineLayout(root.device,&layout,nullptr,&objects.layout));
    auto code=words(shaderPath);mavk::Shader shader(context,code.data(),code.size()*4);VkComputePipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};pipelineInfo.layout=objects.layout;pipelineInfo.stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;pipelineInfo.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;pipelineInfo.stage.module=shader.handle();pipelineInfo.stage.pName="main";mavk::Pipeline pipeline(context,pipelineInfo);
    mavk::Buffer output(context,64,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);auto result=static_cast<uint32_t*>(output.map());unsigned verified=0,viewCount=0;
    for(unsigned target=0;target<2;target++)for(unsigned mip=0;mip<3;mip++){
        if(target&&mip!=1)continue;
        for(unsigned layer=0;layer<2;layer++)for(unsigned srgb=0;srgb<2;srgb++){
            VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};view.image=target?copied.handle():source.handle();view.viewType=VK_IMAGE_VIEW_TYPE_2D;view.format=formats[srgb];view.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,mip,1,layer,1};VkImageView imageView{};ok(vkCreateImageView(root.device,&view,nullptr,&imageView));objects.views.push_back(imageView);viewCount++;
            fence.reset();command.begin();std::fill(result,result+16,0xCCCCCCCCu);output.flush();
            VkDescriptorImageInfo image{objects.sampler,imageView,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};VkDescriptorBufferInfo buffer{output.handle(),0,64};VkWriteDescriptorSet writes[2]{};for(auto& write:writes){write.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;write.dstSet=set;write.descriptorCount=1;}writes[0].descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;writes[0].pImageInfo=&image;writes[1].dstBinding=1;writes[1].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;writes[1].pBufferInfo=&buffer;vkUpdateDescriptorSets(root.device,2,writes,0,nullptr);
            vkCmdBindPipeline(command.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,pipeline.handle());vkCmdBindDescriptorSets(command.handle(),VK_PIPELINE_BIND_POINT_COMPUTE,objects.layout,0,1,&set,0,nullptr);vkCmdDispatch(command.handle(),1,1,1);command.hostReadBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_WRITE_BIT);command.end();handle=command.handle();ok(context.submit(root.queue,submit,fence.handle()));ok(fence.wait(5000000000ull));output.invalidate();
            auto packed=colors[mip][layer];unsigned r=packed>>11,g=(packed>>5)&63,b=packed&31;unsigned channels[]={(r<<3)|(r>>2),(g<<2)|(g>>4),(b<<3)|(b>>2),255};
            for(unsigned i=0;i<16;i++){double value=channels[i%4]/255.0;if(srgb&&i%4!=3)value=value<=0.04045?value/12.92:std::pow((value+0.055)/1.055,2.4);const auto expected=uint32_t(std::lround(value*255));const auto delta=result[i]>expected?result[i]-expected:expected-result[i];if(delta>(srgb?1u:0u))throw std::runtime_error("BC image/view mismatch target="+std::to_string(target)+" mip="+std::to_string(mip)+" layer="+std::to_string(layer)+" srgb="+std::to_string(srgb)+" actual="+std::to_string(result[i])+" expected="+std::to_string(expected));verified++;}
        }
    }
    std::cout<<"IMAGE_SUBRESOURCE_GPU_PASS views="<<viewCount<<" components="<<verified<<" mips=3 layers=2 mutableSrgb=1 paddedRows=1 afterRecordHostWrite=1 compressedCopy=1\n";
}
