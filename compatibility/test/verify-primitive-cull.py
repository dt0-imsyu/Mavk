"""Real-loader whole-primitive result, not full shaderCullDistance/CTS."""
import argparse
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("reports", type=Path)
parser.add_argument("--runtime-sha256", required=True)
args = parser.parse_args()
for guards in (0, 1):
    out = (args.reports / f"cull-{guards}.stdout").read_text()
    err = (args.reports / f"cull-{guards}.stderr").read_text()
    assert "HARDWARE Mali-G925" in out
    assert f"/files/mavk-runtime/{args.runtime_sha256}/payload/libVkLayer_mavk_compat.so" in out
    assert "CULL_NATIVE_FEATURES geometryShader=1 shaderCullDistance=0 requestedCullDistance=0" in out
    for case in range(8):
        culled = case in (2, 5)
        assert f"CULL_CASE index={case} referenceRed=288 actualRed={0 if culled else 288} expectedCull={int(culled)}" in out
    assert "WHOLE_PRIMITIVE_CULL_GPU_PASS cases=8 planes=2 checkedBytes=32768 referenceRedPixels=288 mixedSignUnclipped=1 zeroSurvives=1 restored=1 noFeatureClaim=1" in out
    assert "gaps=0" in err and "MAVK_LAYER_CORE_END liveZero=1" in err
    assert "MAVK_LAYER_GAP" not in err and "FAIL " not in out + err
    assert "MAVK_LAYER_DESCRIPTOR_FREE stages=3 native=1" in err
print("WHOLE_PRIMITIVE_CULL_NATIVE_AND_LAYER_PASS checkedBytes=65536 featureNotAdvertised=1 noCTS")
for mode, marker, draws, submissions in (("routing", "LAYER_EXECUTION_GPU_PASS", 12, 12), ("images", "IMAGE_SUBRESOURCE_GPU_PASS views=16", 16, 18)):
    out = (args.reports / f"cull-{mode}.stdout").read_text()
    err = (args.reports / f"cull-{mode}.stderr").read_text()
    assert marker in out and "HARDWARE Mali-G925" in out
    assert "MAVK_LAYER_CORE_END liveZero=1" in err and "gaps=0" in err
    assert f"MAVK_LAYER_COMMAND_END guarded=1 draws={draws} submissions={submissions}" in err
    assert "MAVK_LAYER_GAP" not in err and "FAIL " not in out + err
print("OPTIONAL_GEOMETRY_DEFAULT_PATH_REGRESSION_PASS commandRouting=1 images=1")
