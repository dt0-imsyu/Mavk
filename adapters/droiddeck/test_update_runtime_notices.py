import json
from pathlib import Path
import tempfile
import unittest
import zipfile

from update_runtime_notices import update
from update_session_runtime import LAYER, read_runtime, sha


class NoticeRollup(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.notices = self.root/'notices'
        (self.notices/'licenses').mkdir(parents=True)
        (self.notices/'licenses/COPYING').write_text('upstream notice')
        self.baseline = self.root/'baseline.zip'
        files = {LAYER: b'unchanged executable'}
        manifest = {LAYER: dict(bytes=len(files[LAYER]), sha256=sha(files[LAYER]), executable=True)}
        files['manifest.json'] = json.dumps(manifest).encode()
        with zipfile.ZipFile(self.baseline, 'w') as archive:
            for name, data in files.items():
                item = zipfile.ZipInfo(name)
                item.external_attr = 0o100755 << 16
                archive.writestr(item, data)

    def run_update(self, **changes):
        args = dict(baseline=self.baseline, notices=self.notices, output=self.root/'out',
                    baseline_hash=sha(self.baseline.read_bytes()))
        args.update(changes)
        return update(**args)

    def test_only_nonexecutable_notice_and_manifest_change(self):
        result = self.run_update()
        self.assertEqual(result['changed_members'], ['manifest.json'])
        self.assertEqual(result['added_members'], ['licenses/COPYING'])
        before, _ = read_runtime(self.baseline)
        after, manifest = read_runtime(self.root/'out/runtime.zip')
        self.assertEqual(before[LAYER][0], after[LAYER][0])
        self.assertEqual(before[LAYER][1].external_attr, after[LAYER][1].external_attr)
        self.assertFalse(manifest['licenses/COPYING']['executable'])
        self.assertFalse(result['corresponding_source_complete'])
        with self.assertRaises(ValueError):
            self.run_update()

    def test_wrong_pin_or_executable_path_rejected_before_output(self):
        with self.assertRaises(ValueError):
            self.run_update(baseline_hash='0'*64)
        (self.notices/'licenses/payload.so').write_bytes(b'not a notice')
        with self.assertRaises(ValueError):
            self.run_update()
        self.assertFalse((self.root/'out').exists())

    def test_nonlicense_root_and_binary_content_rejected(self):
        (self.notices/'outside.txt').write_text('wrong root')
        with self.assertRaises(ValueError):
            self.run_update()
        (self.notices/'outside.txt').unlink()
        (self.notices/'licenses/COPYING').write_bytes(b'binary\0data')
        with self.assertRaises(ValueError):
            self.run_update()
