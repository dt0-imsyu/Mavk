#!/bin/bash
set -eu
work=${DROIDDECK_MAVK_RUNTIME:?}/openbox
mkdir -p "$work"
printf 'Server = https://ca.us.mirror.archlinuxarm.org/$arch/$repo\nServer = https://de3.mirror.archlinuxarm.org/$arch/$repo\n' > "$work/mirrorlist"
sed -e '/^DownloadUser[[:space:]]*=/d' -e "s#^Include *= */etc/pacman.d/mirrorlist#Include = $work/mirrorlist#" /etc/pacman.conf > "$work/pacman.conf"
db=/var/lib/pacman
if [ -f "$work/db/sync/extra.db" ];then db="$work/db";fi
case "${1:-query}" in
 refresh) mkdir -p "$work/db"; exec pacman --config "$work/pacman.conf" --gpgdir "$work/gnupg" --dbpath "$work/db" --noconfirm -Sy ;;
 query) exec pacman --config "$work/pacman.conf" -Si openbox ;;
 plan) exec pacman --config "$work/pacman.conf" -Sp --needed --print-format '%n %v %s' openbox ;;
 download)
   mkdir -p "$work/pkg" "$work/root"
   if [ ! -f "$work/gnupg/pubring.gpg" ] && [ ! -f "$work/gnupg/pubring.kbx" ];then
     pacman-key --gpgdir "$work/gnupg" --init
     pacman-key --gpgdir "$work/gnupg" --populate archlinuxarm
   fi
   pacman --config "$work/pacman.conf" --gpgdir "$work/gnupg" --dbpath "$db" --cachedir "$work/pkg" --noconfirm -Sw -dd openbox startup-notification imlib2 xcb-util
   files=("$work"/pkg/*.pkg.tar.xz "$work"/pkg/*.pkg.tar.zst)
   for file in "${files[@]}";do
     [ -f "$file" ] || continue
     if bsdtar -tf "$file" | grep -Eq '(^/|(^|/)\.\.(/|$))';then exit 23;fi
     bsdtar -xf "$file" --no-same-permissions -C "$work/root" --exclude .PKGINFO --exclude .BUILDINFO --exclude .MTREE --exclude .INSTALL \
       --exclude usr/lib/libobrender.so --exclude usr/lib/libobrender.so.32 --exclude usr/lib/libobt.so --exclude usr/lib/libobt.so.2 \
       --exclude usr/lib/libImlib2.so --exclude usr/lib/libImlib2.so.1 --exclude usr/lib/libstartup-notification-1.so --exclude usr/lib/libstartup-notification-1.so.0 \
       --exclude usr/lib/libxcb-util.so --exclude usr/lib/libxcb-util.so.1
   done
   test -f "$work/root/usr/lib/libobrender.so.32.0.0"
   test -f "$work/root/usr/lib/libobt.so.2.0.2"
   ln -sfn libobrender.so.32.0.0 "$work/root/usr/lib/libobrender.so.32"
   ln -sfn libobt.so.2.0.2 "$work/root/usr/lib/libobt.so.2"
   test -f "$work/root/usr/lib/libImlib2.so.1.12.7"
   test -f "$work/root/usr/lib/libstartup-notification-1.so.0.0.0"
   ln -sfn libImlib2.so.1.12.7 "$work/root/usr/lib/libImlib2.so.1"
   ln -sfn libstartup-notification-1.so.0.0.0 "$work/root/usr/lib/libstartup-notification-1.so.0"
   test -f "$work/root/usr/lib/libxcb-util.so.1.0.0"
   ln -sfn libxcb-util.so.1.0.0 "$work/root/usr/lib/libxcb-util.so.1"
   LD_LIBRARY_PATH="$work/root/usr/lib:/usr/lib" ldd "$work/root/usr/bin/openbox"
   ;;
 *) exit 22 ;;
esac
