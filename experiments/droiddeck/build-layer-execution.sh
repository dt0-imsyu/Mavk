#!/bin/sh
set -eu
tools=/mnt/e/Mavk/artifacts/sarek-native-tools-20261018/root
source=/mnt/e/Mavk/artifacts/sarek-native-source-20261018/source
sdk=/mnt/e/Mavk/artifacts/sarek-native-source-20261018/sdk
repo=/mnt/c/Users/denis/MainProjects/win11ontab/Mavk
out=${MAVK_EXECUTION_OUTPUT:?Fresh output required}
test ! -e "$out" || exit 20
mkdir -p "$out"
export LD_LIBRARY_PATH="$tools/usr/lib/x86_64-linux-gnu"
export TMPDIR=${MAVK_BUILD_TEMP:-/mnt/c/Users/denis/MainProjects/win11ontab/Mavk/artifacts-local/build-temp}
mkdir -p "$TMPDIR"
"$tools/usr/bin/aarch64-linux-gnu-g++" --sysroot="$tools" -std=c++17 -O2 \
 -I "$source/include/vulkan/include" -I "$repo/core/include" \
 -I /mnt/e/Mavk/artifacts/null-cube-harness-20261020 \
 "$repo/compatibility/test/layer-execution.cpp" "$repo/core/src/core.cpp" "$repo/bridge/src/graphics.cpp" \
 "$sdk/usr/lib/libvulkan.so.1.4.357" -Wl,--wrap=vkCmdDraw,--wrap=vkCmdDrawIndexed,--wrap=vkCreatePipelineLayout,--wrap=vkCreateDescriptorSetLayout \
 -o "$out/layer-execution"
