"""Architecture/API routing regression; no game names or application IDs."""
import os
from pathlib import Path
import runpy
import tempfile
import unittest
from unittest.mock import patch

MODULE = runpy.run_path(str(Path(__file__).resolve().parents[2] / "adapters/droiddeck/proton_d3d11.py"))


class Selection(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.proton = self.root / "proton"
        self.proton.write_text('''def setup():
            # check whether any prefix config has changed
            a = g_proton.arch_pe_dir("wine/dxvk", False) + f + ".dll"
            b = g_proton.arch_pe_dir("wine/dxvk", True) + f + ".dll"
''')
        self.pe = self.root / "pe"
        self.pe.mkdir()
        for name in MODULE["NAMES"]:
            (self.pe / (name + ".dll")).write_bytes(b"arm64x fixture")
        self.calls = []

    def check(self, path, machine):
        if not path.is_file():
            raise ValueError("missing payload")
        self.calls.append((path.name, machine))

    def prepare(self, payload=None):
        with patch.dict(os.environ, {}, clear=False):
            os.environ.pop("MAVK_SAREK_I386_PAYLOAD", None)
            if payload is not None:
                os.environ["MAVK_SAREK_I386_PAYLOAD"] = str(payload)
            return MODULE["prepare"](self.proton, self.pe, self.check, lambda p: None)[1]

    def test_optional_payload_keeps_existing_profile_unchanged(self):
        resolve = self.prepare()
        p = type("Proton", (), {"arch_pe_dir": lambda _, kind, is32: "/stock/i386-windows" if is32 else "/stock/aarch64-windows"})()
        self.assertEqual(Path(resolve(p, "d3d9", True)), Path("/stock/i386-windows/d3d9.dll"))
        self.assertEqual(Path(resolve(p, "d3d11", False)), self.pe / "d3d11.dll")

    def test_pe32_d3d9_and_dxgi_only(self):
        payload = self.root / "i386"
        payload.mkdir()
        for name in ("d3d9", "dxgi"):
            (payload / (name + ".dll")).write_bytes(b"i386 fixture")
        resolve = self.prepare(payload)
        p = type("Proton", (), {"arch_pe_dir": lambda _, kind, is32: "/stock/i386-windows" if is32 else "/stock/aarch64-windows"})()
        for name in ("d3d9", "dxgi"):
            self.assertEqual(Path(resolve(p, name, True)), payload / (name + ".dll"))
        self.assertEqual(Path(resolve(p, "d3d11", True)), Path("/stock/i386-windows/d3d11.dll"))
        self.assertEqual(Path(resolve(p, "dxgi", False)), self.pe / "dxgi.dll")
        self.assertEqual(self.calls[-2:], [("d3d9.dll", 0x14c), ("dxgi.dll", 0x14c)])

    def test_rejects_relative_or_incomplete_payload(self):
        for payload in ("relative", self.root / "missing"):
            with self.subTest(payload=payload), self.assertRaises(ValueError):
                self.prepare(payload)

    def test_packaged_pe32_payload_needs_no_private_environment_override(self):
        payload = self.pe / "i386"
        payload.mkdir()
        for name in ("d3d9", "dxgi"):
            (payload / (name + ".dll")).write_bytes(b"i386 fixture")
        resolve = self.prepare()
        p = type("Proton", (), {"arch_pe_dir": lambda _, kind, is32: "/stock/i386-windows"})()
        self.assertEqual(Path(resolve(p, "d3d9", True)), payload / "d3d9.dll")


if __name__ == "__main__":
    unittest.main()
