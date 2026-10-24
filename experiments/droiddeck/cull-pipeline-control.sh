#!/bin/sh
set -eu
p=${MAVK_CULL_TEST_ROOT:-/data/user/0/com.droiddeck.launcher.mavk/files/mavk-aggregate}
test -n "${DROIDDECK_MAVK_RUNTIME:-}"
export VK_LAYER_PATH="$p:$DROIDDECK_MAVK_RUNTIME/payload"
export VK_INSTANCE_LAYERS=VK_LAYER_MAVK_compat:VK_LAYER_HYBRIS_compat
export MAVK_GUARD_REPORTS=0 MAVK_LAYER_TRACE=0 MAVK_LAYER_PROFILE=0
export MAVK_EXPECT_LAYER_PATH="$p/libVkLayer_mavk_compat.so"
export MAVK_CULL_REFERENCE="$p/cull-reference.vert.spv"
export MAVK_EXPERIMENTAL_CULL_PIPELINES=1 MAVK_TEST_CULL_ORIGINAL=1
marker=${MAVK_CULL_TEST_MARKER:-WHOLE_PRIMITIVE_CULL_GPU_PASS}
suffix=${MAVK_CULL_SOURCE_SUFFIX:-vert.spv}
export MAVK_CULL_REFERENCE="$p/cull-reference.$suffix"
unset LD_PRELOAD MAVK_TEST_IMAGE_INIT MAVK_TEST_NULL_IMAGE MAVK_TEST_BOUNDS MAVK_TEST_CULL_NO_GEOMETRY MAVK_TEST_CULL_DESCRIPTOR
for descriptors in 0 1; do
source="$p/cull-original.$suffix";prefix=cull-link
if test "$descriptors" = 1; then
    export MAVK_TEST_CULL_DESCRIPTOR=1 MAVK_CULL_REFERENCE="$p/cull-descriptor-reference.$suffix"
    source="$p/cull-descriptor-original.$suffix";prefix=cull-link-descriptor
fi
for guards in 0 1; do
    export MAVK_LAYER_GUARDS=$guards
    "$p/cull-link-consumer" "$source" "$p/cull.geom.spv" "$p/cull.frag.spv" --primitive-cull >"$p/$prefix-$guards.stdout" 2>"$p/$prefix-$guards.stderr"
    grep "$marker" "$p/$prefix-$guards.stdout"
    grep MAVK_LAYER_CULL_LINK "$p/$prefix-$guards.stderr"
    grep 'MAVK_LAYER_CORE_END liveZero=1' "$p/$prefix-$guards.stderr"
    if grep -E 'MAVK_LAYER_GAP|gaps=[1-9]|FAIL ' "$p/$prefix-$guards.stderr" "$p/$prefix-$guards.stdout"; then exit 29; fi
done
done
unset MAVK_TEST_CULL_DESCRIPTOR
export MAVK_CULL_REFERENCE="$p/cull-reference.$suffix"
export MAVK_LAYER_GUARDS=1 MAVK_TEST_CULL_NO_GEOMETRY=1
status=0
"$p/cull-link-consumer" "$p/cull-original.$suffix" "$p/cull.geom.spv" "$p/cull.frag.spv" --primitive-cull >"$p/cull-link-reject.stdout" 2>"$p/cull-link-reject.stderr" || status=$?
test "$status" != 0
grep 'cull requires native geometry and ordinary VS/FS pipeline' "$p/cull-link-reject.stderr"
(cd "$p" && tar -czf cull-link-results.tar.gz ./cull-link-*.stdout ./cull-link-*.stderr)
