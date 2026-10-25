"""Use the real game's environment selection, then exec only the existing native consumer."""
import os
from pathlib import Path
import runpy
import hashlib
import json

base = Path('/data/user/0/com.droiddeck.launcher.mavk/files')
env = os.environ.copy()
if env.get('MAVK_NORMAL_POINTER') == '1':
    candidate = base / 'mavk-wsi-pointer'
    receipt = json.loads((candidate / 'mavk-ahb.json').read_text())
    digest = hashlib.sha256((candidate / 'libVkLayer_window_system_integration.so').read_bytes()).hexdigest()
    assert receipt['library_sha256'] == digest
    env.update(DROIDDECK_MAVK_XCB_AHB_WSI=str(candidate), DROIDDECK_MAVK_XCB_AHB_SHA256=digest)
module = runpy.run_path(env['DROIDDECK_GAME_ENV_LAUNCHER'])
sync = module['load_sync']()
assert isinstance(sync, dict) and callable(sync['select']), 'bundled sync helper missing'
components = runpy.run_path(str(Path(env['DROIDDECK_GAME_ENV_LAUNCHER']).with_name('droiddeck-wincomponents')))
assert callable(components['apply']), 'bundled component helper missing'
env['STEAM_COMPAT_DATA_PATH'] = '/tmp/mavk-prepare-only/42'
placeholder = ['/mavk-prepare-only/proton', 'waitforexitandrun', '/mavk-prepare-only/game.exe']
_, selected = module['system_graphics_launch'](placeholder, env)
ahb_keys = ('MAVK_WSI_AHB_ALLOC_ONLY', 'MAVK_WSI_AHB_PRESENT', 'MAVK_WSI_XCB_AHB_PROXY', 'MAVK_WSI_AHB_BGRA_BLIT')
if env['DROIDDECK_MAVK_PRESENTATION'] == 'ahb-bgra':
    for key in ahb_keys:
        assert selected[key] == '1', key
    assert 'WSI_X11_FORCE_SHM' not in selected
    assert selected['VK_LAYER_PATH'].split(':')[0] == env['DROIDDECK_MAVK_XCB_AHB_WSI']
    print('NORMAL_SESSION_POLICY_PASS helpers=2 ahb=1 shm=0 protonExecuted=0', flush=True)
else:
    assert env['DROIDDECK_MAVK_PRESENTATION'] == 'x11-shm'
    assert selected['WSI_X11_FORCE_SHM'] == '1'
    assert not any(key in selected for key in ahb_keys)
    print('NORMAL_SESSION_POLICY_PASS helpers=2 ahb=0 shm=1 protonExecuted=0', flush=True)
# The native consumer has no Wine PE accesses; retain only inherited preloads.
if env.get('LD_PRELOAD'):
    selected['LD_PRELOAD'] = env['LD_PRELOAD']
else:
    selected.pop('LD_PRELOAD', None)
selected.pop('STEAM_COMPAT_DATA_PATH', None)
bundle = base / 'mavk-window/payload'
command = ['/usr/lib/ld-linux-aarch64.so.1', '--library-path',
           str(bundle) + ':' + selected['LD_LIBRARY_PATH'],
           str(base / 'mavk-wsi-xcb/sarek-window'), str(bundle / 'libdxvk_d3d11.so')]
os.execve(command[0], command, selected)
