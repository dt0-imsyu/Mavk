#!/bin/sh
# Non-installing tool extraction; no packages or VM data on C are modified.
set -eu
work=/mnt/e/Mavk/artifacts/glibc-cmake-tools1-20261018
test ! -e "$work" || { echo 'Fresh tools directory required';exit 20; }
mkdir -p "$work/packages" "$work/root"
cd "$work/packages"
apt-get -s --no-install-recommends install cmake > "$work/apt-plan.txt"
packages=$(sed -n 's/^Inst \([^ ]*\).*/\1/p' "$work/apt-plan.txt")
test -n "$packages"
apt-get download $packages
for archive in ./*.deb;do dpkg-deb -x "$archive" "$work/root";done
sha256sum ./*.deb > "$work/package-sha256.txt"
export LD_LIBRARY_PATH="$work/root/usr/lib/x86_64-linux-gnu:/mnt/e/Mavk/artifacts/sarek-native-tools-20261018/root/usr/lib/x86_64-linux-gnu"
"$work/root/usr/bin/cmake" --version
