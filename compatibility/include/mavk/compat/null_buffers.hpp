#pragma once
#include <mavk/compat/native_descriptors.hpp>

namespace mavk::compat {
// Real device-owned backing for GUARDED logical-null UBO/SSBO descriptors.
// This is not native nullDescriptor:the native driver only sees valid buffers.
// Required precondition:every shader access uses the matching null metadata.
// DeviceContext outlives the catalog and all returned leases;serialized caller.
class NullBufferResources {
public:
    explicit NullBufferResources(mavk::DeviceContext& context):context_(context){}
    NativeDescriptorResource resolve(const DescriptorBinding& binding,const DescriptorValue& value){
        if(value.status!=DescriptorStatus::Null||
           (binding.kind!=DescriptorKind::UniformBuffer&&binding.kind!=DescriptorKind::StorageBuffer))
            throw std::invalid_argument("null buffer fallback requires logical-null UBO/SSBO");
        if(!buffer_){
            auto candidate=std::make_shared<mavk::Buffer>(context_,16,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT|VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            std::memset(candidate->map(),0,16);candidate->flush();buffer_=std::move(candidate);
        }
        NativeDescriptorResource out;out.owner=buffer_;out.bufferBytes=16;
        out.buffer={buffer_->handle(),0,16};return out;
    }
private:
    mavk::DeviceContext& context_;std::shared_ptr<mavk::Buffer> buffer_;
};
}
