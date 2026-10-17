"""Cold read-only extraction of this probe's own three finite guest logs."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--adb', required=True)
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--attempt', type=int, choices=range(1, 17), default=1)
parser.add_argument('--api', action='store_true', help='Only this standalone API test own stdout/stderr/exit')
parser.add_argument('--api-attempt', type=int, choices=range(1, 5), default=1)
parser.add_argument('--headless', action='store_true', help='Only pinned external test own logs and bounded PPM')
parser.add_argument('--headless-attempt', type=int, choices=range(1, 5), default=1)
parser.add_argument('--window', action='store_true', help='Only standard window consumer own finite logs')
parser.add_argument('--window-attempt', type=int, choices=range(1, 5), default=1)
parser.add_argument('--uniform', action='store_true', help='Only standard UBO consumer own finite logs')
parser.add_argument('--triangle', action='store_true', help='Only pinned triangle own finite logs and benchmark CSV')
parser.add_argument('--triangle-attempt', type=int, choices=range(1,5), default=1)
parser.add_argument('--triangle-fifo', action='store_true', help='Additional bounded FIFO run own exit2.txt and fifo.csv')
parser.add_argument('--compute', action='store_true', help='Only standard Vulkan compute own finite logs')
parser.add_argument('--sampled', action='store_true', help='Only standard Vulkan sampled consumer own finite logs')
parser.add_argument('--texture', action='store_true', help='Only pinned texture example own finite logs and CSV')
args = parser.parse_args()
assert sum((args.api, args.headless, args.window, args.uniform, args.triangle, args.compute, args.sampled, args.texture)) <= 1, 'Choose one finite log namespace'
assert not args.triangle_fifo or args.triangle
assert not args.output.exists(), 'Preserve prior evidence'


def cold():
    text = subprocess.check_output([args.adb, 'shell', '/apex/com.android.virt/bin/vm', 'list'], text=True)
    assert text.strip() == 'Running VMs: []', 'Stop guest before coherent log extraction'


cold()
workspace = Path(__file__).resolve().parents[3]
source = workspace / 'Non-root-Windows-VM/tools/windows-reassessment/extract-installed-setup-evidence.py'
spec = importlib.util.spec_from_file_location('mavk_cold_read_only', source)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
root = module.ColdRoot(args.adb)
boot = root.read(512)
assert boot[3:11] == b'NTFS    ' and boot[72:80].hex() == 'b769f63e81f63ec6'
root.seek(0)
fs = module.NTFS(root)
args.output.mkdir(parents=True)
report = {'read_only': True, 'files': []}
names = ('stdout.txt', 'stderr.txt', 'exit.txt', 'headless.ppm') if args.headless else (('stdout.txt', 'stderr.txt', 'exit.txt', 'benchmark.csv') if args.triangle else (('stdout.txt', 'stderr.txt', 'exit.txt') if args.api or args.window or args.uniform or args.compute or args.sampled else ('stdout.txt', 'exit.txt', 'error.txt')))
if args.triangle_fifo:
    names += ('exit2.txt','fifo.csv')
if args.texture:
    names = ('stdout.txt','stderr.txt','exit2.txt','benchmark.csv','stderr2.txt','exit3.txt','fifo.csv')
for name in names:
    suffix = '' if args.attempt == 1 else str(args.attempt)
    directory = ('MavkWindow' + str(args.window_attempt)) if args.window else ('MavkHeadless' + str(args.headless_attempt) if args.headless else ('MavkIcdApi' + str(args.api_attempt) if args.api else 'MavkComputeProbe' + suffix))
    if args.uniform:
        directory = 'MavkUniform1'
    if args.triangle:
        directory = 'MavkTriangle'+str(args.triangle_attempt)
    if args.compute:
        directory = 'MavkCompute1'
    if args.sampled:
        directory = 'MavkSampled1'
    if args.texture:
        directory = 'MavkTexture1'
    relative = 'Users/uavf/AppData/Local/Temp/' + directory + '/' + name
    try:
        stream = fs.mft.get(relative.replace('/', '\\')).open()
        assert stream.size <= (4*1024*1024 if args.headless and name == 'headless.ppm' else 65536)
        data = stream.read(stream.size)
        assert len(data) == stream.size
        with (args.output / name).open('xb') as target:
            target.write(data)
        report['files'].append({'file': name, 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()})
    except FileNotFoundError:
        report['files'].append({'file': name, 'missing': True})
cold()
report.update(reads=root.read_count, device_bytes_read=root.read_bytes)
with (args.output / 'extraction.json').open('x') as target:
    json.dump(report, target, indent=2)
print(json.dumps(report))
