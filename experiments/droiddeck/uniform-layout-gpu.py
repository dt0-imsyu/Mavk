"""Consumer-derived packed UBO A/B with the existing guarded buffer oracle."""
import os
from pathlib import Path
import subprocess

base = Path('/data/user/0/com.droiddeck.launcher.mavk/files')
root = Path(os.environ.get('MAVK_UNIFORM_ROOT', str(base / 'mavk-uniform-layout')))
assert root.parent == base and root.name.startswith('mavk-')
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
env.pop('LD_PRELOAD', None)
env.update(MAVK_LAYER_GUARDS='1', MAVK_GUARD_REPORTS='0', MAVK_GPU_PROBE='0',
           MAVK_TEST_STD430_UBO='1', VK_INSTANCE_LAYERS='VK_LAYER_MAVK_compat:VK_LAYER_HYBRIS_compat')
scenarios = (
    ('baseline-oob', base / 'mavk-layout-deferred/payload', False),
    ('candidate-oob', root / 'payload', False),
    ('candidate-valid', root / 'payload', True),
)
common_only = os.environ.get('MAVK_COMMON_ONLY') == '1'
for label, layer, valid in (() if common_only else scenarios):
    env.update(VK_LAYER_PATH=f'{layer}:{runtime}/lib', MAVK_EXPECT_LAYER_PATH=str(layer / 'libVkLayer_mavk_compat.so'))
    if valid:
        env['MAVK_TEST_STD430_VALID'] = '1'
    else:
        env.pop('MAVK_TEST_STD430_VALID', None)
    args = [str(root / 'consumer'), str(root / ('ubo-valid.spv' if valid else 'ubo-oob.spv')),
            str(base / 'mavk-selected/triangle.vert.spv'), str(base / 'mavk-selected/triangle.frag.spv'), '--nested']
    result = subprocess.run(args, env=env, capture_output=True, text=True, timeout=30)
    (root / (label + '.stdout')).write_text(result.stdout)
    (root / (label + '.stderr')).write_text(result.stderr)
    if label.startswith('baseline'):
        assert result.returncode != 0 and 'invalid static-use SPIR-V' in result.stderr, result.stderr
    else:
        assert result.returncode == 0 and 'GPU_PASS' in result.stdout, result.stderr
        assert 'MAVK_LAYER_GAP' not in result.stderr and 'MAVK_LAYER_CORE_END liveZero=1' in result.stderr
    print(f'UNIFORM_LAYOUT_AB {label} status={result.returncode}\n{result.stdout}', flush=True)
env.pop('MAVK_TEST_STD430_UBO')
env.pop('MAVK_TEST_STD430_VALID', None)
env.update(MAVK_BUFFER_COMMON='1', MAVK_BUFFER_TEST_ROOT=str(base / 'mavk-aggregate'),
           DROIDDECK_MAVK_RUNTIME=env['DROIDDECK_MAVK_RUNTIME'] if common_only else str(root))
log_path = root / ('common-regressions.log' if common_only else 'regressions.log')
with log_path.open('w') as output:
    control = root / 'control.sh'
    if not control.is_file():
        control = base / 'mavk-aggregate/control.sh'
    arguments = ['/bin/sh', str(control), '--regressions-only']
    if os.environ.get('MAVK_FULL_REGRESSIONS') == '1':
        env.update(MAVK_TEXEL_VIEW_COMPONENTS='1', MAVK_TEXEL_COMPONENTS='1',
                   MAVK_STORAGE_ATOMIC='1', MAVK_STORAGE_TEXEL='1')
        arguments = arguments[:2]
    result = subprocess.run(arguments, env=env, stdout=output, stderr=subprocess.STDOUT, timeout=120)
assert result.returncode == 0, log_path.read_text()
print('COMMON_GPU_REGRESSIONS_PASS runtime=' + env['DROIDDECK_MAVK_RUNTIME'] if common_only else 'UNIFORM_LAYOUT_GPU_AND_REGRESSIONS_PASS', flush=True)
