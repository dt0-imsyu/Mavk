"""Bounded own Win32 WSI consumer; not third-party WSI/conformance/FPS proof."""
import argparse
import json
import re
from pathlib import Path
p = argparse.ArgumentParser()
p.add_argument('--host', type=Path, required=True)
p.add_argument('--guest', type=Path, required=True)
a = p.parse_args()
h = json.loads(a.host.read_text(encoding='utf-8'))
expected = dict(result='BRIDGE_DESCRIPTOR_SESSION_PASS', device='Mali-G925-Immortalis MC12',
                # Hello + DeviceIdentity +12 DrawOffscreen; close is EOF, not an opcode.
                commands=14, gpu_submissions=12, clean_eof=True,
                core_live_zero_after_disconnect=True, validation_layers_enabled=True,
                synchronization_validation_requested=True, validation_errors=0,
                validation_warnings=0, channel='app-owned AVF connectVsock4057')
assert all(type(h.get(k)) is type(v) and h[k] == v for k, v in expected.items()), h
assert h['sent_bytes'] >= 12*320*240*4
stdout = (a.guest/'stdout.txt').read_text(encoding='utf-8-sig')
assert 'VULKAN_DEVICE=Mavk experimental / Mali-G925-Immortalis MC12' in stdout
m = re.search(r'WINDOW_VULKAN_PRESENT_PASS frames=12 distinct_window_grid_hashes=(\d+) size=320x240', stdout)
assert m and 8 <= int(m[1]) <= 12, stdout
assert 'MAVK_ICD_CREATE_INSTANCE' in (a.guest/'stderr.txt').read_text(encoding='utf-8-sig')
assert (a.guest/'exit.txt').read_text(encoding='utf-8-sig').strip() == 'EXIT=0'
print(json.dumps(dict(result='WINDOWS_MALI_VULKAN_WIN32_WSI_FUNCTIONAL_PASS',
                     rendered_and_presented=12, distinct_guest_window_grid_hashes=int(m[1]),
                     size=[320, 240],
                     scope='Own standard Vulkan consumer. CPU readback/blit WSI; NOT zero-copy, third-party WSI, conformance, DWM or Android visible-FPS PASS.'), indent=2))
