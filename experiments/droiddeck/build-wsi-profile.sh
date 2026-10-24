#!/bin/sh
set -eu
repo=/mnt/c/Users/denis/MainProjects/win11ontab/Mavk
out=/mnt/e/Mavk/artifacts/wsi-profile-20261022
tools=/mnt/e/Mavk/artifacts/sarek-native-tools-20261018/root
cmake=/mnt/e/Mavk/artifacts/glibc-cmake-tools1-20261018/root
sdk=/mnt/e/Mavk/artifacts/zink-ui-25.1.9-20261019/sdk
headers="$out/headers/include"
export PATH="$tools/usr/bin:$PATH"
export LD_LIBRARY_PATH="$cmake/usr/lib/x86_64-linux-gnu:$tools/usr/lib/x86_64-linux-gnu"
export PKG_CONFIG_SYSROOT_DIR="$sdk"
export PKG_CONFIG_LIBDIR="$sdk/usr/lib/pkgconfig:$sdk/usr/share/pkgconfig"
test -d "$out/source" && test -f "$headers/vulkan/vulkan_core.h"
"$cmake/usr/bin/cmake" -S "$out/source" -B "$out/build" -G Ninja \
 -DCMAKE_TOOLCHAIN_FILE="$repo/experiments/droiddeck/glibc-aarch64.cmake" \
 -DCMAKE_BUILD_TYPE=Release -DPKG_CONFIG_EXECUTABLE="$tools/usr/bin/pkgconf" \
 -DCMAKE_C_FLAGS="-I$sdk/usr/include" -DCMAKE_CXX_FLAGS="-I$sdk/usr/include" \
 -DCMAKE_SHARED_LINKER_FLAGS="-L$sdk/usr/lib -Wl,-rpath-link=$sdk/usr/lib" \
 -DVULKAN_CXX_INCLUDE="$headers" -DBUILD_WSI_HEADLESS=OFF -DBUILD_WSI_WAYLAND=OFF \
 -DBUILD_WSI_DISPLAY=OFF -DBUILD_WSI_X11=ON -DSELECT_EXTERNAL_ALLOCATOR=dma_buf_heaps \
 -DWSIALLOC_MEMORY_HEAP_NAME=system
"$cmake/usr/bin/cmake" --build "$out/build" -j4
