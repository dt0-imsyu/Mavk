#!/bin/sh
set -eu
repo=/mnt/c/Users/denis/MainProjects/win11ontab/Mavk
tools=/mnt/e/Mavk/artifacts/sarek-native-tools-20261018/root
headers=/mnt/e/Mavk/artifacts/sarek-native-source-20261018/source/include/vulkan/include
out=${MAVK_AHB_OUTPUT:-/mnt/e/Mavk/artifacts/ahb-interop-20261023}
test ! -e "$out" || { echo 'Fresh AHB artifact directory required'; exit 20; }
mkdir -p "$out"
export LD_LIBRARY_PATH="$tools/usr/lib/x86_64-linux-gnu"
export TMPDIR=/mnt/e/Mavk/artifacts/build-temp-20261018
"$tools/usr/bin/aarch64-linux-gnu-gcc" --sysroot="$tools" -std=c11 -O2 -I "$headers" \
 "$repo/experiments/droiddeck/ahb-interop.c" "$repo/adapters/android/ahb_image.c" -ldl -lm -o "$out/ahb-interop"
sha256sum "$out/ahb-interop"
