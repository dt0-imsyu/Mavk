#!/bin/sh
# Local build tools only: download/extract to healthy E,never apt install.
set -eu
work=/mnt/e/Mavk/artifacts/sarek-native-tools-20261018
test ! -e "$work" || { echo 'Fresh tool directory required'; exit 20; }
mkdir -p "$work/packages" "$work/root"
cd "$work/packages"
apt-get -s --no-install-recommends install g++-aarch64-linux-gnu meson ninja-build glslang-tools pkgconf-bin > "$work/apt-plan.txt"
packages=$(sed -n 's/^Inst \([^ ]*\).*/\1/p' "$work/apt-plan.txt")
test -n "$packages"
# apt-get download is non-installing and runs as the ordinary WSL build user.
apt-get download $packages
for archive in ./*.deb; do dpkg-deb -x "$archive" "$work/root"; done
sha256sum ./*.deb > "$work/package-sha256.txt"
export LD_LIBRARY_PATH="$work/root/usr/lib/x86_64-linux-gnu"
export PYTHONPATH="$work/root/usr/lib/python3/dist-packages"
"$work/root/usr/bin/aarch64-linux-gnu-g++" --version | head -1
"$work/root/usr/bin/aarch64-linux-gnu-g++" -print-search-dirs
python3 "$work/root/usr/bin/meson" --version
"$work/root/usr/bin/ninja" --version
