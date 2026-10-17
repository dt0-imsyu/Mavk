"""Verify bounded active Surface loss/recreation; RUNNING is never a PASS."""
import argparse
import json
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--directory', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
assert not a.output.exists()
manifest = json.loads((a.directory/'graphics-lifecycle.json').read_text())
assert manifest['result'] == 'BOUNDED_CYCLES_COMPLETE'
assert (manifest['starts'], manifest['completions'], manifest['recreations']) == (3, 3, 2)
events = manifest['events']
assert sum(e['event'] == 'surface_created' for e in events) == 3
assert sum(e['event'] == 'surface_destroyed' for e in events) == 2
assert sum(e['event'] == 'remove_active_surface' for e in events) == 2
active = None
names = []
for event in events:
    if event['event'] == 'native_start':
        assert active is None, 'Overlapping native owners'
        active = event['report']
        names.append(active)
    if event['event'] == 'native_retired':
        assert active == event['report']
        active = None
assert active is None and len(set(names)) == 3
results = [json.loads((a.directory/name).read_text()) for name in names]
for i, r in enumerate(results):
    assert r['core'] and r['core_live_zero_after_teardown']
    assert r['vendor_id'] == 0x13b5 and r['device_type'] != 4
    assert r['validation_layers_enabled'] and r['synchronization_validation_requested']
    assert r['validation_callback_tests'] == 1 and r['validation_errors'] == 0
    assert r['offscreen_pixels_verified'] == 3330 and r['present_retirement_fences']
    assert r['present_calls'] == len(r['samples']) and r['present_calls'] > 10
    assert r['width'] == (1280 if i == 1 else 1920)
    assert r['height'] == (800 if i == 1 else 1200)
    if i < 2:
        assert r['cancelled'] and r['result'] == 'INTERRUPTED_NOT_SUSTAINED_PASS'
    else:
        assert not r['cancelled'] and r['result'] == 'OFFSCREEN_AND_SURFACE_SUBMISSION_PASS'
compute = json.loads((a.directory/'core-comparison.json').read_text())
assert compute['result'] == 'CORE_COMPUTE_AND_IMAGE_AB_PASS'
assert compute['validation_layers_enabled'] and compute['synchronization_validation_requested']
assert compute['validation_errors'] == 0 and compute['validation_callback_tests'] == 1
assert compute['syncval_expected_unsubmitted_hazards'] == 1
assert compute['core_live_zero']
assert all(r['validation_implementation_version'] == compute['validation_implementation_version'] for r in results)
summary = {
    'result': 'BOUNDED_CORE_SYNCVAL_LIFECYCLE_PASS',
    'scope': 'Compute/image ABBA + two active Surface losses + third sustained Surface, not exhaustive lifetime/security/feature validation',
    'compute_words_verified': compute['verified_words'],
    'image_pixels_verified': compute['verified_image_pixels'],
    'syncval_expected_hazards_never_submitted': compute['syncval_expected_unsubmitted_hazards'],
    'actual_workload_validation_errors': sum(r['validation_errors'] for r in results)+compute['validation_errors'],
    'validation_warnings': sum(r['validation_warnings'] for r in results)+compute['validation_warnings'],
    'graphics_reports': names,
    'graphics_frames_each_cycle': [r['present_calls'] for r in results],
    'all_core_live_zero_after_teardown': True,
}
with a.output.open('x') as f:
    json.dump(summary, f, indent=2)
print(json.dumps(summary))
