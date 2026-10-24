"""Check the shared immutable-sampler GPU oracle; not CTS or a full feature gate."""
import argparse
import json
from pathlib import Path

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('reports', type=Path)
a = p.parse_args()
native = json.loads((a.reports / 'native-android.json').read_text())
control = json.loads((a.reports / 'native-android-control.json').read_text())
for report in (native, control):
    assert report['device'] == 'Mali-G925-Immortalis MC12'
    assert report['core_live_zero'] and report['validation_layers_enabled']
    assert report['synchronization_validation_requested'] and report['validation_callback_tests'] == 1
    assert report['validation_errors'] == report['validation_warnings'] == 0
assert native['copy_control'] is False and control['copy_control'] is True
assert control['result'] == 'NATIVE_IMMUTABLE_SAMPLERS_PASS' and control['verified_components'] == 192
native_pass = native['result'] == 'NATIVE_IMMUTABLE_SAMPLERS_PASS'
if native_pass:
    assert native['verified_components'] == 192
else:
    assert native['result'] == 'NATIVE_IMMUTABLE_SAMPLERS_FAIL'
    assert native['error'] == 'immutable sample/copy/template/layout mismatch phase=1 component=0 actual=108 expected=18'
    assert native['verified_components'] == 0
out = (a.reports / 'guarded-final.stdout').read_text()
err = (a.reports / 'guarded-final.stderr').read_text()
assert 'HARDWARE Mali-G925-Immortalis MC12' in out and 'CANDIDATE_LAYER_MAPPED ' in out
assert 'IMMUTABLE_SAMPLER_GPU_PASS components=192 phases=3 combinedArray=2 separateSamplers=2 ignoredWriteSampler=1 copyPreservesDestination=1 template=1 compatibleLayouts=1 copyControl=0' in out.splitlines()
assert 'MAVK_LAYER_COMMAND_END guarded=1 draws=3 submissions=5' in err
assert 'gaps=0' in err and 'remainingTemplates=0' in err and 'MAVK_LAYER_CORE_END liveZero=1' in err
assert 'MAVK_LAYER_GAP' not in err and 'FAIL ' not in out
print('IMMUTABLE_SAMPLER_BOUNDED_GPU_PASS components=192; ' + ('native also passes' if native_pass else 'native copy mismatch independently reproduced; guarded compatibility corrects it') + '; no CTS/guarded SyncVal/FPS claim')
