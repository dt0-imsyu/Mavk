"""Roll only UTF-8 license/notice text into a pinned runtime, retaining executable bytes."""
import argparse
import copy
import json
from pathlib import Path
import zipfile

from update_session_runtime import read_runtime, sha


def update(baseline, notices, output, baseline_hash):
    if output.exists() or baseline.stat().st_size > 128*1024*1024:
        raise ValueError('Fresh bounded output required')
    if sha(baseline.read_bytes()) != baseline_hash:
        raise ValueError('Baseline checksum mismatch')
    entries, manifest = read_runtime(baseline)
    additions = {}
    for path in sorted(notices.rglob('*')):
        if path.is_symlink():
            raise ValueError('Notice symlinks rejected')
        if not path.is_file():
            continue
        name = path.relative_to(notices).as_posix()
        if (name.split('/')[0] != 'licenses' or '\\' in name or
                path.suffix.lower() not in ('', '.txt', '.md', '.json') or
                not 0 < path.stat().st_size <= 1024*1024):
            raise ValueError('Only bounded notice files permitted')
        data = path.read_bytes()
        data.decode('utf-8')
        if b'\0' in data:
            raise ValueError('Binary notice rejected')
        additions[name] = data
    if not additions or len(additions) > 32 or sum(map(len, additions.values())) > 2*1024*1024:
        raise ValueError('Notice set budget')
    merged = dict(entries)
    for name, data in additions.items():
        item = zipfile.ZipInfo(name, (2026, 10, 25, 0, 0, 0))
        item.external_attr = 0o100644 << 16
        merged[name] = data, item
        manifest[name] = dict(bytes=len(data), sha256=sha(data), executable=False)
    manifest_bytes = json.dumps(manifest, sort_keys=True, indent=2).encode()
    if len(manifest_bytes) > 256*1024 or len(merged) > 512:
        raise ValueError('Runtime manifest budget')
    if sum(len(v[0]) for name, v in merged.items() if name != 'manifest.json') + len(manifest_bytes) > 128*1024*1024:
        raise ValueError('Runtime extraction budget')
    output.mkdir(parents=True)
    target = output/'runtime.zip'
    with zipfile.ZipFile(target, 'x', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for name, (data, original) in sorted(merged.items()):
            item = copy.copy(original)
            item.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(item, manifest_bytes if name == 'manifest.json' else data)
    actual, _ = read_runtime(target)
    for name, (data, item) in entries.items():
        if name != 'manifest.json' and not name.startswith('licenses/'):
            if actual[name][0] != data or actual[name][1].external_attr != item.external_attr:
                raise ValueError('Unexpected executable/runtime change: ' + name)
    record = dict(baseline_sha256=baseline_hash, runtime_sha256=sha(target.read_bytes()),
                  added_members=sorted(set(actual)-set(entries)),
                  changed_members=sorted(n for n in entries if actual[n][0] != entries[n][0]),
                  executable_bytes_unchanged=True, corresponding_source_complete=False)
    (output/'version.txt').write_text(record['runtime_sha256']+'\n', encoding='utf-8')
    (output/'notice-provenance.json').write_text(json.dumps(record, indent=2)+'\n', encoding='utf-8')
    return record


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('baseline', 'notices', 'output'):
        p.add_argument(name, type=Path)
    p.add_argument('--baseline-sha256', required=True)
    a = p.parse_args()
    print(json.dumps(update(a.baseline, a.notices, a.output, a.baseline_sha256), indent=2))
