#!/bin/sh
set -eu
tools=/mnt/e/Mavk/artifacts/sarek-native-tools-20261018/root
inputs=/mnt/e/Mavk/artifacts/wine-arm64-inputs-20261018
repo=/mnt/c/Users/denis/MainProjects/win11ontab/Mavk
out=${MAVK_PE_WINDOW_OUTPUT:?fresh output directory required}
test ! -e "$out"
mkdir -p "$out"
export LD_LIBRARY_PATH="$tools/usr/lib/x86_64-linux-gnu"
for arch in aarch64 x86_64; do
  "$inputs/llvm-mingw/bin/$arch-w64-mingw32-clang++" -std=c++17 -O2 \
    -I /mnt/e/Mavk/artifacts/sarek-d3d11-shaders1-20261018 \
    "$repo/experiments/droiddeck/sarek-d3d11-window.cpp" \
    -static -Wl,--subsystem,windows -Wl,--entry,mainCRTStartup -luser32 \
    -o "$out/mavk-d3d11-$arch.exe"
done
sha256sum "$out"/*.exe
