"""Gate bounded Android Core suite; no guest/API/conformance substitution."""
import argparse
import hashlib
import json
from pathlib import Path
p=argparse.ArgumentParser()
p.add_argument('--reports',type=Path,required=True)
p.add_argument('--fresh',action='store_true')
a=p.parse_args()
s=json.loads((a.reports/'feature-suite.json').read_text(encoding='utf-8'))
assert s['schema']==1 and s['result']=='MAVK_CORE_FEATURE_SUITE_PASS'
assert s['evidence_mode'] in ('FRESH_RUN','CACHED_REPORTS')
if a.fresh:assert s['evidence_mode']=='FRESH_RUN'
expected={'core-comparison.json':'CORE_COMPUTE_AND_IMAGE_AB_PASS',
          'core-sampled.json':'NATIVE_CORE_SAMPLED_MIPS_PASS'}
# Existing two-gate archived suite remains verifiable. A newly implemented
# timeline row requires its real third source; never infer it from old fences.
timeline_rows=[x for x in s['features'] if x['feature']=='core.timeline_semaphore_wrapper']
assert len(timeline_rows)==1
if timeline_rows[0]['implementation']=='NATIVE':expected['core-timeline.json']='NATIVE_CORE_TIMELINE_PASS'
reuse_rows=[x for x in s['features'] if x['feature']=='graphics.retained_resources']
if reuse_rows:
    assert len(reuse_rows)==1 and reuse_rows[0]['implementation']=='NATIVE'
    expected['core-reuse.json']='NATIVE_SHARED_GRAPHICS_REUSE_PASS'
raster_rows=[x for x in s['features'] if x['feature'] in ('graphics.straight_alpha_source_over','graphics.dynamic_scissor')]
if raster_rows:
    assert len(raster_rows)==2 and all(x['implementation']=='NATIVE' for x in raster_rows)
    expected['core-raster.json']='NATIVE_SHARED_RASTER_PASS'
assert {x['file'] for x in s['sources']}==set(expected)
devices=set()
for source in s['sources']:
    raw=(a.reports/source['file']).read_bytes()
    assert hashlib.sha256(raw).hexdigest()==source['sha256'] and source['status']=='PASS'
    r=json.loads(raw);assert r==source['report'] and r['result']==expected[source['file']]
    assert r['core_live_zero'] is True and r['validation_layers_enabled'] is True
    assert r['synchronization_validation_requested'] is True
    assert r['validation_errors']==0 and r['validation_warnings']==0
    devices.add(r['device'])
    if source['file']=='core-sampled.json':
        assert r['pixels_verified']==12288 and r['gpu_submissions']==12 and len(r['samples'])==12
    elif source['file']=='core-comparison.json':assert r['verified_words']>0 and r['verified_image_pixels']>0
    elif source['file']=='core-timeline.json':
        assert r['native_timeline_enabled'] is True and r['verified_words']==8192
        assert r['gpu_submissions']==4 and r['negative_tests']==3 and r['timeout_tests']==1
        assert r['final_counter']==5 and r['host_waits_for_gpu']==2
    elif source['file']=='core-reuse.json':
        assert r['verified_pixels']==32768 and r['frames_per_arm']==16
        assert r['gpu_submissions']==32 and r['negative_tests']==2 and r['distinct_lod_outputs']==4
        assert r['baseline_pipeline_creates']==16 and r['retained_pipeline_creates']==1
        assert r['retained_per_frame_allocations']==0
        assert r['baseline_allocations']>r['retained_allocations']>0
        assert all(r[k]>0 for k in ('retained_initialize_ms','baseline_median_ms','retained_median_ms'))
    else:
        assert r['verified_pixels']==18432 and r['gpu_submissions']==18
        assert r['pipeline_creates']==6 and r['negative_tests']==30
        assert r['source_alpha_cases']==[0,128,255]
        assert r['blend_modes']==['replace','straight_alpha_source_over']
        assert r['clip_phases']==['full','partial','full_reset']
        assert r['per_frame_gpu_allocations']==0
assert len(devices)==1
assert len({x['feature'] for x in s['features']})==len(s['features'])
for row in s['features']:
    assert row['implementation'] in ('NATIVE','EMULATED','UNSUPPORTED') and row['scope']
    if row['implementation']=='NATIVE':assert row['status']=='PASS'
    elif row['implementation']=='EMULATED':assert row['status']=='ADAPTER_GATE_SEPARATE'
    else:assert row['status']=='NOT_IMPLEMENTED'
assert {x['implementation'] for x in s['features']}=={'NATIVE','EMULATED','UNSUPPORTED'}
print(json.dumps(dict(result='ANDROID_BOUNDED_CORE_FEATURE_SUITE_PASS',
    evidence_mode=s['evidence_mode'],device=next(iter(devices)),
    native_features=sum(x['implementation']=='NATIVE' for x in s['features']),
    scope='Existing Core workloads, not full Vulkan, adapter, game, WDDM or CTS coverage.'),indent=2))
