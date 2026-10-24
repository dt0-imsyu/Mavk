#!/bin/sh
set -eu
repo=/mnt/c/Users/denis/MainProjects/win11ontab/Mavk
tools=/mnt/e/Mavk/artifacts/sarek-native-tools-20261018/root
cmake=/mnt/e/Mavk/artifacts/glibc-cmake-tools1-20261018/root
out=/mnt/e/Mavk/artifacts/shader-guards-glibc1-20261018
test ! -e "$out" || { echo 'Fresh compiler build required';exit 20; }
export PATH="$tools/usr/bin:/usr/bin:/bin"
export LD_LIBRARY_PATH="$cmake/usr/lib/x86_64-linux-gnu:$tools/usr/lib/x86_64-linux-gnu"
export TMPDIR=/mnt/e/Mavk/artifacts/build-temp-20261018
"$cmake/usr/bin/cmake" -S "$repo/compatibility/shaders" -B "$out" -G Ninja \
 -DCMAKE_TOOLCHAIN_FILE="$repo/experiments/droiddeck/glibc-aarch64.cmake" \
 -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.10 \
 -DMAVK_SPIRV_TOOLS_SOURCE=/mnt/e/Mavk/research/spirv-tools-v2022.4
"$cmake/usr/bin/cmake" --build "$out" --target mavk-shader-guards mavk-shader-tool mavk-shader-guards-test -j8
