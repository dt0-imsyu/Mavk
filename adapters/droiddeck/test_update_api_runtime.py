import json
from pathlib import Path
import struct
import tempfile
import unittest
import zipfile

from update_api_runtime import update, MEMBERS
from update_session_runtime import LAYER, read_runtime, sha


class ApiRuntime(unittest.TestCase):
    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.root = Path(tmp.name)
        self.baseline = self.root / "baseline.zip"
        files = {LAYER: b"old layer", "proton_d3d11.py": b"# old selector\n",
                 "integration/droiddeck-game-env": b"# old policy\n",
                 "licenses/COPYING": b"retain exact upstream notice"}
        manifest = {k: dict(bytes=len(v), sha256=sha(v), executable=True) for k, v in files.items()}
        files["manifest.json"] = json.dumps(manifest).encode()
        with zipfile.ZipFile(self.baseline, "w") as z:
            for name, data in files.items():
                i = zipfile.ZipInfo(name)
                i.external_attr = 0o100755 << 16
                z.writestr(i, data)
        elf = bytearray(64)
        elf[:6] = b"\x7fELF\x02\x01"
        struct.pack_into("<HH", elf, 16, 3, 183)
        pe = bytearray(128)
        pe[:2] = b"MZ"
        struct.pack_into("<I", pe, 60, 64)
        pe[64:68] = b"PE\0\0"
        struct.pack_into("<H", pe, 68, 0x14c)
        self.inputs = {}
        for n, member in enumerate(sorted(MEMBERS)):
            p = self.root / str(n)
            p.write_bytes(elf if member == LAYER else pe if member.endswith(".dll") else b"# new checked policy\n")
            self.inputs[member] = dict(path=str(p), sha256=sha(p.read_bytes()))

    def run_update(self):
        return update(self.baseline, self.root / "out", sha(self.baseline.read_bytes()), self.inputs)

    def test_exact_components_and_notices_permissions(self):
        receipt = self.run_update()
        self.assertEqual(receipt["unchanged_members"], 1)
        entries, manifest = read_runtime(self.root / "out/runtime.zip")
        self.assertEqual(entries["licenses/COPYING"][0], b"retain exact upstream notice")
        self.assertEqual(entries["licenses/COPYING"][1].external_attr, 0o100755 << 16)
        self.assertTrue(manifest["integration/droiddeck-game-env"]["executable"])
        self.assertFalse(manifest["pe/i386/d3d9.dll"]["executable"])
        with self.assertRaises(ValueError):
            self.run_update()

    def test_checksum_and_component_set_rejected(self):
        self.inputs[LAYER]["sha256"] = "0" * 64
        with self.assertRaises(ValueError):
            self.run_update()
        del self.inputs[LAYER]
        with self.assertRaises(ValueError):
            self.run_update()

    def test_wrong_pe_machine_rejected(self):
        r = self.inputs["pe/i386/d3d9.dll"]
        p = Path(r["path"])
        b = bytearray(p.read_bytes())
        struct.pack_into("<H", b, 68, 0xaa64)
        p.write_bytes(b)
        r["sha256"] = sha(b)
        with self.assertRaises(ValueError):
            self.run_update()


if __name__ == "__main__":
    unittest.main()
