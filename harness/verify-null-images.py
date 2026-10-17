"""Bounded null-image oracle, not a Vulkan feature/conformance acceptance."""
import json
import pathlib
import sys

report = json.loads(pathlib.Path(sys.argv[1]).read_text(encoding="utf-8"))
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
for key, value in expected.items():
    actual = report.get(key)
    if type(actual) is not type(value) or actual != value:
        raise SystemExit(f"FAIL: {key}: {actual!r} != {value!r}")
print("PASS: null combined-sampler/storage-image feasibility,384 guards,1792 words,SyncVal0/0")
