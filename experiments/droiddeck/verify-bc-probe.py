"""Pinned constant-block probe, not general BC/DXVK correctness."""
import argparse
import json
import pathlib
import re

p = argparse.ArgumentParser()
p.add_argument("directory", type=pathlib.Path)
p.add_argument("--require-dxvk-formats", action="store_true")
a = p.parse_args()
baseline = (a.directory / "run/stdout.log").read_text(encoding="utf-8-sig")
actual = (a.directory / "compat/stdout.log").read_text(encoding="utf-8-sig")
summary = json.loads((a.directory / "compat/summary.json").read_text(encoding="utf-8-sig"))
control = json.loads((a.directory / "run/summary.json").read_text(encoding="utf-8-sig"))

def formats(text):
    rows = re.findall(r"^BC_FORMAT format=(\d+) features=0x([0-9a-f]+) image_result=(-?\d+)$", text, re.M)
    if len(rows) != 16 or {int(x[0]) for x in rows} != set(range(131, 147)):
        raise SystemExit("FAIL: missing/duplicate format rows")
    return {int(f): (int(flags, 16), int(result)) for f, flags, result in rows}

before, after = formats(baseline), formats(actual)
if control["exit_code"] != 2 or any(value != (0, -11) for value in before.values()):
    raise SystemExit("FAIL: bare ICD control changed")
if (summary["exit_code"] != 0 or summary["bc_constant_pass"] is not True
        or summary["bc_compatibility_layer"] is not True
        or summary["production_changed"] is not False
        or "DEVICE name=Mali-G925-Immortalis MC12 vendor=13b5 type=1" not in actual):
    raise SystemExit("FAIL: runtime/device/result")
pixels = re.findall(r"^BC_CONSTANT_PIXELS_PASS format=(\d+) components=64$", actual, re.M)
if len(pixels) != 14 or {int(f) for f in pixels} != set(range(133, 147)):
    raise SystemExit("FAIL: hardware sample coverage")
if "BC_SUMMARY available_formats=14 checked_components=896 textureCompressionBC_not_enabled=1" not in actual:
    raise SystemExit("FAIL: coverage summary")
if any(after[f] != (0, -11) for f in (131, 132)):
    raise SystemExit("FAIL: RGB-only BC1 must remain unsupported by this profile")
for f in range(133, 147):
    flags, result = after[f]
    if result or flags & 0xc001 != 0xc001:
        raise SystemExit(f"FAIL: format {f} sample/transfer admission")
missing_linear = [f for f in range(133, 147) if not after[f][0] & 0x1000]
print("PASS: glibc/proot -> hybris compat -> G925,14formats,896 constant components")
print(f"DXVK FORMAT ADMISSION: {'NO-GO' if missing_linear else 'capability-only PASS'}; missing FILTER_LINEAR={missing_linear}")
print("No validation/performance/full-codec/D3D11 PASS is inferred.")
if a.require_dxvk_formats and missing_linear:
    raise SystemExit(2)
