"""Strict standard Windows Vulkan compute + actual Mali execution gate."""
import argparse
import json
from pathlib import Path
p=argparse.ArgumentParser()
p.add_argument('--host',type=Path,required=True)
p.add_argument('--guest',type=Path,required=True)
a=p.parse_args()
h=json.loads(a.host.read_text(encoding='utf-8'))
expected=dict(result='BRIDGE_DESCRIPTOR_SESSION_PASS',device='Mali-G925-Immortalis MC12',
              commands=4,gpu_submissions=2,clean_eof=True,core_live_zero_after_disconnect=True,
              validation_layers_enabled=True,synchronization_validation_requested=True,
              validation_errors=0,validation_warnings=0,channel='app-owned AVF connectVsock4057')
assert all(type(h.get(k)) is type(v) and h[k]==v for k,v in expected.items()),h
assert h['sent_bytes']>=8192*4
lines=set((a.guest/'stdout.txt').read_text(encoding='utf-8-sig').splitlines())
assert {'STANDARD_VULKAN_COMPUTE_WORDS=8192','STANDARD_VULKAN_COMPUTE_PASS',
        'VULKAN_DEVICE=Mavk experimental / Mali-G925-Immortalis MC12'}<=lines,lines
assert (a.guest/'exit.txt').read_text(encoding='utf-8-sig').strip()=='EXIT=0'
assert 'MAVK_ICD_CREATE_INSTANCE' in (a.guest/'stderr.txt').read_text(encoding='utf-8-sig')
print(json.dumps(dict(result='WINDOWS_STANDARD_VULKAN_COMPUTE_MALI_PASS',verified_words=8192,
                     gpu_submissions=2,scope='Own standard API consumer, bounded 1D one-SSBO compute, late mapped writes, guard bytes. NOT general compute/conformance/performance PASS.'),indent=2))
