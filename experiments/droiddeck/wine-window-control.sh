#!/system/bin/sh
set -eu
p=/data/user/0/com.droiddeck.launcher.mavk/files
mkdir -p "$p/mavk-window/sysv"
export BL_SYSV_DIR="$p/mavk-window/sysv"
uid=$(id -u)
case "${1:-run}" in
 run|stop) trace= ;;
 trace) trace="$p/mavk-window/pe/libmavk-wine-protection-trace.so" ;;
 read-fallback) trace="$p/mavk-window/pe/libmavk-wine-protection-trace.so"; export MAVK_WINE_PE_READ_FALLBACK=1 ;;
 *) exit 22 ;;
esac
native=$(cat "$p/mavk-window/run/native-library-dir.txt")
runtime=$(cat "$p/mavk-window/run/runtime.txt")
if [ "${1:-run}" != stop ];then test -S "$p/linuxfs/tmp/.X11-unix/X99";fi
test -s "$p/mavk-window/pe/d3d11.dll"
test -s "$p/mavk-window/pe/dxgi.dll"
export HOME=/root USER=android DISPLAY=:99 XDG_RUNTIME_DIR="$p/.wayland-rt" WAYLAND_DISPLAY=wayland-0
export WINEPREFIX=/root/.mavk-wine-poc WINEARCH=win64 WINEDEBUG=+timestamp,+vulkan,+module
export VK_LOADER_DEBUG=error,warn,layer
export WINEESYNC=0 WINEFSYNC=0
export WINESERVER='/root/.local/share/Steam/steamapps/common/Proton Experimental (ARM64)/files/bin-arm64/wineserver'
export WINELOADER='/root/.local/share/Steam/steamapps/common/Proton Experimental (ARM64)/files/bin-arm64/wine'
export WINEDLLOVERRIDES='mscoree,mshtml=;d3d11,dxgi=n'
export HYBRIS_ANDROID_SDK_VERSION=36 HYBRIS_VULKAN_HAL=/vendor/lib64/hw/vulkan.mali.so
export HYBRIS_LINKER_DIR="$runtime/lib/libhybris/linker"
export HYBRIS_LD_LIBRARY_PATH=/system/lib64:/system_ext/lib64:/vendor/lib64/egl:/vendor/lib64/hw:/vendor/lib64
export HYBRIS_BC_TEXTURES=dxvk MAVK_SAREK_BC_FORMATS=1
export VK_ICD_FILENAMES="$runtime/icd.json" VK_LAYER_PATH="$runtime/wsi:$p/mavk-window/payload:$runtime/lib"
export VK_INSTANCE_LAYERS=VK_LAYER_window_system_integration:VK_LAYER_MAVK_compat:VK_LAYER_HYBRIS_compat
export WSI_X11_FORCE_SHM=1 ENABLE_GAMESCOPE_WSI=0 VK_LOADER_LAYERS_DISABLE='*gamescope*'
export MAVK_LAYER_GUARDS=1 MAVK_SAREK_NULL_PROBE=1 MAVK_SAREK_LEGACY_SPIRV=1
export DXVK_LOG_LEVEL=debug DXVK_LOG_PATH="$p/mavk-window/pe"
export PROOT_LOADER="$native/libproot-loader.so" PROOT_TMP_DIR="$p/mavk-window"
unset LD_LIBRARY_PATH
unset LD_PRELOAD DXVK_MALI_COMPAT || true
if [ "${1:-run}" = stop ];then program="$WINESERVER";set -- -k
else program="$WINELOADER";set -- 'Z:\data\user\0\com.droiddeck.launcher.mavk\files\mavk-window\pe\mavk-d3d11-window.exe' d3d11.dll;fi
exec /system/bin/timeout -k 5 90 "$native/libproot.so" --kill-on-exit \
 -i "$uid:$uid" -r "$p/linuxfs" -b /dev -b /proc -b /sys -b /system -b /vendor -b /apex -b /system_ext \
 -b /product -b /odm -b /linkerconfig/ld.config.txt -b "$p:$p" -b "$p/../cache/shm:/dev/shm" \
 -w "$p/mavk-window/pe" /usr/bin/env PATH=/usr/local/bin:/usr/bin:/bin \
 LD_LIBRARY_PATH="$p/mavk-window/payload:/usr/lib:$runtime/lib" LD_PRELOAD="$trace" \
 "$program" "$@"
