#include <mavk/compat/vulkan_descriptor_updates.hpp>
#include <iostream>
using namespace mavk::compat;
unsigned checks=0,negatives=0;
void check(bool yes){checks++;if(!yes)throw std::runtime_error("texel descriptor assertion");}
template<class F>void reject(F f){try{f();}catch(const std::exception&){negatives++;return;}throw std::runtime_error("invalid texel descriptor accepted");}
template<class T>T handle(uintptr_t n){if constexpr(std::is_pointer_v<T>)return reinterpret_cast<T>(n);else return T(n);}
int main(){try{
    using K=DescriptorKind;using A=GuardMetadataAbi;auto owner=std::make_shared<int>(1);VulkanDescriptorResources resources;auto buffer=handle<VkBuffer>(1),uniformOnly=handle<VkBuffer>(2);auto view=handle<VkBufferView>(3),other=handle<VkBufferView>(4);
    for(uint32_t components=1;components<=4;components++)check(1u-(components>>2)==uint32_t(components<4));
    resources.registerBuffer(buffer,1024,VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT|VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,owner);resources.registerBuffer(uniformOnly,64,VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,owner);
    resources.registerBufferView(view,buffer,VK_FORMAT_R32_UINT,16,VK_WHOLE_SIZE,owner);resources.registerBufferView(other,uniformOnly,VK_FORMAT_R32_SFLOAT,0,64,owner);check(resources.texelView(view,K::StorageTexelBuffer)->texelElements==252);
    VkBufferUsageFlags2CreateInfoKHR flags2{VK_STRUCTURE_TYPE_BUFFER_USAGE_FLAGS_2_CREATE_INFO_KHR};flags2.usage=VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};bufferInfo.pNext=&flags2;bufferInfo.usage=VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;check(descriptor_api::bufferUsage(bufferInfo)==flags2.usage);
    VkBufferViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};viewInfo.pNext=&flags2;
    auto restricted=handle<VkBufferView>(20);auto viewUsage=descriptor_api::bufferViewUsage(viewInfo,resources.buffer(buffer)->bufferUsage);
    resources.registerBufferView(restricted,buffer,VK_FORMAT_R32_UINT,0,64,owner,viewUsage);check(resources.texelView(restricted,K::UniformTexelBuffer)->texelElements==16);reject([&]{resources.texelView(restricted,K::StorageTexelBuffer);});
    flags2.usage=VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;reject([&]{descriptor_api::bufferViewUsage(viewInfo,VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT);});
    flags2.usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT;reject([&]{descriptor_api::bufferViewUsage(viewInfo,~uint64_t{0});});
    VkBaseInStructure unknown{VK_STRUCTURE_TYPE_APPLICATION_INFO,nullptr};viewInfo.pNext=&unknown;reject([&]{descriptor_api::bufferViewUsage(viewInfo,~uint64_t{0});});
    resources.unregisterBufferView(restricted);
    auto oddBuffer=handle<VkBuffer>(50);auto wholeView=handle<VkBufferView>(51);
    resources.registerBuffer(oddBuffer,1030,VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,owner);
    resources.registerBufferView(wholeView,oddBuffer,VK_FORMAT_R32G32B32A32_SFLOAT,16,VK_WHOLE_SIZE,owner);
    check(resources.texelView(wholeView,K::UniformTexelBuffer)->texelElements==63);
    reject([&]{resources.registerBufferView(handle<VkBufferView>(52),oddBuffer,VK_FORMAT_R32G32B32A32_SFLOAT,16,1014,owner);});
    resources.unregisterBufferView(wholeView);resources.unregisterBuffer(oddBuffer);
    VkFormat vectors[]={VK_FORMAT_R32_UINT,VK_FORMAT_R32_SINT,VK_FORMAT_R32_SFLOAT,
        VK_FORMAT_R32G32_UINT,VK_FORMAT_R32G32_SINT,VK_FORMAT_R32G32_SFLOAT,
        VK_FORMAT_R32G32B32_UINT,VK_FORMAT_R32G32B32_SINT,VK_FORMAT_R32G32B32_SFLOAT,
        VK_FORMAT_R32G32B32A32_UINT,VK_FORMAT_R32G32B32A32_SINT,VK_FORMAT_R32G32B32A32_SFLOAT};
    for(unsigned i=0;i<12;i++){
        auto vectorView=handle<VkBufferView>(30+i);auto bytes=descriptor_api::nativeTexelBytes(vectors[i]);
        resources.registerBufferView(vectorView,buffer,vectors[i],16,bytes*6,owner);
        check(bytes==4*(1+i/3));check(descriptor_api::nativeTexelComponents(vectors[i])==1+i/3);
        check(resources.texelView(vectorView,K::UniformTexelBuffer)->texelElements==6);
        reject([&]{resources.registerBufferView(handle<VkBufferView>(60+i),buffer,vectors[i],16,bytes*6-1,owner);});
        resources.unregisterBufferView(vectorView);
    }
    VkFormat rgba8[]={VK_FORMAT_R8G8B8A8_UNORM,VK_FORMAT_R8G8B8A8_SNORM,VK_FORMAT_R8G8B8A8_USCALED,VK_FORMAT_R8G8B8A8_SSCALED,VK_FORMAT_R8G8B8A8_UINT,VK_FORMAT_R8G8B8A8_SINT,VK_FORMAT_R8G8B8A8_SRGB};
    resources.registerBuffer(oddBuffer,1030,VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,owner);
    for(unsigned i=0;i<7;i++){
        auto packedView=handle<VkBufferView>(100+i);
        check(descriptor_api::nativeTexelBytes(rgba8[i])==4);check(descriptor_api::nativeTexelComponents(rgba8[i])==4);
        resources.registerBufferView(packedView,oddBuffer,rgba8[i],16,VK_WHOLE_SIZE,owner);
        check(resources.texelView(packedView,K::UniformTexelBuffer)->texelElements==253);
        reject([&]{resources.registerBufferView(handle<VkBufferView>(120+i),oddBuffer,rgba8[i],16,1014,owner);});
        resources.unregisterBufferView(packedView);
    }
    resources.unregisterBuffer(oddBuffer);
    reject([&]{resources.registerBufferView(view,buffer,VK_FORMAT_R32_UINT,0,64,owner);});reject([&]{resources.registerBufferView(handle<VkBufferView>(9),buffer,VK_FORMAT_R16_UINT,0,64,owner);});reject([&]{resources.registerBufferView(handle<VkBufferView>(9),buffer,VK_FORMAT_R32_UINT,1,64,owner);});reject([&]{resources.registerBufferView(handle<VkBufferView>(9),buffer,VK_FORMAT_R32_UINT,0,63,owner);});reject([&]{resources.registerBufferView(handle<VkBufferView>(9),buffer,VK_FORMAT_R32_UINT,1024,4,owner);});reject([&]{resources.texelView(other,K::StorageTexelBuffer);});
    VulkanDescriptorUpdates updates(resources,{},true);auto one=handle<VkDescriptorSet>(10),two=handle<VkDescriptorSet>(11);
    VkDescriptorSetLayoutBinding bindings[]={{0,VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,2,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{1,VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}};VkDescriptorSetLayoutCreateInfo layout{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};layout.bindingCount=2;layout.pBindings=bindings;updates.registerSet(one,layout);updates.registerSet(two,layout);
    VkBufferView sampled[]={view,VK_NULL_HANDLE};VkWriteDescriptorSet writes[2]{};for(auto& w:writes){w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w.dstSet=one;}writes[0].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;writes[0].descriptorCount=2;writes[0].pTexelBufferView=sampled;writes[1].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;writes[1].dstBinding=1;writes[1].descriptorCount=1;writes[1].pTexelBufferView=&view;updates.update(2,writes,0,nullptr);
    auto first=updates.snapshot(one,A::IndexAndInit);check(first->at(0,0,0).range==252);check(first->at(0,0,1).status==DescriptorStatus::Null);auto native=VulkanDescriptorResources::resolve(first->bindings[1],0,first->at(0,1,0));check(native.texelView==view&&native.texelElements==252);
    VkCopyDescriptorSet copies[2]{};for(size_t i=0;i<2;i++){auto& c=copies[i];c.sType=VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;c.srcSet=one;c.dstSet=two;c.srcBinding=uint32_t(i);c.dstBinding=uint32_t(i);c.descriptorCount=i?1:2;}updates.update(0,nullptr,2,copies);check(updates.snapshot(two,A::BufferRanges)->at(0,0,0).range==252);
    auto bound=updates.snapshotBindings({one,VK_NULL_HANDLE,two},A::IndexAndInit);check(bound->at(2,1,0).range==252);check(bound==updates.snapshotBindings({one,VK_NULL_HANDLE,two},A::IndexAndInit));
    auto components=[](const DescriptorSnapshot& snapshot,uint32_t set,uint32_t binding,uint32_t element){const auto& m=snapshot.metadata;auto table=m.at(2*m.at(0)-1+set);auto entries=m.at(table+binding);return m.at(entries+element);};
    auto formats=updates.snapshotBindings({one,VK_NULL_HANDLE,two},A::IndexInitFormats);
    check(formats->abi==A::IndexInitFormats);check(formats!=bound);check(components(*formats,0,0,0)==1);check(components(*formats,0,0,1)==0);check(components(*formats,2,1,0)==1);
    check(formats==updates.snapshotBindings({one,VK_NULL_HANDLE,two},A::IndexInitFormats));
    auto selected=updates.snapshotUsedBindings({one,VK_NULL_HANDLE,two},A::IndexInitFormats,{{2,1}});check(components(*selected,2,1,0)==1);
    auto scratch=handle<VkDescriptorSet>(201);auto rgbaView=handle<VkBufferView>(202);
    resources.registerBufferView(rgbaView,buffer,VK_FORMAT_R32G32B32A32_UINT,16,64,owner);updates.registerSet(scratch,layout);
    VkWriteDescriptorSet staged[]={writes[0],writes[1]};for(auto& w:staged)w.dstSet=scratch;updates.update(2,staged,0,nullptr);
    std::shared_ptr<const DescriptorSnapshot> earlier;
    for(unsigned phase=0;phase<3;phase++){
        VkBufferView replaced[]={phase==1?rgbaView:view,VK_NULL_HANDLE};staged[0].pTexelBufferView=replaced;updates.update(1,staged,0,nullptr);
        auto current=updates.snapshotUsedBindings({scratch},A::IndexInitFormats,{{0,0}});
        check(components(*current,0,0,0)==(phase==1?4u:1u));check(components(*current,0,0,1)==0);
        if(earlier)check(components(*earlier,0,0,0)==1);if(phase==0)earlier=current;
    }
    updates.unregisterSet(scratch);resources.unregisterBufferView(rgbaView);
    auto bad=writes[1];bad.pTexelBufferView=&other;reject([&]{updates.update(1,&bad,0,nullptr);});check(first==updates.snapshot(one,A::IndexAndInit));bad=writes[0];bad.pTexelBufferView=nullptr;reject([&]{updates.update(1,&bad,0,nullptr);});
    VkDescriptorUpdateTemplateEntry entries[]={{0,0,2,VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,1,16},{1,0,1,VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,33,0}};VkDescriptorUpdateTemplateCreateInfo ci{VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO};ci.templateType=VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET;ci.descriptorSetLayout=handle<VkDescriptorSetLayout>(99);ci.descriptorUpdateEntryCount=2;ci.pDescriptorUpdateEntries=entries;auto compiled=updates.compileTemplate(one,ci);check(compiled.requiredBytes()==33+sizeof(VkBufferView));std::vector<unsigned char> data(compiled.requiredBytes());for(size_t i=0;i<2;i++)std::memcpy(data.data()+1+16*i,&sampled[i],sizeof(VkBufferView));std::memcpy(data.data()+33,&view,sizeof(view));updates.updateWithTemplate(one,compiled,data.data(),data.size());check(first==updates.snapshot(one,A::IndexAndInit));reject([&]{updates.updateWithTemplate(one,compiled,data.data(),data.size()-1);});
    VulkanDescriptorUpdates closed(resources);closed.registerSet(one,layout);reject([&]{closed.update(2,writes,0,nullptr);});
    DescriptorState common({{0,0,1,K::UniformTexelBuffer}});reject([&]{common.bindTexelBuffer(0,0,0,owner,0);});reject([&]{common.bindTexelBuffer(0,0,0,owner,uint64_t(UINT32_MAX)+1);});reject([&]{common.bindImage(0,0,0,owner);});common.bindTexelBuffer(0,0,0,owner,7);check(common.snapshot(A::IndexAndInit)->at(0,0,0).range==7);
    reject([&]{common.snapshot(A::IndexInitFormats);});reject([&]{common.bindTexelBuffer(0,0,0,owner,7,5);});
    common.bindTexelBuffer(0,0,0,owner,7,1);auto scalar=common.snapshot(A::IndexInitFormats);auto generation=common.generation();check(components(*scalar,0,0,0)==1);
    common.bindTexelBuffer(0,0,0,owner,7,1);check(common.generation()==generation&&common.snapshot(A::IndexInitFormats)==scalar);
    common.bindTexelBuffer(0,0,0,owner,7,4);auto rgba=common.snapshot(A::IndexInitFormats);check(common.generation()==generation+1&&rgba!=scalar);check(components(*scalar,0,0,0)==1&&components(*rgba,0,0,0)==4);
    resources.unregisterBufferView(view);reject([&]{updates.snapshotBindings({one,two},A::IndexAndInit);});check(first->at(0,0,0).owner!=nullptr);resources.unregisterBuffer(uniformOnly);reject([&]{resources.texelView(other,K::UniformTexelBuffer);});
    std::cout<<"TEXEL_DESCRIPTORS_PASS checks="<<checks<<" negatives="<<negatives<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
