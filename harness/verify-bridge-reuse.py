"""Bounded shared-Bridge reuse gate; not Windows or visible-FPS evidence."""
import argparse
import hashlib
import json
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('report', type=Path)
a = p.parse_args()
raw = a.report.read_bytes()
r = json.loads(raw)
assert r['result'] == 'BRIDGE_LOCAL_GPU_RETAINED_RENDERER_PASS'
assert r['pixels_verified'] == 4096 and r['uploaded_bytes'] == 5592404
assert r['mip_levels'] == 11 and r['commands'] == 40 and r['negative_tests'] == 16
n = r['native']
assert n['result'] == 'BRIDGE_DESCRIPTOR_SESSION_PASS'
assert n['clean_eof'] is True and n['core_live_zero_after_disconnect'] is True
assert n['commands'] == r['commands'] and n['gpu_submissions'] == 5
assert n['pipeline_creates'] == 3  # compute + two graphics, not four graphics
assert n['validation_layers_enabled'] is True
assert n['synchronization_validation_requested'] is True
assert n['validation_errors'] == 0 and n['validation_warnings'] == 0
assert n['received_bytes'] > r['uploaded_bytes'] and n['sent_bytes'] > 4096 * 4
assert n['device'] == 'Mali-G925-Immortalis MC12'
print(json.dumps(dict(result='SHARED_BRIDGE_REUSE_GATE_PASS',
    sha256=hashlib.sha256(raw).hexdigest(), device=n['device'],
    verified_pixels=r['pixels_verified'], gpu_submissions=n['gpu_submissions'],
    pipeline_creates=n['pipeline_creates'],
    scope='Android same-UID descriptor protocol gate, not guest or visible FPS.'), indent=2))
