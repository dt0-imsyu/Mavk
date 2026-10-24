#!/bin/sh
set -eu
p=${MAVK_DESCRIPTOR_TEST_ROOT:-/data/user/0/com.droiddeck.launcher.mavk/files/mavk-unused}
case "$p" in /data/user/0/com.droiddeck.launcher.mavk/files/mavk-unused|/data/user/0/com.droiddeck.launcher.mavk/files/mavk-selected) ;; *) exit 22;; esac
test -n "${DROIDDECK_MAVK_RUNTIME:-}"
export VK_LAYER_PATH="$p:$DROIDDECK_MAVK_RUNTIME/payload:${VK_LAYER_PATH:-}"
export VK_INSTANCE_LAYERS=VK_LAYER_MAVK_compat:VK_LAYER_HYBRIS_compat
export MAVK_LAYER_GUARDS=${MAVK_EXECUTION_GUARDS:-1} MAVK_GUARD_REPORTS=0 MAVK_LAYER_PROFILE=0
export MAVK_LAYER_TRACE=1
export MAVK_EXPECT_LAYER_PATH="$p/libVkLayer_mavk_compat.so"
unset LD_PRELOAD MAVK_TEST_BOUNDS MAVK_TEST_IMAGE_INIT MAVK_TEST_SUBMIT2 MAVK_TEST_NULL_IMAGE MAVK_TEST_SUBMIT_RETRY
mode=${1:---unused}
case "$mode" in --unused|--empty|--missing-used|--selected|--unwritten-used) shader="$p/gap.spv";vs="$p/triangle.vert.spv";fs="$p/triangle.frag.spv";; --dynamic) shader="$p/dynamic.spv";vs="$p/triangle.vert.spv";fs="$p/triangle.frag.spv";; --immutable) shader="$p/immutable.spv";vs="$p/triangle.vert.spv";fs="$p/triangle.frag.spv";; --immutable-grouped) shader="$p/immutable-grouped.spv";vs="$p/triangle.vert.spv";fs="$p/triangle.frag.spv";mode=--immutable;; --routing) shader="$p/execution.spv";vs="$p/sampled.vert.spv";fs="$p/sampled.frag.spv";mode="";; *) exit 21;; esac
exec "$p/consumer" "$shader" "$vs" "$fs" ${mode:+"$mode"}
