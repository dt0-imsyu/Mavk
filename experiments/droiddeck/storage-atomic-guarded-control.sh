#!/bin/sh
set -eu
p=/data/user/0/com.droiddeck.launcher.mavk/files/mavk-aggregate
test -n "${DROIDDECK_MAVK_RUNTIME:-}"
export VK_LAYER_PATH="$p:$DROIDDECK_MAVK_RUNTIME/payload"
export VK_INSTANCE_LAYERS=VK_LAYER_MAVK_compat:VK_LAYER_HYBRIS_compat
export MAVK_LAYER_GUARDS=1 MAVK_GUARD_REPORTS=0 MAVK_LAYER_PROFILE=0
export MAVK_EXPECT_LAYER_PATH="$p/libVkLayer_mavk_compat.so"
unset LD_PRELOAD MAVK_TEST_BOUNDS MAVK_TEST_IMAGE_INIT MAVK_TEST_SUBMIT2 MAVK_TEST_NULL_IMAGE MAVK_TEST_SUBMIT_RETRY
"$p/guarded-atomic-consumer" "$p/storage-atomic-bounds.spv" "$p/vertex.vert.spv" "$p/vertex.frag.spv" --storage-atomic >"$p/atomic-guarded.stdout" 2>"$p/atomic-guarded.stderr"
grep 'GUARDED_STORAGE_TEXEL_ATOMIC_PASS' "$p/atomic-guarded.stdout"
grep 'MAVK_LAYER_CORE_END liveZero=1' "$p/atomic-guarded.stderr"
if grep -E 'MAVK_LAYER_GAP|gaps=[1-9]|FAIL ' "$p/atomic-guarded.stderr" "$p/atomic-guarded.stdout"; then exit 28; fi
(cd "$p" && tar -czf atomic-guarded-results.tar.gz atomic-guarded.stdout atomic-guarded.stderr)
