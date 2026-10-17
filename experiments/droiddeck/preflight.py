"""Verify actual PoC output and pinned DXVK admission; not a D3D runtime test."""
import argparse
import hashlib
import json
import re
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--artifacts', type=Path, required=True)
p.add_argument('--fork', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
assert not a.output.exists()
for folder in ['run1', 'proot-control']:
    summary = json.loads((a.artifacts/folder/'summary.json').read_text())
    log = (a.artifacts/folder/'stdout.log').read_text()
    assert summary['exit_code'] == 0 and summary['offscreen_pass']
    assert 'GRAPHICS_PIXELS_PASS checked=3330' in log and 'GLIBC_HYBRIS_G925_OFFSCREEN_PASS' in log
    assert 'DEVICE name=Mali-G925-Immortalis MC12 vendor=13b5 type=1' in log
    assert 'multiViewport=0 nonSolid=0' in log
    assert 'EXT VK_EXT_robustness2\n' not in log
driver = int(re.search(r'DEVICE .* driver=(\d+)', log)[1])
patch_path = a.fork/'tools/dxvk/mali-patches/0001-experimental-mali-capabilities-and-guards.patch'
patch_bytes = patch_path.read_bytes()
patch = patch_bytes.decode()
assert '&& properties.driverVersion == 0x0d803000;' in patch
assert 'ENABLE_FEATURE(core.features, fillModeNonSolid, !m_maliCompat)' in patch
assert 'ENABLE_FEATURE(core.features, multiViewport, !m_maliCompat)' in patch
assert driver != 0x0d803000
result = {
    'result': 'GLIBC_PROOT_HARDWARE_OFFSCREEN_PASS_DXVK_PREFLIGHT_NO_GO',
    'scope': 'Exact DroidDeck r9 proot and Linux libraries + pinned fork hybris, minimal subset, Termux debug UID. Not complete DroidDeck app/session PASS.',
    'checked_pixels_per_launch': 3330,
    'driver_raw': driver,
    'multiViewport': False, 'fillModeNonSolid': False, 'robustness2_extension': False,
    'pinned_dxvk_mali_profile_matches_driver': False,
    'dxvk_patch_sha256': hashlib.sha256(patch_bytes).hexdigest(),
    'd3d11_device_executed': False,
    'd3d11_conclusion': 'Current pinned profile cannot waive these required features on this driver. No guard broadening or fake features. This is source/capability preflight, not an observed D3D11 HRESULT.',
    'wsi_or_game_performance_measured': False,
    'next': 'Preserve functional ABI proof; return to Mavk Bridge. A separate G925 compatibility project is required before claiming DXVK applications.',
}
with a.output.open('x') as f:
    json.dump(result, f, indent=2)
print(json.dumps(result))
