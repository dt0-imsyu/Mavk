#!/bin/sh
set -eu
tools=/mnt/e/Mavk/artifacts/sarek-native-tools-20261018/root
source=/mnt/e/Mavk/artifacts/sarek-native-source-20261018/source
out=${MAVK_CONSUMER_OUTPUT:-/mnt/e/Mavk/artifacts/sarek-native-consumer6-20261018}
repo=/mnt/c/Users/denis/MainProjects/win11ontab/Mavk
test ! -e "$out" || { echo 'Fresh consumer output required';exit 20; }
mkdir -p "$out"
export LD_LIBRARY_PATH="$tools/usr/lib/x86_64-linux-gnu"
export TMPDIR=/mnt/e/Mavk/artifacts/build-temp-20261018
"$tools/usr/bin/aarch64-linux-gnu-g++" --sysroot="$tools" -std=c++17 -O2 \
 -I "$source/include/native/windows" -I "$source/include/native/directx" -I "$source/include/vulkan/include" \
 -I /mnt/e/Mavk/artifacts/sarek-native-build1-20261018/src/d3d11/libdxvk_d3d11.so.0.30002.p \
 -I /mnt/e/Mavk/artifacts/sarek-d3d11-shaders1-20261018 \
 "$repo/experiments/droiddeck/sarek-d3d11-consumer.cpp" -static-libstdc++ -static-libgcc \
 -ldl -o "$out/sarek-d3d11-consumer"
"$tools/usr/bin/aarch64-linux-gnu-readelf" -d "$out/sarek-d3d11-consumer"
