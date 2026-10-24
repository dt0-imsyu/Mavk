#!/bin/sh
set -eu
tools=/mnt/e/Mavk/artifacts/sarek-native-tools-20261018/root
inputs=/mnt/e/Mavk/artifacts/wine-arm64-inputs-20261018
source=/mnt/e/Mavk/artifacts/sarek-native-source-20261018/source
build=/mnt/e/Mavk/artifacts/sarek-pe-arm64x-build1-20261018
repo=/mnt/c/Users/denis/MainProjects/win11ontab/Mavk
test ! -e "$build"
test "$(sha256sum "$inputs/llvm-mingw-20261006.tar.xz" | cut -d' ' -f1)" = 5f9c6ed95b2d4bdb2869a488c5fd5857fbdabcf288a0aa3eb1da43f6a08d8ab4
test ! -e "$inputs/llvm-mingw"
mkdir "$inputs/llvm-mingw"
tar -xf "$inputs/llvm-mingw-20261006.tar.xz" -C "$inputs/llvm-mingw" --strip-components=1
export PATH="$inputs/llvm-mingw/bin:$tools/usr/bin:/usr/bin:/bin"
export LD_LIBRARY_PATH="$tools/usr/lib/x86_64-linux-gnu"
export PYTHONPATH="$tools/usr/lib/python3/dist-packages"
export TMPDIR=/mnt/e/Mavk/artifacts/build-temp-20261018
python3 "$tools/usr/bin/meson" setup "$build" "$source" \
 --cross-file "$repo/experiments/droiddeck/sarek-pe-aarch64.ini" \
 --buildtype release --wrap-mode nodownload -Denable_d3d8=false -Denable_d3d9=false \
 -Dnative_sdl3=disabled -Dnative_sdl2=disabled -Dnative_glfw=disabled \
 -Ddxbc-spirv:enable_tests=false -Ddxbc-spirv:enable_tools=false
ninja -C "$build" -j8
