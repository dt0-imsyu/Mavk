"""Bounded command-recording A/B; no GPU-throughput or visible-FPS inference."""
import argparse
import re
import statistics
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("reports", type=Path)
args = parser.parse_args()
timings = {0: [], 1: []}
for run in range(3):
    for trace in (0, 1):
        out = (args.reports / f"trace-{run}-{trace}.stdout").read_text()
        err = (args.reports / f"trace-{run}-{trace}.stderr").read_text()
        assert "HARDWARE Mali-G925" in out
        assert "/files/mavk-aggregate/libVkLayer_mavk_compat.so" in out
        assert "NESTED_BUFFER_GPU_PASS readZero=1 writeSuppressed=1" in out
        assert f"MAVK_LAYER_TRACE_POLICY events={trace} errors=always" in err
        assert "MAVK_LAYER_GUARD_POLICY checks=1 diagnosticReports=0" in err
        assert "MAVK_LAYER_COMMAND_END guarded=1 draws=40961 submissions=1" in err
        assert "diagnosticReads=0" in err and "MAVK_LAYER_GUARD_LOG" not in err
        assert "MAVK_LAYER_CORE_END liveZero=1" in err and "gaps=0" in err
        assert "MAVK_LAYER_GAP" not in err and "FAIL " not in out + err
        assert err.count("MAVK_LAYER_EXECUTE guarded=1 ") == (40961 if trace else 0)
        samples = re.findall(r"^RECORD_OVERHEAD_SAMPLE index=(\d+) calls=8192 wallNs=(\d+) submitted=0$", out, re.M)
        assert [int(index) for index, _ in samples] == list(range(5))
        assert all(int(ns) > 0 for _, ns in samples)
        timings[trace].extend(int(ns) / 8192 for index, ns in samples if int(index) > 0)
for trace, samples in timings.items():
    print(f"TRACE_RECORD_OVERHEAD_PASS trace={trace} samples={len(samples)} medianWallNsPerCall={statistics.median(samples):.1f}")
negative = (args.reports / "trace-missing.stdout").read_text()
error = (args.reports / "trace-missing.stderr").read_text()
assert "STATIC_USED_SET_REJECTION_PASS missingSet=2 submissions=0" in negative
assert "MAVK_LAYER_TRACE_POLICY events=0 errors=always" in error
assert "MAVK_LAYER_GAP operation=guarded execution reason=guarded descriptor compatibility" in error
assert "gaps=1" in error and "MAVK_LAYER_COMMAND_END guarded=1 draws=0 submissions=0" in error
assert "MAVK_LAYER_EXECUTE" not in error and "MAVK_LAYER_CORE_END liveZero=1" in error
print("TRACE_DISABLED_ERROR_VISIBLE_PASS rejectedBeforeSubmit=1")
print(f"TRACE_RECORD_AB_RATIO onOverOff={statistics.median(timings[1])/statistics.median(timings[0]):.3f} noVisibleFPSClaim=1")
