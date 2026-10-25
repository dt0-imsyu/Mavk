"""Temporarily enable Muck-only CPU attribution and optional private GPU probes."""
import argparse
import json
import os
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('action', choices=('enable', 'restore'))
parser.add_argument('--gpu-layer', type=Path)
parser.add_argument('--gpu-current', action='store_true', help='Profile the verified current runtime without selecting another layer')
parser.add_argument('--guard-cse', action='store_true', help='Enable the existing checked metadata-read optimizer, retaining guards')
parser.add_argument('--shared-diagnostics', action='store_true', help='Reuse zero diagnostic storage only when shader reports are disabled')
parser.add_argument('--descriptor-arena', action='store_true', help='Suballocate immutable metadata with command-generation lifetime')
parser.add_argument('--gpu-warmup-ms', type=int, default=30000)
parser.add_argument('--shader-layer', type=Path)
parser.add_argument('--metadata-read-only', action='store_true')
parser.add_argument('--shader-cache-entries', type=int, default=4, choices=(4,64))
args = parser.parse_args()
assert 0 <= args.gpu_warmup_ms <= 60000
root = Path('/data/user/0/com.droiddeck.launcher.mavk/files')
backup = root / 'mavk-perf/profile-backup.json'
sources = (Path('/root/.config/droiddeck/game-environment.json'), root / 'game-environment.json')
expected = {'MAVK_LAYER_PROFILE': '1'}
if args.shared_diagnostics:
    assert args.action == 'enable' and args.shader_layer
    expected['MAVK_SHARED_DIAGNOSTICS'] = '1'
if args.descriptor_arena:
    assert args.action == 'enable' and args.shader_layer
    expected['MAVK_DESCRIPTOR_ARENA'] = '1'
if args.guard_cse:
    assert args.action == 'enable' and (args.gpu_current or args.shader_layer)
    expected['MAVK_GUARD_CSE'] = '1'
assert not (args.gpu_layer and args.shader_layer)
assert not (args.gpu_current and (args.gpu_layer or args.shader_layer))
if args.gpu_current:
    assert args.action == 'enable'
    baseline = Path(os.environ['DROIDDECK_MAVK_RUNTIME']) / 'payload/libVkLayer_mavk_compat.so'
    assert b'hostResetBeforePoll=1' in baseline.read_bytes(), 'Require the corrected GPU query initialization'
    expected.update(MAVK_GPU_PROBE='1', MAVK_GPU_PROBE_WARMUP_MS=str(args.gpu_warmup_ms))
if args.shader_layer:
    assert args.action == 'enable'
    layer = args.shader_layer
    assert (layer.parent == root and layer.name.startswith('mavk-') or
            layer.name == 'payload' and layer.parent.parent == root and layer.parent.name.startswith('mavk-'))
    assert (layer / 'libVkLayer_mavk_compat.so').is_file()
    assert (layer / 'VkLayer_mavk_compat.json').is_file()
    baseline = Path(os.environ['DROIDDECK_MAVK_RUNTIME']) / 'payload'
    assert (baseline / 'VkLayer_hybris_compat.json').is_file()
    expected.update(DROIDDECK_MAVK_LAYER_PATH=str(layer) + ':' + str(baseline),
                    MAVK_GPU_PROBE='0', MAVK_SHADER_CACHE_PROFILE='1',
                    MAVK_METADATA_READ_ONLY='1' if args.metadata_read_only else '0',
                    MAVK_SHADER_CACHE_ENTRIES=str(args.shader_cache_entries))
if args.gpu_layer:
    assert args.action == 'enable'
    layer = args.gpu_layer
    assert layer.parent == root and layer.name.startswith('mavk-')
    assert (layer / 'libVkLayer_mavk_compat.so').is_file()
    assert (layer / 'VkLayer_mavk_compat.json').is_file()
    baseline = Path(os.environ['DROIDDECK_MAVK_RUNTIME']) / 'payload'
    assert (baseline / 'VkLayer_hybris_compat.json').is_file()
    expected.update(MAVK_GPU_PROBE='1', MAVK_GPU_PROBE_WARMUP_MS=str(args.gpu_warmup_ms),
                    DROIDDECK_MAVK_LAYER_PATH=str(layer) + ':' + str(baseline))
if args.action == 'enable':
    assert not backup.exists() or not json.loads(backup.read_text())['active'], 'Restore active profile first'
    saved = {'active': True, 'values': {}, 'expected': expected}
    changes = []
    for source in sources:
        body = json.loads(source.read_text())
        assert body['version'] == 1
        game = body.setdefault('games', {}).setdefault('1625450', {})
        saved['values'][str(source)] = {key: {'present': key in game, 'value': game.get(key)} for key in expected}
        game.update(expected)
        changes.append((source, body))
    backup.write_text(json.dumps(saved, indent=2) + '\n')
else:
    saved = json.loads(backup.read_text())
    assert saved['active'], 'No active diagnostic profile'
    if 'expected' not in saved:
        saved['expected'] = {'MAVK_LAYER_PROFILE': '1'}
        saved['values'] = {source: {'MAVK_LAYER_PROFILE': value} for source, value in saved['values'].items()}
    expected = saved['expected']
    changes = []
    for source in sources:
        body = json.loads(source.read_text())
        assert body['version'] == 1
        game = body.get('games', {}).get('1625450', {})
        for key, value in expected.items():
            assert game.get(key) == value, 'Preserve a concurrently edited profile'
            previous = saved['values'][str(source)][key]
            if previous['present']:
                game[key] = previous['value']
            else:
                game.pop(key, None)
        changes.append((source, body))
for source, body in changes:
    staged = source.with_suffix('.json.staged')
    staged.write_text(json.dumps(body, indent=2) + '\n')
    staged.replace(source)
if args.action == 'restore':
    saved['active'] = False
    backup.write_text(json.dumps(saved, indent=2) + '\n')
print(f'Muck-only profile {args.action}; diagnostic environment only; installed binaries, feature bits and guards unchanged')
