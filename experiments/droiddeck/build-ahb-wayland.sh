#!/bin/sh
set -eu
repo=/mnt/c/Users/denis/MainProjects/win11ontab/Mavk
fork=/mnt/e/Mavk/work/DroidDeck-mavk/app/src/main/cpp/waylandcomp
tools=/mnt/e/Mavk/artifacts/sarek-native-tools-20261018/root
headers=/mnt/e/Mavk/artifacts/sarek-native-source-20261018/source/include/vulkan/include
sdk=/mnt/e/Mavk/artifacts/zink-ui-25.1.9-20261019/sdk
scanner=/mnt/e/Mavk/artifacts/wayland-scanner-tools-20261023/runtime/usr/bin/wayland-scanner
out=${MAVK_AHB_OUTPUT:?Fresh MAVK_AHB_OUTPUT required}
test ! -e "$out" || { echo 'Preserve previous build'; exit 20; }
mkdir -p "$out"
for name in banner-ahb-v1 xdg-shell; do
    "$scanner" client-header "$fork/protocols/$name.xml" "$out/$name-client-protocol.h"
    "$scanner" private-code "$fork/protocols/$name.xml" "$out/$name-protocol.c"
done
export LD_LIBRARY_PATH="$tools/usr/lib/x86_64-linux-gnu"
export TMPDIR=/mnt/e/Mavk/artifacts/build-temp-20261018
"$tools/usr/bin/aarch64-linux-gnu-gcc" --sysroot="$tools" -std=c11 -O2 -DMAVK_AHB_WAYLAND \
 -I "$out" -I "$headers" -idirafter "$sdk/usr/include" "$repo/experiments/droiddeck/ahb-interop.c" \
 "$repo/adapters/android/ahb_image.c" "$out/banner-ahb-v1-protocol.c" "$out/xdg-shell-protocol.c" "$sdk/usr/lib/libwayland-client.so" \
 /mnt/e/Mavk/artifacts/droiddeck-window-root1-20261018/root/usr/lib/libffi.so.8.5.0 \
 -ldl -lm -o "$out/ahb-wayland"
sha256sum "$out/ahb-wayland"
