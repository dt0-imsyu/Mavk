"""Bounded null-image oracle, not a Vulkan feature/conformance acceptance."""
import json
import pathlib
import argparse

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("report", type=pathlib.Path)
parser.add_argument("--shared-state", action="store_true")
parser.add_argument("--array-view", action="store_true")
parser.add_argument("--no-reports", action="store_true")
parser.add_argument("--live-arrays", action="store_true")
parser.add_argument("--volume-view", action="store_true")
parser.add_argument("--cube-view", action="store_true")
parser.add_argument("--sample-operation", choices=("fetch", "explicit_lod"))
args = parser.parse_args()
if sum((args.array_view, args.volume_view, args.cube_view)) > 1:
    parser.error("Choose one sampled image view shape")
report = json.loads(args.report.read_text(encoding="utf-8"))
expected = {
    "result": "EMULATED_NULL_IMAGE_FEASIBILITY_PASS",
    "device": "Mali-G925-Immortalis MC12",
    "verified_words": 1792,
    "guarded_accesses": 384,
    "gpu_submissions": 4,
    "core_live_zero": True,
    "nullDescriptor_advertised": False,
    "validation_layers_enabled": True,
    "synchronization_validation_requested": True,
    "validation_callback_tests": 1,
    "validation_errors": 0,
    "validation_warnings": 0,
    "validation_messages": [],
}
if args.shared_state:
    expected.update(shared_descriptor_state=True, snapshot_cache_reused=True)
if args.array_view:
    expected.update(guarded_accesses=512, null_sampled_array_view=True)
if args.volume_view:
    if args.array_view or args.live_arrays:
        raise SystemExit("3D-array views do not exist; choose volume OR array")
    expected.update(guarded_accesses=512, null_sampled_3d_view=True,
                    null_sampled_array_view=False, expected_guarded_accesses=512)
if args.cube_view:
    if args.live_arrays:
        parser.error("Live-array fixture cannot use cube view")
    expected.update(guarded_accesses=512, null_sampled_cube_view=True,
                    cube_backing_faces=6, fallback_verified_words=14,
                    null_sampled_3d_view=False, null_sampled_array_view=False,
                    expected_guarded_accesses=512, sample_operation="explicit_lod")
if args.live_arrays:
    if not args.array_view:
        raise SystemExit("Live-array oracle requires --array-view")
    expected.update(guarded_accesses=384, live_array_layers=2, array_base_layer=1,
                    expected_guarded_accesses=384)
if args.sample_operation:
    expected["sample_operation"] = args.sample_operation
if args.no_reports:
    if not (args.array_view or args.volume_view or args.cube_view):
        raise SystemExit("No-report oracle requires a supported null image shape")
    expected.update(guarded_accesses=0, expected_guarded_accesses=384 if args.live_arrays else 512, diagnostic_reports=False)
for key, value in expected.items():
    actual = report.get(key)
    if type(actual) is not type(value) or actual != value:
        raise SystemExit(f"FAIL: {key}: {actual!r} != {value!r}")
coverage = f"{expected['expected_guarded_accesses']} expected guarded accesses,pixel/write oracle,reports OFF" if args.no_reports else f"{expected['guarded_accesses']} reported guards"
print(f"PASS: null combined-sampler/storage-image feasibility,{coverage},1792 words,SyncVal0/0")
