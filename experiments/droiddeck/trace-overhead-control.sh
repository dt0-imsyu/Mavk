#!/bin/sh
set -eu
p=/data/user/0/com.droiddeck.launcher.mavk/files/mavk-aggregate
s=/data/user/0/com.droiddeck.launcher.mavk/files/mavk-selected
test -n "${DROIDDECK_MAVK_RUNTIME:-}"
export VK_LAYER_PATH="$p:$DROIDDECK_MAVK_RUNTIME/payload"
export VK_INSTANCE_LAYERS=VK_LAYER_MAVK_compat:VK_LAYER_HYBRIS_compat
export MAVK_LAYER_GUARDS=1 MAVK_GUARD_REPORTS=0 MAVK_LAYER_PROFILE=0
export MAVK_EXPECT_LAYER_PATH="$p/libVkLayer_mavk_compat.so"
unset LD_PRELOAD MAVK_TEST_TEXEL_METADATA
if test "${1:-}" != --error-only; then
for run in 0 1 2; do
    if test "$run" = 1; then modes='1 0'; else modes='0 1'; fi
    for trace in $modes; do
        export MAVK_LAYER_TRACE=$trace
        "$p/record-consumer" "$p/nested.spv" "$s/triangle.vert.spv" "$s/triangle.frag.spv" --record-overhead >"$p/trace-$run-$trace.stdout" 2>"$p/trace-$run-$trace.stderr"
        grep NESTED_BUFFER_GPU_PASS "$p/trace-$run-$trace.stdout"
        grep MAVK_LAYER_COMMAND_END "$p/trace-$run-$trace.stderr"
    done
done
fi
export MAVK_LAYER_TRACE=0
"$p/record-consumer" "$s/gap.spv" "$s/triangle.vert.spv" "$s/triangle.frag.spv" --missing-used >"$p/trace-missing.stdout" 2>"$p/trace-missing.stderr"
grep 'STATIC_USED_SET_REJECTION_PASS missingSet=2 submissions=0' "$p/trace-missing.stdout"
grep 'MAVK_LAYER_GAP operation=guarded execution' "$p/trace-missing.stderr"
(cd "$p" && tar -czf trace-results.tar.gz ./trace-*.stdout ./trace-*.stderr)
