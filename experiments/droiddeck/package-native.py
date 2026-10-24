"""Bounded experimental native-library rollup; no manifest/DEX/runtime changes."""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('baseline', type=Path)
p.add_argument('library', type=Path)
p.add_argument('output', type=Path)
p.add_argument('--sha256', required=True)
a = p.parse_args()
sha = lambda b: hashlib.sha256(b).hexdigest()
assert sha(a.baseline.read_bytes()) == a.sha256, 'Pinned baseline required'
assert not a.output.exists(), 'Preserve previous APK'
library = a.library.read_bytes()
assert library[:5] == b'\x7fELF\x02' and library[18:20] == b'\xb7\x00', 'ARM64 ELF required'
entry = 'lib/arm64-v8a/libdroiddeckwayland.so'
unchanged = {}
def signing(name):
    name = name.upper()
    return name == 'META-INF/MANIFEST.MF' or (name.startswith('META-INF/') and name.endswith(('.SF', '.RSA', '.DSA', '.EC')))
with zipfile.ZipFile(a.baseline) as source:
    assert len(source.namelist()) == len(set(source.namelist()))
    assert entry in source.namelist()
    with zipfile.ZipFile(a.output, 'x') as target:
        for item in source.infolist():
            if signing(item.filename):
                continue
            data = source.read(item)
            if item.filename == entry:
                data = library
            else:
                unchanged[item.filename] = sha(data)
            target.writestr(item, data)
with zipfile.ZipFile(a.output) as result:
    assert all(sha(result.read(name)) == digest for name, digest in unchanged.items())
    assert sha(result.read(entry)) == sha(library)
print(json.dumps(dict(unchanged_members=len(unchanged), replaced=entry, library_sha256=sha(library), unsigned=True)))
