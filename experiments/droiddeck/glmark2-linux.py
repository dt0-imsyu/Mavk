"""Run the same upstream GLES scenes as the independent Android control."""
import os
import re
from pathlib import Path
import subprocess
import sys
import time

root = Path('/data/user/0/com.droiddeck.launcher.mavk/files/mavk-glmark2')
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
wsi = Path(env.get('MAVK_BENCH_WSI_ROOT', str(runtime / 'wsi')))
layer_root = env.get('MAVK_BENCH_LAYER_ROOT')
if not layer_root:
    assert env.get('DROIDDECK_MAVK_RUNTIME'), 'Desktop harness requires an explicit MAVK_BENCH_LAYER_ROOT'
    layer_root = str(Path(env['DROIDDECK_MAVK_RUNTIME']) / 'payload')
layer = Path(layer_root)
mesa = root.parent / 'mavk-runtime/4de2684454d0ba9f1f80ac37e9fc9d37d2dd04be444897622ae8518fc4bb13c0/mesa25/lib'
assert (layer / 'VkLayer_mavk_compat.json').is_file()
env.pop('LD_PRELOAD', None)
env.update(LD_LIBRARY_PATH=f'{mesa}:{root.parent}/linuxfs/usr/lib:{runtime}/lib:' + env.get('LD_LIBRARY_PATH', ''),
           VK_LAYER_PATH=f'{wsi}:{layer}:{runtime}/lib',
           VK_INSTANCE_LAYERS='VK_LAYER_window_system_integration:VK_LAYER_MAVK_compat:VK_LAYER_HYBRIS_compat',
           WSI_X11_FORCE_SHM='1', HYBRIS_BC_TEXTURES='dxvk',
           MAVK_LAYER_GUARDS='1', MAVK_GUARD_REPORTS='0', MAVK_GPU_PROBE='0',
           MAVK_LAYER_PROFILE='0', GALLIUM_DRIVER='zink',
           MESA_LOADER_DRIVER_OVERRIDE='zink', ZINK_DESCRIPTORS='lazy', LIBGL_KOPPER_DRI2='true')
if os.environ.get('MAVK_BENCH_PROFILE') == '1':
    env.update(MAVK_GPU_PROBE='1', MAVK_GPU_PROBE_WARMUP_MS='2000')
env['MAVK_METADATA_READ_ONLY'] = '1' if os.environ.get('MAVK_BENCH_READ_ONLY') == '1' else '0'
env['MAVK_GUARD_CSE'] = '1' if os.environ.get('MAVK_BENCH_CSE') == '1' else '0'
assert sys.argv[1:] in ([], ['--offscreen'])
offscreen = bool(sys.argv[1:])
size = os.environ.get('MAVK_BENCH_SIZE', '1920x1080')
assert size in ('1280x800', '1920x1080', '1920x1200', '2560x1600')
if os.environ.get('MAVK_BENCH_AHB') == '1':
    import hashlib
    assert not offscreen, 'AHB comparison must present real frames'
    wsi = root.parent / 'mavk-wsi-xcb'
    wsi_hash = os.environ.get('MAVK_BENCH_WSI_SHA256',
        'f1b9370ec973f4bb7ba545aaaeb1ed8dcb7ebe8c8280a308131090d02e2c494a')
    assert re.fullmatch(r'[0-9a-f]{64}', wsi_hash)
    assert hashlib.sha256((wsi / 'libVkLayer_window_system_integration.so').read_bytes()).hexdigest() == wsi_hash
    assert (Path(env['XDG_RUNTIME_DIR']) / 'mavk-explicit-ahb.enable').is_file()
    env.update(VK_LAYER_PATH=f'{wsi}:{layer}:{runtime}/lib', MAVK_WSI_AHB_ALLOC_ONLY='1',
               MAVK_WSI_AHB_PRESENT='1', MAVK_WSI_XCB_AHB_PROXY='1')
    if os.environ.get('MAVK_BENCH_BGRA_BLIT') == '1':
        env['MAVK_WSI_AHB_BGRA_BLIT']='1'
    env.pop('WSI_X11_FORCE_SHM', None)
args = [str(root / 'glmark2-es2'), '--size', size, '--debug',
        '--results', 'fps:cpu:shader',
        '--visual-config', 'red=8:green=8:blue=8:alpha=8:depth=24:stencil=8']
args += ['--off-screen=1', '--frame-end', 'finish'] if offscreen else ['--fullscreen']
if os.environ.get('MAVK_BENCH_AHB') == '1':
    args += ['--swap-mode', 'fifo']
scenes = ('jellyfish', 'terrain', 'shadow')
if os.environ.get('MAVK_BENCH_SCENE'):
    assert os.environ['MAVK_BENCH_SCENE'] in scenes
    scenes = (os.environ['MAVK_BENCH_SCENE'],)
for scene in scenes:
    duration = os.environ.get('MAVK_BENCH_DURATION', '10')
    assert duration in ('10', '30')
    args += ['--benchmark', scene + ':duration=' + duration]
log_path = root / ('linux-offscreen-three-scenes.log' if offscreen else 'linux-three-scenes.log')
label = os.environ.get('MAVK_BENCH_LABEL')
if label:
    assert re.fullmatch(r'[a-z0-9-]{1,48}', label), label
    log_path = root / (label + '.log')
def failure_reason(log):
    if 'GROUP_ERROR_FATAL' in log or 'MAVK_LAYER_GAP' in log:
        return 'gpu-fault-or-unsupported-operation'
    if 'CreateSwapchainKHR failed' in log or 'zink: could not create swapchain' in log:
        return 'presentation-initialization-failed'
    return None

def ahb_receipt_reason(log, bgra):
    if 'MAVK_AHB_REQUEST' not in log or 'MAVK_LAYER_INSTANCE' not in log:
        return 'requested-wsi-or-compat-layer-not-observed'
    if bgra and 'MAVK_WSI_BGRA_GPU_BLIT' not in log:
        return 'requested-bgra-conversion-not-observed'
    return None

with log_path.open('w') as output:
    process = subprocess.Popen(args, cwd=root, env=env, stdout=output, stderr=subprocess.STDOUT)
    start = time.monotonic()
    reason = None
    while process.poll() is None:
        log = log_path.read_text(errors='replace')
        reason = failure_reason(log)
        if not reason and time.monotonic() - start > 80:
            reason = 'bounded-timeout'
        if reason:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
            break
        time.sleep(0.5)
    status = process.wait()
reason = reason or failure_reason(log_path.read_text(errors='replace'))
if not reason and os.environ.get('MAVK_BENCH_AHB') == '1':
    receipt = log_path.read_text(errors='replace')
    reason = ahb_receipt_reason(receipt, os.environ.get('MAVK_BENCH_BGRA_BLIT') == '1')
print(f'GLMARK2_EXIT status={status} reason={reason}', flush=True)
raise SystemExit(status if status else (1 if reason else 0))
