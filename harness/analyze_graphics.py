"""Correlate native graphics samples with independent SurfaceFlinger timestamps.

The fixed interior window excludes exactly1s at each end, declared in advance.
Never label native present-call rate as actual compositor presentation FPS.
"""
import argparse
import json
import math
from pathlib import Path

p=argparse.ArgumentParser()
p.add_argument('--native',type=Path,required=True)
p.add_argument('--present',type=Path,required=True)
p.add_argument('--output',type=Path,required=True)
a=p.parse_args()
assert not a.output.exists()
n=json.loads(a.native.read_text())
s=json.loads(a.present.read_text())
assert n['result']=='OFFSCREEN_AND_SURFACE_SUBMISSION_PASS' and not n['cancelled']
assert n['vendor_id']==0x13b5 and n['device_type']!=4
assert (n['width'],n['height'])==(1920,1200)
assert n['offscreen_pixels_verified']>=3000 and n['present_retirement_fences']
assert n['present_calls']==len(n['samples'])
assert 'SurfaceView[org.mavk.probe/org.mavk.probe.GraphicsActivity]' in s['layer']
assert s['columns'][1]=='actual_present_ns'
begin=n['native_start_monotonic_ns'];end=n['native_end_monotonic_ns']
assert 29e9<=end-begin<=32e9
actual=sorted({int(row[1]) for row in s['frames'] if 0<int(row[1])<2**63-1})
whole=[t for t in actual if begin<=t<end]
interior_begin=begin+1_000_000_000
interior_end=end-1_000_000_000
interior=[t for t in actual if interior_begin<=t<interior_end]
assert len(interior)>300,'Missing/cross-clock/stale Surface timestamps; not FPS PASS'
seconds=(interior_end-interior_begin)/1e9
gaps=[(b-c)/1e6 for c,b in zip(interior,interior[1:])]
def stats(values):
    values=sorted(values)
    return {k:values[min(len(values)-1,math.ceil(len(values)*q)-1)] for k,q in [('p50',.5),('p95',.95),('p99',.99)]}|{'min':values[0],'max':values[-1]}
warm=[v for v in n['samples'] if 1000<=v['elapsed_ms']<n['elapsed_seconds']*1000-1000]
result={
    'scope':'Native Mavk harness moving triangle only; not guest GPU, games or backend overhead',
    'core':n.get('core',False),
    'core_live_zero_after_teardown':n.get('core_live_zero_after_teardown'),
    'device':n['device'],'width':n['width'],'height':n['height'],
    'offscreen_pixels_verified':n['offscreen_pixels_verified'],
    'native_present_calls':n['present_calls'],'native_present_call_rate':n['present_call_rate'],
    'native_duration_seconds':(end-begin)/1e9,
    'unique_surface_present_timestamps_full_window':len(whole),
    'fixed_trim_seconds_each_end':1,'fixed_interior_seconds':seconds,
    'unique_surface_present_timestamps_interior':len(interior),
    'fixed_interior_surface_present_fps':len(interior)/seconds,
    'actual_present_gap_ms':stats(gaps),
    'warm_native_cpu_ms':{key:stats([v[key] for v in warm]) for key in ['acquire_ms','record_ms','submit_present_ms','fence_wait_ms']},
    'per_pixel_uniqueness_verified':False,'validation_layer_pass':False,
    'direct_vs_core_overhead_measured':False,
}
a.output.write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result))
