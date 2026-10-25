"""Reuse the existing GPU consumer to check optional diagnostic storage sharing."""
import os
from pathlib import Path
import subprocess
import sys

base = Path('/data/user/0/com.droiddeck.launcher.mavk/files')
name = sys.argv[1] if len(sys.argv)>1 else 'mavk-shared-diagnostics'
assert name in ('mavk-shared-diagnostics', 'mavk-descriptor-arena')
layer = base / name
fixture = base / 'mavk-aggregate'
selected = base / 'mavk-selected'
assert (layer / 'libVkLayer_mavk_compat.so').is_file()
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
                   MAVK_DESCRIPTOR_ARENA='1' if name=='mavk-descriptor-arena' else '0',
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
if name == 'mavk-descriptor-arena':
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
