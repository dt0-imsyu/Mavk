"""Reuse the existing GPU consumer to check optional diagnostic storage sharing."""
import os
from pathlib import Path
import subprocess
import struct
import sys

base = Path('/data/user/0/com.droiddeck.launcher.mavk/files')
name = sys.argv[1] if len(sys.argv)>1 else 'mavk-shared-diagnostics'
assert name in ('mavk-shared-diagnostics', 'mavk-descriptor-arena', 'mavk-uniform-packed', 'mavk-uniform-packed2', 'mavk-native-public', 'mavk-native-public2', 'mavk-native-public4')
layer = base / name
fixture = base / 'mavk-aggregate'
selected = base / 'mavk-selected'
assert (layer / 'libVkLayer_mavk_compat.so').is_file()
if name.startswith('mavk-uniform-packed') and not (layer / 'nested-existing-uint4.spv').exists():
    # Reuse the existing readback oracle, adding the type common in DXVK shaders.
    data = (fixture / 'nested.spv').read_bytes()
    words = list(struct.unpack('<' + 'I' * (len(data)//4), data))
    p, uint_type, first_function = 5, 0, 0
    while p < len(words):
        count, opcode = words[p] >> 16, words[p] & 65535
        assert count and p + count <= len(words)
        if opcode == 21 and words[p+2:p+4] == [32, 0]:
            uint_type = words[p+1]
        if opcode == 54:
            first_function = p
            break
        p += count
    assert uint_type and first_function and words[3] < 0xffffffff
    words[first_function:first_function] = [(4 << 16) | 23, words[3], uint_type, 4]
    words[3] += 1
    (layer / 'nested-existing-uint4.spv').write_bytes(struct.pack('<' + 'I' * len(words), *words))
for shared in (0, 1):
    for reports in (0, 1):
        prefix = layer / f'gpu-shared-{shared}-reports-{reports}'
        assert not prefix.with_suffix('.stdout').exists(), 'Preserve previous GPU evidence'
        env = dict(os.environ)
        env.pop('LD_PRELOAD', None)
        env.update(VK_LAYER_PATH=str(layer) + ':' + env['DROIDDECK_MAVK_RUNTIME'] + '/payload',
                   VK_INSTANCE_LAYERS='VK_LAYER_MAVK_compat:VK_LAYER_HYBRIS_compat',
                   MAVK_LAYER_GUARDS='1', MAVK_LAYER_PROFILE='0', MAVK_LAYER_TRACE='0',
                   MAVK_GPU_PROBE='0', MAVK_GUARD_CSE='0', MAVK_METADATA_READ_ONLY='0',
                   MAVK_GUARD_REPORTS=str(reports), MAVK_SHARED_DIAGNOSTICS=str(shared),
                   MAVK_DESCRIPTOR_ARENA='0' if name=='mavk-shared-diagnostics' else '1',
                   MAVK_UNIFORM_GUARD_METADATA='1' if name.startswith('mavk-uniform-packed') else '0',
                   MAVK_NATIVE_BOUND_SETS='1' if name.startswith('mavk-native-public') else '0',
                   MAVK_EXPECT_LAYER_PATH=str(layer / 'libVkLayer_mavk_compat.so'))
        result = subprocess.run([str(fixture / 'consumer'), str(fixture / 'nested.spv'),
                                 str(selected / 'triangle.vert.spv'),
                                 str(selected / 'triangle.frag.spv'), '--nested'],
                                env=env, capture_output=True, timeout=20)
        prefix.with_suffix('.stdout').write_bytes(result.stdout)
        prefix.with_suffix('.stderr').write_bytes(result.stderr)
        assert result.returncode == 0, result.stderr[-2000:]
        assert b'NESTED_BUFFER_GPU_PASS' in result.stdout, result.stdout[-2000:]
        assert b'MAVK_LAYER_CORE_END liveZero=1' in result.stderr, result.stderr[-2000:]
        print(f'SHARED_DIAGNOSTICS_GPU_PASS shared={shared} reports={reports}', flush=True)
if name != 'mavk-shared-diagnostics':
    prefix = layer / 'gpu-arena-retention'
    assert not prefix.with_suffix('.stdout').exists()
    env.update(MAVK_GUARD_REPORTS='0', MAVK_SHARED_DIAGNOSTICS='1', MAVK_DESCRIPTOR_ARENA='1')
    result = subprocess.run([str(layer / 'arena-retention'), str(fixture / 'nested.spv'),
                             str(selected / 'triangle.vert.spv'), str(selected / 'triangle.frag.spv'),
                             '--arena-retention'], env=env, capture_output=True, timeout=20)
    prefix.with_suffix('.stdout').write_bytes(result.stdout)
    prefix.with_suffix('.stderr').write_bytes(result.stderr)
    assert result.returncode == 0, result.stderr[-3000:]
    assert b'METADATA_ARENA_GPU_PASS generations=2048' in result.stdout, result.stdout[-2000:]
    assert b'MAVK_LAYER_CORE_END liveZero=1' in result.stderr, result.stderr[-2000:]
    print(result.stdout.decode().strip(), flush=True)
if (layer / 'nested-existing-uint4.spv').is_file():
    result = subprocess.run([str(fixture / 'consumer'), str(layer / 'nested-existing-uint4.spv'),
                             str(selected / 'triangle.vert.spv'), str(selected / 'triangle.frag.spv'),
                             '--nested'], env=env, capture_output=True, timeout=20)
    (layer / 'gpu-existing-uint4.stdout').write_bytes(result.stdout)
    (layer / 'gpu-existing-uint4.stderr').write_bytes(result.stderr)
    assert result.returncode == 0 and b'NESTED_BUFFER_GPU_PASS' in result.stdout, result.stderr[-3000:]
    assert b'MAVK_LAYER_CORE_END liveZero=1' in result.stderr
    print('EXISTING_UINT4_GPU_PASS', flush=True)
if name.startswith('mavk-native-public'):
    cases = [('routing', selected/'execution.spv', selected/'sampled.vert.spv', selected/'sampled.frag.spv', [])]
    for mode in ('dynamic', 'resubmit', 'inflight'):
        cases.append((mode, selected/'dynamic.spv', selected/'triangle.vert.spv', selected/'triangle.frag.spv', ['--'+mode]))
    for mode in ('empty', 'unused', 'selected'):
        cases.append((mode, selected/'gap.spv', selected/'triangle.vert.spv', selected/'triangle.frag.spv', ['--'+mode]))
    cases.append(('immutable', selected/'immutable.spv', selected/'triangle.vert.spv', selected/'triangle.frag.spv', ['--immutable']))
    cases.append(('images', base/'mavk-images/image.spv', selected/'triangle.vert.spv', selected/'triangle.frag.spv', ['--images']))
    for label, compute, vertex, fragment, flags in cases:
        result = subprocess.run([str(fixture/'consumer'), str(compute), str(vertex), str(fragment)] + flags,
                                env=env, capture_output=True, timeout=30)
        (layer / (label+'.stdout')).write_bytes(result.stdout)
        (layer / (label+'.stderr')).write_bytes(result.stderr)
        assert result.returncode == 0 and b'PASS' in result.stdout, result.stderr[-3000:]
        assert b'MAVK_LAYER_CORE_END liveZero=1' in result.stderr and b'MAVK_LAYER_GAP' not in result.stderr
        print('NATIVE_PUBLIC_REGRESSION_PASS '+label, flush=True)
