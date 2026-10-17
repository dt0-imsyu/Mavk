"""Finite unchanged upstream window app gate, not FPS/conformance proof."""
import argparse
import csv
import io
import json
import math
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--host', type=Path, required=True)
p.add_argument('--guest', type=Path, required=True)
a = p.parse_args()
h = json.loads(a.host.read_text(encoding='utf-8'))
expected = dict(result='BRIDGE_DESCRIPTOR_SESSION_PASS',
                device='Mali-G925-Immortalis MC12', commands=74, gpu_submissions=24,
                clean_eof=True, core_live_zero_after_disconnect=True,
                validation_layers_enabled=True, synchronization_validation_requested=True,
                validation_errors=0, validation_warnings=0,
                channel='app-owned AVF connectVsock4057')
assert all(type(h.get(k)) is type(v) and h[k] == v for k, v in expected.items()), h
assert h['sent_bytes'] >= 24*320*240*4
extraction = json.loads((a.guest/'extraction.json').read_text())
assert extraction['read_only'] is True
assert (a.guest/'exit2.txt').read_text(encoding='utf-8-sig').strip() == '0'
for name in ('benchmark.csv', 'fifo.csv'):
    rows = list(csv.DictReader(io.StringIO((a.guest/name).read_text(encoding='utf-8-sig'))))
    assert len(rows) == 1, rows
    row = rows[0]
    assert row['device'] == 'Mavk experimental / Mali-G925-Immortalis MC12', row
    assert int(row['frames']) == 24 and int(row['driverversion']) == 1, row
    duration = float(row['duration (ms)'])
    assert math.isfinite(duration) and 0 < duration < 60000, row
    assert abs(float(row['fps']) - 24000/duration) < 0.01, row
stderr = (a.guest/'stderr.txt').read_text(encoding='utf-8-sig')
assert 'MAVK_ICD_CREATE_INSTANCE' in stderr
assert 'Fatal' not in stderr and 'terminating' not in stderr, stderr
print(json.dumps(dict(result='WINDOWS_UNMODIFIED_UPSTREAM_TRIANGLE_WSI_MALI_PASS',
                      frames_per_run=24, scope='Pinned unmodified upstream application, bounded immediate and FIFO window runs; CPU readback/blit presentation. Actual Android frame observed separately in fifo1.png. NOT sustained/unique visible FPS, general WSI, game, DWM or conformance PASS.'), indent=2))
