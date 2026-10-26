#!/bin/sh
# Build the existing Sarek compatibility source for PE32, without modifying it.
set -eu
repo=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
out=${MAVK_SAREK_OUTPUT:-"$repo/artifacts-local/sarek-i386-20261025"}
tools=${MAVK_CROSS_TOOLS:-/mnt/e/Mavk/artifacts/sarek-native-tools-20261018/root}
compiler=${MAVK_MINGW_BIN:-/mnt/e/Mavk/artifacts/wine-arm64-inputs-20261018/llvm-mingw/bin}
source=${MAVK_SAREK_SOURCE:-/mnt/e/Mavk/artifacts/sarek-native-source-20261018/source}
mkdir -p "$out/tmp"
test -d "$out/source" || cp -a "$source" "$out/source"
export PATH="$compiler:$tools/usr/bin:/usr/bin:/bin"
export LD_LIBRARY_PATH="$tools/usr/lib/x86_64-linux-gnu"
export PYTHONPATH="$tools/usr/lib/python3/dist-packages"
export TMPDIR="$out/tmp"
cat > "$out/i386.ini" <<EOF
[binaries]
c = '$compiler/i686-w64-mingw32-clang'
cpp = '$compiler/i686-w64-mingw32-clang++'
ar = '$compiler/i686-w64-mingw32-ar'
strip = '$compiler/i686-w64-mingw32-strip'
windres = '$compiler/i686-w64-mingw32-windres'
[properties]
needs_exe_wrapper = true
[host_machine]
system = 'windows'
cpu_family = 'x86'
cpu = 'i686'
endian = 'little'
EOF
if test ! -e "$out/build/build.ninja"; then
 python3 "$tools/usr/bin/meson" setup "$out/build" "$out/source" \
  --cross-file "$out/i386.ini" --buildtype release --wrap-mode nodownload \
  -Denable_d3d8=false -Denable_d3d10=false -Denable_d3d11=false \
  -Dnative_sdl3=disabled -Dnative_sdl2=disabled -Dnative_glfw=disabled \
  -Ddxbc-spirv:enable_tests=false -Ddxbc-spirv:enable_tools=false
fi
ninja -C "$out/build" -j6
