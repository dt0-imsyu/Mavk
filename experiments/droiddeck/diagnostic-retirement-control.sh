#!/bin/sh
set -eu
p=/data/user/0/com.droiddeck.launcher.mavk/files/mavk-aggregate
s=/data/user/0/com.droiddeck.launcher.mavk/files/mavk-selected
tag=${MAVK_DIAGNOSTIC_TEST_TAG:?baseline or candidate required}
case "$tag" in baseline|candidate) ;; *) exit 22;; esac
test -n "${DROIDDECK_MAVK_RUNTIME:-}"
export VK_LAYER_PATH="$p:$DROIDDECK_MAVK_RUNTIME/payload"
export VK_INSTANCE_LAYERS=VK_LAYER_MAVK_compat:VK_LAYER_HYBRIS_compat
export MAVK_LAYER_GUARDS=1 MAVK_LAYER_PROFILE=0
export MAVK_EXPECT_LAYER_PATH="$p/libVkLayer_mavk_compat.so"
unset LD_PRELOAD MAVK_TEST_TEXEL_METADATA
for reports in 0 1; do
    export MAVK_GUARD_REPORTS=$reports
    "$p/consumer" "$p/nested.spv" "$s/triangle.vert.spv" "$s/triangle.frag.spv" --nested >"$p/diagnostics-$tag-$reports.stdout" 2>"$p/diagnostics-$tag-$reports.stderr"
    grep NESTED_BUFFER_GPU_PASS "$p/diagnostics-$tag-$reports.stdout"
    grep MAVK_LAYER_COMMAND_END "$p/diagnostics-$tag-$reports.stderr"
    grep 'MAVK_LAYER_CORE_END liveZero=1' "$p/diagnostics-$tag-$reports.stderr"
done
(cd "$p" && tar -czf "diagnostics-$tag.tar.gz" ./diagnostics-"$tag"-*.stdout ./diagnostics-"$tag"-*.stderr)
