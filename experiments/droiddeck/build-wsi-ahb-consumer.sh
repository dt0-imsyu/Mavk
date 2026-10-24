#!/bin/sh
set -eu
repo=/mnt/c/Users/denis/MainProjects/win11ontab/Mavk
tools=/mnt/e/Mavk/artifacts/sarek-native-tools-20261018/root
sdk=/mnt/e/Mavk/artifacts/zink-ui-25.1.9-20261019/sdk
headers=/mnt/e/Mavk/artifacts/wsi-profile-20261022/headers/include
out=${MAVK_WSI_CONSUMER_OUTPUT:?Fresh output required}
test ! -e "$out"
mkdir -p "$out"
scanner=/mnt/e/Mavk/artifacts/wayland-scanner-tools-20261023/runtime/usr/bin/wayland-scanner
xml=/mnt/e/Mavk/work/DroidDeck-mavk/app/src/main/cpp/waylandcomp/protocols/xdg-shell.xml
"$scanner" client-header "$xml" "$out/xdg-shell-client-protocol.h"
"$scanner" private-code "$xml" "$out/xdg-shell-protocol.c"
export LD_LIBRARY_PATH="$tools/usr/lib/x86_64-linux-gnu"
export TMPDIR=${MAVK_WSI_TMPDIR:-/mnt/e/Mavk/artifacts/build-temp-20261018}
"$tools/usr/bin/aarch64-linux-gnu-gcc" --sysroot="$tools" -std=c11 -O2 \
 -I "$headers" -I "$out" -isystem "$sdk/usr/include" "$repo/experiments/droiddeck/wsi-ahb-consumer.c" "$out/xdg-shell-protocol.c" \
 "$sdk/usr/lib/libwayland-client.so" /mnt/e/Mavk/artifacts/droiddeck-window-root1-20261018/root/usr/lib/libffi.so.8.5.0 \
 -ldl -lm -o "$out/wsi-ahb-consumer"
sha256sum "$out/wsi-ahb-consumer"
