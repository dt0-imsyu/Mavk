"""Native immutable descriptor generations,not full Vulkan API acceptance."""
import json
import pathlib
import sys

report = json.loads(pathlib.Path(sys.argv[1]).read_text(encoding="utf-8"))
expected = {
    "result": "NATIVE_DESCRIPTOR_GENERATIONS_PASS",
    "device": "Mali-G925-Immortalis MC12",
    "verified_words": 128,
    "generations": 2,
    "native_pool_creates": 2,
    "native_metadata_uploads": 2,
    "native_updates": 2,
    "negative_tests": 3,
    "record_before_submit": True,
    "resource_lease_retirement": True,
    "snapshot_cache_reused": True,
    "native_descriptor_adapter": True,
    "core_live_zero": True,
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
print("PASS: two native generations,128words,cache+retirement,3rejects,SyncVal0/0")
