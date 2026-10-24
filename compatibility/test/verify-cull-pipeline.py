"""Private experimental API-hook proof, NOT public Vulkan feature conformance."""
import argparse
import re
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("reports", type=Path)
parser.add_argument("--layer-path", default="/files/mavk-aggregate/libVkLayer_mavk_compat.so")
mode = parser.add_mutually_exclusive_group()
mode.add_argument("--primitive-id", action="store_true")
mode.add_argument("--tessellation", action="store_true")
args = parser.parse_args()
original_stages = 4 if args.tessellation else 2
native_stages = original_stages + 1
for descriptors in (0, 1):
    for guarded in (0, 1):
        prefix = "cull-link-descriptor" if descriptors else "cull-link"
        out = (args.reports / f"{prefix}-{guarded}.stdout").read_text()
        err = (args.reports / f"{prefix}-{guarded}.stderr").read_text()
        assert "HARDWARE Mali-G925" in out
        assert args.layer_path in out
        assert f"CULL_APPLICATION_STAGES count={original_stages} originalShader=1 manualTransform=0" in out
        assert f"CULL_APPLICATION_DESCRIPTOR uniform={descriptors}" in out
        assert "CULL_NATIVE_FEATURES geometryShader=1 shaderCullDistance=0 requestedCullDistance=0" in out
        if args.tessellation:
            assert "CULL_TESSELLATION_FEATURE native=1 requested=1" in out
            match = re.search(r"^CULL_TESSELLATION_GPU_PASS cases=6 checkedBytes=24576 referenceRedPixels=(\d+) survivingFanPixels=(\d+) tessLevel=2 generatedPrimitiveCull=1 restored=1 noFeatureClaim=1$", out, re.M)
            assert match and int(match[2]) > 0 and int(match[1]) == 3 * int(match[2])
        elif args.primitive_id:
            match = re.search(r"^CULL_PRIMITIVE_ID_GPU_PASS cases=8 primitives=3 checkedBytes=98304 referencePixels=(\d+),(\d+),(\d+) culledIdsNotRenumbered=1 resetBetweenDraws=1 noFeatureClaim=1$", out, re.M)
            assert match and all(int(count) > 0 for count in match.groups())
        else:
            assert "WHOLE_PRIMITIVE_CULL_GPU_PASS cases=8 planes=2 checkedBytes=32768 referenceRedPixels=288 mixedSignUnclipped=1 zeroSurvives=1 restored=1 noFeatureClaim=1" in out
        assert err.count(f"MAVK_LAYER_CULL_LINK originalStages={original_stages} nativeStages={native_stages} planes=2 vertices=3 nativeLimits=checked publicFeature=0") == 1
        assert "MAVK_LAYER_CORE_END liveZero=1" in err and "gaps=0" in err
        assert "MAVK_LAYER_GAP" not in err and "FAIL " not in out + err
        if descriptors and guarded:
            assert f"MAVK_LAYER_PIPELINE guards=native-instrumented stages={native_stages} publicSets=1" in err
            draws = 7 if args.tessellation else 9
            assert f"MAVK_LAYER_COMMAND_END guarded=1 draws={draws} submissions={draws}" in err
negative = (args.reports / "cull-link-reject.stderr").read_text()
assert "MAVK_LAYER_CULL_POLICY experimental=1 publicFeature=0 nativeGeometryEnabled=0" in negative
assert "MAVK_LAYER_GAP operation=vkCreateGraphicsPipelines reason=cull requires native geometry and ordinary VS/FS pipeline" in negative
assert "MAVK_LAYER_CULL_LINK" not in negative and "gaps=1" in negative
assert "MAVK_LAYER_CORE_END liveZero=1" in negative
print(f"EXPERIMENTAL_CULL_PIPELINE_HOOK_GPU_PASS modes=4 checkedBytes={98304 if args.tessellation else 393216 if args.primitive_id else 131072} primitiveId={int(args.primitive_id)} tessellation={int(args.tessellation)} nativeDependencyReject=1 noPublicFeatureClaim=1 noCTS")
