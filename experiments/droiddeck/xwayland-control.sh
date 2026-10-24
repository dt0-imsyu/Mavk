#!/system/bin/sh
set -eu
p=/data/user/0/com.droiddeck.launcher.mavk/files
mkdir -p "$p/mavk-window/sysv"
export BL_SYSV_DIR="$p/mavk-window/sysv"
runtime=$(cat "$p/mavk-window/run/runtime.txt")
test -S "$p/.wayland-rt/wayland-0"
test -f "$p/linuxfs/usr/bin/Xwayland"
export HOME=/root USER=root XDG_RUNTIME_DIR="$p/.wayland-rt" WAYLAND_DISPLAY=wayland-0
if [ -f "$p/mavk-window/xauthority" ];then export XAUTHORITY="$p/mavk-window/xauthority";fi
export HYBRIS_ANDROID_SDK_VERSION=36 HYBRIS_VULKAN_HAL=/vendor/lib64/hw/vulkan.mali.so
export HYBRIS_LINKER_DIR="$runtime/lib/libhybris/linker"
export HYBRIS_LD_LIBRARY_PATH=/system/lib64:/system_ext/lib64:/vendor/lib64/egl:/vendor/lib64/hw:/vendor/lib64
export VK_ICD_FILENAMES="$runtime/icd.json" VK_LAYER_PATH="$runtime/lib"
export VK_INSTANCE_LAYERS=VK_LAYER_HYBRIS_compat
export PROOT_LOADER="$p/mavk-window/root/opt/android-host/loader" PROOT_TMP_DIR="$p/mavk-window"
export LD_LIBRARY_PATH="$p/mavk-window/root/opt/android-host"
unset LD_PRELOAD DXVK_MALI_COMPAT MAVK_LAYER_GUARDS MAVK_SAREK_NULL_PROBE MAVK_SAREK_LEGACY_SPIRV || true
proot_identity=-0
proot_uid=
case "${1:-help}" in
 run-openbox)
   proot_identity=-i;proot_uid=$(id -u):$(id -u)
   export DISPLAY=:99
   wm="$p/mavk-window/openbox/root"
   export XDG_DATA_DIRS="$wm/usr/share:/usr/share"
   set -- /usr/bin/env LD_LIBRARY_PATH="$wm/usr/lib:/usr/lib" "$wm/usr/bin/openbox" --sm-disable --config-file "$wm/etc/xdg/openbox/rc.xml"
   ;;
 help) set -- /usr/bin/Xwayland -help ;;
 wine-version) set -- '/root/.local/share/Steam/steamapps/common/Proton Experimental (ARM64)/files/bin-arm64/wine' --version ;;
 query-openbox) export DROIDDECK_MAVK_RUNTIME="$p/mavk-window"; set -- /bin/bash "$p/mavk-window/openbox-package.sh" query ;;
 plan-openbox) export DROIDDECK_MAVK_RUNTIME="$p/mavk-window"; set -- /bin/bash "$p/mavk-window/openbox-package.sh" plan ;;
 download-openbox) export DROIDDECK_MAVK_RUNTIME="$p/mavk-window"; set -- /bin/bash "$p/mavk-window/openbox-package.sh" download ;;
 refresh-openbox) export DROIDDECK_MAVK_RUNTIME="$p/mavk-window"; set -- /bin/bash "$p/mavk-window/openbox-package.sh" refresh ;;
 server-version) set -- '/root/.local/share/Steam/steamapps/common/Proton Experimental (ARM64)/files/bin-arm64/wineserver' -v ;;
 proton-check) export DROIDDECK_MAVK_RUNTIME="$p/mavk-window"; set -- /usr/bin/python3 "$p/mavk-window/run-proton.py" --check '/root/.local/share/Steam/steamapps/common/Proton Experimental (ARM64)/proton' ;;
 proton-routing-check) set -- /usr/bin/python3 "$p/mavk-window/check_proton_routing.py" "$p/mavk-window" '/root/.local/share/Steam/steamapps/common/Proton Experimental (ARM64)/proton' ;;
 run) set -- /usr/bin/Xwayland :99 -nolock -noreset -nolisten tcp -shm -geometry 1280x800 ;;
 debug-run)
   export XDG_RUNTIME_DIR="$p/mavk-window/run" DISPLAY=:98
   export DROIDDECK_SYSTEM_VULKAN_RUNTIME="$runtime"
   unset XAUTHORITY
   test -S "$XDG_RUNTIME_DIR/wayland-0"
   geometry=${MAVK_DEBUG_GEOMETRY:-1280x800}
   case "$geometry" in 1280x800|1920x1200) ;; *) exit 22 ;; esac
   set -- /usr/bin/Xwayland :98 -nolock -noreset -nolisten tcp -shm -geometry "$geometry"
   ;;
 debug-bench)
   proot_identity=-i;proot_uid=$(id -u):$(id -u)
   export XDG_RUNTIME_DIR="$p/mavk-window/run" DISPLAY=:98
   export DROIDDECK_SYSTEM_VULKAN_RUNTIME="$runtime"
   unset XAUTHORITY
   test -S "$XDG_RUNTIME_DIR/wayland-0"
   set -- /usr/bin/python3 "$p/mavk-glmark2/debug-run.py"
   ;;
 owner-test) export MAVK_X11_SYSV_OWNER=$(id -u); set -- /usr/bin/env LD_PRELOAD="$p/mavk-window/pe/libmavk-x11-sysv-owner.so" /usr/bin/Xwayland :99 -nolock -noreset -nolisten tcp -shm -geometry 1280x800 ;;
 snapshot) export DISPLAY=:99; set -- /usr/bin/python3 "$p/mavk-window/x11-snapshot.py" "$p/mavk-window/x11-root.ppm" ;;
 focus-check) export DISPLAY=:99; set -- /usr/bin/python3 "$p/mavk-window/x11-snapshot.py" "$p/mavk-window/x11-root.ppm" focus-largest ;;
 *) exit 22 ;;
esac
exec /system/bin/timeout -k 3 300 "$p/mavk-window/root/opt/android-host/proot" --kill-on-exit \
 "$proot_identity" ${proot_uid:+"$proot_uid"} -r "$p/linuxfs" -b /dev -b /proc -b /sys -b /system -b /vendor -b /apex -b /system_ext \
 -b /product -b /odm -b /linkerconfig/ld.config.txt -b "$p:$p" -w /root \
 /usr/bin/env PATH=/usr/local/bin:/usr/bin:/bin "$@"
