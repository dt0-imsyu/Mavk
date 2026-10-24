// SPDX-License-Identifier: MIT
// Experimental rendering-only XCB proxy; input is a separate integration gate.
#include "wine_ahb_surface.hpp"
#include "surface.hpp"
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <sys/time.h>
#include <xdg-shell-client-protocol.h>
#include "util/log.hpp"
#include "xcb_pointer.hpp"

namespace wsi::wayland {
namespace {
std::atomic<bool> occupied{false};
struct window {
   mavk::droiddeck::xcb_pointer input;
   wl_display *display = nullptr;
   wl_compositor *compositor = nullptr;
   xdg_wm_base *shell = nullptr;
   wl_surface *surface = nullptr;
   xdg_surface *role = nullptr;
   xdg_toplevel *top = nullptr;
   bool configured = false;
   bool pointer_ready = false;
   ~window() {
      input.shutdown();
      if (top) xdg_toplevel_destroy(top);
      if (role) xdg_surface_destroy(role);
      if (surface) wl_surface_destroy(surface);
      if (shell) xdg_wm_base_destroy(shell);
      if (compositor) wl_compositor_destroy(compositor);
      if (display) { wl_display_flush(display); wl_display_disconnect(display); }
      occupied = false;
   }
};
void ping(void *, xdg_wm_base *shell, uint32_t serial) { xdg_wm_base_pong(shell, serial); }
const xdg_wm_base_listener shell_listener{ping};
void global(void *data, wl_registry *registry, uint32_t name, const char *type, uint32_t version) {
   auto &w = *static_cast<window *>(data);
   if (!std::strcmp(type, "wl_compositor"))
      w.compositor = static_cast<wl_compositor *>(wl_registry_bind(registry, name, &wl_compositor_interface, version < 4 ? version : 4));
   if (!std::strcmp(type, "xdg_wm_base")) {
      w.shell = static_cast<xdg_wm_base *>(wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
      xdg_wm_base_add_listener(w.shell, &shell_listener, nullptr);
   }
   if (!std::strcmp(type,"wl_seat")) w.input.bind(registry,name,version);
}
void removed(void *, wl_registry *, uint32_t) {}
const wl_registry_listener registry_listener{global, removed};
void configure(void *data, xdg_surface *role, uint32_t serial) {
   xdg_surface_ack_configure(role, serial);
   static_cast<window *>(data)->configured = true;
}
const xdg_surface_listener role_listener{configure};
void top_configure(void *data, xdg_toplevel *, int32_t w, int32_t h, wl_array *) {
   static_cast<window *>(data)->input.output(w,h);
}
void close(void *, xdg_toplevel *) {}
const xdg_toplevel_listener top_listener{top_configure, close, nullptr, nullptr};
}
bool wine_ahb_proxy_enabled() {
   const char *proxy = std::getenv("MAVK_WSI_XCB_AHB_PROXY");
   const char *present = std::getenv("MAVK_WSI_AHB_PRESENT");
   return proxy && present && !std::strcmp(proxy, "1") && !std::strcmp(present, "1");
}
util::unique_ptr<wsi::surface> make_wine_ahb_surface(const util::allocator &allocator, uint32_t xid) {
   if (occupied.exchange(true)) return nullptr;
   std::shared_ptr<window> w(new (std::nothrow) window);
   if (!w) { occupied = false; return nullptr; }
   w->display = wl_display_connect(nullptr);
   if (!w->display) return nullptr;
   w->pointer_ready=w->input.init(xid);
   timeval timeout{2, 0};
   if (setsockopt(wl_display_get_fd(w->display), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout))) return nullptr;
   wl_registry *registry = wl_display_get_registry(w->display);
   if (!registry) return nullptr;
   wl_registry_add_listener(registry, &registry_listener, w.get());
   int result = wl_display_roundtrip(w->display);
   wl_registry_destroy(registry);
   if (result < 0 || !w->compositor || !w->shell) return nullptr;
   w->surface = wl_compositor_create_surface(w->compositor);
   if (!w->surface) return nullptr;
   wl_region *input = wl_compositor_create_region(w->compositor);
   if (!input) return nullptr;
   if (!w->pointer_ready) wl_surface_set_input_region(w->surface, input);
   wl_region_destroy(input);
   w->role = xdg_wm_base_get_xdg_surface(w->shell, w->surface);
   if (!w->role) return nullptr;
   xdg_surface_add_listener(w->role, &role_listener, w.get());
   w->top = xdg_surface_get_toplevel(w->role);
   if (!w->top) return nullptr;
   xdg_toplevel_add_listener(w->top, &top_listener, w.get());
   xdg_toplevel_set_title(w->top, "Mavk experimental XCB presentation");
   xdg_toplevel_set_app_id(w->top, "org.mavk.xcb-ahb-render-only");
   xdg_toplevel_set_fullscreen(w->top, nullptr);
   wl_surface_commit(w->surface);
   if (wl_display_roundtrip(w->display) < 0 || !w->configured) return nullptr;
   auto value = surface::make_surface(allocator, w->display, w->surface);
   if (value) value->retain_native_window(w);
   std::fprintf(stderr,"MAVK_XCB_AHB_PROXY result=%d pointer=%u\n",value?0:-1,w->pointer_ready);
   return util::unique_ptr<wsi::surface>(std::move(value));
}
}
