"""Read-only, cold-only existing diagnostic slot plan. Never stages or boots."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--adb', required=True)
p.add_argument('--uavf', type=Path, required=True)
p.add_argument('--client', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
assert not a.output.exists()
a.output.mkdir(parents=True)
spec = importlib.util.spec_from_file_location('mavk_cold_fat', a.uavf/'tools/windows-reassessment/isolated-device-bcd.py')
g = importlib.util.module_from_spec(spec)
spec.loader.exec_module(g)
fat = g.DeviceFat(a.adb, a.output, expected_efi_sha=g.RAM_GOP_HANDOFF_AUDIT_EFI_SHA)
path = '/IDDWORK/idd-motion-source.exe'
original, chain = fat.file(path)
candidate = a.client.read_bytes()
old_sha = hashlib.sha256(original).hexdigest()
new_sha = hashlib.sha256(candidate).hexdigest()
print(json.dumps({'observed_file_bytes':len(original),'candidate_file_bytes':len(candidate),'cluster_bytes':fat.cluster_bytes,'cluster_count':len(chain),'before_sha256':old_sha}))
assert len(original) == 7680 and len(candidate) <= len(original) and fat.cluster_bytes == 8192
assert len(chain) == 1
assert old_sha == '7da58079674c0bd417ab54159506d3185892b4590931b6d4465981bf3608c554'
assert new_sha in {
    '99ef3264b1cc5d7917517a946fb0333ce1d084e4704a293eb4ff2b83f718ad9a',
    '406ed9735b7eea2df8472bcd51b2495ecaad08a6b6303759f38960d3b0ec990b',
    'c9dec0baf7d07c79c849f323db69f97c23a72e9c3fca9236377705c16e663568',
    '7d3a6e29488f7c14bb503e4bb832ff41362322d46df4124cddf68549117fbddf',
    '8574b919eae74f51c0ff8eea047be67698709e4bf7b4b34997fd9773387b809e',
    '3670bbf1af842707460103b0bb6473a1a8cc63d05b9770d1dbc43f0ff64ccae9',
}
original_cluster = fat.read(fat.offset(chain[0]),fat.cluster_bytes)
assert original_cluster[:len(original)] == original
# PE permits an inert trailing overlay; retain FAT file length and original
# unused cluster tail. No directory entry or allocation-table change required.
padded_file = candidate + bytes(len(original)-len(candidate))
candidate_cluster = padded_file + original_cluster[len(original):]
(a.output/'before.bin').write_bytes(original_cluster)
(a.output/'candidate.bin').write_bytes(candidate_cluster)
report = {
    'result': 'COLD_EXISTING_SLOT_PREFLIGHT_PASS',
    'path': path, 'cluster': chain[0], 'offset': fat.offset(chain[0]),
    'file_bytes': len(original), 'input_client_bytes':len(candidate),
    'bytes':len(original_cluster), 'before_file_sha256':old_sha, 'input_client_sha256':new_sha,
    'before_sha256':hashlib.sha256(original_cluster).hexdigest(),
    'candidate_sha256':hashlib.sha256(candidate_cluster).hexdigest(),
    'package': g.PACKAGE, 'disk': g.DISK, 'efi_sha256': g.RAM_GOP_HANDOFF_AUDIT_EFI_SHA,
    'applied': False, 'vm_booted': False,
    'scope': 'Existing diagnostic helper slot only; no FAT/GPT/root data/firmware mutation. Readback needed again immediately before any later apply.',
}
with (a.output/'plan.json').open('x') as f:
    json.dump(report, f, indent=2)
print(json.dumps(report))
