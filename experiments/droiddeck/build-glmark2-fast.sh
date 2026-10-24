#!/bin/sh
set -eu
repo=/mnt/c/Users/denis/MainProjects/win11ontab/Mavk
out=/mnt/e/Mavk/artifacts/glmark2-20261023
task_dir=$(mktemp -d /tmp/mavk-glmark2.XXXXXX)
mkdir "$task_dir/tools" "$task_dir/sdk" "$task_dir/source"
tar -xf "$out/cross-tools-posix.tar" -C "$task_dir/tools"
tar -xf "$out/sdk-posix.tar" -C "$task_dir/sdk"
tar -xf "$out/source.tar" -C "$task_dir/source"
cmake=/mnt/e/Mavk/artifacts/glibc-cmake-tools1-20261018/root
export LD_LIBRARY_PATH="$cmake/usr/lib/x86_64-linux-gnu:$task_dir/tools/usr/lib/x86_64-linux-gnu"
"$cmake/usr/bin/cmake" -S "$repo/experiments/droiddeck/glmark2-build" -B "$task_dir/build" -G Ninja \
 -DCMAKE_MAKE_PROGRAM="$out/ninja-host/bin/ninja" \
 -DMAVK_NULL_NATIVE="${MAVK_GLMARK2_HEADLESS:-OFF}" \
 -DCMAKE_TOOLCHAIN_FILE="$repo/experiments/droiddeck/glmark2-build/cross.cmake" \
 -DCMAKE_BUILD_TYPE=Release -Dtools="$task_dir/tools" -Dsdk="$task_dir/sdk" -Dsource="$task_dir/source"
"$cmake/usr/bin/cmake" --build "$task_dir/build" -j8
if test "${MAVK_GLMARK2_HEADLESS:-OFF}" = ON; then
 cp "$task_dir/build/glmark2-es2" "$out/glmark2-es2-headless"
else
 cp "$task_dir/build/glmark2-es2" "$out/glmark2-es2"
fi
echo GLMARK2_LINUX_BUILD_PASS
