// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <mavk/compat/native_descriptors.hpp>
#include <map>
#include <tuple>

namespace mavk::compat {
// Explicit color-1D/2D/3D, 1D/2D array and sampled-cube subset. Not MS/depth/cube-array
// or public nullDescriptor. Legacy class name retained for existing callers.
// Caller supplies a shader-compatible format and records initialization BEFORE
// use on one externally synchronized queue. No hidden submit/queue-idle waits.
class NullImage2DResources {
    struct View {
        VkDevice device{};PFN_vkDestroyImageView destroy{};VkImageView handle{};
        std::shared_ptr<mavk::Image> image;uint32_t layers=1;bool submitted=false,reserved=false;
        ~View(){if(handle)destroy(device,handle,nullptr);}
    };
    struct Lease {std::shared_ptr<View> view;std::shared_ptr<const void> sampler;};
public:
    class Initialization {
        friend class NullImage2DResources;
        std::vector<std::shared_ptr<View>> views_;mavk::VulkanDispatch dispatch_;
        bool recorded_=false,submitted_=false;
    public:
        Initialization()=default;Initialization(const Initialization&)=delete;
        Initialization& operator=(const Initialization&)=delete;
        Initialization(Initialization&& other) noexcept:views_(std::move(other.views_)),dispatch_(other.dispatch_),recorded_(other.recorded_),submitted_(other.submitted_){other.views_.clear();}
        ~Initialization(){if(!submitted_)for(auto& v:views_)v->reserved=false;}
        size_t size() const{return views_.size();}
        VkImage image(size_t i) const{return views_.at(i)->image->handle();}
        uint32_t layers(size_t i) const{return views_.at(i)->layers;}
        // Record once;do not resubmit this initialization command. The caller
        // may retry a rejected queue submission with the same recorded command.
        void record(VkCommandBuffer cmd){
            if(recorded_||!cmd)throw std::logic_error("null image initialization recording state");
            for(auto& v:views_){
                VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};b.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;b.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;b.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.image=v->image->handle();b.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,v->layers};
                dispatch_.vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&b);
                VkClearColorValue zero{};dispatch_.vkCmdClearColorImage(cmd,b.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&zero,1,&b.subresourceRange);
                b.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;b.newLayout=VK_IMAGE_LAYOUT_GENERAL;b.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;b.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;
                dispatch_.vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,0,0,nullptr,0,nullptr,1,&b);
            }
            recorded_=true;
        }
        // Means ordered on the SAME queue,not GPU completion. Only call after
        // SUCCESSFUL submission;keep this batch/command alive until completed.
        void submitted(){if(!recorded_||submitted_)throw std::logic_error("null image initialization submit state");for(auto& v:views_){v->submitted=true;v->reserved=false;}submitted_=true;}
    };
    explicit NullImage2DResources(mavk::DeviceContext& context):context_(context){}
    bool empty() const{return views_.empty();}
    NativeDescriptorResource resolve(const DescriptorBinding& binding,const DescriptorValue& value,VkFormat format,const NativeDescriptorResource& sampler={},VkImageType type=VK_IMAGE_TYPE_2D,VkImageViewType viewType=VK_IMAGE_VIEW_TYPE_MAX_ENUM){
        if(type!=VK_IMAGE_TYPE_1D&&type!=VK_IMAGE_TYPE_2D&&type!=VK_IMAGE_TYPE_3D)throw std::invalid_argument("null image dimension not admitted");
        if(viewType==VK_IMAGE_VIEW_TYPE_MAX_ENUM)viewType=type==VK_IMAGE_TYPE_1D?VK_IMAGE_VIEW_TYPE_1D:type==VK_IMAGE_TYPE_3D?VK_IMAGE_VIEW_TYPE_3D:VK_IMAGE_VIEW_TYPE_2D;
        const bool cube=viewType==VK_IMAGE_VIEW_TYPE_CUBE;
        const bool compatible=type==VK_IMAGE_TYPE_1D?(viewType==VK_IMAGE_VIEW_TYPE_1D||viewType==VK_IMAGE_VIEW_TYPE_1D_ARRAY):type==VK_IMAGE_TYPE_2D?(viewType==VK_IMAGE_VIEW_TYPE_2D||viewType==VK_IMAGE_VIEW_TYPE_2D_ARRAY||cube):viewType==VK_IMAGE_VIEW_TYPE_3D;
        if(!compatible)throw std::invalid_argument("null image/view dimension mismatch");
        if(value.status!=DescriptorStatus::Null||(binding.kind!=DescriptorKind::SampledImage&&binding.kind!=DescriptorKind::CombinedImageSampler&&binding.kind!=DescriptorKind::StorageImage))throw std::invalid_argument("logical-null color image required");
        if(cube&&binding.kind==DescriptorKind::StorageImage)throw std::invalid_argument("null storage cube not admitted");
        if(binding.kind==DescriptorKind::CombinedImageSampler&&(!sampler.owner||!sampler.image.sampler))throw std::invalid_argument("null combined image requires real sampler lease");
        if(format!=VK_FORMAT_R8G8B8A8_UNORM&&format!=VK_FORMAT_R32_UINT&&format!=VK_FORMAT_R32_SINT&&format!=VK_FORMAT_R32_SFLOAT&&format!=VK_FORMAT_R32G32B32A32_UINT&&format!=VK_FORMAT_R32G32B32A32_SINT&&format!=VK_FORMAT_R32G32B32A32_SFLOAT)throw std::invalid_argument("null color format not admitted");
        auto key=std::make_tuple(type,viewType,format);auto at=views_.find(key);
        if(at==views_.end()){
            const VkImageUsageFlags usage=VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|(cube?0:VK_IMAGE_USAGE_STORAGE_BIT);
            const VkImageCreateFlags flags=cube?VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT:0;
            VkImageFormatProperties properties{};descriptor_detail::check(context_.dispatch().vkGetPhysicalDeviceImageFormatProperties(context_.physicalDevice(),format,type,VK_IMAGE_TILING_OPTIMAL,usage,flags,&properties),"null color native format support");
            auto v=std::make_shared<View>();v->device=context_.vkDevice();v->destroy=context_.dispatch().vkDestroyImageView;v->layers=cube?6:1;
            VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};ci.flags=flags;ci.imageType=type;ci.format=format;ci.extent={1,1,1};ci.mipLevels=1;ci.arrayLayers=v->layers;ci.samples=VK_SAMPLE_COUNT_1_BIT;ci.tiling=VK_IMAGE_TILING_OPTIMAL;ci.usage=usage;
            v->image=std::make_shared<mavk::Image>(context_,ci);VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};vi.image=v->image->handle();vi.viewType=viewType;vi.format=format;vi.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,v->layers};
            descriptor_detail::check(context_.dispatch().vkCreateImageView(v->device,&vi,nullptr,&v->handle),"null color view");at=views_.emplace(key,std::move(v)).first;
        }
        auto lease=std::make_shared<Lease>();lease->view=at->second;lease->sampler=sampler.owner;
        NativeDescriptorResource out;out.owner=std::move(lease);out.image={binding.kind==DescriptorKind::CombinedImageSampler?sampler.image.sampler:VK_NULL_HANDLE,at->second->handle,VK_IMAGE_LAYOUT_GENERAL};return out;
    }
    Initialization pending() const{Initialization out;out.dispatch_=context_.dispatch();for(auto& [format,v]:views_){if(v->reserved)throw std::logic_error("null image initialization already reserved");if(!v->submitted)out.views_.push_back(v);}for(auto& v:out.views_)v->reserved=true;return out;}
private:
    mavk::DeviceContext& context_;std::map<std::tuple<VkImageType,VkImageViewType,VkFormat>,std::shared_ptr<View>> views_;
};
}
