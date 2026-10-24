#!/bin/sh
set -eu
p=/data/user/0/com.droiddeck.launcher.mavk/files/mavk-aggregate
test -n "${DROIDDECK_MAVK_RUNTIME:-}"
export VK_LAYER_PATH="$DROIDDECK_MAVK_RUNTIME/payload"
export VK_INSTANCE_LAYERS=VK_LAYER_MAVK_compat:VK_LAYER_HYBRIS_compat
export MAVK_GUARD_REPORTS=0 MAVK_LAYER_TRACE=0 MAVK_LAYER_PROFILE=0
export MAVK_EXPECT_LAYER_PATH="$DROIDDECK_MAVK_RUNTIME/payload/libVkLayer_mavk_compat.so"
export MAVK_CULL_REFERENCE="$p/cull-reference.vert.spv"
unset LD_PRELOAD MAVK_TEST_IMAGE_INIT MAVK_TEST_NULL_IMAGE MAVK_TEST_BOUNDS
if test "${1:-}" = --regressions; then
    s=/data/user/0/com.droiddeck.launcher.mavk/files/mavk-selected
    export MAVK_LAYER_GUARDS=1
    "$p/cull-consumer" "$s/execution.spv" "$s/sampled.vert.spv" "$s/sampled.frag.spv" >"$p/cull-routing.stdout" 2>"$p/cull-routing.stderr"
    "$p/cull-consumer" /data/user/0/com.droiddeck.launcher.mavk/files/mavk-images/image.spv "$s/triangle.vert.spv" "$s/triangle.frag.spv" --images >"$p/cull-images.stdout" 2>"$p/cull-images.stderr"
    for name in routing images; do
        grep GPU_PASS "$p/cull-$name.stdout"
        grep 'MAVK_LAYER_CORE_END liveZero=1' "$p/cull-$name.stderr"
        if grep -E 'MAVK_LAYER_GAP|gaps=[1-9]|FAIL ' "$p/cull-$name.stdout" "$p/cull-$name.stderr"; then exit 30; fi
    done
    (cd "$p" && tar -czf cull-results.tar.gz ./cull-*.stdout ./cull-*.stderr)
    exit 0
fi
for guards in 0 1; do
    export MAVK_LAYER_GUARDS=$guards
    "$p/cull-consumer" "$p/cull.vert.spv" "$p/cull.geom.spv" "$p/cull.frag.spv" --primitive-cull >"$p/cull-$guards.stdout" 2>"$p/cull-$guards.stderr"
    grep WHOLE_PRIMITIVE_CULL_GPU_PASS "$p/cull-$guards.stdout"
    grep 'MAVK_LAYER_CORE_END liveZero=1' "$p/cull-$guards.stderr"
    if grep -E 'MAVK_LAYER_GAP|gaps=[1-9]|FAIL ' "$p/cull-$guards.stderr" "$p/cull-$guards.stdout"; then exit 29; fi
done
(cd "$p" && tar -czf cull-results.tar.gz ./cull-*.stdout ./cull-*.stderr)
