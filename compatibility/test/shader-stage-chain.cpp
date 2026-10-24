// SPDX-License-Identifier: Apache-2.0
#include "../../adapters/vulkan-layer/shader_stage_chain.hpp"
#include <cstdio>
#include <cstdint>

static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL line=%d %s\n",__LINE__,#x);return 1;}}while(0)
static bool rejects(const VkPipelineShaderStageCreateInfo& info){
    try{shaderStageWithModule(info,reinterpret_cast<VkShaderModule>(uintptr_t(2)));return false;}
    catch(const std::invalid_argument&){return true;}
}
int main(){
    VkSpecializationInfo specialization{};
    VkPipelineShaderStageCreateInfo stage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stage.module=reinterpret_cast<VkShaderModule>(uintptr_t(1));stage.pName="main";
    stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;stage.flags=VK_PIPELINE_SHADER_STAGE_CREATE_REQUIRE_FULL_SUBGROUPS_BIT;
    stage.pSpecializationInfo=&specialization;
    const auto replacement=reinterpret_cast<VkShaderModule>(uintptr_t(2));
    auto native=shaderStageWithModule(stage,replacement);
    CHECK(native.module==replacement&&native.pNext==nullptr);
    VkPipelineShaderStageRequiredSubgroupSizeCreateInfo subgroup{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO};subgroup.requiredSubgroupSize=16;
    stage.pNext=&subgroup;native=shaderStageWithModule(stage,replacement);
    CHECK(native.pNext==&subgroup&&native.module==replacement);
    CHECK(native.flags==stage.flags&&native.stage==stage.stage);
    CHECK(native.pName==stage.pName&&native.pSpecializationInfo==&specialization);
    CHECK(stage.module!=replacement&&stage.pNext==&subgroup&&subgroup.requiredSubgroupSize==16);
    VkBaseInStructure unknown{VK_STRUCTURE_TYPE_APPLICATION_INFO,nullptr};
    subgroup.pNext=&unknown;CHECK(rejects(stage));subgroup.pNext=nullptr;
    stage.pNext=&unknown;CHECK(rejects(stage));
    VkShaderModuleCreateInfo inlineModule{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    stage.module=VK_NULL_HANDLE;stage.pNext=&inlineModule;
    native=shaderStageWithModule(stage,replacement);CHECK(native.pNext==nullptr&&native.module==replacement);
    inlineModule.pNext=&subgroup;CHECK(rejects(stage));inlineModule.pNext=nullptr;
    stage.pNext=&subgroup;CHECK(rejects(stage));
    stage.module=reinterpret_cast<VkShaderModule>(uintptr_t(1));stage.pNext=&inlineModule;CHECK(rejects(stage));
    std::printf("SHADER_STAGE_CHAIN_PASS checks=%u\n",checks);
}
