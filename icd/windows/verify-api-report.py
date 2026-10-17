"""Verify the experimental API consumer + actual Mali report, not conformance."""
import argparse
import json
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--host', type=Path, required=True)
p.add_argument('--guest', type=Path, required=True, help='Cold own API logs directory')
mode = p.add_mutually_exclusive_group()
mode.add_argument('--vertices', action='store_true', help='Require vertex-buffer API test, not earlier VertexIndex result')
mode.add_argument('--indexed', action='store_true', help='Require staging-copy and indexed-buffer API test')
mode.add_argument('--matrix', action='store_true', help='Require nonidentity64B vertex matrix and partial push updates')
a = p.parse_args()
host = json.loads(a.host.read_text(encoding='utf-8'))

def text(path):
    b = path.read_bytes()
    return b.decode('utf-16' if b.startswith((b'\xff\xfe', b'\xfe\xff')) else 'utf-8-sig')

expected = dict(result='BRIDGE_DESCRIPTOR_SESSION_PASS', device='Mali-G925-Immortalis MC12',
                commands=3, gpu_submissions=1, clean_eof=True,
                core_live_zero_after_disconnect=True, validation_layers_enabled=True,
                synchronization_validation_requested=True, validation_errors=0,
                validation_warnings=0, channel='app-owned AVF connectVsock4057')
assert all(type(host.get(k)) is type(v) and host[k] == v for k, v in expected.items()), host
lines = set(text(a.guest/'stdout.txt').splitlines())
samples = 3906 if a.matrix else 3330
prefix = 'STANDARD_VULKAN_MATRIX' if a.matrix else ('STANDARD_VULKAN_INDEXED_STAGING' if a.indexed else ('STANDARD_VULKAN_VERTEX_BUFFER' if a.vertices else 'STANDARD_VULKAN_BLUE'))
pass_marker = 'STANDARD_VULKAN_OFFSCREEN_PASS' if prefix == 'STANDARD_VULKAN_BLUE' else prefix + '_PASS'
assert {prefix + '_PIXELS=' + str(samples), pass_marker,
        'VULKAN_DEVICE=Mavk experimental / Mali-G925-Immortalis MC12'} <= lines, lines
assert text(a.guest/'exit.txt').strip() == 'EXIT=0'
assert 'MAVK_ICD_CREATE_INSTANCE' in text(a.guest/'stderr.txt')
print(json.dumps(dict(result=('WINDOWS_ARM64_VULKAN_API_MATRIX_MALI_PASS' if a.matrix else ('WINDOWS_ARM64_VULKAN_API_INDEXED_STAGING_MALI_PASS' if a.indexed else ('WINDOWS_ARM64_VULKAN_API_VERTEX_BUFFER_MALI_PASS' if a.vertices else 'WINDOWS_ARM64_VULKAN_API_OFFSCREEN_MALI_PASS'))),
    app_verified_blue_pixels=samples, gpu_submissions=1,
    scope='Independent API-only test in this tree; limited non-conformant ICD. NOT third-party game, swapchain, WDDM, DWM or Vulkan 1.3 PASS.'), indent=2))
