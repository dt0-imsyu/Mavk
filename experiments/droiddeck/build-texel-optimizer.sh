#!/bin/sh
set -eu
tools=/mnt/e/Mavk/artifacts/sarek-native-tools-20261018/root
spirv=/mnt/e/Mavk/research/spirv-tools-v2022.4
pinned=/mnt/e/Mavk/artifacts/shader-guards-glibc1-20261018
out=${MAVK_OPT_OUTPUT:?Fresh optimizer output required}
test ! -e "$out" || exit 20
mkdir -p "$out"
export LD_LIBRARY_PATH="$tools/usr/lib/x86_64-linux-gnu"
export TMPDIR=/mnt/e/Mavk/artifacts/build-temp-20261018
"$tools/usr/bin/aarch64-linux-gnu-g++" --sysroot="$tools" -O3 -DNDEBUG -std=c++11 -fPIC -fno-exceptions \
 -DSPIRV_COLOR_TERMINAL -DSPIRV_LINUX -DSPIRV_TIMER_ENABLED \
 -I "$spirv" -I "$spirv/include" -I "$spirv/external/spirv-headers/include" -I "$pinned/spirv-tools" \
 -c "$spirv/source/opt/inst_bindless_check_pass.cpp" -o "$out/inst_bindless_check_pass.cpp.o"
cp "$pinned/spirv-tools/source/opt/libSPIRV-Tools-opt.a" "$out/libSPIRV-Tools-opt.a"
"$tools/usr/bin/aarch64-linux-gnu-ar" r "$out/libSPIRV-Tools-opt.a" "$out/inst_bindless_check_pass.cpp.o"
sha256sum "$out/libSPIRV-Tools-opt.a" "$spirv/source/opt/inst_bindless_check_pass.cpp"
