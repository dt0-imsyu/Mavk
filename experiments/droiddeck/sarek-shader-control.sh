#!/data/data/com.termux/files/usr/bin/bash
# One common compiler ABI/admission regression using actual Sarek shader input.
set -euo pipefail
export PATH=/data/data/com.termux/files/usr/bin:/system/bin
p=/data/data/com.termux/files/home/mavk-droiddeck-poc-20261015
slot=sarek-shader1
test ! -e "$p/$slot"
mkdir "$p/$slot"
tar -xf /data/local/tmp/mavk-sarek-shader-payload.tar -C "$p/$slot"
cd "$p/$slot"
sha256sum -c SHA256SUMS
chmod 700 mavk-shader-tool
export PROOT_LOADER="$p/root/opt/android-host/loader"
export PROOT_TMP_DIR="$p"
export LD_LIBRARY_PATH="$p/root/opt/android-host"
unset LD_PRELOAD VK_INSTANCE_LAYERS || true
runtime(){
 timeout 30 "$p/root/opt/android-host/proot" --kill-on-exit -r "$p/root" \
  -b /dev -b /proc -b /sys -b "$p:$p" -w "$p/$slot" \
  /usr/lib/ld-linux-aarch64.so.1 --library-path "$p/root/usr/lib" "$@"
}
if runtime "$p/$slot/mavk-shader-tool" "$p/$slot/sarek-video-blit.frag.spv" "$p/$slot/rejected.spv" > negative.log 2>&1;then
 echo 'FAIL: Vulkan1.1 unexpectedly accepted Sarek SPIR-V1.6';exit 31
fi
cat negative.log
test ! -e rejected.spv
runtime "$p/$slot/mavk-shader-tool" "$p/$slot/sarek-video-blit.frag.spv" "$p/$slot/guarded.spv" --vulkan13
sha256sum sarek-video-blit.frag.spv guarded.spv
echo GLIBC_SAREK_SHADER13_GUARD_COMPILE_PASS
