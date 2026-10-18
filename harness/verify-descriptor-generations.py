"""Native immutable descriptor generations,not full Vulkan API acceptance."""
import json
import pathlib
import sys

args = sys.argv[1:]
vulkan_api = "--vulkan-api" in args
templates = "--templates" in args
if templates:
    args.remove("--templates")
if vulkan_api:
    args.remove("--vulkan-api")
if len(args) != 1:
    raise SystemExit("usage: verify-descriptor-generations.py [--vulkan-api] [--templates] REPORT")
report = json.loads(pathlib.Path(args[0]).read_text(encoding="utf-8"))
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
if vulkan_api or templates:
    expected.update(vulkan_descriptor_updates=True, api_write_structures=0 if templates else 2,
                    api_copy_structures=1)
if templates:
    expected.update(vulkan_descriptor_templates=True, api_template_updates=2,
                    api_template_entries=1, template_required_bytes=57)
for key, value in expected.items():
    actual = report.get(key)
    if type(actual) is not type(value) or actual != value:
        raise SystemExit(f"FAIL: {key}: {actual!r} != {value!r}")
print(f"PASS: {'template updates,' if templates else ''}two native generations,128words,cache+retirement,3rejects,SyncVal0/0")
