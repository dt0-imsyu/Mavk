// Isolated compressed-image API probe. Known constant blocks,not full codec
// conformance. Failure uses process teardown,never a production resource path.
static float bc5_oracle(unsigned pixel,unsigned channel,unsigned phase,int sign){
    int a=sign?(phase==0?127:-127):(phase==0?255:0),b=sign?(phase==0?-127:127):(phase==0?0:255);
    int index=(int)((pixel+3*channel+phase)%8);
    float scale=sign?127.0f:255.0f;
    if(index==0)return a/scale;if(index==1)return b/scale;
    if(a>b)return ((8-index)*a+(int)(index-1)*b)/(7*scale);
    if(index==6)return sign?-1.0f:0.0f;if(index==7)return 1.0f;
    return ((6-index)*a+(int)(index-1)*b)/(5*scale);
}
static int bc_probe(VkQueue queue,uint32_t family,const char* shader,int linear){
#define BC_LOAD(n) PFN_vk##n n=(PFN_vk##n)get(instance,"vk" #n);if(!n)return fail("resolve " #n,-1)
    BC_LOAD(GetPhysicalDeviceFormatProperties);BC_LOAD(GetPhysicalDeviceImageFormatProperties);
    BC_LOAD(FlushMappedMemoryRanges);BC_LOAD(CreateSampler);BC_LOAD(DestroySampler);
    BC_LOAD(CreateDescriptorSetLayout);BC_LOAD(DestroyDescriptorSetLayout);
    BC_LOAD(CreateDescriptorPool);BC_LOAD(DestroyDescriptorPool);BC_LOAD(AllocateDescriptorSets);BC_LOAD(UpdateDescriptorSets);
    BC_LOAD(CreateComputePipelines);BC_LOAD(CmdBindDescriptorSets);BC_LOAD(CmdDispatch);BC_LOAD(CmdCopyBufferToImage);
#undef BC_LOAD
    unsigned available=0,checked=0;
    VkImageUsageFlags usage=VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    for(unsigned f=0;f<16;f++){
        if(linear&&f!=10&&f!=11)continue;
        VkFormat format=VK_FORMAT_BC1_RGB_UNORM_BLOCK+f;VkFormatProperties fp;GetPhysicalDeviceFormatProperties(physical,format,&fp);
        VkImageFormatProperties ip;VkResult supported=GetPhysicalDeviceImageFormatProperties(physical,format,VK_IMAGE_TYPE_2D,VK_IMAGE_TILING_OPTIMAL,usage,0,&ip);
        printf("BC_FORMAT format=%u features=0x%x image_result=%d\n",format,fp.optimalTilingFeatures,supported);
        if(supported==VK_ERROR_FORMAT_NOT_SUPPORTED||!(fp.optimalTilingFeatures&VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT))continue;
        if(supported!=VK_SUCCESS)return fail("BC image properties",supported);available++;
        if(linear&&!(fp.optimalTilingFeatures&VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))return fail("BC5 linear not supported",VK_ERROR_FORMAT_NOT_SUPPORTED);
        for(unsigned phase=0;phase<(linear?3u:1u);phase++){
        unsigned bytes=(f<4||f==8||f==9)?8:16;uint8_t block[16]={0};float expected[4]={0,0,0,1};
        if(f<8){unsigned offset=f<4?0:8;block[offset+1]=248;expected[0]=1;
            if(f==4||f==5)memset(block,255,8);if(f==6||f==7)block[0]=255;
        }else if(f<12){block[0]=(f&1)?127:255;expected[0]=1;if(f>=10){block[8]=block[0];expected[1]=1;}}
        else if(f>=14){block[0]=64;expected[3]=0;}
        if(linear)for(unsigned c=0;c<2;c++){
            block[8*c]=(f&1)?(phase==0?127:129):(phase==0?255:0);
            block[8*c+1]=(f&1)?(phase==0?129:127):(phase==0?0:255);
            uint64_t indices=0;for(unsigned i=0;i<16;i++)indices|=(uint64_t)((i+3*c+phase)%8)<<(3*i);
            for(unsigned i=0;i<6;i++)block[8*c+2+i]=(uint8_t)(indices>>(8*i));
        }
        VkImage image;VkImageCreateInfo ci={.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,.imageType=VK_IMAGE_TYPE_2D,.format=format,.extent={4,4,1},.mipLevels=1,.arrayLayers=1,.samples=VK_SAMPLE_COUNT_1_BIT,.tiling=VK_IMAGE_TILING_OPTIMAL,.usage=usage};OK(vkCreateImage(device,&ci,NULL,&image));
        VkMemoryRequirements req;vkGetImageMemoryRequirements(device,image,&req);uint32_t type=memory_type(req.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);if(type==UINT32_MAX)return fail("BC image memory",-1);
        VkMemoryAllocateInfo ai={.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,.allocationSize=req.size,.memoryTypeIndex=type};VkDeviceMemory imageMemory;OK(vkAllocateMemory(device,&ai,NULL,&imageMemory));OK(vkBindImageMemory(device,image,imageMemory,0));
        VkImageView view;VkImageViewCreateInfo vi={.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,.image=image,.viewType=VK_IMAGE_VIEW_TYPE_2D,.format=format,.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1}};OK(vkCreateImageView(device,&vi,NULL,&view));
        VkBuffer buffers[2];VkDeviceMemory memory[2];void* maps[2];
        for(unsigned j=0;j<2;j++){VkBufferCreateInfo bi={.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,.size=j?256:16,.usage=j?VK_BUFFER_USAGE_STORAGE_BUFFER_BIT:VK_BUFFER_USAGE_TRANSFER_SRC_BIT};OK(vkCreateBuffer(device,&bi,NULL,&buffers[j]));vkGetBufferMemoryRequirements(device,buffers[j],&req);type=memory_type(req.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);if(type==UINT32_MAX)return fail("BC buffer memory",-1);ai.allocationSize=req.size;ai.memoryTypeIndex=type;OK(vkAllocateMemory(device,&ai,NULL,&memory[j]));OK(vkBindBufferMemory(device,buffers[j],memory[j],0));OK(vkMapMemory(device,memory[j],0,VK_WHOLE_SIZE,0,&maps[j]));memset(maps[j],0xCC,j?256:16);}
        memcpy(maps[0],block,bytes);VkMappedMemoryRange ranges[2]={{.sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,.memory=memory[0],.size=VK_WHOLE_SIZE},{.sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,.memory=memory[1],.size=VK_WHOLE_SIZE}};OK(FlushMappedMemoryRanges(device,2,ranges));
        VkSampler sampler;VkSamplerCreateInfo si={.sType=VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,.magFilter=linear?VK_FILTER_LINEAR:VK_FILTER_NEAREST,.minFilter=linear?VK_FILTER_LINEAR:VK_FILTER_NEAREST,.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST,.addressModeU=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,.addressModeV=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE};OK(CreateSampler(device,&si,NULL,&sampler));
        VkDescriptorSetLayoutBinding bindings[]={{0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_COMPUTE_BIT,NULL},{1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,NULL}};
        VkDescriptorSetLayoutCreateInfo dl={.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,.bindingCount=2,.pBindings=bindings};VkDescriptorSetLayout setLayout;OK(CreateDescriptorSetLayout(device,&dl,NULL,&setLayout));
        VkDescriptorPoolSize sizes[]={{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1},{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1}};VkDescriptorPoolCreateInfo dpi={.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,.maxSets=1,.poolSizeCount=2,.pPoolSizes=sizes};VkDescriptorPool descriptors;OK(CreateDescriptorPool(device,&dpi,NULL,&descriptors));
        VkDescriptorSetAllocateInfo da={.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,.descriptorPool=descriptors,.descriptorSetCount=1,.pSetLayouts=&setLayout};VkDescriptorSet set;OK(AllocateDescriptorSets(device,&da,&set));
        VkDescriptorImageInfo ii={sampler,view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};VkDescriptorBufferInfo bo={buffers[1],0,256};VkWriteDescriptorSet writes[]={{.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,.dstSet=set,.dstBinding=0,.descriptorCount=1,.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,.pImageInfo=&ii},{.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,.dstSet=set,.dstBinding=1,.descriptorCount=1,.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,.pBufferInfo=&bo}};UpdateDescriptorSets(device,2,writes,0,NULL);
        size_t codeBytes=0;uint32_t* code=read_shader(shader,&codeBytes);if(!code)return fail("BC compute shader",-1);VkShaderModuleCreateInfo sm={.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,.codeSize=codeBytes,.pCode=code};VkShaderModule module;OK(vkCreateShaderModule(device,&sm,NULL,&module));free(code);
        VkPipelineLayoutCreateInfo pl={.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,.setLayoutCount=1,.pSetLayouts=&setLayout};VkPipelineLayout layout;OK(vkCreatePipelineLayout(device,&pl,NULL,&layout));VkComputePipelineCreateInfo pc={.sType=VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,.stage={.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,.stage=VK_SHADER_STAGE_COMPUTE_BIT,.module=module,.pName="main"},.layout=layout};VkPipeline pipeline;OK(CreateComputePipelines(device,VK_NULL_HANDLE,1,&pc,NULL,&pipeline));
        VkCommandPoolCreateInfo poolInfo={.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,.queueFamilyIndex=family};VkCommandPool pool;OK(vkCreateCommandPool(device,&poolInfo,NULL,&pool));VkCommandBufferAllocateInfo ca={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,.commandPool=pool,.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,.commandBufferCount=1};VkCommandBuffer command;OK(vkAllocateCommandBuffers(device,&ca,&command));VkCommandBufferBeginInfo begin={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};OK(vkBeginCommandBuffer(command,&begin));
        VkImageMemoryBarrier barrier={.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT,.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED,.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.image=image,.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1}};vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,NULL,0,NULL,1,&barrier);
        VkBufferImageCopy copy={.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1},.imageExtent={4,4,1}};CmdCopyBufferToImage(command,buffers[0],image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
        barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,0,NULL,0,NULL,1,&barrier);
        vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline);CmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_COMPUTE,layout,0,1,&set,0,NULL);CmdDispatch(command,1,1,1);
        VkMemoryBarrier host={.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER,.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT,.dstAccessMask=VK_ACCESS_HOST_READ_BIT};vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&host,0,NULL,0,NULL);OK(vkEndCommandBuffer(command));
        VkFence fence;VkFenceCreateInfo fc={.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};OK(vkCreateFence(device,&fc,NULL,&fence));VkSubmitInfo submit={.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,.commandBufferCount=1,.pCommandBuffers=&command};OK(vkQueueSubmit(queue,1,&submit,fence));OK(vkWaitForFences(device,1,&fence,VK_TRUE,5000000000ull));OK(vkInvalidateMappedMemoryRanges(device,1,&ranges[1]));
        float* values=maps[1];for(unsigned p=0;p<16;p++)for(unsigned c=0;c<4;c++){
            float reference=expected[c];
            if(linear){reference=c==3?1.0f:0.0f;if(c<2){unsigned x=p%4,y=p/4,x1=x<3?x+1:3,y1=y<3?y+1:3;reference=(bc5_oracle(y*4+x,c,phase,f&1)+bc5_oracle(y*4+x1,c,phase,f&1)+bc5_oracle(y1*4+x,c,phase,f&1)+bc5_oracle(y1*4+x1,c,phase,f&1))*.25f;}}
            if(!isfinite(values[p*4+c])||fabsf(values[p*4+c]-reference)>(linear?.001f:.005f)){printf("BC_MISMATCH format=%u phase=%u pixel=%u channel=%u actual=%g expected=%g\n",format,phase,p,c,values[p*4+c],reference);return 3;}checked++;
        }
        if(linear)printf("BC_LINEAR_PIXELS_PASS format=%u phase=%u components=64\n",format,phase);
        else printf("BC_CONSTANT_PIXELS_PASS format=%u components=64\n",format);
        vkDestroyFence(device,fence,NULL);vkDestroyCommandPool(device,pool,NULL);vkDestroyPipeline(device,pipeline,NULL);vkDestroyPipelineLayout(device,layout,NULL);vkDestroyShaderModule(device,module,NULL);DestroyDescriptorPool(device,descriptors,NULL);DestroyDescriptorSetLayout(device,setLayout,NULL);DestroySampler(device,sampler,NULL);vkDestroyImageView(device,view,NULL);vkDestroyImage(device,image,NULL);vkFreeMemory(device,imageMemory,NULL);for(unsigned j=0;j<2;j++){vkUnmapMemory(device,memory[j]);vkDestroyBuffer(device,buffers[j],NULL);vkFreeMemory(device,memory[j],NULL);}
        }
    }
    printf("BC_SUMMARY available_formats=%u checked_components=%u textureCompressionBC_not_enabled=1\n",available,checked);
    return available&&checked==available*64*(linear?3:1)?0:2;
}
