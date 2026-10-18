"""R32_UINT texel feasibility,NOT full nullDescriptor/robustness2 acceptance."""
import json
import pathlib
import sys

if len(sys.argv) != 2:
    raise SystemExit("usage: verify-null-texels.py REPORT")
report = json.loads(pathlib.Path(sys.argv[1]).read_text(encoding="utf-8"))
expected = dict(result="EMULATED_NULL_TEXEL_FEASIBILITY_PASS",
                device="Mali-G925-Immortalis MC12", verified_words=2304,
                guarded_accesses=1152, null_guards=768, uniform_oob_guards=128,
                storage_oob_guards=256, gpu_submissions=2,
                runtime_guard_compiler=True, vulkan_descriptor_updates=True,
                bound_descriptor_snapshots=True, snapshot_cache_reused=True,
                core_live_zero=True, nullDescriptor_advertised=False,
                robustBufferAccess2_advertised=False, validation_layers_enabled=True,
                synchronization_validation_requested=True, validation_callback_tests=1,
                validation_errors=0, validation_warnings=0, validation_messages=[])
for key, expected_value in expected.items():
    value = report.get(key)
    if type(value) is not type(expected_value) or value != expected_value:
        raise SystemExit(f"FAIL: {key}: {value!r} != {expected_value!r}")
print("PASS: R32_UINT texel buffers,2304values,1152guards,SyncVal0/0; full features closed")
