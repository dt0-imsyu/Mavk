// SPDX-License-Identifier: MIT
#pragma once
#include "wsi/surface.hpp"
namespace wsi::wayland {
bool wine_ahb_proxy_enabled();
util::unique_ptr<wsi::surface> make_wine_ahb_surface(const util::allocator &allocator, uint32_t window);
}
