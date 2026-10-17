"""Rebuilt baseline parity, actual RG32F image sampling and signed linear values."""
import argparse
import json
import pathlib
import re

p = argparse.ArgumentParser()
p.add_argument("directory", type=pathlib.Path)
a = p.parse_args()

def load(name, build, result):
    folder = a.directory / name
    text = (folder / "stdout.log").read_text(encoding="utf-8-sig")
    state = json.loads((folder / "summary.json").read_text(encoding="utf-8-sig"))
    if (state["exit_code"] != 0 or state["bc_layer_build"] != build
            or state["bc_compatibility_layer"] is not True
            or state["production_changed"] is not False or result not in text
            or "DEVICE name=Mali-G925-Immortalis MC12 vendor=13b5 type=1" not in text):
        raise SystemExit(f"FAIL: {name} runtime/result")
    return text

before = load("baseline-run", "baseline", "GLIBC_HYBRIS_G925_BC_CONSTANT_PASS")
after = load("rg32f-run", "rg32f", "GLIBC_HYBRIS_G925_BC_CONSTANT_PASS")
linear = load("linear-run", "linear", "GLIBC_HYBRIS_G925_BC_LINEAR_PASS")
for text, fixed in ((before, False), (after, True)):
    rows = re.findall(r"^BC_FORMAT format=(\d+) features=0x([0-9a-f]+) image_result=(-?\d+)$", text, re.M)
    expected = {f: (0, -11) if f < 133 else (0xc001 if f in (141, 142) and not fixed else 0xd001, 0) for f in range(131, 147)}
    if len(rows) != 16 or {int(f): (int(flags, 16), int(r)) for f, flags, r in rows} != expected:
        raise SystemExit("FAIL: format profile/parity")
    pixels = re.findall(r"^BC_CONSTANT_PIXELS_PASS format=(\d+) components=64$", text, re.M)
    if len(pixels) != 14 or {int(f) for f in pixels} != set(range(133, 147)):
        raise SystemExit("FAIL: constant-block regressions")
rows = re.findall(r"^BC_LINEAR_PIXELS_PASS format=(\d+) phase=(\d+) components=64$", linear, re.M)
if len(rows) != 6 or {(int(f), int(phase)) for f, phase in rows} != {(f, phase) for f in (141, 142) for phase in range(3)}:
    raise SystemExit("FAIL: signed/unsigned palette linear coverage")
if "BC_SUMMARY available_formats=2 checked_components=384 textureCompressionBC_not_enabled=1" not in linear:
    raise SystemExit("FAIL: linear summary")
print("PASS: rebuild parity,14format constant regression,BC5 U/S actual bilinear384components")
print("BC5 backing bytes/texel=8 (was4); no performance/full-conformance/nullDescriptor/DXVK PASS")
