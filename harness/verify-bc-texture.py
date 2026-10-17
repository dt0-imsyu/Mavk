"""Bounded common BC texture lifecycle acceptance, not Vulkan/DXVK admission."""
import argparse
import hashlib
import json
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument("report",type=Path);a=p.parse_args()
raw=a.report.read_bytes();r=json.loads(raw)
assert r["result"]=="EMULATED_BC_TEXTURE_LIFECYCLE_PASS" and r["device"]=="Mali-G925-Immortalis MC12"
for k,v in {"verified_pixels":6144,"variants":2,"gpu_submissions":12,"pipeline_creates":2,"decoded_blocks":18,"partial_upload_bytes":256,"negative_tests":6}.items():
    assert r[k]==v,(k,r[k])
for k in ("image_pipeline_retained","srgb_native_sampling","core_live_zero","validation_layers_enabled","synchronization_validation_requested"):
    assert r[k] is True,k
assert r["textureCompressionBC_advertised"] is False
assert r["validation_callback_tests"]>=1 and r["validation_errors"]==0 and r["validation_warnings"]==0 and not r["validation_messages"]
print("BC_TEXTURE_LIFECYCLE_VERIFY_PASS sha256="+hashlib.sha256(raw).hexdigest()+" NOT_FULL_VULKAN_API_OR_DXVK_PASS")
