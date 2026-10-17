"""Verify actual upstream native UI packet slice, not full app/guest coverage."""
import argparse
import hashlib
import json
from pathlib import Path
p=argparse.ArgumentParser()
p.add_argument('report',type=Path)
mode=p.add_mutually_exclusive_group()
mode.add_argument('--layers',action='store_true')
mode.add_argument('--bridge',action='store_true')
a=p.parse_args()
raw=a.report.read_bytes();r=json.loads(raw)
assert r['result']==('NATIVE_UI_BRIDGE_LAYERS_PASS' if a.bridge else 'NATIVE_UPSTREAM_UI_LAYERS_PASS' if a.layers else 'NATIVE_UPSTREAM_UI_PACKETS_PASS')
assert r['upstream_commit']=='4ee2501c8280cf71d1f9ce231cd8ebd47b58adde'
assert r['imgui_version']=='1.65' and r['device']=='Mali-G925-Immortalis MC12'
assert r['frames']==2 and r['verified_pixels']==4608
assert r['gpu_submissions']==3 and r['pipeline_creates']==(5 if a.bridge else 4 if a.layers else 2)
if a.layers or a.bridge:
    assert r['passes_per_frame']==2 and r['negative_tests']==(19 if a.bridge else 4)
    assert r['shared_color_targets']==1 and r['intermediate_readbacks']==0
if a.bridge:
    assert r['bridge_commands']==30
    assert r['transport']=='in-process encoded Bridge request/response, NOT FD/vsock'
assert 0<r['font_width']<=1024 and 0<r['font_height']<=1024
assert r['widget_draws']>=2 and r['nonzero_base_vertex_draws']>=4
assert r['indexed_draws']>=r['widget_draws']+4 and r['changed_widget_pixels']>=10
assert r['core_live_zero'] is True and r['validation_layers_enabled'] is True
assert r['synchronization_validation_requested'] is True
assert r['validation_errors']==0 and r['validation_warnings']==0
print(json.dumps(dict(result='NATIVE_UI_BRIDGE_LAYER_GATE_PASS' if a.bridge else 'NATIVE_UPSTREAM_UI_LAYER_GATE_PASS' if a.layers else 'NATIVE_UPSTREAM_UI_PACKET_GATE_PASS',
    sha256=hashlib.sha256(raw).hexdigest(),verified_pixels=r['verified_pixels'],
    changed_widget_pixels=r['changed_widget_pixels'],
    scope='Original UI packets; layered mode preserves background color across passes. Not a complete app/guest/Surface or visible-FPS gate.'),indent=2))
