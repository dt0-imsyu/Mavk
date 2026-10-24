#!/bin/sh
set -eu
tools=/mnt/e/Mavk/artifacts/sarek-native-tools-20261018/root
source=/mnt/e/Mavk/artifacts/sarek-native-source-20261018/source
build=/mnt/e/Mavk/artifacts/sarek-native-build1-20261018
repo=/mnt/c/Users/denis/MainProjects/win11ontab/Mavk
export PATH="$tools/usr/bin:$PATH"
export LD_LIBRARY_PATH="$tools/usr/lib/x86_64-linux-gnu"
export PYTHONPATH="$tools/usr/lib/python3/dist-packages"
export TMPDIR=/mnt/e/Mavk/artifacts/build-temp-20261018
test ! -e "$build" || { echo 'Preserve prior build;fresh build directory required';exit 20; }
python3 "$tools/usr/bin/meson" setup "$build" "$source" \
 --cross-file "$repo/experiments/droiddeck/sarek-native-aarch64.ini" \
 --buildtype release --wrap-mode nodownload -Denable_d3d8=false -Denable_d3d9=false \
 -Dnative_sdl3=enabled -Dnative_sdl2=disabled -Dnative_glfw=disabled \
 -Ddxbc-spirv:enable_tests=false -Ddxbc-spirv:enable_tools=false
ninja -C "$build" -j8
