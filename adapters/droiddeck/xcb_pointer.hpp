// SPDX-License-Identifier: MIT
#pragma once
#include <X11/Xlib.h>
#include <wayland-client.h>
#include <dlfcn.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace mavk::droiddeck {

// Own X connection: never consume the Vulkan application's X events.
class xcb_pointer {
   Display *display{};
   Window window{};
   void *library{};
   int (*fake_motion)(Display *, int, int, int, unsigned long){};
   int (*fake_button)(Display *, unsigned int, Bool, unsigned long){};
   wl_pointer *pointer{};
   wl_seat *seat{};
   int x{}, y{}, width{}, height{}, output_width{}, output_height{};
   unsigned held{};
   bool valid{};

   void refresh() {
      XWindowAttributes attributes{};
      Window child{};
      valid = XGetWindowAttributes(display, window, &attributes) &&
         XTranslateCoordinates(display, window, attributes.root, 0, 0, &x, &y, &child);
      if (valid) { width=attributes.width; height=attributes.height; }
   }
   void motion(wl_fixed_t px, wl_fixed_t py) {
      while (XPending(display)) {
         XEvent event{}; XNextEvent(display,&event);
         if (event.type==DestroyNotify) valid=false;
         else if (event.type==ConfigureNotify || event.type==ReparentNotify) refresh();
      }
      if (!valid || width<=0 || height<=0 || output_width<=0 || output_height<=0) return;
      const int sx=std::clamp(int(std::floor(wl_fixed_to_double(px)*width/output_width)),0,width-1);
      const int sy=std::clamp(int(std::floor(wl_fixed_to_double(py)*height/output_height)),0,height-1);
      fake_motion(display,DefaultScreen(display),x+sx,y+sy,CurrentTime);
      XFlush(display);
   }
   static void capabilities(void *data, wl_seat *value, uint32_t caps) {
      auto &self=*static_cast<xcb_pointer *>(data);
      if (!(caps & WL_SEAT_CAPABILITY_POINTER) || self.pointer) return;
      self.pointer=wl_seat_get_pointer(value);
      static const wl_pointer_listener listener=[] {
         wl_pointer_listener result{};
         result.enter=[](void *d,wl_pointer *,uint32_t,wl_surface *,wl_fixed_t x,wl_fixed_t y) {
            static_cast<xcb_pointer *>(d)->motion(x,y);
         };
         result.leave=[](void *,wl_pointer *,uint32_t,wl_surface *) {};
         result.motion=[](void *d,wl_pointer *,uint32_t,wl_fixed_t x,wl_fixed_t y) {
            static_cast<xcb_pointer *>(d)->motion(x,y);
         };
         result.button=[](void *d,wl_pointer *,uint32_t,uint32_t,uint32_t code,uint32_t state) {
            auto &s=*static_cast<xcb_pointer *>(d);
            unsigned button=code==0x110?1:code==0x111?3:code==0x112?2:0;
            if (!button || !s.valid) return;
            const bool down=state==WL_POINTER_BUTTON_STATE_PRESSED;
            if (down) s.held|=1u<<button; else s.held&=~(1u<<button);
            s.fake_button(s.display,button,down,CurrentTime); XFlush(s.display);
         };
         result.axis=[](void *d,wl_pointer *,uint32_t,uint32_t axis,wl_fixed_t delta) {
            auto &s=*static_cast<xcb_pointer *>(d);
            if (!s.valid || !delta) return;
            unsigned b=axis==WL_POINTER_AXIS_VERTICAL_SCROLL?(delta<0?4:5):(delta<0?6:7);
            s.fake_button(s.display,b,True,CurrentTime);
            s.fake_button(s.display,b,False,CurrentTime); XFlush(s.display);
         };
         result.frame=[](void *,wl_pointer *) {};
         result.axis_source=[](void *,wl_pointer *,uint32_t) {};
         result.axis_stop=[](void *,wl_pointer *,uint32_t,uint32_t) {};
         result.axis_discrete=[](void *,wl_pointer *,uint32_t,int32_t) {};
         return result;
      }();
      wl_pointer_add_listener(self.pointer,&listener,&self);
   }
public:
   xcb_pointer()=default;
   xcb_pointer(const xcb_pointer &)=delete;
   xcb_pointer &operator=(const xcb_pointer &)=delete;
   ~xcb_pointer() { shutdown(); }
   void shutdown() {
      if (pointer) wl_pointer_destroy(pointer);
      if (seat) wl_seat_destroy(seat);
      if (display) {
         for(unsigned b=1;b<=3;++b) if(held&(1u<<b)) fake_button(display,b,False,CurrentTime);
         XCloseDisplay(display);
      }
      if (library) dlclose(library);
      pointer=nullptr; seat=nullptr; display=nullptr; library=nullptr; held=0; valid=false;
   }
   bool init(uint32_t xid) {
      library=dlopen("libXtst.so.6",RTLD_NOW|RTLD_LOCAL);
      if (!library) return false;
      fake_motion=reinterpret_cast<decltype(fake_motion)>(dlsym(library,"XTestFakeMotionEvent"));
      fake_button=reinterpret_cast<decltype(fake_button)>(dlsym(library,"XTestFakeButtonEvent"));
      if (!fake_motion || !fake_button || !(display=XOpenDisplay(nullptr))) return false;
      window=xid; XSelectInput(display,window,StructureNotifyMask); refresh();
      std::fprintf(stderr,"MAVK_XCB_POINTER xid=%x rect=%d,%d,%d,%d ready=%u\n",xid,x,y,width,height,valid);
      return valid;
   }
   void output(int w,int h) { if(w>0&&h>0) { output_width=w;output_height=h; } }
   void bind(wl_registry *registry,uint32_t name,uint32_t version) {
      if (!valid || seat) return;
      seat=static_cast<wl_seat *>(wl_registry_bind(registry,name,&wl_seat_interface,std::min(version,5u)));
      static const wl_seat_listener listener{capabilities,[](void *,wl_seat *,const char *){}};
      wl_seat_add_listener(seat,&listener,this);
   }
};
}
