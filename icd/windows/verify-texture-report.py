"""Finite unchanged texture app gate. Android image inspection separate."""
import argparse
import csv
import io
import json
import math
from pathlib import Path
p=argparse.ArgumentParser()
p.add_argument('--host',type=Path,required=True)
p.add_argument('--guest',type=Path,required=True)
a=p.parse_args()
h=json.loads(a.host.read_text(encoding='utf-8'))
expected=dict(result='BRIDGE_DESCRIPTOR_SESSION_PASS',device='Mali-G925-Immortalis MC12',
    commands=79,gpu_submissions=25,clean_eof=True,core_live_zero_after_disconnect=True,
    validation_layers_enabled=True,synchronization_validation_requested=True,
    validation_errors=0,validation_warnings=0,channel='app-owned AVF connectVsock4057')
assert all(type(h.get(k)) is type(v) and h[k]==v for k,v in expected.items()),h
assert h['received_bytes']>1398100 and h['sent_bytes']>=24*320*240*4
assert json.loads((a.guest/'extraction.json').read_text())['read_only'] is True
def guest_text(name):
    raw=(a.guest/name).read_bytes()
    return raw.decode('utf-16' if raw.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig')
assert guest_text('exit2.txt').strip()=='0'
assert guest_text('exit3.txt').strip()=='0'
rows=list(csv.DictReader(io.StringIO((a.guest/'benchmark.csv').read_text(encoding='utf-8-sig'))))
assert len(rows)==1
r=rows[0]
assert r['device']=='Mavk experimental / Mali-G925-Immortalis MC12' and int(r['frames'])==24
duration=float(r['duration (ms)']);assert math.isfinite(duration) and 0<duration<60000
assert abs(float(r['fps'])-24000/duration)<0.01
stderr=(a.guest/'stderr.txt').read_text(encoding='utf-8-sig')
assert 'MAVK_ICD_CREATE_INSTANCE' in stderr and 'Fatal' not in stderr and 'terminating' not in stderr
fifo=list(csv.DictReader(io.StringIO(guest_text('fifo.csv'))))
assert len(fifo)==1 and int(fifo[0]['frames'])==24 and fifo[0]['device']==r['device']
print(json.dumps(dict(result='WINDOWS_UNMODIFIED_UPSTREAM_TEXTURE_WSI_MALI_PASS',frames=24,
    scope='Pinned unchanged textured application, original KTX10mips, actual Mali render and CPU readback/blit WSI. NOT game, DWM acceleration, sustained unique visible FPS or conformance proof.'),indent=2))
