// SPDX-License-Identifier: Apache-2.0
inline bool gpuSubmitEligible(uint32_t count,const VkSubmitInfo* submits,bool prelude)noexcept{
    return !prelude&&count==1&&submits&&submits[0].commandBufferCount&&
        submits[0].commandBufferCount<=~uint32_t(0)-2&&submits[0].pCommandBuffers&&
        !submits[0].waitSemaphoreCount&&!submits[0].pNext;
}
inline bool gpuSubmitEligible(uint32_t count,const VkSubmitInfo2* submits,bool prelude)noexcept{
    if(prelude||count!=1||!submits||!submits[0].commandBufferInfoCount||
       submits[0].commandBufferInfoCount>~uint32_t(0)-2||!submits[0].pCommandBufferInfos||
       submits[0].waitSemaphoreInfoCount||submits[0].pNext||submits[0].flags)return false;
    for(uint32_t i=0;i<submits[0].commandBufferInfoCount;i++)
        if(submits[0].pCommandBufferInfos[i].pNext||submits[0].pCommandBufferInfos[i].deviceMask>1)return false;
    return true;
}
inline bool gpuDecorateSubmit(const VkSubmitInfo& original,const GpuSample& sample,
    std::vector<VkCommandBuffer>& commands,VkSubmitInfo& measured)noexcept{
    try{
        commands.reserve(size_t(original.commandBufferCount)+2);
        commands.push_back(sample.marks[0]);
        commands.insert(commands.end(),original.pCommandBuffers,original.pCommandBuffers+original.commandBufferCount);
        commands.push_back(sample.marks[1]);
        measured=original;measured.commandBufferCount=uint32_t(commands.size());measured.pCommandBuffers=commands.data();
        return true;
    }catch(...){
        std::fprintf(stderr,"MAVK_GPU_PROBE_ERROR operation=decorate-submit1 originalSubmission=preserved\n");return false;
    }
}
inline bool gpuDecorateSubmit(const VkSubmitInfo2& original,const GpuSample& sample,
    std::vector<VkCommandBufferSubmitInfo>& commands,VkSubmitInfo2& measured)noexcept{
    try{
        commands.reserve(size_t(original.commandBufferInfoCount)+2);
        VkCommandBufferSubmitInfo mark{VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};mark.deviceMask=1;
        mark.commandBuffer=sample.marks[0];commands.push_back(mark);
        commands.insert(commands.end(),original.pCommandBufferInfos,original.pCommandBufferInfos+original.commandBufferInfoCount);
        mark.commandBuffer=sample.marks[1];commands.push_back(mark);
        measured=original;measured.commandBufferInfoCount=uint32_t(commands.size());measured.pCommandBufferInfos=commands.data();
        return true;
    }catch(...){
        std::fprintf(stderr,"MAVK_GPU_PROBE_ERROR operation=decorate-submit2 originalSubmission=preserved\n");return false;
    }
}
