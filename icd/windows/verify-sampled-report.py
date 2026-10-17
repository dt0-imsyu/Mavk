"""Strict finite Windows standard-API texture and actual native Mali gate."""
import argparse
import json
from pathlib import Path
p=argparse.ArgumentParser()
p.add_argument('--host',type=Path,required=True)
p.add_argument('--guest',type=Path,required=True)
a=p.parse_args()
h=json.loads(a.host.read_text(encoding='utf-8'))
expected=dict(result='BRIDGE_DESCRIPTOR_SESSION_PASS',device='Mali-G925-Immortalis MC12',
    commands=29,gpu_submissions=5,clean_eof=True,core_live_zero_after_disconnect=True,
    validation_layers_enabled=True,synchronization_validation_requested=True,
    validation_errors=0,validation_warnings=0,channel='app-owned AVF connectVsock4057')
assert all(type(h.get(k)) is type(v) and h[k]==v for k,v in expected.items()),h
assert h['received_bytes']>2*5592404 and h['sent_bytes']>=3*4096
lines=set((a.guest/'stdout.txt').read_text(encoding='utf-8-sig').splitlines())
assert {'STANDARD_VULKAN_SAMPLED_PIXELS=3072','STANDARD_VULKAN_SAMPLED_PASS',
        'VULKAN_DEVICE=Mavk experimental / Mali-G925-Immortalis MC12'}<=lines,lines
assert (a.guest/'exit.txt').read_text(encoding='utf-8-sig').strip()=='EXIT=0'
assert 'MAVK_ICD_CREATE_INSTANCE' in (a.guest/'stderr.txt').read_text(encoding='utf-8-sig')
print(json.dumps(dict(result='WINDOWS_STANDARD_VULKAN_SAMPLED_MALI_PASS',
    verified_pixels=3072,gpu_submissions=5,
    scope='Own standard Vulkan consumer, 1024 RGBA11mips, 3 renders, reuse and content invalidation, late mapped writes. NOT external textured app, performance, games or conformance proof.'),indent=2))
