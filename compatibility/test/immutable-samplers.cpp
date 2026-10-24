#include <mavk/compat/vulkan_descriptor_updates.hpp>
#include <iostream>
using namespace mavk::compat;
unsigned checks=0,negatives=0;
void check(bool pass){++checks;if(!pass)throw std::runtime_error("immutable sampler assertion");}
template<class F>void reject(F run){try{run();}catch(const std::exception&){++negatives;return;}throw std::runtime_error("invalid immutable sampler operation accepted");}
template<class T>T handle(uintptr_t n){if constexpr(std::is_pointer_v<T>)return reinterpret_cast<T>(n);else return T(n);}
int main(){try{
    auto owner=std::make_shared<int>(1);VulkanDescriptorResources resources;auto s0=handle<VkSampler>(1),s1=handle<VkSampler>(2),ycbcr=handle<VkSampler>(3);auto view=handle<VkImageView>(4);
    resources.registerSampler(s0,owner);resources.registerSampler(s1,owner);resources.registerSampler(ycbcr,owner,true);resources.registerImageView(view,VK_IMAGE_USAGE_SAMPLED_BIT,owner);
    VulkanDescriptorUpdates updates(resources,{},true);using A=GuardMetadataAbi;
    auto a=handle<VkDescriptorSet>(10),b=handle<VkDescriptorSet>(11),mutableSet=handle<VkDescriptorSet>(12),samplers=handle<VkDescriptorSet>(13),mutableSamplers=handle<VkDescriptorSet>(14),replacement=handle<VkDescriptorSet>(15),spilled=handle<VkDescriptorSet>(16);
    VkSampler fixed[]={s0,s1};VkDescriptorSetLayoutBinding binding{0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,2,VK_SHADER_STAGE_COMPUTE_BIT,fixed};VkDescriptorSetLayoutCreateInfo layout{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};layout.bindingCount=1;layout.pBindings=&binding;
    updates.registerSet(a,layout);fixed[0]=s1;fixed[1]=s0;updates.registerSet(b,layout);binding.pImmutableSamplers=nullptr;updates.registerSet(mutableSet,layout);
    fixed[0]=s0;fixed[1]=s1;binding.descriptorType=VK_DESCRIPTOR_TYPE_SAMPLER;binding.pImmutableSamplers=fixed;updates.registerSet(samplers,layout);binding.pImmutableSamplers=nullptr;updates.registerSet(mutableSamplers,layout);fixed[0]=fixed[1]=VK_NULL_HANDLE;
    auto initialized=updates.snapshot(samplers,A::IndexAndInit);check(VulkanDescriptorResources::resolve(initialized->bindings[0],0,initialized->at(0,0,0)).image.sampler==s0);check(VulkanDescriptorResources::resolve(initialized->bindings[0],1,initialized->at(0,0,1)).image.sampler==s1);
    reject([&]{updates.snapshot(a,A::IndexAndInit);});
    VkDescriptorImageInfo infos[]={{handle<VkSampler>(999),view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},{VK_NULL_HANDLE,view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
    VkWriteDescriptorSet w{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};w.dstSet=a;w.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;w.descriptorCount=2;w.pImageInfo=infos;updates.update(1,&w,0,nullptr);
    auto snapshot=updates.snapshot(a,A::IndexAndInit);auto native=[&](auto snapshot,uint32_t i){return VulkanDescriptorResources::resolve(snapshot->bindings[0],i,snapshot->at(0,0,i));};
    check(native(snapshot,0).image.sampler==s0&&native(snapshot,1).image.sampler==s1);updates.update(1,&w,0,nullptr);check(updates.snapshot(a,A::IndexAndInit)==snapshot);
    VkCopyDescriptorSet copy{VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET};copy.srcSet=a;copy.dstSet=b;copy.descriptorCount=2;updates.update(0,nullptr,1,&copy);check(native(updates.snapshot(b,A::IndexAndInit),0).image.sampler==s1);check(native(updates.snapshot(b,A::IndexAndInit),1).image.sampler==s0);
    copy.dstSet=mutableSet;updates.update(0,nullptr,1,&copy);check(native(updates.snapshot(mutableSet,A::IndexAndInit),0).image.sampler==s0);
    infos[0].sampler=s1;infos[1].sampler=s0;w.dstSet=mutableSet;updates.update(1,&w,0,nullptr);copy.srcSet=mutableSet;copy.dstSet=a;updates.update(0,nullptr,1,&copy);check(native(updates.snapshot(a,A::IndexAndInit),0).image.sampler==s0);
    copy.srcSet=samplers;copy.dstSet=mutableSamplers;updates.update(0,nullptr,1,&copy);check(native(updates.snapshot(mutableSamplers,A::IndexAndInit),1).image.sampler==s1);copy.dstSet=samplers;reject([&]{updates.update(0,nullptr,1,&copy);});
    auto bad=w;bad.dstSet=samplers;bad.descriptorType=VK_DESCRIPTOR_TYPE_SAMPLER;reject([&]{updates.update(1,&bad,0,nullptr);});
    VkDescriptorUpdateTemplateEntry entry{0,0,2,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,0,sizeof(VkDescriptorImageInfo)};VkDescriptorUpdateTemplateCreateInfo ci{VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO};ci.templateType=VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET;ci.descriptorSetLayout=handle<VkDescriptorSetLayout>(20);ci.descriptorUpdateEntryCount=1;ci.pDescriptorUpdateEntries=&entry;
    auto compiled=updates.compileTemplate(a,ci);entry.descriptorType=VK_DESCRIPTOR_TYPE_SAMPLER;reject([&]{updates.compileTemplate(samplers,ci);});
    updates.unregisterSet(a);fixed[0]=s0;fixed[1]=s1;binding.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;binding.pImmutableSamplers=fixed;updates.registerSet(replacement,layout);fixed[0]=fixed[1]=VK_NULL_HANDLE;infos[0].sampler=infos[1].sampler=handle<VkSampler>(999);updates.updateWithTemplate(replacement,compiled,infos,sizeof(infos));check(native(updates.snapshot(replacement,A::IndexAndInit),0).image.sampler==s0);reject([&]{updates.updateWithTemplate(b,compiled,infos,sizeof(infos));});
    infos[0].imageView=VK_NULL_HANDLE;w.dstSet=replacement;w.descriptorCount=1;updates.update(1,&w,0,nullptr);auto null=updates.snapshot(replacement,A::IndexAndInit);check(null->at(0,0,0).status==DescriptorStatus::Null);auto tuple=std::static_pointer_cast<const VulkanDescriptorResources::Resource>(null->at(0,0,0).owner);check(tuple->sampling->sampler==s0);
    copy.srcSet=replacement;copy.dstSet=b;copy.descriptorCount=1;updates.update(0,nullptr,1,&copy);auto other=updates.snapshot(b,A::IndexAndInit);check(std::static_pointer_cast<const VulkanDescriptorResources::Resource>(other->at(0,0,0).owner)->sampling->sampler==s1);
    VkSampler refs[]={s0,s1};VkDescriptorSetLayoutBinding consecutive[]={{0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_COMPUTE_BIT,refs},{1,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_COMPUTE_BIT,refs+1}};layout.bindingCount=2;layout.pBindings=consecutive;updates.registerSet(spilled,layout);infos[0].imageView=view;w.dstSet=spilled;w.descriptorCount=2;updates.update(1,&w,0,nullptr);auto split=updates.snapshot(spilled,A::IndexAndInit);check(split->at(0,1,0).status==DescriptorStatus::Bound);
    consecutive[1].pImmutableSamplers=nullptr;updates.registerSet(handle<VkDescriptorSet>(17),layout);w.dstSet=handle<VkDescriptorSet>(17);reject([&]{updates.update(1,&w,0,nullptr);});
    consecutive[0].pImmutableSamplers=&ycbcr;reject([&]{updates.registerSet(handle<VkDescriptorSet>(18),layout);});
    resources.unregisterSampler(s0);reject([&]{updates.snapshot(replacement,A::IndexAndInit);});check(initialized->at(0,0,0).owner!=nullptr);
    {
        bool deleted=false;std::weak_ptr<int> weak;VulkanDescriptorResources isolated;
        auto owned=std::shared_ptr<int>(new int(2),[&](int* p){deleted=true;delete p;});weak=owned;isolated.registerSampler(s0,owned);owned.reset();
        VkDescriptorSetLayoutBinding immutable{0,VK_DESCRIPTOR_TYPE_SAMPLER,1,VK_SHADER_STAGE_COMPUTE_BIT,&s0};auto retained=ImmutableSamplerBindings::capture(isolated,1,&immutable);isolated.unregisterSampler(s0);check(!weak.expired()&&!deleted);retained.reset();check(weak.expired()&&deleted);
        immutable.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;immutable.pImmutableSamplers=reinterpret_cast<const VkSampler*>(uintptr_t(1));check(ImmutableSamplerBindings::capture(isolated,1,&immutable)->handles.empty());
    }
    std::cout<<"IMMUTABLE_SAMPLERS_HOST_PASS checks="<<checks<<" negatives="<<negatives<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
