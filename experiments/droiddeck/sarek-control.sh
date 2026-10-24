#!/data/data/com.termux/files/usr/bin/bash
# Isolated real Sarek consumer, existing glibc/hybris runtime; no VM changes.
set -euo pipefail
export PATH=/data/data/com.termux/files/usr/bin:/system/bin
p=/data/data/com.termux/files/home/mavk-droiddeck-poc-20261015
slot=${1:-sarek-native1}
case "$slot" in sarek-native[1-9]|sarek-native[1-9][0-9]) ;; *) exit 21;; esac
test ! -e "$p/$slot"
test -f "$p/root/opt/android-host/proot"
mkdir "$p/$slot"
tar -xf /data/local/tmp/mavk-sarek-native-payload.tar -C "$p/$slot"
cd "$p/$slot"
sha256sum -c SHA256SUMS
chmod 700 sarek-d3d11-consumer
cp "$p/hybris/lib/VkLayer_hybris_compat.json" VkLayer_hybris_compat.json
export HYBRIS_ANDROID_SDK_VERSION=36
export HYBRIS_VULKAN_HAL=/vendor/lib64/hw/vulkan.mali.so
export HYBRIS_LINKER_DIR="$p/hybris/lib/libhybris/linker"
export HYBRIS_LD_LIBRARY_PATH=/system/lib64:/system_ext/lib64:/vendor/lib64/egl:/vendor/lib64/hw:/vendor/lib64
export HYBRIS_BC_TEXTURES=dxvk
export MAVK_SAREK_BC_FORMATS=1
export VK_ICD_FILENAMES="$p/icd.json"
export VK_LAYER_PATH="$p/$slot"
export VK_INSTANCE_LAYERS=VK_LAYER_HYBRIS_compat
unset MAVK_LAYER_GUARDS MAVK_TEST_BOUNDS MAVK_TEST_IMAGE_INIT MAVK_TEST_SUBMIT2 MAVK_TEST_NULL_IMAGE MAVK_TEST_SUBMIT_RETRY MAVK_SAREK_NULL_PROBE MAVK_SAREK_LEGACY_SPIRV || true
case "${2:-hybris}" in
 guarded|guarded2) test -f "$p/$slot/libVkLayer_mavk_compat.so";export VK_INSTANCE_LAYERS=VK_LAYER_MAVK_compat:VK_LAYER_HYBRIS_compat;export MAVK_LAYER_GUARDS=1 MAVK_TEST_BOUNDS=1 MAVK_TEST_IMAGE_INIT=1; if [ "$2" = guarded2 ];then export MAVK_TEST_SUBMIT2=1;fi ;;
 mavk) test -f "$p/$slot/libVkLayer_mavk_compat.so";export VK_INSTANCE_LAYERS=VK_LAYER_MAVK_compat:VK_LAYER_HYBRIS_compat;export VK_LOADER_DEBUG=layer,error,warn ;;
 hybris) ;;
 *) exit 22 ;;
esac
if [ "${3:-0}" = 1 ];then test "${MAVK_LAYER_GUARDS:-0}" = 1;export MAVK_TEST_NULL_IMAGE=1;fi
if [ "${4:-0}" = 1 ];then test "${MAVK_LAYER_GUARDS:-0}" = 1;export MAVK_TEST_SUBMIT_RETRY=1;fi
if [ "${5:-0}" = 1 ];then
 test "${MAVK_LAYER_GUARDS:-0}" = 1
 unset MAVK_TEST_BOUNDS MAVK_TEST_IMAGE_INIT MAVK_TEST_NULL_IMAGE MAVK_TEST_SUBMIT_RETRY
 export MAVK_SAREK_NULL_PROBE=1
 echo 'MAVK_DIAGNOSTIC_ONLY nullDescriptor=FALSE correctness=UNPROVEN'
fi
if [ "${6:-0}" = 1 ];then test "${MAVK_SAREK_NULL_PROBE:-0}" = 1;export MAVK_SAREK_LEGACY_SPIRV=1;fi
export DXVK_WSI_DRIVER=SDL3
export SDL_VIDEODRIVER=offscreen
export DXVK_LOG_LEVEL=debug
export DXVK_LOG_PATH="$p/$slot"
export PROOT_LOADER="$p/root/opt/android-host/loader"
export PROOT_TMP_DIR="$p"
export LD_LIBRARY_PATH="$p/root/opt/android-host"
unset LD_PRELOAD || true
set +e
timeout -k 5 60 "$p/root/opt/android-host/proot" --kill-on-exit -r "$p/root" \
 -b /dev -b /proc -b /sys -b /system -b /vendor -b /apex -b /system_ext \
 -b /product -b /odm -b /linkerconfig/ld.config.txt -b "$p:$p" -w "$p/$slot" \
 /usr/lib/ld-linux-aarch64.so.1 --library-path "$p/$slot:$p/root/usr/lib:$p/hybris/lib" \
 "$p/$slot/sarek-d3d11-consumer" "$p/$slot/libdxvk_d3d11.so" > stdout.log 2> stderr.log
status=$?
set -e
cat stdout.log stderr.log
echo "SAREK_PROCESS_EXIT=$status"
exit "$status"
