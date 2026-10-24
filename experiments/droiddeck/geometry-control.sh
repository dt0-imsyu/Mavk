#!/system/bin/sh
set -eu
p=/data/user/0/com.droiddeck.launcher.mavk/files
native=${MAVK_NATIVE_LIBRARY_DIR:?Resolve the installed package nativeLibraryDir before running}
export PROOT_LOADER="$native/libproot-loader.so" PROOT_TMP_DIR="$p/mavk-window"
export HOME=/root DISPLAY=:99 XAUTHORITY="$p/mavk-session/xauthority"
export BL_SYSV_DIR="$p/mavk-session/sysv"
unset LD_PRELOAD LD_LIBRARY_PATH
exec /system/bin/timeout -k 2 15 "$native/libproot.so" --kill-on-exit \
 -0 -r "$p/linuxfs" -b /dev -b /proc -b /sys \
 -b /system -b /vendor -b /apex -b /system_ext -b /product -b /odm \
 -b /linkerconfig/ld.config.txt -b "$p:$p" -b "$p/../cache/shm:/dev/shm" -w /root \
 /usr/bin/env PATH=/usr/local/bin:/usr/bin:/bin "$@"
