#!/bin/sh
set -eu
repo=/mnt/c/Users/denis/MainProjects/win11ontab/Mavk
bundle=/mnt/e/Mavk/artifacts/glmark2-20261023
out=/mnt/e/Mavk/artifacts/wsi-optimal-20261023
mkdir -p "$out"
task_dir=$(mktemp -d /tmp/mavk-wsi.XXXXXX)
mkdir "$task_dir/tools" "$task_dir/sdk" "$task_dir/source" "$task_dir/headers"
tar -xf "$bundle/cross-tools-posix.tar" -C "$task_dir/tools"
tar -xf "$bundle/sdk-posix.tar" -C "$task_dir/sdk"
tar -xf "$bundle/wsi-optimal-source.tar" -C "$task_dir/source"
tar -xf "$bundle/wsi-headers.tar" -C "$task_dir/headers"
cmake=/mnt/e/Mavk/artifacts/glibc-cmake-tools1-20261018/root
export LD_LIBRARY_PATH="$cmake/usr/lib/x86_64-linux-gnu:$task_dir/tools/usr/lib/x86_64-linux-gnu"
export PKG_CONFIG_SYSROOT_DIR="$task_dir/sdk"
export PKG_CONFIG_LIBDIR="$task_dir/sdk/usr/lib/pkgconfig:$task_dir/sdk/usr/share/pkgconfig"
"$cmake/usr/bin/cmake" -S "$task_dir/source" -B "$task_dir/build" -G Ninja \
 -DCMAKE_MAKE_PROGRAM="$bundle/ninja-host/bin/ninja" \
 -DCMAKE_TOOLCHAIN_FILE="$repo/experiments/droiddeck/glmark2-build/cross.cmake" \
 -DCMAKE_BUILD_TYPE=Release -Dtools="$task_dir/tools" -Dsdk="$task_dir/sdk" \
 -DPKG_CONFIG_EXECUTABLE="$task_dir/tools/usr/bin/pkgconf" \
 -DCMAKE_C_FLAGS="-I$task_dir/sdk/usr/include" -DCMAKE_CXX_FLAGS="-I$task_dir/sdk/usr/include" \
 -DCMAKE_SHARED_LINKER_FLAGS="-L$task_dir/sdk/usr/lib -Wl,-rpath-link,$task_dir/sdk/usr/lib" \
 -DVULKAN_CXX_INCLUDE="$task_dir/headers/include" -DBUILD_WSI_HEADLESS=OFF -DBUILD_WSI_WAYLAND=OFF \
 -DBUILD_WSI_DISPLAY=OFF -DBUILD_WSI_X11=ON -DSELECT_EXTERNAL_ALLOCATOR=dma_buf_heaps -DWSIALLOC_MEMORY_HEAP_NAME=system
"$cmake/usr/bin/cmake" --build "$task_dir/build" -j8
cp "$task_dir/build/libVkLayer_window_system_integration.so" "$out/"
cp "$task_dir/build/VkLayer_window_system_integration.json" "$out/"
echo WSI_OPTIMAL_BUILD_PASS
