#!/bin/sh
set -eu
tools=/mnt/e/Mavk/artifacts/sarek-native-tools-20261018/root
source=/mnt/e/Mavk/artifacts/sarek-native-source-20261018/source
sdk=/mnt/e/Mavk/artifacts/sarek-native-source-20261018/sdk
repo=/mnt/c/Users/denis/MainProjects/win11ontab/Mavk
out=${MAVK_WINDOW_OUTPUT:?fresh output directory required}
test ! -e "$out"
mkdir -p "$out"
export LD_LIBRARY_PATH="$tools/usr/lib/x86_64-linux-gnu"
export TMPDIR=/mnt/e/Mavk/artifacts/build-temp-20261018
"$tools/usr/bin/aarch64-linux-gnu-g++" --sysroot="$tools" -std=c++17 -O2 \
 -I "$source/include/native/windows" -I "$source/include/native/directx" -I "$sdk/usr/include" \
 -I /mnt/e/Mavk/artifacts/sarek-d3d11-shaders1-20261018 \
 "$repo/experiments/droiddeck/sarek-d3d11-window.cpp" "$sdk/usr/lib/libSDL3.so" \
 -static-libstdc++ -static-libgcc -ldl -o "$out/sarek-d3d11-window"
