#!/bin/sh
set -eu
p=/data/user/0/com.droiddeck.launcher.mavk/files/mavk-aggregate
test -n "${DROIDDECK_MAVK_RUNTIME:-}"
export VK_LAYER_PATH="$p:$DROIDDECK_MAVK_RUNTIME/payload:${VK_LAYER_PATH:-}"
export VK_INSTANCE_LAYERS=VK_LAYER_MAVK_compat:VK_LAYER_HYBRIS_compat
export MAVK_LAYER_GUARDS=1 MAVK_GUARD_REPORTS=0 MAVK_LAYER_PROFILE=0
export MAVK_EXPECT_LAYER_PATH="$p/libVkLayer_mavk_compat.so"
unset LD_PRELOAD MAVK_TEST_BOUNDS MAVK_TEST_IMAGE_INIT MAVK_TEST_SUBMIT2 MAVK_TEST_NULL_IMAGE MAVK_TEST_SUBMIT_RETRY
if test "${1:-}" = --native; then
    export VK_INSTANCE_LAYERS=VK_LAYER_HYBRIS_compat
    unset MAVK_EXPECT_LAYER_PATH
    name=vertex-native
else
    name=vertex-protected
fi
"$p/vertex-consumer" "$p/nested.spv" "$p/vertex.vert.spv" "$p/vertex.frag.spv" --vertex-robust >"$p/$name.stdout" 2>"$p/$name.stderr"
cat "$p/$name.stdout"
