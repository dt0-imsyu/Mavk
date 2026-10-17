"""Strict bounded feasibility result; not nullDescriptor feature acceptance."""
import json
import pathlib
import sys

report = json.loads(pathlib.Path(sys.argv[1]).read_text(encoding="utf-8"))
expected = {
    "result": "EMULATED_NULL_BUFFER_RANGE_FEASIBILITY_PASS",
    "device": "Mali-G925-Immortalis MC12",
    "verified_words": 1536,
    "guarded_accesses": 896,
    "gpu_submissions": 2,
    "core_live_zero": True,
    "nullDescriptor_advertised": False,
    "robustBufferAccess2_advertised": False,
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
if not 0 < report.get("gpu_total_ms", 0) < 5000:
    raise SystemExit("FAIL: missing/out-of-range GPU timestamps")
print("PASS: scalar null/range feasibility,896 guards,1536 words,SyncVal0/0; full features remain closed")
