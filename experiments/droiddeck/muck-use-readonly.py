import json
import os
import argparse
from pathlib import Path

parser = argparse.ArgumentParser(description='Select an isolated Muck layer diagnostic, or restore the common runtime.')
parser.add_argument('--common', action='store_true')
parser.add_argument('--layer-directory', type=Path, default=Path('/data/user/0/com.droiddeck.launcher.mavk/files/mavk-execution'))
args = parser.parse_args()
candidate = args.layer_directory
assert candidate.parent == Path('/data/user/0/com.droiddeck.launcher.mavk/files') and candidate.name.startswith('mavk-'), 'Only direct app-private diagnostic directories'
folder = Path('/root/.config/droiddeck')
path = folder / 'game-environment.json'
if args.common:
    changes = []
    for source in (path, Path('/data/user/0/com.droiddeck.launcher.mavk/files/game-environment.json')):
        data = json.loads(source.read_text())
        assert data['version'] == 1
        settings = data.get('games', {}).get('1625450', {})
        selected = settings.get('DROIDDECK_MAVK_LAYER_PATH')
        if selected is None:
            continue
        assert selected.split(':')[0] == str(candidate), 'Preserve unknown layer selection'
        settings.pop('DROIDDECK_MAVK_LAYER_PATH')
        changes.append((source, data))
    for source, data in changes:
        backup = source.with_name(source.name + '.before-common-runtime')
        if not backup.exists():
            backup.write_bytes(source.read_bytes())
        staged = source.with_suffix('.json.staged')
        staged.write_text(json.dumps(data, indent=2) + '\n')
        staged.replace(source)
    print('Removed only Muck diagnostic layer path; common session runtime will supply it')
    raise SystemExit(0)
body = json.loads(path.read_text())
assert body['version'] == 1
game = body.setdefault('games', {}).setdefault('1625450', {})
baseline = Path(os.environ['DROIDDECK_MAVK_RUNTIME']) / 'payload'
assert (baseline / 'VkLayer_hybris_compat.json').is_file()
assert (candidate / 'VkLayer_mavk_compat.json').is_file() and (candidate / 'libVkLayer_mavk_compat.so').is_file()
game.update({
    'DROIDDECK_MAVK_LAYER_PATH': str(candidate) + ':' + str(baseline),
    'MAVK_GUARD_REPORTS': '0',
    'MAVK_LAYER_PROFILE': '0',
    'DXVK_LOG_LEVEL': 'info',
    'DXVK_LOG_PATH': '/data/user/0/com.droiddeck.launcher.mavk/files/mavk-window',
    'PROTON_LOG': '1',
    'WINEDEBUG': '-all',
    'PROTON_LOG_DIR': '/data/user/0/com.droiddeck.launcher.mavk/files/mavk-window',
})
staged = path.with_suffix('.json.staged')
staged.write_text(json.dumps(body, indent=2) + '\n')
staged.replace(path)
# Android republishes its settings on resume/launch. Preserve just this game's
# diagnostic overrides there too; never replace shared or other-game settings.
app_path = Path('/data/user/0/com.droiddeck.launcher.mavk/files/game-environment.json')
app_body = json.loads(app_path.read_text()) if app_path.exists() else {'version': 1}
assert app_body['version'] == 1
app_backup = app_path.with_name('game-environment.json.before-muck-profile')
if not app_backup.exists():
    app_backup.write_text(json.dumps(app_body, indent=2) + '\n')
app_body.setdefault('games', {}).setdefault('1625450', {}).update(game)
app_staged = app_path.with_suffix('.json.staged')
app_staged.write_text(json.dumps(app_body, indent=2) + '\n')
app_staged.replace(app_path)
print('Muck-only read-only-guard layer selected; native feature queries unchanged')
