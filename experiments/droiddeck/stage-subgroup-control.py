#!/usr/bin/python3
"""Compare old/new stage admission using the existing guarded GPU consumer."""
import os
from pathlib import Path
import subprocess

base = Path('/data/user/0/com.droiddeck.launcher.mavk/files')
private = Path(os.environ.get('MAVK_SUBGROUP_ROOT', str(base / 'mavk-stage-subgroup'))).resolve()
assert private.is_relative_to(base)
env = os.environ.copy()
pids = subprocess.check_output(['pgrep', '-x', 'Xwayland'], text=True).split()
assert len(pids) == 1
for entry in Path('/proc/' + pids[0] + '/environ').read_bytes().split(b'\0'):
    key, sep, value = entry.partition(b'=')
    name = key.decode()
    if sep and (name.startswith(('DROIDDECK_', 'HYBRIS_', 'VK_', 'LD_')) or
                name in ('DISPLAY', 'XAUTHORITY', 'XDG_RUNTIME_DIR', 'WAYLAND_DISPLAY')):
        env[name] = value.decode()
runtime = Path(env['DROIDDECK_SYSTEM_VULKAN_RUNTIME'])
for name in ('libVkLayer_hybris_compat.so', 'VkLayer_hybris_compat.json'):
    target = private / 'payload' / name
    if not target.exists():
        target.symlink_to(runtime / 'lib' / name)
(private / 'consumer').chmod(0o700)
env.pop('LD_PRELOAD', None)
env.update(MAVK_LAYER_GUARDS='1', MAVK_GUARD_REPORTS='0', MAVK_GPU_PROBE='0',
           MAVK_LAYER_PROFILE='0', VK_INSTANCE_LAYERS='VK_LAYER_MAVK_compat:VK_LAYER_HYBRIS_compat')
args = [str(private / 'consumer'), str(base / 'mavk-aggregate/nested.spv'),
        str(base / 'mavk-selected/triangle.vert.spv'), str(base / 'mavk-selected/triangle.frag.spv'), '--nested']
for name, layer in (('baseline', base / 'mavk-aggregate'), ('candidate', private / 'payload')):
    env.update(MAVK_TEST_SUBGROUP16='1', MAVK_EXPECT_LAYER_PATH=str(layer / 'libVkLayer_mavk_compat.so'),
               VK_LAYER_PATH=f'{layer}:{runtime}/lib')
    env.pop('MAVK_TEST_EXPECT_PUSH', None)
    result = subprocess.run(args, env=env, capture_output=True, text=True, timeout=30)
    (private / (name + '.stdout')).write_text(result.stdout)
    (private / (name + '.stderr')).write_text(result.stderr)
    if name == 'baseline':
        gap = 'guarded descriptor layout/allocator unsupported' if env.get('MAVK_TEST_ZERO_LAYOUT_FLAGS') else 'extended shader stage unsupported'
        assert result.returncode != 0 and gap in result.stderr
    else:
        assert result.returncode == 0 and 'NESTED_BUFFER_GPU_PASS' in result.stdout, result.stderr
        assert 'MAVK_LAYER_GAP' not in result.stderr and 'MAVK_LAYER_CORE_END liveZero=1' in result.stderr
    print(f'SUBGROUP_AB {name} status={result.returncode}\n{result.stdout}', flush=True)
if env.get('MAVK_TEST_DEFERRED_LAYOUTS'):
    zero = env.pop('MAVK_TEST_ZERO_LAYOUT_FLAGS', None)
    env['MAVK_TEST_DEFERRED_UNUSED'] = '1'
    unused_args = [str(private / 'consumer'), str(base / 'mavk-selected/gap.spv'),
                   str(base / 'mavk-selected/triangle.vert.spv'), str(base / 'mavk-selected/triangle.frag.spv'), '--unused']
    for name, layer in (('baseline', base / 'mavk-aggregate'), ('candidate', private / 'payload')):
        env.update(VK_LAYER_PATH=f'{layer}:{runtime}/lib', MAVK_EXPECT_LAYER_PATH=str(layer / 'libVkLayer_mavk_compat.so'))
        result = subprocess.run(unused_args, env=env, capture_output=True, text=True, timeout=30)
        (private / (name + '-deferred-unused.stdout')).write_text(result.stdout)
        (private / (name + '-deferred-unused.stderr')).write_text(result.stderr)
        if name == 'baseline':
            assert result.returncode != 0 and 'guarded descriptor layout/allocator unsupported' in result.stderr
        else:
            assert result.returncode == 0 and 'UNUSED_LAYOUT_GPU_PASS' in result.stdout, result.stderr
            assert 'MAVK_LAYER_GAP' not in result.stderr and 'MAVK_LAYER_CORE_END liveZero=1' in result.stderr
        print(f'DEFERRED_UNUSED_AB {name} status={result.returncode}', flush=True)
    env.pop('MAVK_TEST_DEFERRED_UNUSED')
    env.update(MAVK_TEST_ZERO_LAYOUT_FLAGS='1', MAVK_TEST_DEFERRED_USED='1')
    result = subprocess.run(args, env=env, capture_output=True, text=True, timeout=30)
    (private / 'deferred-used-reject.stdout').write_text(result.stdout)
    (private / 'deferred-used-reject.stderr').write_text(result.stderr)
    assert result.returncode != 0 and 'statically used extended descriptor schema unsupported' in result.stderr, result.stderr
    assert 'submissions=0' in result.stderr and 'MAVK_LAYER_CORE_END liveZero=1' in result.stderr
    print('DEFERRED_USED_REJECT_PASS submissions=0', flush=True)
    env.pop('MAVK_TEST_DEFERRED_USED')
    if zero:
        env['MAVK_TEST_ZERO_LAYOUT_FLAGS'] = zero
    else:
        env.pop('MAVK_TEST_ZERO_LAYOUT_FLAGS')
env.pop('MAVK_TEST_SUBGROUP16')
env.update(MAVK_BUFFER_COMMON='1', MAVK_BUFFER_TEST_ROOT=str(base / 'mavk-aggregate'),
           DROIDDECK_MAVK_RUNTIME=str(private))
with (private / 'regressions.log').open('w') as output:
    result = subprocess.run(['/bin/sh', str(base / 'mavk-aggregate/control.sh'), '--regressions-only'],
                            env=env, stdout=output, stderr=subprocess.STDOUT, timeout=90)
assert result.returncode == 0, (private / 'regressions.log').read_text()
env.update(MAVK_EXPERIMENTAL_CULL_PIPELINES='1', MAVK_TEST_CULL_ORIGINAL='1')
for descriptors in (False, True):
    prefix = 'cull-descriptor' if descriptors else 'cull'
    env['MAVK_CULL_REFERENCE'] = str(base / f'mavk-aggregate/{prefix}-reference.vert.spv')
    if descriptors:
        env['MAVK_TEST_CULL_DESCRIPTOR'] = '1'
    args = [str(base / 'mavk-aggregate/cull-link-consumer'),
            str(base / f'mavk-aggregate/{prefix}-original.vert.spv'),
            str(base / 'mavk-aggregate/cull.geom.spv'), str(base / 'mavk-aggregate/cull.frag.spv'), '--primitive-cull']
    result = subprocess.run(args, env=env, capture_output=True, text=True, timeout=30)
    (private / (prefix + '.stdout')).write_text(result.stdout)
    (private / (prefix + '.stderr')).write_text(result.stderr)
    assert result.returncode == 0 and 'WHOLE_PRIMITIVE_CULL_GPU_PASS' in result.stdout, result.stderr
    assert 'MAVK_LAYER_GAP' not in result.stderr and 'MAVK_LAYER_CORE_END liveZero=1' in result.stderr
    print(f'SUBGROUP_CULL_REGRESSION {prefix} PASS', flush=True)
if env.get('MAVK_TEST_EXTENSION_ENUM'):
    env.update(MAVK_LAYER_GUARDS='0')
    env.pop('MAVK_TEST_EXPECT_PUSH', None)
    result = subprocess.run([str(private / 'consumer'), *args[1:4], '--requirements'], env=env,
                            capture_output=True, text=True, timeout=30)
    (private / 'tracking-capabilities.stdout').write_text(result.stdout)
    (private / 'tracking-capabilities.stderr').write_text(result.stderr)
    assert result.returncode == 0 and 'EXTENSION_ENUM_PASS' in result.stdout, result.stderr
    print('TRACKING_NATIVE_CAPABILITIES_PASS queryOnly=1', flush=True)
print('SUBGROUP_GPU_AB_AND_REGRESSIONS_PASS', flush=True)
