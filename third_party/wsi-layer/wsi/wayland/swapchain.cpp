/*
 * Copyright (c) 2017-2026 Arm Limited.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define VK_USE_PLATFORM_WAYLAND_KHR 1

#include "surface_properties.hpp"
#include <cstdint>
#include <cstring>
#include <cassert>
#include <unistd.h>
#include <cstdlib>
#include <cerrno>
#include <cstdio>
#include <climits>
#include <functional>

#include "swapchain.hpp"
#include "util/drm/drm_utils.hpp"
#include "util/format_modifiers.hpp"
#include "util/helpers.hpp"
#include "util/log.hpp"
#include "util/macros.hpp"
#include "wl_helpers.hpp"

#include <wsi/extensions/external_memory_extension.hpp>
#include <wsi/extensions/image_compression_control.hpp>
#include <wsi/extensions/swapchain_maintenance.hpp>
#include <wsi/extensions/present_id.hpp>
#include <wsi/extensions/mutable_format_extension.hpp>

#include "present_timing_handler.hpp"
#include "present_id_wayland.hpp"
#include "present_wait_wayland.hpp"
#include "wp_presentation_feedback.hpp"
#include "wsi-ahb-memory.hpp"
#include "banner-ahb-v1-client-protocol.h"
#include <sys/socket.h>
#include <sys/time.h>

namespace wsi
{
namespace wayland
{

class wayland_image_data : public swapchain_image_data
{
public:
   wayland_image_data(wayland::wayland_owner<wl_buffer> buffer)
      : m_buffer(std::move(buffer))
   {
      assert(m_buffer != nullptr);
   }

   ~wayland_image_data() override = default;

   wl_buffer *get_buffer()
   {
      return m_buffer.get();
   }

   void remove_proxy()
   {
      wl_proxy_set_queue(reinterpret_cast<wl_proxy *>(m_buffer.get()), nullptr);
   }
   bool ahb=false, busy=false, released=false, returned=false;

private:
   wayland::wayland_owner<wl_buffer> m_buffer;
};

swapchain::swapchain(layer::device_private_data &dev_data, const VkAllocationCallbacks *pAllocator,
                     surface &wsi_surface)
   : swapchain_base(dev_data, pAllocator)
   , m_display(wsi_surface.get_wl_display())
   , m_surface(wsi_surface.get_wl_surface())
   , m_wsi_surface(&wsi_surface)
   , m_buffer_queue(nullptr)
   , m_wsi_allocator(nullptr)
   , m_image_factory(m_allocator, m_device_data)
{
}

swapchain::~swapchain()
{
   if (m_ahb_present)
   {
      wl_surface_attach(m_surface, nullptr, 0, 0);
      wl_surface_commit(m_surface);
      wl_display_flush(m_display);
   }
   teardown();

   if (m_buffer_queue != nullptr)
   {
      if (auto *present_timing_ext = get_swapchain_extension<wsi_ext_present_timing_wayland>())
      {
         present_timing_ext->clear_pending_present_feedback_list();
      }

      if (auto *present_id_ext = get_swapchain_extension<wsi_ext_present_id_wayland>())
      {
         present_id_ext->clear_pending_present_feedback_list();
      }

      for (auto &img : m_swapchain_images)
      {
         auto data = img.get_data<wayland_image_data>();
         if (data != nullptr)
         {
            data->remove_proxy();
         }
      }

      wl_display_roundtrip_queue(m_display, m_buffer_queue);
      if (m_ahb_factory) banner_ahb_v1_destroy(m_ahb_factory);
      wl_event_queue_destroy(m_buffer_queue);
   }
}

VkResult swapchain::add_required_extensions(VkDevice device, const VkSwapchainCreateInfoKHR *swapchain_create_info)
{
   UNUSED(device);

   if (m_device_data.is_present_id_enabled() ||
       (swapchain_create_info->flags & VK_SWAPCHAIN_CREATE_PRESENT_ID_2_BIT_KHR))
   {
      if (!add_swapchain_extension(m_allocator.make_unique<wsi_ext_present_id_wayland>()))
      {
         return VK_ERROR_OUT_OF_HOST_MEMORY;
      }
   }

   if (m_device_data.is_swapchain_maintenance1_enabled())
   {
      if (!add_swapchain_extension(m_allocator.make_unique<wsi_ext_swapchain_maintenance1>(m_allocator)))
      {
         return VK_ERROR_OUT_OF_HOST_MEMORY;
      }
   }

   bool present_wait2;
   constexpr VkSwapchainCreateFlagsKHR present_wait2_mask =
      (VK_SWAPCHAIN_CREATE_PRESENT_ID_2_BIT_KHR | VK_SWAPCHAIN_CREATE_PRESENT_WAIT_2_BIT_KHR);
   present_wait2 = (swapchain_create_info->flags & present_wait2_mask) == present_wait2_mask;

   if (m_device_data.is_present_wait_enabled() || present_wait2)
   {
      if (!add_swapchain_extension(m_allocator.make_unique<wsi_ext_present_wait_wayland>(
             *get_swapchain_extension<wsi_ext_present_id>(true), present_wait2)))
      {
         return VK_ERROR_OUT_OF_HOST_MEMORY;
      }
   }

   if (m_device_data.should_layer_handle_frame_boundary_events())
   {
      if (!add_swapchain_extension(m_allocator.make_unique<wsi_ext_frame_boundary>(m_device_data)))
      {
         return VK_ERROR_OUT_OF_HOST_MEMORY;
      }
   }

   bool swapchain_support_enabled = swapchain_create_info->flags & VK_SWAPCHAIN_CREATE_PRESENT_TIMING_BIT_EXT;
   if (swapchain_support_enabled)
   {
      std::optional<VkTimeDomainKHR> image_first_pixel_out_time_domain;
      if (m_wsi_surface->get_presentation_time_interface() != nullptr)
      {
         switch (m_wsi_surface->clockid())
         {
         case CLOCK_MONOTONIC:
            image_first_pixel_out_time_domain = VK_TIME_DOMAIN_CLOCK_MONOTONIC_KHR;
            break;
         case CLOCK_MONOTONIC_RAW:
            image_first_pixel_out_time_domain = VK_TIME_DOMAIN_CLOCK_MONOTONIC_RAW_KHR;
            break;
         }
      }

      if (!add_swapchain_extension(wsi_ext_present_timing_wayland::create(
             m_device, m_allocator, image_first_pixel_out_time_domain, swapchain_create_info->minImageCount)))
      {
         return VK_ERROR_OUT_OF_HOST_MEMORY;
      }
   }
   return VK_SUCCESS;
}

VkResult swapchain::init_platform(VkDevice device, const VkSwapchainCreateInfoKHR *swapchain_create_info,
                                  bool &use_presentation_thread)
{
   UNUSED(device);
   UNUSED(swapchain_create_info);
   UNUSED(use_presentation_thread);

   if ((m_display == nullptr) || (m_surface == nullptr) || (m_wsi_surface->get_dmabuf_interface() == nullptr))
   {
      return VK_ERROR_INITIALIZATION_FAILED;
   }

   m_buffer_queue = wl_display_create_queue(m_display);
   if (m_buffer_queue == nullptr)
   {
      WSI_LOG_ERROR("Failed to create buffer wl queue.");
      return VK_ERROR_INITIALIZATION_FAILED;
   }

   /*
    * When VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR or
    * VK_PRESENT_MODE_FIFO_LATEST_READY_EXT has been chosen by the application we
    * don't initialize the page flip thread so the present_image function can be
    * called during vkQueuePresent. Only FIFO waits for frame callbacks; the other
    * modes commit every image and the compositor shows the newest at each vblank,
    * so IMMEDIATE behaves like MAILBOX (a compositor never tears without the
    * tearing-control protocol).
    *
    * Without explicit sync, though, the presentation thread is what waits for rendering to finish before a
    * buffer is committed (the image factory's wait_on_present_fence): without it the compositor would be
    * handed buffers the GPU is still rendering, so it is needed in every present mode.
    */
   const bool has_explicit_sync = (m_wsi_surface->get_surface_sync_interface() != nullptr);
   use_presentation_thread =
      !has_explicit_sync ||
      (WAYLAND_FIFO_PRESENTATION_THREAD_ENABLED && (m_present_mode != VK_PRESENT_MODE_FIFO_LATEST_READY_EXT) &&
       (m_present_mode != VK_PRESENT_MODE_MAILBOX_KHR) && (m_present_mode != VK_PRESENT_MODE_IMMEDIATE_KHR));

   /* Keep the alpha channel only when the app asked for a premultiplied-alpha surface; otherwise the
    * OPAQUE emulation in create_wl_buffer drops it (ARGB->XRGB). */
   m_has_alpha = (swapchain_create_info->compositeAlpha == VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR);

   auto present_wait = get_swapchain_extension<wsi_ext_present_wait_wayland>();
   if (present_wait)
   {
      present_wait->set_wayland_dispatcher(m_display, m_buffer_queue);
   }

   auto *present_timing_ext = get_swapchain_extension<wsi_ext_present_timing_wayland>();
   if (present_timing_ext != nullptr)
   {
      present_timing_ext->init(m_display, m_buffer_queue);
   }

   const char *ahb = std::getenv("MAVK_WSI_AHB_ALLOC_ONLY");
   if (ahb && std::strcmp(ahb, "1") == 0)
   {
      const char *present = std::getenv("MAVK_WSI_AHB_PRESENT");
      m_ahb_present = present && std::strcmp(present, "1") == 0;
      std::fprintf(stderr, "MAVK_AHB_PLATFORM mode=%u presentId=%u presentWait=%u maintenance=%u\n",
         (unsigned)m_present_mode, (unsigned)m_device_data.is_present_id_enabled(),
         (unsigned)m_device_data.is_present_wait_enabled(), (unsigned)m_device_data.is_swapchain_maintenance1_enabled());
      if (m_ahb_present)
      {
         if (m_present_mode != VK_PRESENT_MODE_FIFO_KHR || m_device_data.is_present_id_enabled() ||
             m_device_data.is_present_wait_enabled() || m_device_data.is_swapchain_maintenance1_enabled())
            return VK_ERROR_FEATURE_NOT_PRESENT;
         use_presentation_thread=false;
         m_device_data.disp.GetDeviceQueue(device, m_device_data.get_best_queue_family_index(), 0u, &m_queue);
         TRY(m_device_data.SetDeviceLoaderData(device, m_queue));
         auto proxy = make_proxy_with_queue(m_display, m_buffer_queue);
         if (!proxy) return VK_ERROR_OUT_OF_HOST_MEMORY;
         auto registry = wayland_owner<wl_registry>{wl_display_get_registry(proxy.get())};
         static const wl_registry_listener listener{
            [](void *data, wl_registry *reg, uint32_t name, const char *interface, uint32_t version) {
               if (!std::strcmp(interface, "banner_ahb_v1"))
                  static_cast<swapchain *>(data)->bind_ahb(reg, name, version);
            }, nullptr};
         if (!registry || wl_registry_add_listener(registry.get(), &listener, this) < 0 ||
             wl_display_roundtrip_queue(m_display, m_buffer_queue) < 0 ||
             wl_display_roundtrip_queue(m_display, m_buffer_queue) < 0 ||
             !m_ahb_factory || m_ahb_mode != 1) return VK_ERROR_FEATURE_NOT_PRESENT;
      }
      return init_image_factory(*swapchain_create_info);
   }

   auto wsi_allocator = swapchain_wsialloc_allocator::create();
   if (!wsi_allocator.has_value())
   {
      return VK_ERROR_INITIALIZATION_FAILED;
   }

   m_wsi_allocator = m_allocator.make_unique<swapchain_wsialloc_allocator>(std::move(wsi_allocator.value()));
   if (m_wsi_allocator == nullptr)
   {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
   }

   return init_image_factory(*swapchain_create_info);
}

VkResult swapchain::init_image_factory(const VkSwapchainCreateInfoKHR &swapchain_create_info)
{
   const char *ahb = std::getenv("MAVK_WSI_AHB_ALLOC_ONLY");
   m_ahb_allocation_probe = ahb && std::strcmp(ahb, "1") == 0;
   if (m_ahb_allocation_probe)
   {
      std::fprintf(stderr, "MAVK_AHB_REQUEST flags=%u format=%u extent=%ux%u images=%u layers=%u sharing=%u customAllocator=%u\n",
         (unsigned)swapchain_create_info.flags, (unsigned)swapchain_create_info.imageFormat,
         swapchain_create_info.imageExtent.width, swapchain_create_info.imageExtent.height,
         swapchain_create_info.minImageCount, swapchain_create_info.imageArrayLayers,
         (unsigned)swapchain_create_info.imageSharingMode,
         m_allocator.get_original_callbacks() ? 1u : 0u);
      auto *chain = static_cast<const VkBaseInStructure *>(swapchain_create_info.pNext);
      for (unsigned count=0; chain && count<8; chain=chain->pNext, ++count)
         std::fprintf(stderr, "MAVK_AHB_REQUEST_CHAIN type=%u\n", (unsigned)chain->sType);
      if (swapchain_create_info.flags || swapchain_create_info.pNext ||
          (swapchain_create_info.imageFormat != VK_FORMAT_R8G8B8A8_UNORM &&
           !(swapchain_create_info.imageFormat==VK_FORMAT_B8G8R8A8_UNORM && mavk_ahb_bgra_enabled())) ||
          swapchain_create_info.imageArrayLayers != 1 ||
          swapchain_create_info.imageSharingMode != VK_SHARING_MODE_EXCLUSIVE ||
          m_allocator.get_original_callbacks()) return VK_ERROR_FEATURE_NOT_PRESENT;
      auto image_creator = m_allocator.make_unique<vulkan_image_handle_creator>(m_allocator, swapchain_create_info);
      auto memory_creator = m_allocator.make_unique<ahb_image_backing_creator>(m_device_data,
         swapchain_create_info.imageExtent, swapchain_create_info.imageUsage, m_ahb_present, swapchain_create_info.imageFormat);
      if (!image_creator || !memory_creator) return VK_ERROR_OUT_OF_HOST_MEMORY;
      m_image_factory.init(std::move(image_creator), std::move(memory_creator), false, !m_ahb_present, true);
      return VK_SUCCESS;
   }
   std::variant<VkResult, util::unique_ptr<vulkan_image_handle_creator>> image_handle_creator_result =
      create_image_creator(swapchain_create_info);
   if (auto error = std::get_if<VkResult>(&image_handle_creator_result))
   {
      return *error;
   }

   auto image_handle_creator =
      std::get<util::unique_ptr<vulkan_image_handle_creator>>(std::move(image_handle_creator_result));

   auto compression_control = image_create_compression_control::create(m_device, &swapchain_create_info);
   /* An OPAQUE swapchain is presented through the alpha-less variant of its format (see create_wl_buffer),
    * so only pick a modifier the compositor also supports for that variant. */
   const auto &surface_formats =
      m_has_alpha ? m_wsi_surface->get_formats() : m_wsi_surface->get_formats_with_opaque_variant();
   if (surface_formats.empty())
   {
      /* get_surface_compatible_formats() reads an empty list as "any modifier the device supports is
       * fine", so stop here rather than attaching a buffer the compositor never advertised. */
      WSI_LOG_ERROR("The compositor supports no usable format/modifier pair for this swapchain");
      return VK_ERROR_INITIALIZATION_FAILED;
   }
   auto sc_img_create_ext_mem_result = swapchain_image_create_external_memory::create(
      image_handle_creator->get_image_create_info(), compression_control, *m_wsi_allocator, surface_formats,
      m_device_data.physical_device, m_allocator);
   if (auto error = std::get_if<VkResult>(&sc_img_create_ext_mem_result))
   {
      return *error;
   }

   auto sc_img_create_ext_mem =
      std::get<util::unique_ptr<swapchain_image_create_external_memory>>(std::move(sc_img_create_ext_mem_result));

   auto external_image_create_info = sc_img_create_ext_mem->get_external_image_create_info();
   TRY_LOG_CALL(image_handle_creator->add_extension(std::move(sc_img_create_ext_mem)));

   wsialloc_create_info_args wsialloc_args = { external_image_create_info.selected_format,
                                               external_image_create_info.flags, external_image_create_info.extent,
                                               external_image_create_info.explicit_compression };

   auto backing_memory_creator =
      m_allocator.make_unique<external_image_backing_memory_creator>(m_device_data, *m_wsi_allocator, wsialloc_args);
   if (backing_memory_creator == nullptr)
   {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
   }

   /* RK3588: when the compositor lacks explicit sync the surface has no sync interface, so fall back
    * to implicit sync - create a CPU-waitable (non-exportable) present fence and wait on it instead
    * of handing an acquire fence to the compositor. */
   bool has_explicit_sync = (m_wsi_surface->get_surface_sync_interface() != nullptr);
   m_image_factory.init(std::move(image_handle_creator), std::move(backing_memory_creator), has_explicit_sync,
                        !has_explicit_sync);
   return VK_SUCCESS;
}

VWL_CAPI_CALL(void) buffer_release(void *data, struct wl_buffer *wayl_buffer) VWL_API_POST
{
   auto sc = reinterpret_cast<swapchain *>(data);
   sc->release_buffer(wayl_buffer);
}

void swapchain::release_buffer(struct wl_buffer *wayl_buffer)
{
   uint32_t i;
   for (i = 0; i < m_swapchain_images.size(); i++)
   {
      auto data = m_swapchain_images[i].get_data<wayland_image_data>();
      if (data && data->get_buffer() == wayl_buffer)
      {
         if (data->ahb)
         {
            if (!data->busy || data->released) { set_error_state(VK_ERROR_SURFACE_LOST_KHR); break; }
            data->released=true;
            if (data->returned) { data->busy=false; unpresent_image(i); }
            break;
         }
         /* Some compositors might not deliver wp_presentation_feedback events if the images are pushed to compositor quick enough
          * in presentation modes that allow it (mailbox). If that happens, double check if these images were submitted for feedback
          * and handle it as a 'image discarded' event. */
         auto *present_timing_ext = get_swapchain_extension<wsi_ext_present_timing_wayland>();
         if (present_timing_ext != nullptr)
         {
            present_timing_ext->mark_buffer_release(i);
         }
         auto *present_id_ext = get_swapchain_extension<wsi_ext_present_id_wayland>();
         if (present_id_ext != nullptr)
         {
            present_id_ext->mark_buffer_release(i);
         }
         unpresent_image(i);
         break;
      }
   }

   /* check we found a buffer to unpresent */
   assert(i < m_swapchain_images.size());
}

static struct wl_buffer_listener buffer_listener = { buffer_release };

void swapchain::bind_ahb(wl_registry *registry, uint32_t name, uint32_t version)
{
   if (version < 3 || m_ahb_factory) return;
   m_ahb_factory=static_cast<banner_ahb_v1 *>(wl_registry_bind(registry, name, &banner_ahb_v1_interface, 3));
   static const banner_ahb_v1_listener listener{
      [](void *data, banner_ahb_v1 *, uint32_t enabled) { static_cast<swapchain *>(data)->ahb_mode(enabled); },
      [](void *data, banner_ahb_v1 *, wl_buffer *buffer, int32_t fd) { static_cast<swapchain *>(data)->release_ahb(buffer, fd); },
      [](void *data, banner_ahb_v1 *, wl_buffer *buffer) { static_cast<swapchain *>(data)->release_ahb(buffer, -1); }};
   if (m_ahb_factory) banner_ahb_v1_add_listener(m_ahb_factory, &listener, this);
}

void swapchain::release_ahb(wl_buffer *buffer, int fd)
{
   for (uint32_t i=0; i<m_swapchain_images.size(); ++i)
   {
      auto data=m_swapchain_images[i].get_data<wayland_image_data>();
      if (!data || data->get_buffer() != buffer) continue;
      if (!data->ahb || !data->busy || data->returned)
      { if (fd>=0) close(fd); set_error_state(VK_ERROR_SURFACE_LOST_KHR); return; }
      auto &memory=swapchain_image_factory::get_backing_memory_from_image<image_backing_memory_ahb>(m_swapchain_images[i]);
      if (!memory.return_dependency(fd)) { set_error_state(VK_ERROR_SURFACE_LOST_KHR); return; }
      data->returned=true;
      if (data->released) { data->busy=false; unpresent_image(i); }
      return;
   }
   if (fd>=0) close(fd);
   set_error_state(VK_ERROR_SURFACE_LOST_KHR);
}

wayland_owner<wl_buffer> swapchain::create_wl_buffer(image_backing_memory_external &image_external_memory)
{
   auto image_create_info = get_image_factory().get_image_handle_creator().get_image_create_info();
   assert(image_create_info.extent.width <= INT32_MAX);
   assert(image_create_info.extent.height <= INT32_MAX);

   external_memory &ext_memory = image_external_memory.get_external_memory();

   /* create a wl_buffer using the dma_buf protocol */
   zwp_linux_buffer_params_v1 *params = zwp_linux_dmabuf_v1_create_params(m_wsi_surface->get_dmabuf_interface());
   const uint64_t modifier = image_external_memory.get_image_create_info().selected_format.modifier;
   const auto modifier_hi = static_cast<uint32_t>(modifier >> 32);
   const auto modifier_low = static_cast<uint32_t>(modifier & 0xFFFFFFFF);
   for (uint32_t plane = 0; plane < ext_memory.get_num_planes(); plane++)
   {
      zwp_linux_buffer_params_v1_add(params, ext_memory.get_buffer_fds()[plane], plane, ext_memory.get_offsets()[plane],
                                     static_cast<uint32_t>(ext_memory.get_strides()[plane]), modifier_hi, modifier_low);
   }

   auto fourcc = util::drm::vk_to_drm_format(image_create_info.format);
   /* Emulate OPAQUE composite alpha by presenting through the alpha-less variant of the format (e.g.
    * ARGB8888 -> XRGB8888) - unless the app requested a premultiplied-alpha surface, in which case keep
    * the alpha channel. */
   if (!m_has_alpha)
   {
      fourcc = util::drm::drm_opaque_fourcc(fourcc);
   }
   auto buffer =
      zwp_linux_buffer_params_v1_create_immed(params, static_cast<int32_t>(image_create_info.extent.width),
                                              static_cast<int32_t>(image_create_info.extent.height), fourcc, 0);
   zwp_linux_buffer_params_v1_destroy(params);

   wl_proxy_set_queue(reinterpret_cast<wl_proxy *>(buffer), m_buffer_queue);
   auto res = wl_buffer_add_listener(buffer, &buffer_listener, this);
   if (res < 0)
   {
      wl_buffer_destroy(buffer);
      return nullptr;
   }

   return wayland_owner<wl_buffer>(buffer);
}

VkResult swapchain::allocate_and_bind_swapchain_image(swapchain_image &image)
{
   if (m_ahb_allocation_probe)
   {
      if (!image.get_image()) return VK_ERROR_INITIALIZATION_FAILED;
      if (!m_ahb_present) return VK_SUCCESS;
      auto &memory=swapchain_image_factory::get_backing_memory_from_image<image_backing_memory_ahb>(image);
      int wire[2];
      if (socketpair(AF_UNIX, SOCK_STREAM|SOCK_CLOEXEC, 0, wire)) return VK_ERROR_OUT_OF_HOST_MEMORY;
      util::fd_owner sender(wire[0]), receiver(wire[1]);
      timeval timeout{2,0};
      if (setsockopt(wire[1], SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) ||
          ahb_native_api::get().send_handle(memory.buffer(), wire[0])) return VK_ERROR_INITIALIZATION_FAILED;
      const auto &desc=memory.description();
      auto buffer=banner_ahb_v1_create_buffer(m_ahb_factory, wire[1], desc.width, desc.height, desc.stride, 3);
      if (!buffer) return VK_ERROR_OUT_OF_HOST_MEMORY;
      wayland_owner<wl_buffer> owned(buffer);
      wl_proxy_set_queue(reinterpret_cast<wl_proxy *>(buffer), m_buffer_queue);
      if (wl_buffer_add_listener(buffer, &buffer_listener, this) < 0) return VK_ERROR_INITIALIZATION_FAILED;
      auto data=m_allocator.make_unique<wayland_image_data>(std::move(owned));
      if (!data) return VK_ERROR_OUT_OF_HOST_MEMORY;
      data->ahb=true; image.set_data(std::move(data));
      return VK_SUCCESS;
   }
   util::unique_lock<util::recursive_mutex> image_status_lock(m_image_status_mutex);
   if (!image_status_lock)
   {
      WSI_LOG_ERROR("Failed to acquire mutex lock in allocate_and_bind_swapchain_image.\n");
      return VK_ERROR_INITIALIZATION_FAILED;
   }

   auto &backing_memory = swapchain_image_factory::get_backing_memory_from_image<image_backing_memory_external>(image);
   TRY_LOG_CALL(backing_memory.allocate());

   wayland_owner<wl_buffer> buffer = nullptr;
   buffer = create_wl_buffer(backing_memory);
   if (buffer == nullptr)
   {
      WSI_LOG_ERROR("Failed to create wl_buffer");
      return VK_ERROR_INITIALIZATION_FAILED;
   }

   TRY_LOG_CALL(backing_memory.import_and_bind(image.get_image()));

   auto image_data = m_allocator.make_unique<wayland_image_data>(std::move(buffer));
   if (image_data == nullptr)
   {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
   }
   image.set_data(std::move(image_data));
   return VK_SUCCESS;
}

void swapchain::present_image(const pending_present_request &pending_present)
{
   if (m_ahb_allocation_probe && !m_ahb_present)
   {
      WSI_LOG_ERROR("AHB allocation-only probe: presentation is not implemented");
      set_error_state(VK_ERROR_FEATURE_NOT_PRESENT);
      return;
   }
   auto &image = m_swapchain_images[pending_present.image_index];
   auto image_data = image.get_data<wayland_image_data>();

   /* if a frame is already pending, wait for a hint to present again */
   if (!m_wsi_surface->wait_next_frame_event())
   {
      set_error_state(VK_ERROR_SURFACE_LOST_KHR);
   }

   wl_surface_attach(m_surface, image_data->get_buffer(), 0, 0);
   if (m_ahb_present)
   {
      auto &memory=swapchain_image_factory::get_backing_memory_from_image<image_backing_memory_ahb>(image);
      const int fd=memory.take_present_fd();
      util::fd_owner dependency(fd);
      if (m_ahb_mode != 1 || fd < -1 || image_data->busy)
      { set_error_state(VK_ERROR_SURFACE_LOST_KHR); return; }
      image_data->busy=true; image_data->released=false; image_data->returned=false;
      if (fd >= 0) banner_ahb_v1_set_acquire_fence(m_ahb_factory, image_data->get_buffer(), fd);
      else banner_ahb_v1_set_acquire_ready(m_ahb_factory, image_data->get_buffer());
   }

   /* RK3588: only hand an acquire fence to the compositor when explicit sync is available. Without
    * it the present fence is non-exportable and CPU-waited in swapchain_image::wait_present instead. */
   if (!m_ahb_present && m_wsi_surface->get_surface_sync_interface() != nullptr)
   {
      auto present_sync_fd = static_cast<sync_fd_fence_sync *>(image.get_present_fence())->export_sync_fd();
      if (!present_sync_fd.has_value())
      {
         WSI_LOG_ERROR("Failed to export present fence.");
         set_error_state(VK_ERROR_SURFACE_LOST_KHR);
      }
      else if (present_sync_fd->is_valid())
      {
         zwp_linux_surface_synchronization_v1_set_acquire_fence(m_wsi_surface->get_surface_sync_interface(),
                                                                present_sync_fd->get());
      }
   }

   /* TODO: work out damage */
   wl_surface_damage(m_surface, 0, 0, INT32_MAX, INT32_MAX);

   if (m_present_mode == VK_PRESENT_MODE_FIFO_KHR)
   {
      if (!m_wsi_surface->set_frame_callback())
      {
         set_error_state(VK_ERROR_SURFACE_LOST_KHR);
      }
   }

   auto *present_id_ext = get_swapchain_extension<wsi_ext_present_id_wayland>();
   if (present_id_ext != nullptr)
   {
      if (m_wsi_surface->get_presentation_time_interface() != nullptr && pending_present.present_id)
      {
         wp_presentation *pres = m_wsi_surface->get_presentation_time_interface();
         struct wp_presentation_feedback *feedback = wp_presentation_feedback(pres, m_wsi_surface->get_wl_surface());
         wl_proxy_set_queue(reinterpret_cast<wl_proxy *>(feedback), m_buffer_queue);
         presentation_feedback *feedback_obj = present_id_ext->insert_into_pending_present_feedback_list(
            pending_present.present_id, pending_present.image_index, feedback);
         if (feedback_obj == nullptr)
         {
            WSI_LOG_ERROR("Error adding to pending present feedback list");
            set_error_state(VK_ERROR_SURFACE_LOST_KHR);
            return;
         }
         register_wp_presentation_feedback_listener(feedback, feedback_obj);
      }
   }
   auto *present_timing_ext = get_swapchain_extension<wsi_ext_present_timing_wayland>();
   if (present_timing_ext != nullptr)
   {
      if ((present_timing_ext->is_stage_pending_for_image_index(pending_present.image_index,
                                                                VK_PRESENT_STAGE_IMAGE_FIRST_PIXEL_OUT_BIT_EXT)) &&
          (m_wsi_surface->get_presentation_time_interface() != nullptr))
      {
         wp_presentation *pres = m_wsi_surface->get_presentation_time_interface();
         struct wp_presentation_feedback *feedback = wp_presentation_feedback(pres, m_wsi_surface->get_wl_surface());
         wl_proxy_set_queue(reinterpret_cast<wl_proxy *>(feedback), m_buffer_queue);
         presentation_feedback *feedback_obj = present_timing_ext->insert_into_pending_present_feedback_list(
            pending_present.image_index, feedback, pending_present.present_id);
         if (feedback_obj == nullptr)
         {
            WSI_LOG_ERROR("Error adding to pending present feedback list");
            set_error_state(VK_ERROR_SURFACE_LOST_KHR);
            return;
         }
         register_wp_presentation_feedback_listener(feedback, feedback_obj);
      }
   }

   wl_surface_commit(m_surface);
   int res = wl_display_flush(m_display);
   if (res < 0)
   {
      WSI_LOG_ERROR("error flushing the display");
      /* Setting the swapchain as invalid */
      set_error_state(VK_ERROR_SURFACE_LOST_KHR);
   }

   auto *ext = get_swapchain_extension<wsi_ext_present_id_wayland>();
   if (ext != nullptr)
   {
      if (m_wsi_surface->get_presentation_time_interface() == nullptr)
      {
         ext->mark_delivered(pending_present.present_id);
      }
   }
}

bool swapchain::free_image_found()
{
   for (auto &img : m_swapchain_images)
   {
      if (img.get_status() == swapchain_image::FREE)
      {
         return true;
      }
   }
   return false;
}

VkResult swapchain::get_free_buffer(uint64_t *timeout)
{
   int ms_timeout, res;
   const uint64_t timeout_ns = *timeout;
   const uint64_t timeout_ms = timeout_ns / 1000000ULL;
   ms_timeout = timeout_ms > static_cast<uint64_t>(INT_MAX) ? INT_MAX : static_cast<int>(timeout_ms);

   /* The current dispatch_queue implementation will return if any
    * events are returned, even if no events are dispatched to the buffer
    * queue. Therefore dispatch repeatedly until a buffer has been freed.
    */
   do
   {
      res = dispatch_queue(m_display, m_buffer_queue, ms_timeout);
   } while (!free_image_found() && res > 0);

   if (res > 0)
   {
      *timeout = 0;
      return VK_SUCCESS;
   }
   else if (res == 0)
   {
      if (*timeout == 0)
      {
         return VK_NOT_READY;
      }
      else
      {
         return VK_TIMEOUT;
      }
   }
   else
   {
      return VK_ERROR_SURFACE_LOST_KHR;
   }
}

std::variant<VkResult, util::unique_ptr<vulkan_image_handle_creator>> swapchain::create_image_creator(
   const VkSwapchainCreateInfoKHR &swapchain_create_info)
{
   auto image_handle_creator = m_allocator.make_unique<vulkan_image_handle_creator>(m_allocator, swapchain_create_info);
   if (image_handle_creator == nullptr)
   {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
   }

   if (is_mutable_format_enabled())
   {
      const auto *image_format_list = util::find_extension<VkImageFormatListCreateInfo>(
         VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO, swapchain_create_info.pNext);
      auto mutable_format_uptr = swapchain_image_create_mutable_format::create_unique(image_format_list, m_allocator);
      if (!mutable_format_uptr)
      {
         return VK_ERROR_OUT_OF_HOST_MEMORY;
      }
      TRY_LOG_CALL(image_handle_creator->add_extension(std::move(mutable_format_uptr)));
   }

   return image_handle_creator;
}

swapchain_image_factory &swapchain::get_image_factory()
{
   return m_image_factory;
}

} // namespace wayland
} // namespace wsi
