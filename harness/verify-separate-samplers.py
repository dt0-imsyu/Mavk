"""Bounded separate RGBA8 sampling acceptance,not full nullDescriptor support."""
import json
import pathlib
import sys
if len(sys.argv) != 2:
    raise SystemExit("usage: verify-separate-samplers.py REPORT")
report = json.loads(pathlib.Path(sys.argv[1]).read_text(encoding="utf-8"))
expected = dict(result="SEPARATE_IMAGE_SAMPLER_FEASIBILITY_PASS",
    device="Mali-G925-Immortalis MC12",verified_components=2048,
    guarded_accesses=256,public_descriptor_sets=2,guard_descriptor_set=2,
    filter_modes=2,sampler_swaps=True,null_alpha_zero=True,runtime_guard_compiler=True,
    vulkan_descriptor_updates=True,snapshot_cache_reused=True,core_live_zero=True,
    nullDescriptor_advertised=False,validation_layers_enabled=True,
    synchronization_validation_requested=True,validation_callback_tests=1,
    validation_errors=0,validation_warnings=0,validation_messages=[])
for key, expected_value in expected.items():
    value=report.get(key)
    if type(value) is not type(expected_value) or value!=expected_value:
        raise SystemExit(f"FAIL: {key}: {value!r} != {expected_value!r}")
print("PASS: separate image/sampler,2048components,256guards,nearest+bilinear,SyncVal0/0")
