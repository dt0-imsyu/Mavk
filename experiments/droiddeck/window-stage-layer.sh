#!/system/bin/sh
set -eu
p=/data/user/0/com.droiddeck.launcher.mavk/files/mavk-window
test "$#" -eq 1
case "$1" in /data/local/tmp/mavk-layer*.so) ;; *) exit 21;; esac
test -f "$1"
cd "$p/payload"
cp "$1" libVkLayer_mavk_compat.so
sha256sum libdxvk_d3d11.so libdxvk_dxgi.so.0 libSDL3.so.0 libVkLayer_hybris_compat.so libVkLayer_mavk_compat.so sarek-d3d11-consumer sarek-d3d11-window Sarek-LICENSE.txt SPIRV-Headers-LICENSE.txt SPIRV-Tools-LICENSE.txt VkLayer_mavk_compat.json VkLayer_hybris_compat.json > SHA256SUMS
