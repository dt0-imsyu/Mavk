#!/bin/sh
set -eu
repo=${MAVK_SOURCE_DIR:-$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)}
source=${MAVK_WSI_SOURCE:-$repo/third_party/wsi-layer}
out=${MAVK_WSI_OUTPUT:-$repo/recovery/wsi-build}
tools=${MAVK_CROSS_TOOLS:-/mnt/e/Mavk/artifacts/sarek-native-tools-20261018/root}
cmake=${MAVK_CMAKE_ROOT:-/mnt/e/Mavk/artifacts/glibc-cmake-tools1-20261018/root}
sdk=${MAVK_GLIBC_SDK:-/mnt/e/Mavk/artifacts/zink-ui-25.1.9-20261019/sdk}
scanner=${MAVK_WAYLAND_SCANNER:-/mnt/e/Mavk/artifacts/wayland-scanner-tools-20261023/runtime/usr/bin/wayland-scanner}
headers=${MAVK_VULKAN_HEADERS:-/mnt/e/Mavk/artifacts/wsi-profile-20261022/headers/include}
ninja=${MAVK_NINJA:-/mnt/e/Mavk/artifacts/glmark2-20261023/ninja-host/bin/ninja}
mkdir -p "$out"
export LD_LIBRARY_PATH="$cmake/usr/lib/x86_64-linux-gnu:$tools/usr/lib/x86_64-linux-gnu"
export TMPDIR=${MAVK_WSI_TMPDIR:-$out/tmp}
mkdir -p "$TMPDIR"
export PKG_CONFIG_SYSROOT_DIR="$sdk"
export PKG_CONFIG_LIBDIR="$sdk/usr/lib/pkgconfig:$sdk/usr/share/pkgconfig"
"$cmake/usr/bin/cmake" -S "$source" -B "$out/build" -G Ninja \
 -DCMAKE_MAKE_PROGRAM="$ninja" \
 -DCMAKE_TOOLCHAIN_FILE="$repo/experiments/droiddeck/glibc-aarch64.cmake" \
 -DCMAKE_BUILD_TYPE=Release -DPKG_CONFIG_EXECUTABLE="$tools/usr/bin/pkgconf" \
 -DMAVK_SOURCE_DIR="$repo" -DCMAKE_C_FLAGS="-isystem $sdk/usr/include" -DCMAKE_CXX_FLAGS="-isystem $sdk/usr/include" \
 -DCMAKE_SHARED_LINKER_FLAGS="-L$sdk/usr/lib -Wl,-rpath-link,$sdk/usr/lib" \
 -DVULKAN_CXX_INCLUDE="$headers" -DBUILD_WSI_HEADLESS=OFF -DBUILD_WSI_WAYLAND=ON \
 -DBUILD_WSI_DISPLAY=OFF -DBUILD_WSI_X11=ON -DWAYLAND_SCANNER_EXEC="$scanner" \
 -DSELECT_EXTERNAL_ALLOCATOR=dma_buf_heaps -DWSIALLOC_MEMORY_HEAP_NAME=system
"$cmake/usr/bin/cmake" --build "$out/build" -j8
sha256sum "$out/build/libVkLayer_window_system_integration.so"
