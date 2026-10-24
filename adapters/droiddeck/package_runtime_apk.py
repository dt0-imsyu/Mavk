"""Asset-only experimental APK rollup; signing/alignment are separate SDK steps."""
import argparse
import hashlib
from pathlib import Path
import zipfile
from update_session_runtime import read_runtime


def signature(name):
    upper = name.upper()
    return upper == "META-INF/MANIFEST.MF" or (upper.startswith("META-INF/") and
                                              upper.endswith((".RSA", ".DSA", ".EC", ".SF")))


def package(baseline, baseline_hash, runtime, version, output, launch_hook=None, launch_hook_baseline_hash=None):
    if output.exists():
        raise ValueError("Preserve existing APK")
    if hashlib.sha256(baseline.read_bytes()).hexdigest() != baseline_hash:
        raise ValueError("Baseline APK differs")
    read_runtime(runtime)
    if version.read_text().strip() != hashlib.sha256(runtime.read_bytes()).hexdigest():
        raise ValueError("Runtime version mismatch")
    replacements = {"assets/mavk/runtime.zip": runtime.read_bytes(),
                    "assets/mavk/version.txt": version.read_bytes()}
    if (launch_hook is None) != (launch_hook_baseline_hash is None):
        raise ValueError("Launch hook requires its baseline hash")
    hook_member = "assets/linuxfs/usr/local/bin/steam-compatibility"
    if launch_hook is not None:
        replacements[hook_member] = launch_hook.read_bytes()
    untouched = {}
    with zipfile.ZipFile(baseline) as source:
        names = [item.filename for item in source.infolist()]
        if len(names) != len(set(names)) or not set(replacements).issubset(names):
            raise ValueError("Duplicate or missing baseline APK members")
        if launch_hook is not None and hashlib.sha256(source.read(hook_member)).hexdigest() != launch_hook_baseline_hash:
            raise ValueError("Launch hook baseline differs")
        with zipfile.ZipFile(output, "x") as target:
            for item in source.infolist():
                if signature(item.filename):
                    continue
                original = source.read(item)
                data = replacements.get(item.filename, original)
                if item.filename not in replacements:
                    untouched[item.filename] = hashlib.sha256(original).digest()
                target.writestr(item, data)
    with zipfile.ZipFile(output) as result:
        for name, digest in untouched.items():
            if hashlib.sha256(result.read(name)).digest() != digest:
                raise ValueError("Unexpected APK change: " + name)
    return len(untouched)


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("baseline", type=Path)
    p.add_argument("runtime", type=Path)
    p.add_argument("version", type=Path)
    p.add_argument("output", type=Path)
    p.add_argument("--baseline-sha256", required=True)
    p.add_argument("--launch-hook", type=Path)
    p.add_argument("--launch-hook-baseline-sha256")
    a = p.parse_args()
    count = package(a.baseline, a.baseline_sha256, a.runtime, a.version, a.output,
                    a.launch_hook, a.launch_hook_baseline_sha256)
    print(f"APK_ASSET_ROLLUP_PASS unchanged_members={count}; unsigned, not installed")
