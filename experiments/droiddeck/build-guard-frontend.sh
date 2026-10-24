#!/bin/sh
set -eu
repo=/mnt/c/Users/denis/MainProjects/win11ontab/Mavk
tools=/mnt/e/Mavk/artifacts/sarek-native-tools-20261018/root
spirv=/mnt/e/Mavk/research/spirv-tools-v2022.4
pinned=/mnt/e/Mavk/artifacts/shader-guards-glibc1-20261018
out=${MAVK_FRONTEND_OUTPUT:?Fresh frontend output required}
test ! -e "$out" || exit 20
mkdir -p "$out"
export LD_LIBRARY_PATH="$tools/usr/lib/x86_64-linux-gnu"
export TMPDIR=/mnt/e/Mavk/artifacts/build-temp-20261018
probe_flags=
if test "${MAVK_TEST_TEXEL_COMPONENT_PROBE:-0}" = 1; then probe_flags=-DMAVK_TEST_TEXEL_COMPONENT_PROBE; fi
for unit in guards static_use; do
 "$tools/usr/bin/aarch64-linux-gnu-g++" --sysroot="$tools" -std=c++17 -O2 -fPIC $probe_flags \
  -I "$repo/compatibility/include" -I "$spirv/include" -I "$spirv/external/spirv-headers/include" \
  -I "$spirv" -I "$pinned/spirv-tools" -c "$repo/compatibility/shaders/$unit.cpp" -o "$out/$unit.o"
done
"$tools/usr/bin/aarch64-linux-gnu-ar" rcs "$out/libmavk-shader-guards.a" "$out/guards.o" "$out/static_use.o"
"$tools/usr/bin/aarch64-linux-gnu-g++" --sysroot="$tools" -std=c++17 -O2 \
 -I "$repo/compatibility/include" -I "$spirv/include" \
 "$repo/compatibility/test/static-use.cpp" "$out/libmavk-shader-guards.a" \
 "$pinned/spirv-tools/source/opt/libSPIRV-Tools-opt.a" "$pinned/spirv-tools/source/libSPIRV-Tools.a" \
 -o "$out/static-use-test"
sha256sum "$out/libmavk-shader-guards.a" "$out/static-use-test"
