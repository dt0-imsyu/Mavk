#!/usr/bin/python3
"""Bounded official Linux ARM64 GravityMark run in the existing desktop."""
import os
from pathlib import Path
import subprocess
import sys
import time

root = Path('/data/user/0/com.droiddeck.launcher.mavk/files/mavk-gravity-linux')
env = os.environ.copy()
pids = subprocess.check_output(['pgrep', '-x', 'Xwayland'], text=True).split()
assert len(pids) == 1, pids
for entry in Path('/proc/' + pids[0] + '/environ').read_bytes().split(b'\0'):
    key, sep, value = entry.partition(b'=')
    name = key.decode()
    if sep and (name.startswith(('DROIDDECK_', 'HYBRIS_', 'VK_', 'LD_')) or
                name in ('DISPLAY', 'XAUTHORITY', 'XDG_RUNTIME_DIR', 'WAYLAND_DISPLAY')):
        env[name] = value.decode()
runtime = Path(env['DROIDDECK_SYSTEM_VULKAN_RUNTIME'])
layer = Path(env.get('MAVK_BENCH_LAYER_ROOT', str(root.parent / 'mavk-aggregate'))).resolve()
assert layer.is_relative_to(root.parent) and (layer / 'VkLayer_mavk_compat.json').is_file()
env['LD_LIBRARY_PATH'] = str(root / 'bin') + ':' + env.get('LD_LIBRARY_PATH', '')
env.update(VK_LAYER_PATH=f'{runtime}/wsi:{layer}:{runtime}/lib',
           VK_INSTANCE_LAYERS='VK_LAYER_window_system_integration:VK_LAYER_MAVK_compat:VK_LAYER_HYBRIS_compat',
           WSI_X11_FORCE_SHM='1', HYBRIS_BC_TEXTURES='dxvk',
           MAVK_LAYER_GUARDS='1', MAVK_GUARD_REPORTS='0', MAVK_GPU_PROBE='0',
           MAVK_SAREK_BC_FORMATS='1', MAVK_LAYER_PROFILE='0')
env.pop('LD_PRELOAD', None)
assert sys.argv[1:] in ([], ['--opengl'], ['--gl-info'])
api = '-gl' if sys.argv[1:] else '-vk'
if api == '-gl':
    mesa = root.parent / 'mavk-runtime/4de2684454d0ba9f1f80ac37e9fc9d37d2dd04be444897622ae8518fc4bb13c0/mesa25/lib'
    assert (mesa / 'libGL.so.1').is_file()
    env['LD_LIBRARY_PATH'] = str(mesa) + ':' + env['LD_LIBRARY_PATH']
    env.update(GALLIUM_DRIVER='zink', MESA_LOADER_DRIVER_OVERRIDE='zink', ZINK_DESCRIPTORS='lazy')
args = [str(root / 'bin/GravityMark.arm64'), api, '-width', '1920', '-height', '1080',
        '-fullscreen', '0', '-temporal', '1', '-asteroids', '100000', '-benchmark', '1',
        '-vsync', '0', '-close', '1', '-count', '1', '-times', str(root / 'times.txt'),
        '-image', str(root / 'result.png')]
log_path = root / ('linux-gl.log' if api == '-gl' else 'linux-run.log')
if sys.argv[1:] == ['--gl-info']:
    args = ['/usr/bin/glxinfo', '-B']
    log_path = root / 'linux-gl-info.log'
with log_path.open('w') as out:
    process = subprocess.Popen(args, cwd=root / 'bin', env=env, stdout=out, stderr=subprocess.STDOUT)
    start = time.monotonic()
    reason = None
    while process.poll() is None:
        log = log_path.read_text(errors='replace')
        if 'GROUP_ERROR_FATAL' in log:
            reason = 'GPU-fault'
        elif 'Initialization error' in log:
            reason = 'initialization-error'
        elif 'format is not supported' in log and time.monotonic() - start > 10:
            reason = 'unsupported-format-invalid-benchmark'
        elif time.monotonic() - start > 240:
            reason = 'bounded-timeout'
        if reason:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
            break
        time.sleep(1)
    status = process.wait()
print(f'GRAVITY_LINUX_EXIT status={status} reason={reason}', flush=True)
