"""Reversible Muck-only candidate selection; never overwrites mapped libraries."""
import hashlib
import json
from pathlib import Path
import sys

base = Path('/data/user/0/com.droiddeck.launcher.mavk/files')
work = base / 'mavk-wsi-resize'
backup = work / 'profile-backup.json'
paths = (base / 'game-environment.json', base / 'linuxfs/root/.config/droiddeck/game-environment.json')
keys = ('DROIDDECK_MAVK_XCB_AHB_WSI', 'DROIDDECK_MAVK_XCB_AHB_SHA256')
restore = sys.argv[1:] == ['restore']
if restore:
    saved = json.loads(backup.read_text())
    assert saved['active']
else:
    assert sys.argv[1:] == ['enable']
    assert not backup.exists() or not json.loads(backup.read_text())['active']
    receipt = json.loads((work / 'mavk-ahb.json').read_text())
    digest = hashlib.sha256((work / 'libVkLayer_window_system_integration.so').read_bytes()).hexdigest()
    assert digest == receipt['library_sha256']
    saved = {'active': True, 'values': {}, 'changes': dict(zip(keys, (str(work), digest)))}
pending = []
for path in paths:
    body = json.loads(path.read_text())
    assert body['version'] == 1
    game = body.setdefault('games', {}).setdefault('1625450', {})
    if not restore:
        saved['values'][str(path)] = {key: {'present': key in game, 'value': game.get(key)} for key in keys}
        game.update(saved['changes'])
    else:
        for key in keys:
            assert game.get(key) == saved['changes'][key], 'Preserve concurrent edits'
            old = saved['values'][str(path)][key]
            if old['present']:
                game[key] = old['value']
            else:
                game.pop(key, None)
    pending.append((path, body))
if not restore:
    backup.write_text(json.dumps(saved, indent=2) + '\n')
for path, body in pending:
    staged = path.with_suffix('.json.resize-staged')
    staged.write_text(json.dumps(body, indent=2) + '\n')
    staged.replace(path)
if restore:
    saved['active'] = False
    backup.write_text(json.dumps(saved, indent=2) + '\n')
print('Muck-only AHB resize candidate', 'restored' if restore else 'enabled')
