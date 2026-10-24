"""Add an opt-in AHB component to a verified runtime without replacing existing members."""
import argparse
import copy
import json
from pathlib import Path
import struct
import zipfile

from update_session_runtime import read_runtime, sha

PREFIX = 'presentation/ahb/'
LIBRARY = 'libVkLayer_window_system_integration.so'
MEMBERS = (LIBRARY, 'VkLayer_window_system_integration.json', 'mavk-ahb.json', 'LICENSE.wsi-layer')
POLICY = 'integration/droiddeck-game-env'
HELPERS = ('droiddeck-esync', 'droiddeck-wincomponents')


def package(baseline, component, launcher, output, baseline_hash, library_hash, launcher_hash, helpers, helper_hashes,
            replace_component=False):
    if output.exists():
        raise ValueError('Fresh output directory required')
    if baseline.stat().st_size > 128 * 1024 * 1024:
        raise ValueError('Baseline archive size budget')
    if sha(baseline.read_bytes()) != baseline_hash:
        raise ValueError('Baseline runtime checksum mismatch')
    entries, manifest = read_runtime(baseline)
    existing = {name for name in entries if name.startswith(PREFIX) or name.startswith('integration/')}
    expected = {PREFIX + name for name in MEMBERS} | {POLICY} | {'integration/' + name for name in HELPERS}
    if replace_component and existing != expected:
        raise ValueError('Replacement requires the exact existing presentation component')
    if existing and not replace_component:
        raise ValueError('Do not overwrite an existing presentation component')
    additions = {}
    for name in MEMBERS:
        path = component / name
        if path.is_symlink() or not path.is_file() or not 0 < path.stat().st_size <= 32 * 1024 * 1024:
            raise ValueError('Invalid component file: ' + name)
        additions[PREFIX + name] = path.read_bytes()
    library = additions[PREFIX + LIBRARY]
    if (sha(library) != library_hash or len(library) < 64 or library[:6] != b'\x7fELF\x02\x01'
            or struct.unpack_from('<HH', library, 16) != (3, 183)):
        raise ValueError('Pinned ARM64 ELF64 shared WSI library required')
    receipt = json.loads(additions[PREFIX + 'mavk-ahb.json'])
    if (not isinstance(receipt, dict) or type(receipt.get('version')) is not int or receipt['version'] != 1
            or receipt.get('library_sha256') != library_hash or receipt.get('present_modes') != ['fifo']
            or receipt.get('min_images') != 3 or receipt.get('bgra8_gpu_blit') is not True):
        raise ValueError('BGRA/FIFO/three-buffer receipt mismatch')
    layer = json.loads(additions[PREFIX + 'VkLayer_window_system_integration.json']).get('layer', {})
    if (layer.get('name') != 'VK_LAYER_window_system_integration'
            or layer.get('library_path') not in (LIBRARY, './' + LIBRARY)):
        raise ValueError('Layer manifest must name the bundled WSI library')
    if launcher.is_symlink() or not 0 < launcher.stat().st_size <= 1024 * 1024:
        raise ValueError('Launch policy size/type invalid')
    policy = launcher.read_bytes()
    if sha(policy) != launcher_hash:
        raise ValueError('Launch policy checksum mismatch')
    additions[POLICY] = policy
    if set(helper_hashes) != set(HELPERS):
        raise ValueError('Both launch-policy helpers must be pinned')
    for name in HELPERS:
        path = helpers / name
        if path.is_symlink() or not path.is_file() or not 0 < path.stat().st_size <= 1024 * 1024:
            raise ValueError('Invalid launch-policy helper: ' + name)
        data = path.read_bytes()
        if sha(data) != helper_hashes[name]:
            raise ValueError('Launch-policy helper checksum mismatch: ' + name)
        additions['integration/' + name] = data
    new_manifest = copy.deepcopy(manifest)
    for name, data in additions.items():
        new_manifest[name] = dict(bytes=len(data), sha256=sha(data), executable=name.endswith('.so') or name.startswith('integration/'))
    encoded = json.dumps(new_manifest, sort_keys=True, indent=2).encode()
    if len(encoded) > 256 * 1024 or len(new_manifest) + 1 > 512:
        raise ValueError('Runtime manifest extraction budget')
    if sum(len(data) for name, (data, _) in entries.items() if name not in additions and name != 'manifest.json') + sum(map(len, additions.values())) + len(encoded) > 128 * 1024 * 1024:
        raise ValueError('Runtime extraction size budget')
    output.mkdir(parents=True)
    destination = output / 'runtime.zip'
    with zipfile.ZipFile(destination, 'x', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for name, (data, original) in sorted(entries.items()):
            if name in additions:
                continue
            item = copy.copy(original)
            item.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(item, encoded if name == 'manifest.json' else data)
        for name, data in sorted(additions.items()):
            item = zipfile.ZipInfo(name, (2026, 10, 24, 0, 0, 0))
            item.compress_type = zipfile.ZIP_DEFLATED
            item.external_attr = (0o100755 if new_manifest[name]['executable'] else 0o100644) << 16
            archive.writestr(item, data)
    actual, _ = read_runtime(destination)
    for name, (data, item) in entries.items():
        if name != 'manifest.json' and name not in additions and (actual[name][0] != data or actual[name][1].external_attr != item.external_attr):
            raise ValueError('Original runtime changed: ' + name)
    digest = sha(destination.read_bytes())
    (output / 'version.txt').write_text(digest + '\n', encoding='utf-8')
    record = dict(runtime_sha256=digest, baseline_sha256=baseline_hash, library_sha256=library_hash,
                  launcher_sha256=launcher_hash, helper_sha256=helper_hashes, added_members=sorted(set(additions)-set(entries)),
                  unchanged_members=sum(actual[name][0] == data for name, (data, _) in entries.items()),
                  changed_members=sorted(name for name, (data, _) in entries.items() if actual[name][0] != data),
                  scope='Optional component only; no APK install, default selection or game compatibility claim')
    (output / 'presentation-provenance.json').write_text(json.dumps(record, indent=2) + '\n', encoding='utf-8')
    return record


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('baseline', 'component', 'launcher', 'output'):
        p.add_argument(name, type=Path)
    for name in ('baseline', 'library', 'launcher'):
        p.add_argument('--' + name + '-sha256', required=True)
    p.add_argument('--helpers', type=Path, required=True)
    p.add_argument('--replace-component', action='store_true', help='Replace only the exact existing optional component')
    for name in HELPERS:
        p.add_argument('--' + name + '-sha256', required=True)
    a = p.parse_args()
    print(json.dumps(package(a.baseline, a.component, a.launcher, a.output,
                             a.baseline_sha256, a.library_sha256, a.launcher_sha256, a.helpers,
                             {name: getattr(a, name.replace('-', '_') + '_sha256') for name in HELPERS},
                             replace_component=a.replace_component), indent=2))
