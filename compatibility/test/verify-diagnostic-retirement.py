"""Check actual GPU output and report policy; not a rendering benchmark or CTS."""
import argparse
import re
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("reports", type=Path)
parser.add_argument("--baseline", action="store_true")
args = parser.parse_args()
tag = "baseline" if args.baseline else "candidate"
for enabled in (0, 1):
    output = (args.reports / f"diagnostics-{tag}-{enabled}.stdout").read_text()
    error = (args.reports / f"diagnostics-{tag}-{enabled}.stderr").read_text()
    assert "HARDWARE Mali-G925" in output
    assert "/files/mavk-aggregate/libVkLayer_mavk_compat.so" in output
    assert "NESTED_BUFFER_GPU_PASS readZero=1 writeSuppressed=1" in output
    assert f"MAVK_LAYER_GUARD_POLICY checks=1 diagnosticReports={enabled}" in error
    assert "MAVK_LAYER_CORE_END liveZero=1" in error and "gaps=0" in error
    assert "MAVK_LAYER_GAP" not in error and "FAIL " not in error + output
    logs = re.findall(r"^MAVK_LAYER_GUARD_LOG words=(\d+)$", error, re.M)
    if enabled or args.baseline:
        assert logs, "Enabled reporting must still read and emit diagnostics"
        if enabled:
            assert sum(map(int, logs)) > 0, "OOB shader diagnostics must remain live"
    else:
        assert not logs, "Disabled reporting must not emit per-generation logs"
    if not args.baseline:
        reads = re.search(r"MAVK_LAYER_COMMAND_END .* diagnosticReads=(\d+)", error)
        assert reads and int(reads[1]) == len(logs), "Actual diagnostic read count"
    print(f"DIAGNOSTIC_RETIREMENT_GPU_PASS reports={enabled} reads={len(logs)} baseline={args.baseline}")
