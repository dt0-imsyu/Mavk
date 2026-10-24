#!/bin/sh
set -eu
base=/mnt/e/Mavk/artifacts/zink-ui-25.1.9-20261019
tools=/mnt/e/Mavk/artifacts/sarek-native-tools-20261018/root
host=$base/host-tools/root
repo=/mnt/c/Users/denis/MainProjects/win11ontab/Mavk
export PATH="$host/usr/bin:$tools/usr/bin:/usr/bin:/bin"
export LD_LIBRARY_PATH="$tools/usr/lib/x86_64-linux-gnu:$host/usr/lib/x86_64-linux-gnu"
export PYTHONPATH="$host/usr/lib/python3/dist-packages:$tools/usr/lib/python3/dist-packages"
export BISON_PKGDATADIR="$host/usr/share/bison"
export M4="$host/usr/bin/m4"
export TMPDIR=/mnt/e/Mavk/artifacts/build-temp-20261018
grep -q MAVK_EMULATED_CALL_ONCE "$base/mesa-25.1.9/src/c11/threads.h" || {
  echo 'Apply experiments/droiddeck/zink-ui-glibc243.patch to the pinned source first.' >&2
  exit 20
}
if [ ! -f "$base/build/build.ninja" ]; then
  python3 "$tools/usr/bin/meson" setup "$base/build" "$base/mesa-25.1.9" \
    --cross-file "$repo/experiments/droiddeck/zink-ui-aarch64.ini" \
    --buildtype release --wrap-mode nodownload --prefix /opt/mavk-zink-ui --libdir lib \
    -Dplatforms=x11 -Dgallium-drivers=zink -Dvulkan-drivers= -Dllvm=disabled \
    -Dglx=dri -Degl=enabled -Dgbm=disabled -Dgles1=disabled -Dgles2=enabled \
    -Dgallium-va=disabled -Dgallium-vdpau=disabled -Dgallium-xa=disabled \
    -Dvideo-codecs= -Dbuild-tests=false -Dvalgrind=disabled -Dlibunwind=disabled \
    -Dlmsensors=disabled -Dandroid-libbacktrace=disabled -Dglvnd=disabled
fi
ninja -C "$base/build" -j8
DESTDIR="$base/stage" python3 "$tools/usr/bin/meson" install -C "$base/build" --no-rebuild
