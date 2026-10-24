import hashlib
from pathlib import Path
import struct
import zipfile

base = Path("E:/Mavk/artifacts/ddnet-20.1.1-20261019")
source = base / "DDNet-20.1.1-win-arm64.zip"
assert hashlib.sha256(source.read_bytes()).hexdigest() == "3221662ceaac3e3d0b06bc9db8cbcd23de14f60f2807ca01db9c56d0b3cd0d18"
output = base / "arm64-binaries"
output.mkdir(exist_ok=False)
count = 0
with zipfile.ZipFile(source) as archive, zipfile.ZipFile(base / "DDNet-20.1.1-win64.zip") as control:
    old = {Path(n).name: control.read(n) for n in control.namelist() if n.endswith(".spv")}
    for name in archive.namelist():
        if name.endswith(".spv"):
            assert archive.read(name) == old[Path(name).name]
        if not name.endswith((".dll", ".exe")):
            continue
        data = archive.read(name)
        pe = struct.unpack_from("<I", data, 60)[0]
        assert data[:2] == b"MZ" and data[pe:pe + 4] == b"PE\0\0"
        machine = struct.unpack_from("<H", data, pe + 4)[0]
        assert machine in (0xaa64, 0xa641), (name, hex(machine))
        target = output / Path(name).name
        assert not target.exists()
        target.write_bytes(data)
        count += 1
print("ARM64 PE binaries verified:", count, "; original SPIR-V matches AMD64 archive")
