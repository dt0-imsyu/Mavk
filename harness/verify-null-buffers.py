"""Strict bounded feasibility result; not nullDescriptor feature acceptance."""
import json
import pathlib
import argparse

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("report", type=pathlib.Path)
parser.add_argument("--atomics", action="store_true")
parser.add_argument("--shared-state", action="store_true")
parser.add_argument("--runtime-compiler", action="store_true")
parser.add_argument("--multi-sets", action="store_true")
args = parser.parse_args()
report = json.loads(args.report.read_text(encoding="utf-8"))
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
if args.atomics:
    expected.update(result="EMULATED_NULL_BUFFER_ATOMIC_FEASIBILITY_PASS",
                    guarded_accesses=1024, atomic_iadd_tested=True)
if args.shared_state:
    expected.update(shared_descriptor_state=True, snapshot_cache_reused=True)
if args.runtime_compiler:
    if not args.atomics:
        raise SystemExit("FAIL: runtime compiler acceptance currently requires atomic workload")
    expected.update(runtime_guard_compiler=True,guard_cache_hit=True,
                    guard_compilations=1,guard_rejections=4,
                    original_shader_bytes=3232,guarded_shader_bytes=9372)
    for field in ("guard_compile_ms", "guard_cache_ms"):
        value = report.get(field)
        if type(value) not in (int, float) or not 0 <= value < 30000:
            raise SystemExit(f"FAIL: compiler timing {field}")
if args.multi_sets:
    if not args.runtime_compiler:
        raise SystemExit("FAIL: multiset gate requires runtime compiler")
    expected.update(public_descriptor_sets=3,guard_descriptor_set=3,empty_public_sets=1,
                    vulkan_descriptor_bindings=True,bound_public_sets=2)
for key, value in expected.items():
    actual = report.get(key)
    if type(actual) is not type(value) or actual != value:
        raise SystemExit(f"FAIL: {key}: {actual!r} != {value!r}")
if not 0 < report.get("gpu_total_ms", 0) < 5000:
    raise SystemExit("FAIL: missing/out-of-range GPU timestamps")
print(f"PASS: null/range{'/atomic' if args.atomics else ''} feasibility,{expected['guarded_accesses']} guards,1536 words,SyncVal0/0; full features remain closed")
