#!/bin/sh
set -eu
out=${MAVK_WINDOW_ROOT:?fresh output required}
archive=/mnt/e/Mavk/artifacts/droiddeck-linuxfs-r9.tar.zst
zstd=/mnt/e/Mavk/artifacts/droiddeck-build-tools-20261018/root/usr/bin/zstd
test ! -e "$out"
test "$(sha256sum "$archive" | cut -d' ' -f1)" = d3240c1531531615b0dd06b6ad56e92db9015a8641eb18c3a613835ae45992e7
mkdir -p "$out/root"
tar --use-compress-program="$zstd" -xf "$archive" -C "$out/root" --wildcards \
 './usr/lib/libc.so.6' './usr/lib/libm.so.6' './usr/lib/ld-linux-aarch64.so.1' \
 './usr/lib/libdl.so.2' './usr/lib/libpthread.so.0' './usr/lib/librt.so.1' \
 './usr/lib/libstdc++.so.6*' './usr/lib/libgcc_s.so.1' './usr/lib/libxcb.so*' \
 './usr/lib/libX11.so*' './usr/lib/libX11-xcb.so*' './usr/lib/libXau.so*' \
 './usr/lib/libXdmcp.so*' './usr/lib/libwayland*.so*' './usr/lib/libffi.so*' \
 './usr/lib/libexpat.so*' './usr/lib/libvulkan.so*' './usr/lib/libzstd.so*' \
 './usr/lib/libxshmfence.so*' './usr/lib/libxkbcommon.so*' './opt/android-host/*'
tar -cf "$out/window-root.tar" -C "$out" root
du -sh "$out/root"
