#!/bin/sh
set -eu
p=/data/user/0/com.droiddeck.launcher.mavk/files/mavk-aggregate
test -n "${DROIDDECK_MAVK_RUNTIME:-}"
export VK_LAYER_PATH="$p:$DROIDDECK_MAVK_RUNTIME/payload"
export VK_INSTANCE_LAYERS=VK_LAYER_MAVK_compat:VK_LAYER_HYBRIS_compat
export MAVK_LAYER_GUARDS=1 MAVK_GUARD_REPORTS=0 MAVK_LAYER_PROFILE=0
export MAVK_EXPECT_LAYER_PATH="$p/libVkLayer_mavk_compat.so"
name=view-components-candidate
if test "${MAVK_VIEW_COMPONENTS_BASELINE:-0}" = 1; then
    name=view-components-baseline
    export VK_LAYER_PATH="$DROIDDECK_MAVK_RUNTIME/payload"
    export MAVK_EXPECT_LAYER_PATH="$DROIDDECK_MAVK_RUNTIME/payload/libVkLayer_mavk_compat.so"
fi
unset LD_PRELOAD MAVK_TEST_BOUNDS MAVK_TEST_IMAGE_INIT MAVK_TEST_SUBMIT2 MAVK_TEST_NULL_IMAGE MAVK_TEST_SUBMIT_RETRY
status=0
"$p/view-components-consumer" "$p/view-components-uint.spv" "$p/view-components-float.spv" "$p/vertex.frag.spv" --texel-components-views >"$p/$name.stdout" 2>"$p/$name.stderr" || status=$?
if test "$name" = view-components-baseline; then
    test "$status" != 0
    grep 'texel view components mismatch format=98 phase=0 word=15 got=0 expected=1' "$p/$name.stderr"
    echo 'VIEW_COMPONENT_SUBSTITUTION_BASELINE_FAIL_CONFIRMED'
else
    test "$status" = 0
    grep 'TEXEL_VIEW_COMPONENTS_GPU_PASS' "$p/$name.stdout"
    grep 'MAVK_LAYER_CORE_END liveZero=1' "$p/$name.stderr"
    if grep -E 'MAVK_LAYER_GAP|gaps=[1-9]|FAIL ' "$p/$name.stderr" "$p/$name.stdout"; then exit 28; fi
fi
(cd "$p" && tar -czf "$name.tar.gz" "$name.stdout" "$name.stderr")
