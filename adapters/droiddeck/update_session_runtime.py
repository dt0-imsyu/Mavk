"""Roll one verified Mavk library into an existing manifest-checked runtime.

No Proton, Mesa, hybris, Steam data or per-game configuration changes.
"""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import stat
import struct
import zipfile

LAYER = "payload/libVkLayer_mavk_compat.so"
MANIFEST = "manifest.json"
LIMIT = 128 * 1024 * 1024


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read_runtime(path):
    if path.stat().st_size > LIMIT:
        raise ValueError("Archive exceeds size limit")
    entries = {}
    with zipfile.ZipFile(path) as archive:
        total = 0
        for item in archive.infolist():
            name = item.filename
            parts = name.split("/")
            if (not name or item.is_dir() or name.startswith("/") or "\\" in name or
                    any(p in ("", ".", "..") for p in parts) or
                    PurePosixPath(name).is_absolute() or name in entries or
                    stat.S_ISLNK(item.external_attr >> 16)):
                raise ValueError("Unsafe or duplicate runtime entry: " + name)
            total += item.file_size
            if item.file_size > 32 * 1024 * 1024 or total > LIMIT or len(entries) >= 512:
                raise ValueError("Runtime extraction budget")
            entries[name] = (archive.read(item), item)
    raw = entries.get(MANIFEST, (b"", None))[0]
    if not raw or len(raw) > 256 * 1024:
        raise ValueError("Missing or oversized manifest")
    def unique_object(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise ValueError("Duplicate manifest key")
            result[key] = value
        return result
    manifest = json.loads(raw, object_pairs_hook=unique_object)
    if not isinstance(manifest, dict):
        raise ValueError("Manifest object required")
    if set(entries) != set(manifest) | {MANIFEST} or LAYER not in manifest:
        raise ValueError("Manifest entry mismatch")
    for name, record in manifest.items():
        data, item = entries[name]
        if (not isinstance(record, dict) or type(record.get("bytes")) is not int or record["bytes"] != len(data) or
                record.get("sha256") != sha(data) or
                type(record.get("executable")) is not bool or
                record["executable"] != bool((item.external_attr >> 16) & 0o111)):
            raise ValueError("Runtime integrity mismatch: " + name)
    return entries, manifest


def update(baseline, layer, output, baseline_hash, layer_hash):
    if output.exists():
        raise ValueError("Fresh output directory required")
    if baseline.stat().st_size > LIMIT or layer.stat().st_size > 32 * 1024 * 1024:
        raise ValueError("Input size budget")
    if sha(baseline.read_bytes()) != baseline_hash:
        raise ValueError("Baseline archive checksum mismatch")
    entries, manifest = read_runtime(baseline)
    replacement = layer.read_bytes()
    if sha(replacement) != layer_hash:
        raise ValueError("Replacement library checksum mismatch")
    if (len(replacement) < 64 or len(replacement) > 32 * 1024 * 1024 or
            replacement[:6] != b"\x7fELF\x02\x01" or
            struct.unpack_from("<HH", replacement, 16) != (3, 183)):
        raise ValueError("ARM64 little-endian ELF64 library required")
    manifest[LAYER] = dict(manifest[LAYER], bytes=len(replacement), sha256=layer_hash)
    new_manifest = json.dumps(manifest, sort_keys=True, indent=2).encode()
    output.mkdir(parents=True)
    destination = output / "runtime.zip"
    with zipfile.ZipFile(destination, "x", compression=zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for name, (data, original) in sorted(entries.items()):
            data = replacement if name == LAYER else new_manifest if name == MANIFEST else data
            item = zipfile.ZipInfo(name, original.date_time)
            item.compress_type = zipfile.ZIP_DEFLATED
            item.external_attr = original.external_attr
            archive.writestr(item, data)
    updated, _ = read_runtime(destination)
    changed = sorted(name for name in entries if entries[name][0] != updated[name][0])
    if set(changed) != {LAYER, MANIFEST}:
        raise ValueError("Unexpected changed runtime members: " + repr(changed))
    digest = sha(destination.read_bytes())
    (output / "version.txt").write_text(digest + "\n", encoding="utf-8")
    record = {"baseline_sha256": baseline_hash, "runtime_sha256": digest,
              "layer_sha256": layer_hash, "changed_members": changed,
              "unchanged_members": len(entries) - len(changed),
              "scope": "Layer rollup only; not APK install or full-game compatibility PASS"}
    (output / "rollup-provenance.json").write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    return record


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline", type=Path)
    parser.add_argument("layer", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--baseline-sha256", required=True)
    parser.add_argument("--layer-sha256", required=True)
    args = parser.parse_args()
    print(json.dumps(update(args.baseline, args.layer, args.output,
                            args.baseline_sha256, args.layer_sha256), indent=2))
