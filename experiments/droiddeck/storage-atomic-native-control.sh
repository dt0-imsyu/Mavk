#!/bin/sh
set -eu
p=/data/user/0/com.droiddeck.launcher.mavk/files/mavk-aggregate
test -n "${DROIDDECK_MAVK_RUNTIME:-}"
export VK_LAYER_PATH="$DROIDDECK_MAVK_RUNTIME/payload"
export VK_INSTANCE_LAYERS=VK_LAYER_HYBRIS_compat
unset LD_PRELOAD MAVK_EXPECT_LAYER_PATH MAVK_TEST_BOUNDS MAVK_TEST_IMAGE_INIT MAVK_TEST_SUBMIT2 MAVK_TEST_NULL_IMAGE MAVK_TEST_SUBMIT_RETRY
LD_DEBUG=libs "$p/atomic-consumer" "$p/storage-atomic.spv" "$p/vertex.vert.spv" "$p/vertex.frag.spv" --storage-atomic-native >"$p/atomic-native.stdout" 2>"$p/atomic-native.stderr"
grep 'NATIVE_STORAGE_TEXEL_ATOMIC_PASS' "$p/atomic-native.stdout"
grep -E 'calling init:.*libVkLayer_hybris_compat' "$p/atomic-native.stderr"
if grep -E 'calling init:.*libVkLayer_mavk_compat' "$p/atomic-native.stderr"; then exit 25; fi
(cd "$p" && tar -czf atomic-native-results.tar.gz atomic-native.stdout atomic-native.stderr)
