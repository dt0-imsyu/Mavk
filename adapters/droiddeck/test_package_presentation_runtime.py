import json
from pathlib import Path
import struct
import tempfile
import unittest
import zipfile

from package_presentation_runtime import HELPERS, LIBRARY, MEMBERS, POLICY, PREFIX, package
from update_session_runtime import LAYER, read_runtime, sha


class PresentationPackage(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.component = self.root / 'component'
        self.component.mkdir()
        elf = bytearray(64)
        elf[:6] = b'\x7fELF\x02\x01'
        struct.pack_into('<HH', elf, 16, 3, 183)
        (self.component / LIBRARY).write_bytes(elf)
        self.library_hash = sha(elf)
        self.receipt = dict(version=1, library_sha256=self.library_hash, present_modes=['fifo'], min_images=3, bgra8_gpu_blit=True)
        (self.component / 'mavk-ahb.json').write_text(json.dumps(self.receipt))
        (self.component / 'VkLayer_window_system_integration.json').write_text(json.dumps(dict(
            layer=dict(name='VK_LAYER_window_system_integration', library_path='./' + LIBRARY))))
        (self.component / 'LICENSE.wsi-layer').write_text('fixture license')
        self.launcher = self.root / 'launcher'
        self.launcher.write_bytes(b'fixture launcher')
        self.helpers = self.root / 'helpers'
        self.helpers.mkdir()
        for name in HELPERS:
            (self.helpers / name).write_bytes(('fixture ' + name).encode())
        self.helper_hashes = {name: sha((self.helpers / name).read_bytes()) for name in HELPERS}
        self.baseline = self.root / 'baseline.zip'
        files = {LAYER: b'original core', 'mesa25/lib/preserve.so': b'original Mesa'}
        manifest = {name: dict(bytes=len(data), sha256=sha(data), executable=True) for name, data in files.items()}
        files['manifest.json'] = json.dumps(manifest).encode()
        with zipfile.ZipFile(self.baseline, 'w') as z:
            for name, data in files.items():
                item = zipfile.ZipInfo(name)
                item.external_attr = 0o100755 << 16
                z.writestr(item, data)

    def run_package(self, **changes):
        args = dict(baseline=self.baseline, component=self.component, launcher=self.launcher,
                    output=self.root / 'output', baseline_hash=sha(self.baseline.read_bytes()),
                    library_hash=self.library_hash, launcher_hash=sha(self.launcher.read_bytes()),
                    helpers=self.helpers, helper_hashes=self.helper_hashes)
        args.update(changes)
        return package(**args)

    def test_only_optional_members_added_and_manifest_changes(self):
        result = self.run_package()
        self.assertEqual(result['changed_members'], ['manifest.json'])
        self.assertEqual(set(result['added_members']), {PREFIX+n for n in MEMBERS} | {POLICY} | {'integration/' + n for n in HELPERS})
        entries, _ = read_runtime(self.root / 'output/runtime.zip')
        self.assertEqual(entries[LAYER][0], b'original core')
        self.assertEqual(entries['mesa25/lib/preserve.so'][0], b'original Mesa')
        self.assertEqual(entries[POLICY][0], self.launcher.read_bytes())
        for name in HELPERS:
            self.assertEqual(entries['integration/' + name][0], (self.helpers / name).read_bytes())
        with self.assertRaises(ValueError):
            self.run_package()

    def test_input_pins_and_receipt_are_enforced(self):
        for key in ('baseline_hash', 'library_hash', 'launcher_hash'):
            with self.assertRaises(ValueError):
                self.run_package(**{key: '0' * 64})
        (self.component / 'mavk-ahb.json').write_text(json.dumps(dict(self.receipt, min_images=2)))
        with self.assertRaises(ValueError):
            self.run_package()

    def test_layer_cannot_redirect_outside_component(self):
        (self.component / 'VkLayer_window_system_integration.json').write_text(json.dumps(dict(
            layer=dict(name='VK_LAYER_window_system_integration', library_path='/vendor/lib64/other.so'))))
        with self.assertRaises(ValueError):
            self.run_package()

    def test_missing_or_modified_helpers_rejected_before_output(self):
        with self.assertRaises(ValueError):
            self.run_package(helper_hashes={})
        (self.helpers / HELPERS[0]).write_bytes(b'wrong helper')
        with self.assertRaises(ValueError):
            self.run_package()
        self.assertFalse((self.root / 'output').exists())

    def test_explicit_replacement_preserves_non_presentation_bytes(self):
        self.run_package()
        baseline = self.root / 'output/runtime.zip'
        replacement = bytearray((self.component / LIBRARY).read_bytes())
        replacement.extend(b'candidate')
        (self.component / LIBRARY).write_bytes(replacement)
        new_hash = sha(replacement)
        (self.component / 'mavk-ahb.json').write_text(json.dumps(dict(self.receipt, library_sha256=new_hash)))
        result = self.run_package(baseline=baseline, baseline_hash=sha(baseline.read_bytes()),
                                  library_hash=new_hash, output=self.root / 'replacement', replace_component=True)
        self.assertEqual(result['added_members'], [])
        self.assertEqual(set(result['changed_members']), {'manifest.json', PREFIX+LIBRARY, PREFIX+'mavk-ahb.json'})
        before, _ = read_runtime(baseline)
        after, _ = read_runtime(self.root / 'replacement/runtime.zip')
        for name in (LAYER, 'mesa25/lib/preserve.so'):
            self.assertEqual(before[name][0], after[name][0])
            self.assertEqual(before[name][1].external_attr, after[name][1].external_attr)

    def test_replacement_requires_complete_existing_component(self):
        with self.assertRaises(ValueError):
            self.run_package(replace_component=True)
