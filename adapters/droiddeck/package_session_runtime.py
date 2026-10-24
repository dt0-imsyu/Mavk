"""Package the verified private-session payload; never include Steam data or test programs."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import tarfile
import zipfile

parser = argparse.ArgumentParser()
parser.add_argument("export", type=Path)
parser.add_argument("output", type=Path)
args = parser.parse_args()
repo = Path(__file__).resolve().parents[2]
exact = {
    "payload/libVkLayer_mavk_compat.so", "payload/VkLayer_mavk_compat.json",
    "payload/libVkLayer_hybris_compat.so", "payload/VkLayer_hybris_compat.json",
    "pe/d3d10core.dll", "pe/d3d11.dll", "pe/dxgi.dll",
    "pe/libmavk-x11-sysv-owner.so", "pe/libmavk-wine-protection-trace.so",
}
files = {}
with tarfile.open(args.export) as source:
    members = {member.name.rstrip("/"): member for member in source.getmembers()}
    for name, member in members.items():
        if not (name in exact or name.startswith("openbox/root/")) or member.isdir():
            continue
        path = PurePosixPath(name)
        if path.is_absolute() or ".." in path.parts:
            raise ValueError("Unsafe payload path")
        mode = member.mode & 0o111
        if member.issym():
            target = path.parent / member.linkname
            if ".." in target.parts or target.is_absolute():
                raise ValueError("Unsafe library alias")
            member = members[str(target)]
            mode = member.mode & 0o111
        if not member.isfile():
            raise ValueError("Non-file payload")
        files[name] = (source.extractfile(member).read(), bool(mode))
if not exact.issubset(files):
    raise ValueError("Missing required payload")
trusted = {
    "payload/libVkLayer_mavk_compat.so": Path("E:/Mavk/artifacts/mavk-glibc-layer28-20261018/libVkLayer_mavk_compat.so"),
    "pe/libmavk-x11-sysv-owner.so": Path("E:/Mavk/artifacts/wine-arm64-inputs-20261018/libmavk-x11-sysv-owner.so"),
    "pe/libmavk-wine-protection-trace.so": Path("E:/Mavk/artifacts/wine-arm64-inputs-20261018/libmavk-wine-protection-trace9.so"),
}
for name in ("d3d10core", "d3d11", "dxgi"):
    folder = "d3d10" if name == "d3d10core" else name
    trusted["pe/" + name + ".dll"] = Path("E:/Mavk/artifacts/sarek-pe-arm64x-build1-20261018/src") / folder / (name + ".dll")
for name, original in trusted.items():
    if hashlib.sha256(files[name][0]).digest() != hashlib.sha256(original.read_bytes()).digest():
        raise ValueError("Payload differs from verified build: " + name)
files["pe/libmavk-wine-protection-trace.so"] = (
    Path("E:/Mavk/artifacts/wine-arm64-inputs-20261018/libmavk-wine-protection-trace10.so").read_bytes(), True)
files["payload/libVkLayer_mavk_compat.so"] = (
    Path("E:/Mavk/artifacts/mavk-layer-ui-bounded-log-20261019/libVkLayer_mavk_compat.so").read_bytes(), True)
mesa = Path("E:/Mavk/artifacts/zink-ui-25.1.9-20261019")
for name, aliases in {
    "libgallium-25.1.9.so": [],
    "libGL.so.1.2.0": ["libGL.so.1", "libGL.so"],
    "libEGL.so.1.0.0": ["libEGL.so.1", "libEGL.so"],
    "libGLESv2.so.2.0.0": ["libGLESv2.so.2", "libGLESv2.so"],
}.items():
    data = (mesa / "stage/opt/mavk-zink-ui/lib" / name).read_bytes()
    for output_name in [name, *aliases]:
        files["mesa25/lib/" + output_name] = (data, True)
for name in ("docs/license.rst", "src/c11/threads.h", "src/c11/impl/threads_posix.c"):
    files["licenses/Mesa25-" + Path(name).name] = ((mesa / "mesa-25.1.9" / name).read_bytes(), False)
for name in ("run-proton.py", "proton_d3d11.py"):
    files[name] = ((repo / "adapters/droiddeck" / name).read_bytes(), True)
for name in ("LICENSE", "THIRD_PARTY_NOTICES.md"):
    files["licenses/Mavk-" + name] = ((repo / name).read_bytes(), False)
files["licenses/DroidDeck-GPL3-LICENSE"] = ((repo / "adapters/droiddeck/LICENSE.DroidDeck").read_bytes(), False)
sarek = Path("E:/Mavk/artifacts/sarek-native-source-20261018/source/LICENSE")
files["licenses/Sarek-LICENSE"] = (sarek.read_bytes(), False)
for name, original in {
    "SPIRV-Tools-LICENSE": Path("E:/Mavk/research/spirv-tools-v2022.4/LICENSE"),
    "SPIRV-Headers-LICENSE": Path("E:/Mavk/research/spirv-tools-v2022.4/external/spirv-headers/LICENSE"),
    "bcdec-LICENSE": repo / "third_party/bcdec/LICENSE",
    "hybris-Apache2-LICENSE": Path("E:/Mavk/artifacts/hybris-bc5-layer1-20261018/rg32f-build/licenses/libhybris-Apache2.txt"),
    "hybris-bcdec-LICENSE": Path("E:/Mavk/artifacts/hybris-bc5-layer1-20261018/rg32f-build/licenses/bcdec-MIT.txt"),
}.items():
    files["licenses/" + name] = (original.read_bytes(), False)
manifest = {name: {"sha256": hashlib.sha256(data).hexdigest(), "bytes": len(data), "executable": executable}
            for name, (data, executable) in sorted(files.items())}
files["manifest.json"] = (json.dumps(manifest, sort_keys=True, indent=2).encode(), False)
args.output.mkdir(parents=True, exist_ok=True)
archive = args.output / "runtime.zip"
with zipfile.ZipFile(archive, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=6) as output:
    for name, (data, executable) in sorted(files.items()):
        info = zipfile.ZipInfo(name, (2026, 10, 19, 0, 0, 0))
        info.compress_type = zipfile.ZIP_DEFLATED
        info.external_attr = (0o100755 if executable else 0o100644) << 16
        output.writestr(info, data)
digest = hashlib.sha256(archive.read_bytes()).hexdigest()
(args.output / "version.txt").write_text(digest + "\n")
print(json.dumps({"archive": str(archive), "sha256": digest, "files": len(manifest),
                  "uncompressed_bytes": sum(len(data) for data, _ in files.values())}))
