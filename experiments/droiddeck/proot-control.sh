#!/data/data/com.termux/files/usr/bin/bash
set -euo pipefail
export PATH=/data/data/com.termux/files/usr/bin:/system/bin
p=/data/data/com.termux/files/home/mavk-droiddeck-poc-20261015
test -f "$p/root/opt/android-host/proot"
export HYBRIS_ANDROID_SDK_VERSION=36
export HYBRIS_VULKAN_HAL=/vendor/lib64/hw/vulkan.mali.so
export HYBRIS_LINKER_DIR="$p/hybris/lib/libhybris/linker"
export HYBRIS_LD_LIBRARY_PATH=/system/lib64:/system_ext/lib64:/vendor/lib64/egl:/vendor/lib64/hw:/vendor/lib64
export VK_ICD_FILENAMES="$p/icd.json"
export PROOT_LOADER="$p/root/opt/android-host/loader"
export PROOT_TMP_DIR="$p"
unset VK_INSTANCE_LAYERS VK_LAYER_PATH LD_PRELOAD || true
echo STAGE=exact_droiddeck_proot_binary_control
export LD_LIBRARY_PATH="$p/root/opt/android-host"
timeout 20 "$p/root/opt/android-host/proot" --kill-on-exit -r "$p/root" \
 -b /dev -b /proc -b /sys -b /system -b /vendor -b /apex -b /system_ext \
 -b /product -b /odm -b /linkerconfig/ld.config.txt -b "$p:$p" -w / \
 /usr/lib/ld-linux-aarch64.so.1 --library-path "$p/root/usr/lib:$p/hybris/lib" \
 "$p/probe" "$p/root/usr/lib/libvulkan.so.1" "$p/triangle.vert.spv" "$p/triangle.frag.spv"
