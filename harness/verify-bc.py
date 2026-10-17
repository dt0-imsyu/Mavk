"""Strict bounded BC compatibility acceptance, not full Vulkan BC support."""
import argparse
import hashlib
import json
from pathlib import Path

p=argparse.ArgumentParser()
p.add_argument("report",type=Path)
p.add_argument("--channels",action="store_true")
p.add_argument("--hdr",action="store_true")
a=p.parse_args()
data=a.report.read_bytes()
r=json.loads(data)
assert not (a.channels and a.hdr)
assert r["result"]==("EMULATED_BC67_NATIVE_SAMPLING_PASS" if a.hdr else "EMULATED_BC45_NATIVE_SAMPLING_PASS" if a.channels else "EMULATED_BC123_NATIVE_SAMPLING_PASS")
assert r["device"]=="Mali-G925-Immortalis MC12" and r["classification"]=="EMULATED"
if a.hdr:
    assert (r["verified_pixels"],r["variants"],r["mips_per_variant"],r["gpu_submissions"])==(12288,3,4,12)
    assert r["negative_input_components"]>0 and r["hdr_input_components"]>0
else:
    assert (r["verified_pixels"],r["formats"],r["mips_per_format"],r["gpu_submissions"])==(16384,4,4,16)
assert r["core_live_zero"] is True and r["textureCompressionBC_advertised"] is False
assert r["validation_layers_enabled"] is True and r["synchronization_validation_requested"] is True
assert r["validation_callback_tests"]>=1
assert r["validation_errors"]==0 and r["validation_warnings"]==0 and not r["validation_messages"]
if not a.channels and not a.hdr:
    assert [s["format"] for s in r["samples"]]==[0,1,2,3]
print(("BC67" if a.hdr else "BC45" if a.channels else "BC123")+"_NATIVE_SAMPLING_VERIFY_PASS sha256="+hashlib.sha256(data).hexdigest()+" NOT_FULL_BC_OR_DXVK_PASS")
