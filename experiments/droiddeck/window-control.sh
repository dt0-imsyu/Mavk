#!/system/bin/sh
set -eu
p=/data/user/0/com.droiddeck.launcher.mavk/files/mavk-window
test -S "$p/run/wayland-0"
test -s "$p/run/runtime.txt"
runtime=$(cat "$p/run/runtime.txt")
case "$runtime" in /data/user/0/com.droiddeck.launcher.mavk/files/system_vulkan/*|/data/data/com.droiddeck.launcher.mavk/files/system_vulkan/*) ;; *) exit 21;; esac
test -d "$p/payload"
mkdir -p "$p/home/cache"
export HOME="$p/home" XDG_CACHE_HOME="$p/home/cache" XDG_RUNTIME_DIR="$p/run" WAYLAND_DISPLAY=wayland-0
export SDL_VIDEODRIVER=wayland DXVK_WSI_DRIVER=SDL3 DXVK_LOG_LEVEL=debug DXVK_LOG_PATH="$p"
export HYBRIS_ANDROID_SDK_VERSION=36 HYBRIS_VULKAN_HAL=/vendor/lib64/hw/vulkan.mali.so
export HYBRIS_LINKER_DIR="$runtime/lib/libhybris/linker"
export HYBRIS_LD_LIBRARY_PATH=/system/lib64:/system_ext/lib64:/vendor/lib64/egl:/vendor/lib64/hw:/vendor/lib64
export HYBRIS_BC_TEXTURES=dxvk MAVK_SAREK_BC_FORMATS=1
export VK_ICD_FILENAMES="$runtime/icd.json" VK_LAYER_PATH="$p/payload:$runtime/lib"
export VK_INSTANCE_LAYERS=VK_LAYER_MAVK_compat:VK_LAYER_HYBRIS_compat MAVK_LAYER_GUARDS=1
export MAVK_SAREK_NULL_PROBE=1 MAVK_SAREK_LEGACY_SPIRV=1
unset DXVK_MALI_COMPAT LD_PRELOAD MAVK_TEST_BOUNDS MAVK_TEST_IMAGE_INIT MAVK_TEST_SUBMIT2 MAVK_TEST_NULL_IMAGE MAVK_TEST_SUBMIT_RETRY || true
export PROOT_LOADER="$p/root/opt/android-host/loader" PROOT_TMP_DIR="$p"
export LD_LIBRARY_PATH="$p/root/opt/android-host"
cd "$p/payload"
sha256sum -c SHA256SUMS
set +e
/system/bin/timeout -k 5 40 "$p/root/opt/android-host/proot" --kill-on-exit -r "$p/root" \
 -b /dev -b /proc -b /sys -b /system -b /vendor -b /apex -b /system_ext \
 -b /product -b /odm -b /linkerconfig/ld.config.txt \
 -b /data/user/0/com.droiddeck.launcher.mavk/files:/data/user/0/com.droiddeck.launcher.mavk/files \
 -w "$p" /usr/lib/ld-linux-aarch64.so.1 \
 --library-path "$p/payload:$p/root/usr/lib:$runtime/lib" \
 "$p/payload/sarek-d3d11-window" "$p/payload/libdxvk_d3d11.so" > "$p/stdout.log" 2> "$p/stderr.log"
status=$?
set -e
cat "$p/stdout.log"
tail -n 80 "$p/stderr.log"
echo "WINDOW_PROCESS_EXIT=$status"
exit "$status"
