#!/bin/sh
set -eu
p=${MAVK_BUFFER_TEST_ROOT:-/data/user/0/com.droiddeck.launcher.mavk/files/mavk-nested}
case "$p" in /data/user/0/com.droiddeck.launcher.mavk/files/mavk-nested|/data/user/0/com.droiddeck.launcher.mavk/files/mavk-overflow|/data/user/0/com.droiddeck.launcher.mavk/files/mavk-wide|/data/user/0/com.droiddeck.launcher.mavk/files/mavk-aggregate) ;; *) exit 22;; esac
s=/data/user/0/com.droiddeck.launcher.mavk/files/mavk-selected
test -n "${DROIDDECK_MAVK_RUNTIME:-}"
export VK_LAYER_PATH="$p:$DROIDDECK_MAVK_RUNTIME/payload:${VK_LAYER_PATH:-}"
export VK_INSTANCE_LAYERS=VK_LAYER_MAVK_compat:VK_LAYER_HYBRIS_compat
export MAVK_LAYER_GUARDS=1 MAVK_GUARD_REPORTS=0 MAVK_LAYER_PROFILE=0
export MAVK_EXPECT_LAYER_PATH="$p/libVkLayer_mavk_compat.so"
if test "${MAVK_BUFFER_COMMON:-0}" = 1; then
    export VK_LAYER_PATH="$DROIDDECK_MAVK_RUNTIME/payload"
    export MAVK_EXPECT_LAYER_PATH="$DROIDDECK_MAVK_RUNTIME/payload/libVkLayer_mavk_compat.so"
fi
unset LD_PRELOAD MAVK_TEST_BOUNDS MAVK_TEST_IMAGE_INIT MAVK_TEST_SUBMIT2 MAVK_TEST_NULL_IMAGE MAVK_TEST_SUBMIT_RETRY
run() {
    name=$1; shift
    "$p/consumer" "$@" >"$p/$name.stdout" 2>"$p/$name.stderr"
}
if test "${1:-}" = --regressions-only; then
    run nested "$p/nested.spv" "$s/triangle.vert.spv" "$s/triangle.frag.spv" --nested
fi
if test "${1:-}" = --aggregate-variants; then
    variants="aggregate aggregate-array aggregate-matrix aggregate-memory"
    if test -f "$p/aggregate-copy.spv"; then variants="$variants aggregate-copy"; fi
    if test -f "$p/aggregate-copy-operands.spv"; then variants="$variants aggregate-copy-operands"; fi
    for variant in $variants; do
        run "$variant" "$p/$variant.spv" "$s/triangle.vert.spv" "$s/triangle.frag.spv" --aggregate
        grep 'AGGREGATE_BUFFER_GPU_PASS' "$p/$variant.stdout"
        grep 'MAVK_LAYER_CORE_END liveZero=1' "$p/$variant.stderr"
        if grep -E 'MAVK_LAYER_GAP|gaps=[1-9]|FAIL ' "$p/$variant.stderr" "$p/$variant.stdout"; then exit 28; fi
    done
    (cd "$p" && tar -czf variants-results.tar.gz ./aggregate*.stdout ./aggregate*.stderr)
    exit 0
fi
if test "${1:-}" != --regressions-only; then
if test "${MAVK_TEXEL_VIEW_COMPONENTS:-0}" = 1; then
    "$p/view-components-consumer" "$p/view-components-uint.spv" "$p/view-components-float.spv" "$s/triangle.frag.spv" --texel-components-views >"$p/view-components.stdout" 2>"$p/view-components.stderr"
    grep 'TEXEL_VIEW_COMPONENTS_GPU_PASS' "$p/view-components.stdout"
    grep 'MAVK_LAYER_CORE_END liveZero=1' "$p/view-components.stderr"
    if grep -E 'MAVK_LAYER_GAP|gaps=[1-9]|FAIL ' "$p/view-components.stderr" "$p/view-components.stdout"; then exit 36; fi
fi
if test "${MAVK_TEXEL_COMPONENTS:-0}" = 1; then
    "$p/components-consumer" "$p/storage-components.spv" "$s/triangle.vert.spv" "$s/triangle.frag.spv" --storage-components >"$p/texel-components.stdout" 2>"$p/texel-components.stderr"
    grep 'STORAGE_TEXEL_COMPONENTS_PASS' "$p/texel-components.stdout"
    grep 'MAVK_LAYER_CORE_END liveZero=1' "$p/texel-components.stderr"
    if grep -E 'MAVK_LAYER_GAP|gaps=[1-9]|FAIL ' "$p/texel-components.stderr" "$p/texel-components.stdout"; then exit 35; fi
fi
if test "${MAVK_STORAGE_ATOMIC:-0}" = 1; then
    "$p/guarded-atomic-consumer" "$p/storage-atomic-bounds.spv" "$s/triangle.vert.spv" "$s/triangle.frag.spv" --storage-atomic >"$p/atomic-guarded.stdout" 2>"$p/atomic-guarded.stderr"
    grep 'GUARDED_STORAGE_TEXEL_ATOMIC_PASS' "$p/atomic-guarded.stdout"
    grep 'MAVK_LAYER_CORE_END liveZero=1' "$p/atomic-guarded.stderr"
    if grep -E 'MAVK_LAYER_GAP|gaps=[1-9]|FAIL ' "$p/atomic-guarded.stderr" "$p/atomic-guarded.stdout"; then exit 34; fi
fi
if test "${MAVK_STORAGE_TEXEL:-0}" = 1; then
    "$p/storage-consumer" "$p/storage-texel.spv" "$s/triangle.vert.spv" "$s/triangle.frag.spv" --storage-texel >"$p/storage-texel.stdout" 2>"$p/storage-texel.stderr"
    grep 'STORAGE_TEXEL_BOUNDS_GPU_PASS' "$p/storage-texel.stdout"
    grep 'MAVK_LAYER_CORE_END liveZero=1' "$p/storage-texel.stderr"
    if grep -E 'MAVK_LAYER_GAP|gaps=[1-9]|FAIL ' "$p/storage-texel.stderr" "$p/storage-texel.stdout"; then exit 33; fi
fi
if test -f "$p/texel-consumer"; then
    "$p/texel-consumer" "$p/texel-none.spv" "$p/texel-zeroextend.spv" "$s/triangle.frag.spv" --texel-bounds >"$p/texel-bounds.stdout" 2>"$p/texel-bounds.stderr"
    grep 'TEXEL_BOUNDS_GPU_PASS' "$p/texel-bounds.stdout"
    grep 'MAVK_LAYER_CORE_END liveZero=1' "$p/texel-bounds.stderr"
    if grep -E 'MAVK_LAYER_GAP|gaps=[1-9]|FAIL ' "$p/texel-bounds.stderr" "$p/texel-bounds.stdout"; then exit 32; fi
fi
run nested "$p/nested.spv" "$s/triangle.vert.spv" "$s/triangle.frag.spv" --nested
if test -f "$p/copied-pointer.spv"; then
    run copied-pointer "$p/copied-pointer.spv" "$s/triangle.vert.spv" "$s/triangle.frag.spv" --nested
    grep 'NESTED_BUFFER_GPU_PASS' "$p/copied-pointer.stdout"
    grep 'MAVK_LAYER_CORE_END liveZero=1' "$p/copied-pointer.stderr"
    if grep -E 'MAVK_LAYER_GAP|gaps=[1-9]|FAIL ' "$p/copied-pointer.stderr" "$p/copied-pointer.stdout"; then exit 31; fi
fi
if test -f "$p/aggregate-copy-operands.spv"; then
    run aggregate-copy-operands "$p/aggregate-copy-operands.spv" "$s/triangle.vert.spv" "$s/triangle.frag.spv" --aggregate
    grep 'AGGREGATE_BUFFER_GPU_PASS' "$p/aggregate-copy-operands.stdout"
    grep 'MAVK_LAYER_CORE_END liveZero=1' "$p/aggregate-copy-operands.stderr"
    if grep -E 'MAVK_LAYER_GAP|gaps=[1-9]|FAIL ' "$p/aggregate-copy-operands.stderr" "$p/aggregate-copy-operands.stdout"; then exit 30; fi
fi
if test -f "$p/aggregate-copy.spv"; then
    run aggregate-copy "$p/aggregate-copy.spv" "$s/triangle.vert.spv" "$s/triangle.frag.spv" --aggregate
    grep 'AGGREGATE_BUFFER_GPU_PASS' "$p/aggregate-copy.stdout"
    grep 'MAVK_LAYER_CORE_END liveZero=1' "$p/aggregate-copy.stderr"
    if grep -E 'MAVK_LAYER_GAP|gaps=[1-9]|FAIL ' "$p/aggregate-copy.stderr" "$p/aggregate-copy.stdout"; then exit 29; fi
fi
if test -f "$p/aggregate.spv"; then
    run aggregate "$p/aggregate.spv" "$s/triangle.vert.spv" "$s/triangle.frag.spv" --aggregate
    grep 'AGGREGATE_BUFFER_GPU_PASS' "$p/aggregate.stdout"
    grep 'MAVK_LAYER_CORE_END liveZero=1' "$p/aggregate.stderr"
    if grep -E 'MAVK_LAYER_GAP|gaps=[1-9]|FAIL ' "$p/aggregate.stderr" "$p/aggregate.stdout"; then exit 27; fi
fi
if test -f "$p/high.spv"; then
    for variant in high negative valid; do
        unset MAVK_WIDE_VALID
        if test "$variant" = valid; then export MAVK_WIDE_VALID=1; fi
        run "wide-$variant" "$p/$variant.spv" "$s/triangle.vert.spv" "$s/triangle.frag.spv" --wide
        grep 'GPU_PASS' "$p/wide-$variant.stdout"
        grep 'MAVK_LAYER_CORE_END liveZero=1' "$p/wide-$variant.stderr"
        if grep -E 'MAVK_LAYER_GAP|gaps=[1-9]|FAIL ' "$p/wide-$variant.stderr" "$p/wide-$variant.stdout"; then exit 26; fi
    done
    unset MAVK_WIDE_VALID
fi
if test -f "$p/overflow.spv"; then
    run overflow "$p/overflow.spv" "$s/triangle.vert.spv" "$s/triangle.frag.spv" --nested
    grep 'NESTED_BUFFER_GPU_PASS' "$p/overflow.stdout"
    grep 'MAVK_LAYER_CORE_END liveZero=1' "$p/overflow.stderr"
    if grep -E 'MAVK_LAYER_GAP|gaps=[1-9]|FAIL ' "$p/overflow.stderr" "$p/overflow.stdout"; then exit 24; fi
fi
if test -f "$p/overflow-add.spv"; then
    run overflow-add "$p/overflow-add.spv" "$s/triangle.vert.spv" "$s/triangle.frag.spv" --nested
    grep 'NESTED_BUFFER_GPU_PASS' "$p/overflow-add.stdout"
    grep 'MAVK_LAYER_CORE_END liveZero=1' "$p/overflow-add.stderr"
    if grep -E 'MAVK_LAYER_GAP|gaps=[1-9]|FAIL ' "$p/overflow-add.stderr" "$p/overflow-add.stdout"; then exit 25; fi
fi
fi
for mode in dynamic resubmit inflight; do
    run "$mode" "$s/dynamic.spv" "$s/triangle.vert.spv" "$s/triangle.frag.spv" "--$mode"
done
for mode in empty unused selected; do
    run "$mode" "$s/gap.spv" "$s/triangle.vert.spv" "$s/triangle.frag.spv" "--$mode"
done
run immutable "$s/immutable.spv" "$s/triangle.vert.spv" "$s/triangle.frag.spv" --immutable
run grouped "$s/immutable-grouped.spv" "$s/triangle.vert.spv" "$s/triangle.frag.spv" --immutable
run routing "$s/execution.spv" "$s/sampled.vert.spv" "$s/sampled.frag.spv"
run texel /data/user/0/com.droiddeck.launcher.mavk/files/mavk-rgba8/unorm.spv /data/user/0/com.droiddeck.launcher.mavk/files/mavk-rgba8/uint.spv "$s/triangle.frag.spv" --texel
run images /data/user/0/com.droiddeck.launcher.mavk/files/mavk-images/image.spv "$s/triangle.vert.spv" "$s/triangle.frag.spv" --images
for mode in nested dynamic resubmit inflight empty unused selected immutable grouped routing texel images; do
    grep 'PASS' "$p/$mode.stdout"
    grep 'MAVK_LAYER_CORE_END liveZero=1' "$p/$mode.stderr"
    if grep -E 'MAVK_LAYER_GAP|gaps=[1-9]|FAIL ' "$p/$mode.stderr" "$p/$mode.stdout"; then exit 23; fi
done
(cd "$p" && tar -czf results.tar.gz ./*.stdout ./*.stderr)
