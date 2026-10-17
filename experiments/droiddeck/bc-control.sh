#!/data/data/com.termux/files/usr/bin/bash
set -euo pipefail
export PATH=/data/data/com.termux/files/usr/bin:/system/bin
p=/data/data/com.termux/files/home/mavk-droiddeck-poc-20261015
case "${1:-baseline}" in
 baseline) slot=bc-api1 ;;
 compat) slot=bc-api2 ;;
 rebuilt) slot=bc-api3 ;;
 rg32f) slot=bc-api4 ;;
 linear) slot=bc-api5 ;;
 *) exit 21 ;;
esac
test -f "$p/root/opt/android-host/proot"
test ! -e "$p/$slot"
test "$(sha256sum /data/local/tmp/mavk-bc-api1-probe | cut -d' ' -f1)" = c26215035b04b7365208ece305bd2d432e9bb8993e2767876576a52ad93d5cbf
test "$(sha256sum /data/local/tmp/mavk-bc-api1.comp.spv | cut -d' ' -f1)" = ab7ab2423123881047e7acfc40fb76faef0541c02ce73aaa2acf6b0cc8813578
mkdir "$p/$slot"
mode=bc
if test "$slot" = bc-api5; then
 test "$(sha256sum /data/local/tmp/mavk-bc5-linear-probe | cut -d' ' -f1)" = 8a9ac482d4525541024b45d27b9859f5096ed767ed1dde32220523adc7c0cf7e
 test "$(sha256sum /data/local/tmp/mavk-bc5-linear.comp.spv | cut -d' ' -f1)" = 7e45614e3f1e18a3b53bfe65cf0b02a8b786e765f9f7a9cdf643faec97077911
 cp /data/local/tmp/mavk-bc5-linear-probe "$p/$slot/probe"
 cp /data/local/tmp/mavk-bc5-linear.comp.spv "$p/$slot/sample.comp.spv"
 mode=bc-linear
else
 cp /data/local/tmp/mavk-bc-api1-probe "$p/$slot/probe"
 cp /data/local/tmp/mavk-bc-api1.comp.spv "$p/$slot/sample.comp.spv"
fi
chmod 700 "$p/$slot/probe"
export HYBRIS_ANDROID_SDK_VERSION=36
export HYBRIS_VULKAN_HAL=/vendor/lib64/hw/vulkan.mali.so
export HYBRIS_LINKER_DIR="$p/hybris/lib/libhybris/linker"
export HYBRIS_LD_LIBRARY_PATH=/system/lib64:/system_ext/lib64:/vendor/lib64/egl:/vendor/lib64/hw:/vendor/lib64
export HYBRIS_BC_TEXTURES=dxvk
export VK_ICD_FILENAMES="$p/icd.json"
export PROOT_LOADER="$p/root/opt/android-host/loader"
export PROOT_TMP_DIR="$p"
unset VK_INSTANCE_LAYERS VK_LAYER_PATH LD_PRELOAD || true
if test "$slot" != bc-api1; then
 export VK_LAYER_PATH="$p/hybris/lib"
 export VK_INSTANCE_LAYERS=VK_LAYER_HYBRIS_compat
fi
case "$slot" in
 bc-api3)
  test "$(sha256sum /data/local/tmp/mavk-bc5-baseline.so | cut -d' ' -f1)" = 5dcd297cb2ed3017b402fac39144f8abd0881c0e6353368c519b06b11b44cc12
  cp /data/local/tmp/mavk-bc5-baseline.so "$p/$slot/libVkLayer_hybris_compat.so"
  ;;
 bc-api4|bc-api5)
  test "$(sha256sum /data/local/tmp/mavk-bc5-rg32f.so | cut -d' ' -f1)" = 179870fcf9e1cdfe0495d829df547b53bd950c63e2981ff162b53a4d02687654
  cp /data/local/tmp/mavk-bc5-rg32f.so "$p/$slot/libVkLayer_hybris_compat.so"
  ;;
esac
if test "$slot" = bc-api3 || test "$slot" = bc-api4 || test "$slot" = bc-api5; then
 cp "$p/hybris/lib/VkLayer_hybris_compat.json" "$p/$slot/VkLayer_hybris_compat.json"
 export VK_LAYER_PATH="$p/$slot"
fi
export LD_LIBRARY_PATH="$p/root/opt/android-host"
echo STAGE=bc_compressed_image_api
timeout 60 "$p/root/opt/android-host/proot" --kill-on-exit -r "$p/root" \
 -b /dev -b /proc -b /sys -b /system -b /vendor -b /apex -b /system_ext \
 -b /product -b /odm -b /linkerconfig/ld.config.txt -b "$p:$p" -w / \
 /usr/lib/ld-linux-aarch64.so.1 --library-path "$p/root/usr/lib:$p/hybris/lib" \
 "$p/$slot/probe" "$p/root/usr/lib/libvulkan.so.1" "$p/$slot/sample.comp.spv" unused "$mode"
