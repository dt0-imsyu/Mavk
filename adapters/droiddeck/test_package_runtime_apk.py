import hashlib
import json
from pathlib import Path
import tempfile
import unittest
import zipfile

from package_runtime_apk import package
from update_session_runtime import LAYER


class ApkPackage(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.runtime = self.root / 'runtime.zip'
        data = b'unchanged core'
        manifest = {LAYER: dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest(), executable=True)}
        with zipfile.ZipFile(self.runtime, 'w') as z:
            item = zipfile.ZipInfo(LAYER)
            item.external_attr = 0o100755 << 16
            z.writestr(item, data)
            z.writestr('manifest.json', json.dumps(manifest))
        self.version = self.root / 'version.txt'
        self.version.write_text(hashlib.sha256(self.runtime.read_bytes()).hexdigest())
        self.baseline = self.root / 'baseline.apk'
        with zipfile.ZipFile(self.baseline, 'w') as z:
            z.writestr('classes.dex', b'preserve bytecode')
            z.writestr('assets/mavk/runtime.zip', b'old archive')
            z.writestr('assets/mavk/version.txt', b'old version')
            z.writestr('assets/linuxfs/usr/local/bin/steam-compatibility', b'old hook')
        self.hook = self.root / 'hook'
        self.hook.write_bytes(b'candidate hook')

    def run_package(self, **kwargs):
        return package(self.baseline, hashlib.sha256(self.baseline.read_bytes()).hexdigest(),
                       self.runtime, self.version, self.root / 'output.apk', **kwargs)

    def test_default_preserves_hook_and_bytecode(self):
        self.assertEqual(self.run_package(), 2)
        with zipfile.ZipFile(self.root / 'output.apk') as z:
            self.assertEqual(z.read('classes.dex'), b'preserve bytecode')
            self.assertEqual(z.read('assets/linuxfs/usr/local/bin/steam-compatibility'), b'old hook')

    def test_optional_hook_requires_matching_baseline(self):
        with self.assertRaises(ValueError):
            self.run_package(launch_hook=self.hook)
        self.assertFalse((self.root / 'output.apk').exists())
        with self.assertRaises(ValueError):
            self.run_package(launch_hook=self.hook, launch_hook_baseline_hash='0' * 64)
        self.assertFalse((self.root / 'output.apk').exists())

    def test_pinned_hook_and_only_requested_assets_change(self):
        self.assertEqual(self.run_package(launch_hook=self.hook,
                         launch_hook_baseline_hash=hashlib.sha256(b'old hook').hexdigest()), 1)
        with zipfile.ZipFile(self.root / 'output.apk') as z:
            self.assertEqual(z.read('classes.dex'), b'preserve bytecode')
            self.assertEqual(z.read('assets/linuxfs/usr/local/bin/steam-compatibility'), b'candidate hook')
