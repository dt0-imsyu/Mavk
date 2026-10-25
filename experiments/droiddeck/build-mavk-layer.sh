#!/bin/sh
set -eu
tools=/mnt/e/Mavk/artifacts/sarek-native-tools-20261018/root
source=/mnt/e/Mavk/artifacts/sarek-native-source-20261018/source
sdk=/mnt/e/Mavk/artifacts/sarek-native-source-20261018/sdk
out=${MAVK_LAYER_OUTPUT:?Fresh C: layer output required}
guards=/mnt/e/Mavk/artifacts/shader-guards-glibc1-20261018
guard_archive=${MAVK_GUARD_ARCHIVE:-/mnt/e/Mavk/artifacts/guard-cse-frontend-20261023/libmavk-shader-guards.a}
opt_archive=${MAVK_SPIRV_OPT_ARCHIVE:-/mnt/e/Mavk/artifacts/texel-components-optimizer-20261022/libSPIRV-Tools-opt.a}
repo=/mnt/c/Users/denis/MainProjects/win11ontab/Mavk
test ! -e "$out" || { echo 'Fresh layer output required';exit 20; }
mkdir -p "$out"
export LD_LIBRARY_PATH="$tools/usr/lib/x86_64-linux-gnu"
export TMPDIR=${MAVK_BUILD_TEMP:-/mnt/c/Users/denis/MainProjects/win11ontab/Mavk/artifacts-local/build-temp}
mkdir -p "$TMPDIR"
"$tools/usr/bin/aarch64-linux-gnu-g++" --sysroot="$tools" -std=c++17 -O2 -fPIC -fvisibility=hidden -shared \
 -I "$source/include/vulkan/include" -I "$repo/core/include" -I "$repo/compatibility/include" \
 -I /mnt/e/Mavk/research/spirv-tools-v2022.4/include -I /mnt/e/Mavk/research/spirv-tools-v2022.4/external/spirv-headers/include \
 "$repo/adapters/vulkan-layer/layer.cpp" "$repo/core/src/core.cpp" \
 "$repo/compatibility/shaders/cull_distance.cpp" \
 "$guard_archive" "$opt_archive" "$guards/spirv-tools/source/libSPIRV-Tools.a" \
 "$sdk/usr/lib/libvulkan.so.1.4.357" -Wl,-z,defs -Wl,--exclude-libs,ALL -Wl,-soname,libVkLayer_mavk_compat.so \
 -o "$out/libVkLayer_mavk_compat.so"
cp "$repo/adapters/vulkan-layer/VkLayer_mavk_compat.json" "$out/"
"$tools/usr/bin/aarch64-linux-gnu-readelf" -d "$out/libVkLayer_mavk_compat.so"
