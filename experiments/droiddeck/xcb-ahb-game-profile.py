"""Reversible Muck-only rendering experiment; does not claim input support."""
import hashlib
import json
import sys
from pathlib import Path

base = Path('/data/user/0/com.droiddeck.launcher.mavk/files')
private = base / 'mavk-wsi-xcb'
state = private / 'game-profile-backup.json'
script = Path('/usr/local/bin/droiddeck-game-env')
staged = private / 'droiddeck-game-env.candidate'
backup_script = private / 'droiddeck-game-env.before'
values = {
    'DROIDDECK_MAVK_XCB_AHB_WSI': str(private),
    'DROIDDECK_MAVK_XCB_AHB_SHA256': 'f1b9370ec973f4bb7ba545aaaeb1ed8dcb7ebe8c8280a308131090d02e2c494a',
}
def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()
def write_json(path, value):
    temp = path.with_name(path.name + '.xcb-staged')
    temp.write_text(json.dumps(value, indent=2) + '\n')
    temp.replace(path)

if sys.argv[1] == 'enable':
    assert not state.exists(), 'Restore previous profile first'
    assert digest(private / 'libVkLayer_window_system_integration.so') == values['DROIDDECK_MAVK_XCB_AHB_SHA256']
    assert (Path('/run/droiddeck') / 'mavk-explicit-ahb.enable').is_file()
    saved = {'script_before': digest(script), 'script_after': digest(staged), 'values': {}}
    changes = []
    for path in (Path('/root/.config/droiddeck/game-environment.json'), base / 'game-environment.json'):
        body = json.loads(path.read_text())
        assert body['version'] == 1
        game = body.setdefault('games', {}).setdefault('1625450', {})
        saved['values'][str(path)] = {key: {'present': key in game, 'value': game.get(key)} for key in values}
        game.update(values)
        changes.append((path, body))
    backup_script.write_bytes(script.read_bytes())
    write_json(state, saved)
    script.write_bytes(staged.read_bytes())
    for path, body in changes:
        write_json(path, body)
    print('Enabled only Muck XCB/AHB render experiment; exact script/config rollback saved')
elif sys.argv[1] == 'restore':
    saved = json.loads(state.read_text())
    assert digest(script) == saved['script_after'], 'Do not overwrite concurrent script changes'
    assert digest(backup_script) == saved['script_before']
    changes = []
    for name, old in saved['values'].items():
        path = Path(name)
        body = json.loads(path.read_text())
        game = body['games']['1625450']
        for key, value in values.items():
            assert game.get(key) == value, 'Do not overwrite concurrent game configuration'
            if old[key]['present']:
                game[key] = old[key]['value']
            else:
                game.pop(key, None)
        changes.append((path, body))
    script.write_bytes(backup_script.read_bytes())
    for path, body in changes:
        write_json(path, body)
    state.rename(private / 'game-profile-restored.json')
    print('Restored original launch script and only the two Muck experiment keys')
else:
    raise SystemExit('enable or restore required')
