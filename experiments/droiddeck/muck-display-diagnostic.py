import json
import sys
from pathlib import Path

folder = Path('/root/.config/droiddeck')
path = folder / 'game-environment.json'
backup = folder / 'game-environment.json.before-muck-display'
if not backup.exists():
    backup.write_text(path.read_text() if path.exists() else '{"version":1,"shared":{},"games":{}}\n')
body = json.loads(path.read_text()) if path.exists() else {'version': 1}
assert body['version'] == 1
if '--restore' in sys.argv:
    previous = json.loads(backup.read_text()).get('games', {}).get('1625450')
    if previous is None:
        body.setdefault('games', {}).pop('1625450', None)
    else:
        body.setdefault('games', {})['1625450'] = previous
    path.write_text(json.dumps(body, indent=2) + '\n')
    print('Restored only Muck environment from pre-diagnostic backup')
    raise SystemExit(0)
game = body.setdefault('games', {}).setdefault('1625450', {})
game.update({
    'DXVK_LOG_LEVEL': 'debug',
    'DXVK_LOG_PATH': '/data/user/0/com.droiddeck.launcher.mavk/files/mavk-window',
    'LD_PRELOAD': '/data/user/0/com.droiddeck.launcher.mavk/files/mavk-window/muck-wsi-trace.so',
    'WINEDEBUG': '+timestamp,+vulkan,+dxgi',
    'PROTON_LOG': '1',
    'PROTON_LOG_DIR': '/data/user/0/com.droiddeck.launcher.mavk/files/mavk-window',
})
staged = path.with_suffix('.json.staged')
staged.write_text(json.dumps(body, indent=2) + '\n')
staged.replace(path)
print('Muck-only checked X11 SHM presentation and DXVK logging enabled; backup:', backup)
