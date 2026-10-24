// SPDX-License-Identifier: Apache-2.0
#include <vulkan/vulkan.h>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <vector>
#include <cstdint>

static bool failAllocation=false;
void* operator new(std::size_t bytes){
    if(failAllocation){failAllocation=false;throw std::bad_alloc();}
    if(void* pointer=std::malloc(bytes?bytes:1))return pointer;
    throw std::bad_alloc();
}
void operator delete(void* pointer)noexcept{std::free(pointer);}
void operator delete(void* pointer,std::size_t)noexcept{std::free(pointer);}
#include "../../adapters/vulkan-layer/gpu_profile.hpp"
#include "../../adapters/vulkan-layer/gpu_profile_submit.hpp"

static unsigned checks=0;
#define CHECK(value) do{++checks;if(!(value)){std::fprintf(stderr,"FAILED line=%d %s\n",__LINE__,#value);return 1;}}while(0)
template<class T>T handle(uintptr_t value){return reinterpret_cast<T>(value);}
int main(){
    GpuSample sample;sample.marks[0]=handle<VkCommandBuffer>(0x10);sample.marks[1]=handle<VkCommandBuffer>(0x20);
    VkCommandBuffer originals[]{handle<VkCommandBuffer>(0x30),handle<VkCommandBuffer>(0x40)};
    VkSemaphore signal=handle<VkSemaphore>(0x50);VkPipelineStageFlags stages=VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkSubmitInfo original{VK_STRUCTURE_TYPE_SUBMIT_INFO};original.commandBufferCount=2;
    original.pCommandBuffers=originals;original.signalSemaphoreCount=1;original.pSignalSemaphores=&signal;
    original.pWaitDstStageMask=&stages;
    CHECK(gpuSubmitEligible(1,&original,false));
    CHECK(!gpuSubmitEligible(1,&original,true));
    CHECK(!gpuSubmitEligible(0,static_cast<const VkSubmitInfo*>(nullptr),false));
    CHECK(!gpuSubmitEligible(2,&original,false));
    auto changed=original;changed.commandBufferCount=0;CHECK(!gpuSubmitEligible(1,&changed,false));
    changed=original;changed.commandBufferCount=~uint32_t(0);CHECK(!gpuSubmitEligible(1,&changed,false));
    changed=original;changed.waitSemaphoreCount=1;CHECK(!gpuSubmitEligible(1,&changed,false));
    changed=original;changed.pNext=&stages;CHECK(!gpuSubmitEligible(1,&changed,false));
    std::vector<VkCommandBuffer> commands;VkSubmitInfo measured{};
    CHECK(gpuDecorateSubmit(original,sample,commands,measured));
    CHECK(measured.commandBufferCount==4&&commands[0]==sample.marks[0]&&commands[3]==sample.marks[1]);
    CHECK(commands[1]==originals[0]&&commands[2]==originals[1]);
    CHECK(measured.signalSemaphoreCount==1&&measured.pSignalSemaphores==&signal&&measured.pWaitDstStageMask==&stages);
    CHECK(original.pCommandBuffers==originals&&original.commandBufferCount==2);
    std::vector<VkCommandBuffer> failed;VkSubmitInfo untouched{};untouched.commandBufferCount=73;
    failAllocation=true;const bool decorated=gpuDecorateSubmit(original,sample,failed,untouched);
    CHECK(!decorated&&!failAllocation&&untouched.commandBufferCount==73);
    const VkSubmitInfo* nativeArgument=decorated?&untouched:&original;
    CHECK(nativeArgument==&original&&nativeArgument->pCommandBuffers==originals);

    VkCommandBufferSubmitInfo infos[2]{{VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO},{VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO}};
    infos[0].commandBuffer=originals[0];infos[0].deviceMask=1;
    infos[1].commandBuffer=originals[1];infos[1].deviceMask=1;
    VkSemaphoreSubmitInfo semaphore{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    semaphore.semaphore=signal;semaphore.value=81;semaphore.stageMask=VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    VkSubmitInfo2 original2{VK_STRUCTURE_TYPE_SUBMIT_INFO_2};original2.commandBufferInfoCount=2;
    original2.pCommandBufferInfos=infos;original2.signalSemaphoreInfoCount=1;original2.pSignalSemaphoreInfos=&semaphore;
    CHECK(gpuSubmitEligible(1,&original2,false));CHECK(!gpuSubmitEligible(1,&original2,true));
    CHECK(!gpuSubmitEligible(0,static_cast<const VkSubmitInfo2*>(nullptr),false));
    CHECK(!gpuSubmitEligible(2,&original2,false));
    auto changed2=original2;changed2.flags=VK_SUBMIT_PROTECTED_BIT;CHECK(!gpuSubmitEligible(1,&changed2,false));
    changed2=original2;changed2.pNext=&stages;CHECK(!gpuSubmitEligible(1,&changed2,false));
    changed2=original2;changed2.waitSemaphoreInfoCount=1;CHECK(!gpuSubmitEligible(1,&changed2,false));
    changed2=original2;changed2.commandBufferInfoCount=~uint32_t(0);CHECK(!gpuSubmitEligible(1,&changed2,false));
    infos[0].deviceMask=2;CHECK(!gpuSubmitEligible(1,&original2,false));infos[0].deviceMask=1;
    infos[1].pNext=&stages;CHECK(!gpuSubmitEligible(1,&original2,false));infos[1].pNext=nullptr;
    std::vector<VkCommandBufferSubmitInfo> commands2;VkSubmitInfo2 measured2{};
    CHECK(gpuDecorateSubmit(original2,sample,commands2,measured2));
    CHECK(measured2.commandBufferInfoCount==4&&commands2[0].commandBuffer==sample.marks[0]&&commands2[3].commandBuffer==sample.marks[1]);
    CHECK(commands2[1].commandBuffer==originals[0]&&commands2[2].commandBuffer==originals[1]&&commands2[1].deviceMask==1);
    CHECK(commands2[0].deviceMask==1&&commands2[3].deviceMask==1&&commands2[0].pNext==nullptr);
    CHECK(measured2.pSignalSemaphoreInfos==&semaphore&&measured2.signalSemaphoreInfoCount==1&&semaphore.value==81);
    CHECK(original2.pCommandBufferInfos==infos&&original2.commandBufferInfoCount==2);
    std::vector<VkCommandBufferSubmitInfo> failed2;VkSubmitInfo2 untouched2{};untouched2.commandBufferInfoCount=91;
    failAllocation=true;const bool decorated2=gpuDecorateSubmit(original2,sample,failed2,untouched2);
    CHECK(!decorated2&&!failAllocation&&untouched2.commandBufferInfoCount==91);
    const VkSubmitInfo2* nativeArgument2=decorated2?&untouched2:&original2;
    CHECK(nativeArgument2==&original2&&nativeArgument2->pSignalSemaphoreInfos==&semaphore);
    std::printf("GPU_PROFILE_SUBMIT_HOST_PASS checks=%u gpuExecution=NOT_TESTED\n",checks);
}
