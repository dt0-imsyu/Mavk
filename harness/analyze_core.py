"""Bounded direct/Core ABBA; no throughput or no-sync-error claim."""
import argparse
import json
import math
import statistics
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--reports', type=Path, nargs='+', required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
assert not a.output.exists()
reports = [json.loads(path.read_text()) for path in a.reports]
for r in reports:
    assert r['result'] == 'CORE_COMPUTE_AND_IMAGE_AB_PASS'
    assert r['vendor_id'] == 0x13b5 and r['same_device_queue'] and r['core_live_zero']
    assert len(r['phases']) == 12 and len(r['image_phases']) == 12
    assert all(v['core_live_zero'] for v in r['phases'] + r['image_phases'])

def stats(values):
    values = sorted(values)
    return {name: values[min(len(values)-1, math.ceil(len(values)*q)-1)]
            for name, q in [('p50', .5), ('p95', .95), ('p99', .99)]}

rows = []
for words in [4096, 65536, 262144]:
    row = {'words': words, 'runs': []}
    for r in reports:
        phases = [v for v in r['phases'] if v['words'] == words]
        assert [v['core'] for v in phases] == [False, True, True, False]
        assert len({v['memory_type'] for v in phases}) == 1
        run = {}
        for core, name in [(False, 'direct'), (True, 'core')]:
            arms = [v for v in phases if v['core'] == core]
            samples = [s for v in arms for s in v['samples'] if s['round'] >= r['warmup_rounds']]
            assert len(samples) == 224
            run[name] = {k: stats([s[k] for s in samples]) for k in
                         ['record_ms', 'submit_ms', 'wait_ms', 'gpu_ms', 'verify_ms']}
            run[name]['record_plus_submit_ms'] = stats([s['record_ms']+s['submit_ms'] for s in samples])
            run[name]['create_ms'] = [v['create_ms'] for v in arms]
            run[name]['teardown_ms'] = [v['teardown_ms'] for v in arms]
        run['core_minus_direct_record_submit_p50_us'] = 1000*(
            run['core']['record_plus_submit_ms']['p50']-run['direct']['record_plus_submit_ms']['p50'])
        row['runs'].append(run)
    row['median_run_delta_us'] = statistics.median(v['core_minus_direct_record_submit_p50_us'] for v in row['runs'])
    rows.append(row)

result = {
    'scope': 'Same-device compute and image transfer correctness/resource lifecycle. Not graphics-pipeline/Surface Core PASS.',
    'reports': [str(v) for v in a.reports], 'device': reports[0]['device'],
    'compute_words_verified': sum(r['verified_words'] for r in reports),
    'image_pixels_verified': sum(r['verified_image_pixels'] for r in reports),
    'all_core_live_zero': True, 'compute_ab': rows,
    'validation_layer_pass': False,
    'overhead_caveat': 'Sequential on-device timing; small deltas may be scheduler/DVFS noise, not causal speedup. No zero-overhead claim.',
}
with a.output.open('x') as f:
    json.dump(result, f, indent=2)
print(json.dumps({k: v for k, v in result.items() if k != 'compute_ab'}))
for row in rows:
    print('words', row['words'], 'median run delta us', row['median_run_delta_us'])
