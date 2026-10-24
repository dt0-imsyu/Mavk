"""Exercise only launch preparation against actual private files; never execute a game or Proton."""
import hashlib
from pathlib import Path
import runpy

base = Path('/data/user/0/com.droiddeck.launcher.mavk/files')
private = base / 'mavk-wsi-xcb'
original = base / 'linuxfs/usr/local/bin/droiddeck-game-env'
digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
before = digest(original)
module = runpy.run_path(str(private / 'droiddeck-game-env.selection-candidate'))
prepare = module['system_graphics_launch']
env = dict(
    DROIDDECK_SYSTEM_VULKAN_RUNTIME=str(base / 'system_vulkan/8a17ef888360ab1a61b58ee0a5984f5ab1ad9dbabbd8cbcd12b4a52de52be042'),
    DROIDDECK_SYSTEM_VULKAN_GPU='mali', DROIDDECK_SYSTEM_VULKAN_DXVK='managed',
    DROIDDECK_MAVK_RUNTIME=str(base / 'mavk-runtime/7549290e1a5b9685b6bbc29c1c58088a6cb24aa65d12c85e8903cc83196e56fa'),
    DROIDDECK_MAVK_GUEST_ROOT='/rootfs', STEAM_COMPAT_DATA_PATH='/compatdata/42',
    DROIDDECK_MAVK_XCB_AHB_WSI=str(private),
    DROIDDECK_MAVK_XCB_AHB_SHA256='99a12d70e88e14babcf27ad8dcb22804334a9e769bce2dcc89c7c5646096e663',
    XDG_RUNTIME_DIR=str(base / 'mavk-window/run'), DROIDDECK_MAVK_PRESENTATION='ahb-bgra',
)
command = ['/proton', 'waitforexitandrun', '/game.exe']
launch, selected = prepare(command, env)
assert launch[0] == env['DROIDDECK_MAVK_RUNTIME'] + '/run-proton.py'
assert selected['MAVK_WSI_AHB_BGRA_BLIT'] == '1'
assert selected['MAVK_WSI_XCB_AHB_PROXY'] == '1'
assert 'WSI_X11_FORCE_SHM' not in selected
print('DEVICE_AHB_SELECTION_PASS actualFiles=1 execs=0')
_, fallback = prepare(command, dict(selected, DROIDDECK_MAVK_PRESENTATION='x11-shm',
                                   DROIDDECK_MAVK_XCB_AHB_SHA256='0' * 64))
assert fallback['WSI_X11_FORCE_SHM'] == '1'
assert not any(k in fallback for k in ('MAVK_WSI_AHB_ALLOC_ONLY', 'MAVK_WSI_AHB_PRESENT',
                                      'MAVK_WSI_XCB_AHB_PROXY', 'MAVK_WSI_AHB_BGRA_BLIT'))
print('DEVICE_SHM_SELECTION_PASS unavailableCandidateIgnored=1 execs=0')
try:
    prepare(command, dict(env, DROIDDECK_MAVK_XCB_AHB_SHA256='0' * 64))
except module['SystemGraphicsError']:
    print('DEVICE_BAD_PIN_REJECT_PASS')
else:
    raise AssertionError('Unpinned AHB backend accepted')
assert digest(original) == before
print('DEVICE_SELECTION_PREFLIGHT_PASS originalScriptUnchanged=1 steamOrGameStarted=0')
