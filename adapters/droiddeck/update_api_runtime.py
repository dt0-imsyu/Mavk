"""Package the checked common layer, PE32 API payload and normal launch policy.

No installation, guest data, account state or presentation-library replacement.
All other archive members and permissions must remain identical.
"""
import argparse
import ast
import json
from pathlib import Path
import runpy
import stat
import struct
import zipfile

from update_session_runtime import read_runtime, sha, LAYER, MANIFEST

MEMBERS = {LAYER, "proton_d3d11.py", "integration/droiddeck-game-env",
           "pe/i386/d3d9.dll", "pe/i386/dxgi.dll"}


def update(baseline, output, baseline_hash, inputs):
    if output.exists():
        raise ValueError("Fresh output directory required")
    if sha(baseline.read_bytes()) != baseline_hash:
        raise ValueError("Baseline archive checksum mismatch")
    if set(inputs) != MEMBERS:
        raise ValueError("Exact API component set required")
    entries, manifest = read_runtime(baseline)
    if not {LAYER, "proton_d3d11.py", "integration/droiddeck-game-env"} <= set(entries):
        raise ValueError("Existing checked launcher policy required")
    if not manifest["integration/droiddeck-game-env"]["executable"]:
        raise ValueError("Launch policy must remain executable")
    check_pe = runpy.run_path(str(Path(__file__).with_name("run-proton.py")))["check_machine"]
    replacements = {}
    for member, record in inputs.items():
        path = Path(record["path"])
        if not path.is_file() or not 0 < path.stat().st_size <= 32 * 1024 * 1024:
            raise ValueError("Missing or oversized API component")
        data = path.read_bytes()
        if sha(data) != record["sha256"]:
            raise ValueError("API input checksum mismatch: " + member)
        if member == LAYER:
            if (len(data) < 64 or data[:6] != b"\x7fELF\x02\x01" or
                    struct.unpack_from("<HH", data, 16) != (3, 183)):
                raise ValueError("ARM64 ELF shared library required")
        elif member.endswith(".dll"):
            check_pe(path, 0x14c)
        else:
            ast.parse(data.decode("utf-8"))
        replacements[member] = data
        executable = manifest.get(member, {}).get("executable", False)
        manifest[member] = dict(bytes=len(data), sha256=sha(data), executable=executable)
    output.mkdir(parents=True)
    destination = output / "runtime.zip"
    with zipfile.ZipFile(destination, "x", compression=zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for member in sorted(set(entries) | MEMBERS):
            data = replacements.get(member, entries.get(member, (None, None))[0])
            if member == MANIFEST:
                data = json.dumps(manifest, indent=2, sort_keys=True).encode()
            original = entries.get(member, (None, None))[1]
            info = zipfile.ZipInfo(member, original.date_time if original else (2026, 10, 26, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = original.external_attr if original else (stat.S_IFREG | 0o644) << 16
            archive.writestr(info, data)
    updated, _ = read_runtime(destination)
    for name, (data, info) in entries.items():
        if name not in MEMBERS | {MANIFEST}:
            if updated[name][0] != data or updated[name][1].external_attr != info.external_attr:
                raise ValueError("Unrelated runtime member changed")
    receipt = dict(version=1, baseline_sha256=baseline_hash, runtime_sha256=sha(destination.read_bytes()),
                   inputs=inputs, unchanged_members=len(set(entries) - MEMBERS - {MANIFEST}),
                   scope="Experimental API package; not Vulkan conformance or a full source rebuild")
    (output / "api-update.json").write_text(json.dumps(receipt, indent=2) + "\n")
    (output / "version.txt").write_text(receipt["runtime_sha256"] + "\n")
    return receipt


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--baseline-sha256", required=True)
    parser.add_argument("--inputs", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(update(args.baseline, args.output, args.baseline_sha256,
                            json.loads(args.inputs.read_text())), indent=2))
