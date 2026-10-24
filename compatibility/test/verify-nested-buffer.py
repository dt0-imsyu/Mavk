"""Bounded guard fix and existing GPU regressions; not Vulkan CTS."""
import argparse
import re
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("reports", type=Path)
parser.add_argument("--overflow", action="store_true")
parser.add_argument("--wide", action="store_true")
parser.add_argument("--common", action="store_true")
parser.add_argument("--runtime-sha256", default="73ec2df437ab0314ea0c8db06334b05ae97cd56d71af8f17173e0328fbbe0347")
parser.add_argument("--aggregate", action="store_true")
parser.add_argument("--aggregate-variants-only", action="store_true")
parser.add_argument("--copy", action="store_true")
parser.add_argument("--copy-operands", action="store_true")
parser.add_argument("--copied-pointer", action="store_true")
parser.add_argument("--texel-bounds", action="store_true")
parser.add_argument("--storage-texel", action="store_true")
parser.add_argument("--storage-atomic", action="store_true")
parser.add_argument("--texel-components", action="store_true")
parser.add_argument("--texel-view-components", action="store_true")
parser.add_argument("--no-diagnostic-reads", action="store_true")
parser.add_argument("--no-event-trace", action="store_true")
parser.add_argument("--layer-path", help="Exact isolated layer mapping required in every report")
args = parser.parse_args()
assert re.fullmatch(r"[0-9a-f]{64}", args.runtime_sha256), "Runtime hash required"
if args.texel_view_components:
    args.texel_components = True
if args.texel_components:
    args.storage_atomic = True
if args.storage_atomic:
    args.storage_texel = True
if args.storage_texel:
    args.texel_bounds = True
if args.texel_bounds:
    args.copied_pointer = True
if args.copied_pointer:
    args.copy_operands = True
if args.copy_operands:
    args.copy = True
if args.aggregate_variants_only:
    variants = ["aggregate", "aggregate-array", "aggregate-matrix", "aggregate-memory"]
    if args.copy:
        variants.append("aggregate-copy")
    if args.copy_operands:
        variants.append("aggregate-copy-operands")
    for name in variants:
        out = (args.reports / f"{name}.stdout").read_text()
        err = (args.reports / f"{name}.stderr").read_text()
        assert "HARDWARE Mali-G925" in out and "/files/mavk-aggregate/libVkLayer_mavk_compat.so" in out, name
        assert "AGGREGATE_BUFFER_GPU_PASS phases=3 partialReadZero=1 partialWriteSuppressed=1 fullReadWrite=1 checkedWords=192" in out.splitlines(), name
        assert "MAVK_LAYER_COMMAND_END guarded=1 draws=3 submissions=3" in err, name
        assert "gaps=0" in err and "MAVK_LAYER_CORE_END liveZero=1" in err, name
        assert "MAVK_LAYER_GAP" not in err and "FAIL " not in out + err, name
    print(f"AGGREGATE_VARIANTS_GPU_PASS forms={len(variants)} checkedWords={len(variants)*192} noCTS")
    raise SystemExit(0)
if args.copy:
    args.aggregate = True
if args.aggregate:
    args.wide = True
if args.wide:
    args.overflow = True
counts = {"nested": (1, 1), "dynamic": (4, 2), "resubmit": (4, 4),
          "inflight": (4, 4), "empty": (4, 4), "unused": (4, 4),
          "selected": (4, 2), "immutable": (3, 5), "grouped": (3, 5),
          "routing": (12, 12), "texel": (2, 2), "images": (16, 18)}
if args.overflow:
    counts["overflow"] = (1, 1)
    counts["overflow-add"] = (1, 1)
if args.wide:
    counts.update({f"wide-{name}": (1, 1) for name in ("high", "negative", "valid")})
if args.aggregate:
    counts["aggregate"] = (3, 3)
if args.copy:
    counts["aggregate-copy"] = (3, 3)
if args.copy_operands:
    counts["aggregate-copy-operands"] = (3, 3)
if args.copied_pointer:
    counts["copied-pointer"] = (1, 1)
if args.texel_bounds:
    counts["texel-bounds"] = (2, 2)
if args.storage_texel:
    counts["storage-texel"] = (1, 1)
if args.storage_atomic:
    counts["atomic-guarded"] = (5, 5)
if args.texel_components:
    counts["texel-components"] = (3, 3)
if args.texel_view_components:
    counts["view-components"] = (18, 18)
for name, (draws, submissions) in counts.items():
    out = (args.reports / f"{name}.stdout").read_text()
    err = (args.reports / f"{name}.stderr").read_text()
    assert "PASS" in out and "HARDWARE Mali-G925" in out, name
    expected_path = args.layer_path or (f"/files/mavk-runtime/{args.runtime_sha256}/payload/libVkLayer_mavk_compat.so" if args.common else f"/files/mavk-{'aggregate' if args.aggregate else 'wide' if args.wide else 'overflow' if args.overflow else 'nested'}/libVkLayer_mavk_compat.so")
    assert expected_path in out, name
    assert f"MAVK_LAYER_COMMAND_END guarded=1 draws={draws} submissions={submissions}" in err, name
    assert "gaps=0" in err and "MAVK_LAYER_CORE_END liveZero=1" in err, name
    assert "MAVK_LAYER_GAP" not in err and "FAIL " not in out + err, name
    if args.no_diagnostic_reads:
        assert "MAVK_LAYER_GUARD_POLICY checks=1 diagnosticReports=0" in err, name
        assert "MAVK_LAYER_GUARD_LOG" not in err, name
        assert re.search(r"MAVK_LAYER_COMMAND_END .* diagnosticReads=0(?:\n|$)", err), name
    if args.no_event_trace:
        assert "MAVK_LAYER_TRACE_POLICY events=0 errors=always" in err, name
        assert not re.search(r"^MAVK_LAYER_(EXECUTE|UPDATE|TEMPLATE|SECONDARY) ", err, re.M), name
if args.storage_atomic:
    assert "GUARDED_STORAGE_TEXEL_ATOMIC_PASS phases=5 checkedWords=320 validReadModifyWrite=1 oobZeroNoEffects=1 negativeZero=1 diagnosticNull=1 restoredValid=1" in (args.reports / "atomic-guarded.stdout").read_text().splitlines()
    print("GUARDED_STORAGE_TEXEL_ATOMIC_GPU_PASS noCTS noFullFeatureClaim")
if args.texel_components:
    assert "STORAGE_TEXEL_COMPONENTS_PASS phases=3 checkedWords=192 oobAlphaOne=1 nullAllZero=1 validAlphaOne=1 sentinelPreserved=1 restoredValid=1" in (args.reports / "texel-components.stdout").read_text().splitlines()
    print("STORAGE_TEXEL_COMPONENT_SUBSTITUTION_GPU_PASS typedOobAndNullDiffer noCTS")
if args.texel_view_components:
    assert "TEXEL_VIEW_COMPONENTS_GPU_PASS formats=6 phases=18 checkedWords=1872 shaderPipelines=2 perViewMetadata=1 nullAllZero=1 restoredValid=1" in (args.reports / "view-components.stdout").read_text().splitlines()
    assert "MAVK_LAYER_PIPELINE_END pipelines=2" in (args.reports / "view-components.stderr").read_text()
    print("RUNTIME_TEXEL_FORMAT_METADATA_GPU_PASS pipelines=2 formats=6 noCTS")
nested = (args.reports / "nested.stdout").read_text().splitlines()
assert "NESTED_BUFFER_GPU_PASS readZero=1 writeSuppressed=1 sentinelWords=31 inputWords=32 descriptorRange=4 allocationBytes=128" in nested
print("NESTED_BUFFER_GPU_AND_11_REGRESSIONS_PASS noCTS noFeatureAdvertisement")
if args.overflow:
    assert "NESTED_BUFFER_GPU_PASS readZero=1 writeSuppressed=1 sentinelWords=31 inputWords=32 descriptorRange=4 allocationBytes=128" in (args.reports / "overflow.stdout").read_text().splitlines()
    assert "NESTED_BUFFER_GPU_PASS readZero=1 writeSuppressed=1 sentinelWords=31 inputWords=32 descriptorRange=4 allocationBytes=128" in (args.reports / "overflow-add.stdout").read_text().splitlines()
    print("BUFFER_OFFSET_OVERFLOW_GPU_PASS index=1073741824 stride=4 noUnprotectedExecution")
    print("BUFFER_OFFSET_ADD_OVERFLOW_GPU_PASS index=1073741823 stride=4 memberOffset=16")
if args.wide:
    for name in ("high", "negative"):
        assert "NESTED_BUFFER_GPU_PASS readZero=1 writeSuppressed=1 sentinelWords=31 inputWords=32 descriptorRange=4 allocationBytes=128" in (args.reports / f"wide-{name}.stdout").read_text().splitlines()
    assert "WIDE_BUFFER_VALID_GPU_PASS readValue=2779096485 writeSuppressed=1 sentinelWords=31 inputWords=32 descriptorRange=4 allocationBytes=128" in (args.reports / "wide-valid.stdout").read_text().splitlines()
    print("WIDE_BUFFER_GPU_PASS uint64High signed64Negative valid64Read noCTS")
if args.aggregate:
    assert "AGGREGATE_BUFFER_GPU_PASS phases=3 partialReadZero=1 partialWriteSuppressed=1 fullReadWrite=1 checkedWords=192" in (args.reports / "aggregate.stdout").read_text().splitlines()
    print("AGGREGATE_BUFFER_GPU_PASS wholeStruct vec2Members threeRanges noCTS")
if args.copy:
    assert "AGGREGATE_BUFFER_GPU_PASS phases=3 partialReadZero=1 partialWriteSuppressed=1 fullReadWrite=1 checkedWords=192" in (args.reports / "aggregate-copy.stdout").read_text().splitlines()
    print("COPY_MEMORY_GPU_PASS descriptorBounds typedLowering noCTS")
if args.copy_operands:
    assert "AGGREGATE_BUFFER_GPU_PASS phases=3 partialReadZero=1 partialWriteSuppressed=1 fullReadWrite=1 checkedWords=192" in (args.reports / "aggregate-copy-operands.stdout").read_text().splitlines()
    print("COPY_MEMORY_OPERANDS_GPU_PASS spirv14 Vulkan12 distinctReadWriteMasks noCTS")
if args.copied_pointer:
    assert "NESTED_BUFFER_GPU_PASS readZero=1 writeSuppressed=1 sentinelWords=31 inputWords=32 descriptorRange=4 allocationBytes=128" in (args.reports / "copied-pointer.stdout").read_text().splitlines()
    print("COPIED_BUFFER_POINTER_GPU_PASS readZero writeSuppressed noUnprotectedExecution noCTS")
if args.texel_bounds:
    assert "TEXEL_BOUNDS_GPU_PASS formats=2 components=512 coordinates=0,63,-1,64,1073741824 imageOperands=None,ZeroExtend" in (args.reports / "texel-bounds.stdout").read_text().splitlines()
    print("TEXEL_IMAGE_OPERANDS_GPU_PASS validAndOutOfBounds zeroExtensionPreserved noCTS")
if args.storage_texel:
    assert "STORAGE_TEXEL_BOUNDS_GPU_PASS checkedWords=64 viewElements=4 allocationElements=32 readZero=1 writeSuppressed=1 validReadWrite=1 negativeReadZero=1" in (args.reports / "storage-texel.stdout").read_text().splitlines()
    print("STORAGE_TEXEL_BOUNDS_GPU_PASS viewBoundedReadWrite sentinelPreserved noCTS")
