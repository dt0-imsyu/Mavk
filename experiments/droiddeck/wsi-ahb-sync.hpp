#pragma once
#include <unistd.h>
#include <utility>
#include <algorithm>
#include <util/helpers.hpp>
#include <util/file_descriptor.hpp>
#include "ahb-blit.h"

class mavk_ahb_sync {
public:
   explicit mavk_ahb_sync(layer::device_private_data &data) : d(data) {}
   ~mavk_ahb_sync()
   {
      if (returned >= 0) close(returned);
      if (outgoing >= 0) close(outgoing);
      if (pool) d.disp.DestroyCommandPool(d.device, pool, nullptr);
      for (auto semaphore : semaphores) if (semaphore) d.disp.DestroySemaphore(d.device, semaphore, nullptr);
      for (auto fence : fences) if (fence) d.disp.DestroyFence(d.device, fence, nullptr);
   }

   VkResult init(VkImage value, VkImage conversion_source=VK_NULL_HANDLE, VkExtent2D conversion_extent={})
   {
      image = value;
      source_image=conversion_source; extent=conversion_extent;
      auto gdpa = d.disp.get_fn<PFN_vkGetDeviceProcAddr>("vkGetDeviceProcAddr");
      if (!gdpa) return VK_ERROR_INITIALIZATION_FAILED;
      barrier = reinterpret_cast<PFN_vkCmdPipelineBarrier>((*gdpa)(d.device, "vkCmdPipelineBarrier"));
      if (!barrier) return VK_ERROR_INITIALIZATION_FAILED;
      if (source_image)
      {
         blit=reinterpret_cast<PFN_vkCmdBlitImage>((*gdpa)(d.device,"vkCmdBlitImage"));
         if (!blit || !extent.width || !extent.height || extent.width>INT32_MAX || extent.height>INT32_MAX)
            return VK_ERROR_FEATURE_NOT_PRESENT;
      }
      family = d.get_best_queue_family_index();
      d.disp.GetDeviceQueue(d.device, family, 0u, &queue);
      if (!queue) return VK_ERROR_INITIALIZATION_FAILED;
      TRY(d.SetDeviceLoaderData(d.device, queue));
      VkCommandPoolCreateInfo pi{};
      pi.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      pi.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
      pi.queueFamilyIndex=family;
      TRY(d.disp.CreateCommandPool(d.device, &pi, nullptr, &pool));
      VkCommandBufferAllocateInfo ai{};
      ai.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      ai.commandPool=pool; ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY; ai.commandBufferCount=2;
      TRY(d.disp.AllocateCommandBuffers(d.device, &ai, commands));
      for (auto command : commands) TRY(d.SetDeviceLoaderData(d.device, command));
      for (auto &fence : fences)
      {
         VkFenceCreateInfo info{}; info.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
         TRY(d.disp.CreateFence(d.device, &info, nullptr, &fence));
      }
      for (unsigned i=0; i<3; ++i)
      {
         VkExportSemaphoreCreateInfo export_info{};
         export_info.sType=VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
         export_info.handleTypes=VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT;
         VkSemaphoreCreateInfo info{}; info.sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
         if (i) info.pNext=&export_info;
         TRY(d.disp.CreateSemaphore(d.device, &info, nullptr, &semaphores[i]));
      }
      return VK_SUCCESS;
   }

   VkResult acquire(VkSemaphore application_semaphore, VkFence application_fence, uint64_t timeout)
   {
      if (acquired || (!first && returned < -1)) return VK_ERROR_INITIALIZATION_FAILED;
      if (release_pending) TRY(d.disp.WaitForFences(d.device, 1u, &fences[1], VK_TRUE, timeout));
      else if (acquire_pending) TRY(d.disp.WaitForFences(d.device, 1u, &fences[0], VK_TRUE, timeout));
      release_pending=acquire_pending=false;
      const bool has_wait=returned >= -1;
      if (has_wait)
      {
         VkImportSemaphoreFdInfoKHR info{};
         info.sType=VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_FD_INFO_KHR;
         info.semaphore=semaphores[0]; info.flags=VK_SEMAPHORE_IMPORT_TEMPORARY_BIT;
         info.handleType=VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT; info.fd=returned;
         TRY(d.disp.ImportSemaphoreFdKHR(d.device, &info)); returned=-2;
      }
      TRY(record(0, first ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_GENERAL,
                 VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, true));
      TRY(d.disp.ResetFences(d.device, 1u, &fences[0]));
      VkPipelineStageFlags stage=VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
      VkSubmitInfo submit{}; submit.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submit.waitSemaphoreCount=has_wait ? 1u : 0u;
      submit.pWaitSemaphores=has_wait ? &semaphores[0] : nullptr;
      submit.pWaitDstStageMask=has_wait ? &stage : nullptr;
      submit.commandBufferCount=1; submit.pCommandBuffers=&commands[0];
      submit.signalSemaphoreCount=1; submit.pSignalSemaphores=&semaphores[1];
      TRY(d.disp.QueueSubmit(queue, 1u, &submit, fences[0])); acquire_pending=true;
      int fd=-2; TRY(export_fd(semaphores[1], &fd));
      util::fd_owner dependency(fd);
      if (application_semaphore)
      {
         int copy=fd >= 0 ? dup(fd) : fd;
         if (fd >= 0 && copy < 0) return VK_ERROR_OUT_OF_HOST_MEMORY;
         util::fd_owner copy_owner(copy);
         VkImportSemaphoreFdInfoKHR info{};
         info.sType=VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_FD_INFO_KHR;
         info.semaphore=application_semaphore; info.flags=VK_SEMAPHORE_IMPORT_TEMPORARY_BIT;
         info.handleType=VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT; info.fd=copy;
         TRY(d.disp.ImportSemaphoreFdKHR(d.device, &info)); copy_owner.release();
      }
      if (application_fence)
      {
         VkImportFenceFdInfoKHR info{}; info.sType=VK_STRUCTURE_TYPE_IMPORT_FENCE_FD_INFO_KHR;
         info.fence=application_fence; info.flags=VK_FENCE_IMPORT_TEMPORARY_BIT;
         info.handleType=VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT; info.fd=fd;
         TRY(d.disp.ImportFenceFdKHR(d.device, &info)); dependency.release();
      }
      acquired=true; first=false;
      return VK_SUCCESS;
   }

   VkResult present(VkQueue submitted_queue, const wsi::queue_submit_semaphores &sync, const void *next)
   {
      if (!acquired || submitted_queue != queue || next || outgoing >= -1)
         return VK_ERROR_FEATURE_NOT_PRESENT;
      TRY(record(1, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_GENERAL, false));
      TRY(d.disp.ResetFences(d.device, 1u, &fences[1]));
      util::vector<VkPipelineStageFlags> stages(d.get_allocator());
      util::vector<VkSemaphore> signals(d.get_allocator());
      if (!stages.try_resize(sync.wait_semaphores_count) || !signals.try_resize(sync.signal_semaphores_count+1))
         return VK_ERROR_OUT_OF_HOST_MEMORY;
      std::fill(stages.begin(), stages.end(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
      for (uint32_t i=0; i<sync.signal_semaphores_count; ++i) signals[i]=sync.signal_semaphores[i];
      signals[sync.signal_semaphores_count]=semaphores[2];
      VkSubmitInfo submit{}; submit.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submit.waitSemaphoreCount=sync.wait_semaphores_count; submit.pWaitSemaphores=sync.wait_semaphores;
      submit.pWaitDstStageMask=stages.data(); submit.commandBufferCount=1; submit.pCommandBuffers=&commands[1];
      submit.signalSemaphoreCount=sync.signal_semaphores_count+1; submit.pSignalSemaphores=signals.data();
      TRY(d.disp.QueueSubmit(queue, 1u, &submit, fences[1])); release_pending=true;
      TRY(export_fd(semaphores[2], &outgoing)); acquired=false;
      return VK_SUCCESS;
   }
   bool return_dependency(int fd)
   {
      if (returned >= -1 || fd < -1) { if (fd>=0) close(fd); return false; }
      returned=fd; return true;
   }
   int take_present_fd() { return std::exchange(outgoing, -2); }

private:
   VkResult export_fd(VkSemaphore semaphore, int *fd)
   {
      VkSemaphoreGetFdInfoKHR info{}; info.sType=VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR;
      info.semaphore=semaphore; info.handleType=VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT;
      return d.disp.GetSemaphoreFdKHR(d.device, &info, fd);
   }
   VkResult record(unsigned index, VkImageLayout old_layout, VkImageLayout new_layout, bool incoming)
   {
      TRY(d.disp.ResetCommandBuffer(commands[index], 0u));
      VkCommandBufferBeginInfo begin{}; begin.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
      TRY(d.disp.BeginCommandBuffer(commands[index], &begin));
      VkImageMemoryBarrier source_return{};
      if (!incoming && source_image)
      {
         VkImageMemoryBarrier prepare[2]{};
         for (auto &entry : prepare)
         {
            entry.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            entry.srcAccessMask=VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT;
            entry.oldLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            entry.srcQueueFamilyIndex=entry.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
            entry.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
         }
         prepare[0].image=source_image; prepare[0].newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
         prepare[0].dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
         prepare[1].image=image; prepare[1].newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
         prepare[1].dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
         barrier(commands[index],VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,
            0u,0u,nullptr,0u,nullptr,2u,prepare);
         mavk_record_bgra_rgba_blit(blit,commands[index],source_image,image,extent);
         source_return=prepare[0];
         source_return.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
         source_return.newLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
         source_return.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT; source_return.dstAccessMask=0;
      }
      VkImageMemoryBarrier change{}; change.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      change.srcAccessMask=incoming ? 0u : VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT;
      change.dstAccessMask=incoming ? VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT : 0u;
      change.oldLayout=old_layout; change.newLayout=new_layout;
      change.srcQueueFamilyIndex=incoming ? VK_QUEUE_FAMILY_FOREIGN_EXT : family;
      change.dstQueueFamilyIndex=incoming ? family : VK_QUEUE_FAMILY_FOREIGN_EXT;
      change.image=image; change.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
      if (!incoming && source_image)
      {
         change.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
         change.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
         VkImageMemoryBarrier release[2]{source_return,change};
         barrier(commands[index],VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0u,0u,nullptr,0u,nullptr,2u,release);
         return d.disp.EndCommandBuffer(commands[index]);
      }
      barrier(commands[index], incoming ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
         incoming ? VK_PIPELINE_STAGE_ALL_COMMANDS_BIT : VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
         0u, 0u, nullptr, 0u, nullptr, 1u, &change);
      return d.disp.EndCommandBuffer(commands[index]);
   }
   layer::device_private_data &d;
   VkImage image{};
   VkImage source_image{};
   VkExtent2D extent{};
   PFN_vkCmdBlitImage blit{};
   VkQueue queue{};
   uint32_t family{};
   VkCommandPool pool{};
   VkCommandBuffer commands[2]{};
   VkFence fences[2]{};
   VkSemaphore semaphores[3]{};
   PFN_vkCmdPipelineBarrier barrier{};
   int returned=-2, outgoing=-2;
   bool first=true, acquired=false, acquire_pending=false, release_pending=false;
};
