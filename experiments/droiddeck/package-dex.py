"""Experimental DEX-only APK rollup; leaves native libraries and runtime intact."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import zipfile


def digest(data):
    return hashlib.sha256(data).hexdigest()


def classes(data):
    if len(data) < 112 or not re.fullmatch(rb'dex\n0[0-9]{2}\x00', data[:8]):
        raise ValueError('DEX magic/header')
    if struct.unpack_from('<I', data, 32)[0] != len(data):
        raise ValueError('DEX size')
    strings = struct.unpack_from('<I', data, 60)[0]
    types = struct.unpack_from('<I', data, 68)[0]
    count, offset = struct.unpack_from('<II', data, 96)
    result = set()
    for i in range(count):
        type_id = struct.unpack_from('<I', data, offset + i * 32)[0]
        string_id = struct.unpack_from('<I', data, types + type_id * 4)[0]
        position = struct.unpack_from('<I', data, strings + string_id * 4)[0]
        for _ in range(5):
            byte = data[position]
            position += 1
            if byte < 128:
                break
        else:
            raise ValueError('DEX string length')
        end = data.index(b'\0', position)
        name = data[position:end].decode('ascii')
        if name in result:
            raise ValueError('Duplicate DEX class: ' + name)
        result.add(name)
    return result


def signature(name):
    return name.upper() == 'META-INF/MANIFEST.MF' or (
        name.upper().startswith('META-INF/') and name.upper().endswith(('.SF', '.RSA', '.DSA', '.EC')))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('baseline', type=Path)
    parser.add_argument('dex_root', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--sha256', required=True)
    args = parser.parse_args()
    assert digest(args.baseline.read_bytes()) == args.sha256, 'Baseline hash mismatch'
    assert not args.output.exists(), 'Preserve existing output'
    paths = sorted(args.dex_root.rglob('*.dex'))
    assert 1 <= len(paths) <= 64
    replacements, new_classes = {}, set()
    for i, path in enumerate(paths, 1):
        data = path.read_bytes()
        assert len(data) <= 64 * 1024 * 1024
        names = classes(data)
        assert not names & new_classes, 'Duplicate class across new DEX files'
        new_classes.update(names)
        replacements['classes' + (str(i) if i > 1 else '') + '.dex'] = data
    unchanged, old_classes = {}, set()
    with zipfile.ZipFile(args.baseline) as source:
        assert len(source.namelist()) == len(set(source.namelist()))
        for item in source.infolist():
            if re.fullmatch(r'classes(?:[2-9]|[1-9][0-9]+)?\.dex', item.filename):
                old_classes.update(classes(source.read(item)))
        added, removed = sorted(new_classes - old_classes), sorted(old_classes - new_classes)
        assert not removed, 'Unexpected removed classes: ' + repr(removed[:20])
        unexpected = [name for name in added if not any(
            prefix in name for prefix in ('/wayland/FrameLoop', '/wayland/CompositorHost'))]
        assert not unexpected, 'Unexpected added classes: ' + repr(unexpected[:20])
        with zipfile.ZipFile(args.output, 'x') as target:
            for item in source.infolist():
                if signature(item.filename) or re.fullmatch(r'classes(?:[2-9]|[1-9][0-9]+)?\.dex', item.filename):
                    continue
                data = source.read(item)
                unchanged[item.filename] = digest(data)
                target.writestr(item, data)
            for name, data in replacements.items():
                target.writestr(name, data, compress_type=zipfile.ZIP_DEFLATED)
    with zipfile.ZipFile(args.output) as result:
        assert all(digest(result.read(name)) == value for name, value in unchanged.items())
    print(json.dumps(dict(unchanged_members=len(unchanged), added_classes=added,
                          removed_classes=removed, dex_files=len(paths), unsigned=True), indent=2))


if __name__ == '__main__':
    main()
