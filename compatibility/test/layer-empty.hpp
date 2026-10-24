void emptyLayoutsDraw(Root& root,mavk::DeviceContext& context,const char* vertexPath,const char* fragmentPath,bool unusedNonempty=false){
    struct EmptyObjects {
        VkDevice device;VkDescriptorPool pool{};
        ~EmptyObjects(){injectEmptyLayouts=false;if(pool)vkDestroyDescriptorPool(device,pool,nullptr);for(auto& layout:emptyLayouts){if(layout)vkDestroyDescriptorSetLayout(device,layout,nullptr);layout=VK_NULL_HANDLE;}}
    } objects{root.device};
    VkDescriptorSetLayoutBinding unusedBinding{5,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,1,VK_SHADER_STAGE_ALL,nullptr};
    VkDescriptorSetLayoutCreateInfo ci{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};if(unusedNonempty){ci.bindingCount=1;ci.pBindings=&unusedBinding;}ok(vkCreateDescriptorSetLayout(root.device,&ci,nullptr,&emptyLayouts[0]));
    VkDescriptorSetLayoutBinding reserved{17,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,unusedNonempty?2u:0u,unusedNonempty?VK_SHADER_STAGE_ALL:0u,nullptr};ci.bindingCount=1;ci.pBindings=&reserved;ok(vkCreateDescriptorSetLayout(root.device,&ci,nullptr,&emptyLayouts[1]));
    ci.bindingCount=unusedNonempty?1:0;ci.pBindings=unusedNonempty?&unusedBinding:nullptr;ok(vkCreateDescriptorSetLayout(root.device,&ci,nullptr,&emptyLayouts[2]));
    VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,1};VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=1;if(unusedNonempty){pool.poolSizeCount=1;pool.pPoolSizes=&size;}ok(vkCreateDescriptorPool(root.device,&pool,nullptr,&objects.pool));
    VkDescriptorSet unused{};VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};allocate.descriptorPool=objects.pool;allocate.descriptorSetCount=1;allocate.pSetLayouts=emptyLayouts;ok(vkAllocateDescriptorSets(root.device,&allocate,&unused));
    const auto vs=words(vertexPath),fs=words(fragmentPath);const float push[]={0,0,0,0},background[]={0,0,0,1};
    const auto reference=mavk::bridge::drawOffscreen(context,root.queue,16,16,vs,fs,push,background,nullptr,16,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT);
    unsigned red=0,black=0;for(size_t i=0;i<reference.size();i+=4){if(reference[i]==255&&reference[i+1]==0&&reference[i+2]==0&&reference[i+3]==255)red++;else if(reference[i]==0&&reference[i+1]==0&&reference[i+2]==0&&reference[i+3]==255)black++;else throw std::runtime_error("descriptorless triangle oracle");}
    if(reference.size()!=1024||!red||!black)throw std::runtime_error("descriptorless reference coverage");
    const auto before=emptyLayoutInjections;injectEmptyLayouts=true;const auto empty=mavk::bridge::drawOffscreen(context,root.queue,16,16,vs,fs,push,background,nullptr,16,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT);injectEmptyLayouts=false;
    if(emptyLayoutInjections!=before+1)throw std::runtime_error("all-empty native layout injection did not execute");
    if(empty!=reference)throw std::runtime_error("all-empty layouts/unbound sets changed render");
    std::cout<<(unusedNonempty?"UNUSED_LAYOUT_GPU_PASS mixedNonemptySlots=1 allUnusedNonemptySlots=3 unboundUnwrittenSet=1":"EMPTY_LAYOUT_GPU_PASS mixedEmptySlots=1 allEmptySlots=3 unboundEmptySet=1 zeroCountReserved=1")<<" comparedComponents=1024 redPixels="<<red<<" blackPixels="<<black<<'\n';
}
