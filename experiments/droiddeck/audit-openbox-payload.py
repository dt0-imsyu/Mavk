"""Compare actual packaged Openbox dependencies to retained package archives.

No package installation, execution, signature-verification claim or code changes.
The caller supplies small owned archives; output excludes binaries and account data.
"""
import argparse
import hashlib
import io
import json
import posixpath
from pathlib import Path
import tarfile
from zipfile import ZipFile

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('packages', type=Path)
p.add_argument('runtime', type=Path)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--openbox-source', type=Path, required=True)
p.add_argument('--startup-source', type=Path, required=True)
a = p.parse_args()
assert not a.output.exists(), 'Fresh evidence directory required'
sha = lambda value: hashlib.sha256(value).hexdigest()
evidence, owners, notices = [], {}, {}
with tarfile.open(a.packages) as outer, ZipFile(a.runtime) as runtime:
    payload = {name[len('openbox/root/'):]: runtime.read(name) for name in runtime.namelist()
               if name.startswith('openbox/root/')}
    for entry in outer:
        if not entry.isfile() or not entry.name.endswith('.pkg.tar.xz'):
            continue
        assert entry.size <= 4*1024*1024
        package = outer.extractfile(entry).read()
        with tarfile.open(fileobj=io.BytesIO(package)) as archive:
            members = {m.name.removeprefix('./'): m for m in archive}
            info = archive.extractfile(members['.PKGINFO']).read().decode()
            metadata = {}
            for line in info.splitlines():
                if ' = ' in line:
                    key, value = line.split(' = ', 1)
                    if key in ('pkgname', 'pkgver', 'arch', 'license', 'url'):
                        metadata.setdefault(key, []).append(value)
            matched, mismatched = [], []
            for name, data in payload.items():
                member = members.get(name)
                if member is None:
                    continue
                if member.issym():
                    target = posixpath.normpath(posixpath.join(posixpath.dirname(name), member.linkname))
                    member = members.get(target)
                if member is None or not member.isfile():
                    continue
                original = archive.extractfile(member).read()
                if original == data:
                    owners[name] = metadata['pkgname'][0]
                    matched.append(dict(path=name, sha256=sha(data)))
                else:
                    mismatched.append(name)
                if '/licenses/' in name and original == data:
                    notices[name.replace('usr/share/licenses/', '')] = data
            evidence.append(dict(metadata=metadata, archive=Path(entry.name).name,
                                 archive_sha256=sha(package), matched=matched, mismatched=mismatched))
sources = []
for label, path, url in (
    ('openbox', a.openbox_source, 'https://openbox.org/dist/openbox/openbox-3.6.1.tar.gz'),
    ('startup-notification', a.startup_source,
     'https://www.freedesktop.org/software/startup-notification/releases/startup-notification-0.12.tar.gz')):
    with tarfile.open(path) as archive:
        for member in archive:
            if member.isfile() and Path(member.name).name in ('COPYING', 'AUTHORS'):
                assert member.size <= 128*1024
                notices[label+'/'+Path(member.name).name] = archive.extractfile(member).read()
    sources.append(dict(name=label, url=url, sha256=sha(path.read_bytes())))
assert evidence and not any(record['mismatched'] for record in evidence), 'Modified package payload'
unattributed = sorted(set(payload)-set(owners))
report = dict(packages=evidence, upstream_sources=sources, matched_files=len(owners),
              unattributed_files=unattributed, signature_reverified=False,
              corresponding_source_complete=False,
              scope='Exact package byte attribution and notices, not full distribution compliance')
a.output.mkdir(parents=True)
for name, data in notices.items():
    target = a.output/'notices'/name
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(data)
(a.output/'package-attribution.json').write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps(dict(packages=len(evidence), matched_files=len(owners),
                      unattributed_files=unattributed, notices=sorted(notices)), indent=2))
