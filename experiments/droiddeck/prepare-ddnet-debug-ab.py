"""Remove only non-semantic shader debug information from isolated DDNet test data."""
import hashlib
import json
from pathlib import Path
import subprocess
import zipfile

base = Path("E:/Mavk/artifacts/ddnet-20.1.1-20261019")
output = base / "shader-debug-ab"
output.mkdir(exist_ok=False)
tools = Path("C:/Users/denis/AppData/Local/Android/Sdk/ndk/28.2.13676358/shader-tools/windows-x86_64")
records = {}
with zipfile.ZipFile(base / "DDNet-20.1.1-win64.zip") as archive:
    for name in archive.namelist():
        if not name.startswith("DDNet-20.1.1-win64/data/shader/vulkan/") or not name.endswith(".spv"):
            continue
        source = output / (Path(name).name + ".original")
        target = output / Path(name).name
        source.write_bytes(archive.read(name))
        subprocess.run([str(tools / "spirv-val.exe"), "--target-env", "vulkan1.1", str(source)], check=True)
        subprocess.run([str(tools / "spirv-opt.exe"), "--strip-debug", str(source), "-o", str(target)], check=True)
        subprocess.run([str(tools / "spirv-val.exe"), "--target-env", "vulkan1.1", str(target)], check=True)
        records[target.name] = {"original": hashlib.sha256(source.read_bytes()).hexdigest(),
                                "stripped": hashlib.sha256(target.read_bytes()).hexdigest()}
(output / "hashes.json").write_text(json.dumps(records, indent=2))
print("Validated original and debug-stripped shaders:", len(records))
