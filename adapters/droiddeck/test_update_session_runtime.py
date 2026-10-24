import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import unittest
import warnings
import zipfile

spec = importlib.util.spec_from_file_location("rollup", Path(__file__).with_name("update_session_runtime.py"))
rollup = importlib.util.module_from_spec(spec)
spec.loader.exec_module(rollup)


class RuntimeRollup(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.elf = bytearray(64)
        self.elf[:6] = b"\x7fELF\x02\x01"
        struct.pack_into("<HH", self.elf, 16, 3, 183)
        self.layer = self.root / "layer.so"
        self.layer.write_bytes(self.elf)

    def baseline(self, extra=None):
        path = self.root / "baseline.zip"
        files = {rollup.LAYER: b"old library", "licenses/license.txt": b"retain me"}
        manifest = {name: {"bytes": len(data), "sha256": rollup.sha(data), "executable": True}
                    for name, data in files.items()}
        files[rollup.MANIFEST] = json.dumps(manifest).encode()
        if extra:
            files.update(extra)
        with zipfile.ZipFile(path, "w") as archive:
            for name, data in files.items():
                item = zipfile.ZipInfo(name)
                item.external_attr = 0o100755 << 16
                archive.writestr(item, data)
        return path

    def run_update(self, baseline, baseline_hash=None, layer_hash=None):
        return rollup.update(baseline, self.layer, self.root / "new",
                             baseline_hash or rollup.sha(baseline.read_bytes()),
                             layer_hash or rollup.sha(self.layer.read_bytes()))

    def test_changes_only_layer_and_manifest(self):
        result = self.run_update(self.baseline())
        self.assertEqual(result["unchanged_members"], 1)
        self.assertEqual(result["changed_members"], ["manifest.json", rollup.LAYER])
        files, _ = rollup.read_runtime(self.root / "new/runtime.zip")
        self.assertEqual(files["licenses/license.txt"][0], b"retain me")

    def test_wrong_inputs_and_existing_output_rejected(self):
        baseline = self.baseline()
        for kwargs in ({"baseline_hash": "bad"}, {"layer_hash": "bad"}):
            with self.assertRaises(ValueError):
                self.run_update(baseline, **kwargs)
        self.run_update(baseline)
        with self.assertRaises(ValueError):
            self.run_update(baseline)

    def test_unsafe_path_rejected(self):
        with self.assertRaises(ValueError):
            self.run_update(self.baseline({"../escape": b"x"}))

    def test_unlisted_entry_rejected(self):
        with self.assertRaises(ValueError):
            self.run_update(self.baseline({"unlisted": b"x"}))

    def test_non_arm64_shared_library_rejected(self):
        baseline = self.baseline()
        struct.pack_into("<H", self.elf, 16, 2)
        self.layer.write_bytes(self.elf)
        with self.assertRaises(ValueError):
            self.run_update(baseline)

    def test_bad_manifest_shape_and_member_hash_rejected(self):
        for extra in ({"manifest.json": b"[]"},
                      {"licenses/license.txt": b"changed"}):
            with self.assertRaises(ValueError):
                self.run_update(self.baseline(extra))

    def test_duplicate_archive_member_rejected(self):
        baseline = self.baseline()
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", UserWarning)
            with zipfile.ZipFile(baseline, "a") as archive:
                archive.writestr("licenses/license.txt", b"retain me")
        with self.assertRaises(ValueError):
            self.run_update(baseline)

    def test_duplicate_manifest_key_rejected(self):
        with self.assertRaises(ValueError):
            self.run_update(self.baseline({"manifest.json": b'{"entry":{},"entry":{}}'}))


if __name__ == "__main__":
    unittest.main()
