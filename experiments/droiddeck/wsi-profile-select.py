"""Select/restore a Muck-only WSI diagnostic; no common runtime replacement."""
import argparse
import json
from pathlib import Path
import shutil

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('action', choices=('enable', 'restore'))
parser.add_argument('--cached', action='store_true')
parser.add_argument('--optimal', action='store_true')
parser.add_argument('--work', default='mavk-wsi-profile')
args = parser.parse_args()
root = Path('/data/user/0/com.droiddeck.launcher.mavk/files')
assert args.work.startswith('mavk-') and '/' not in args.work and '..' not in args.work
work = root / args.work
original = root / 'system_vulkan/8a17ef888360ab1a61b58ee0a5984f5ab1ad9dbabbd8cbcd12b4a52de52be042'
candidate = work / 'runtime'
backup = work / 'settings-backup.json'
sources = (Path('/root/.config/droiddeck/game-environment.json'), root / 'game-environment.json')
changes = {'DROIDDECK_SYSTEM_VULKAN_RUNTIME': str(candidate), 'MAVK_WSI_PROFILE': '1',
           'MAVK_WSI_CACHED_READBACK': '1' if args.cached else '0',
           'MAVK_WSI_OPTIMAL_READBACK': '1' if args.optimal else '0'}
if args.action == 'enable':
    assert not backup.exists() or not json.loads(backup.read_text())['active']
    assert (work / 'libVkLayer_window_system_integration.so').is_file()
    candidate.mkdir(exist_ok=True)
    for entry in original.iterdir():
        if entry.name == 'wsi':
            continue
        destination = candidate / entry.name
        if destination.is_symlink():
            assert destination.readlink() == entry
        else:
            assert not destination.exists()
            destination.symlink_to(entry, target_is_directory=entry.is_dir())
    (candidate / 'wsi').mkdir(exist_ok=True)
    shutil.copy2(original / 'wsi/VkLayer_window_system_integration.json', candidate / 'wsi')
    library = candidate / 'wsi/libVkLayer_window_system_integration.so'
    if library.exists():
        assert library.read_bytes() == (work / library.name).read_bytes(), 'Do not overwrite a possibly mapped library'
    else:
        shutil.copy2(work / library.name, library)
    saved = {'active': True, 'values': {}, 'changes': changes}
else:
    saved = json.loads(backup.read_text())
    assert saved['active']
    changes = saved['changes']
pending = []
for source in sources:
    body = json.loads(source.read_text())
    assert body['version'] == 1
    game = body.setdefault('games', {}).setdefault('1625450', {})
    if args.action == 'enable':
        saved['values'][str(source)] = {key: {'present': key in game, 'value': game.get(key)} for key in changes}
        game.update(changes)
    else:
        for key, expected in changes.items():
            assert game.get(key) == expected, 'Preserve concurrent setting edits'
            previous = saved['values'][str(source)][key]
            if previous['present']:
                game[key] = previous['value']
            else:
                game.pop(key, None)
    pending.append((source, body))
if args.action == 'enable':
    backup.write_text(json.dumps(saved, indent=2) + '\n')
for source, body in pending:
    staged = source.with_suffix('.json.staged')
    staged.write_text(json.dumps(body, indent=2) + '\n')
    staged.replace(source)
if args.action == 'restore':
    saved['active'] = False
    backup.write_text(json.dumps(saved, indent=2) + '\n')
print('Muck-only WSI ' + args.action + '; original runtime bytes unchanged')
