#!/system/bin/sh
set -eu
p=/data/user/0/com.droiddeck.launcher.mavk/files/mavk-window
export PROOT_LOADER="$p/root/opt/android-host/loader" PROOT_TMP_DIR="$p"
export LD_LIBRARY_PATH="$p/root/opt/android-host"
exec "$p/root/opt/android-host/proot" --kill-on-exit -r "$p/root" \
 -b /dev -b /proc -b /data/user/0/com.droiddeck.launcher.mavk/files:/data/user/0/com.droiddeck.launcher.mavk/files \
 -w "$p" /usr/lib/ld-linux-aarch64.so.1 --library-path "$p/root/usr/lib" \
 "$p/reflection-test" "$p/reflection.spv"
