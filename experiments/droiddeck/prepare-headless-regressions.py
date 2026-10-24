"""Adapt the existing GPU regression script to app-owned direct glibc loading.

Only executable dispatch and output paths change; fixtures/checks stay intact.
The generated script uses Android toybox sh/grep/tar, not a guest desktop.
"""
import argparse
from pathlib import Path
import re

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('output', type=Path)
p.add_argument('--result-name', default='mavk-guard-cse-gpu')
a = p.parse_args()
assert not a.output.exists(), 'Preserve existing generated script'
assert re.fullmatch(r'mavk-[a-z0-9-]{1,64}', a.result_name), 'App-private result name required'
source = Path(__file__).with_name('nested-buffer-control.sh').read_text()
source, calls = re.subn(r'(?m)^(\s*)"\$p/([^"/]*consumer)"',
                        r'\1load_probe "$p/\2"', source)
assert calls == 6, 'Review changed regression executable sites: ' + str(calls)
source, logs = re.subn(r'"\$p/([^"\n]+\.(?:stdout|stderr))"', r'"$r/\1"', source)
assert logs >= 40, 'Regression report paths changed'
source = source.replace('(cd "$p" && tar', '(cd "$r" && tar')
prefix = '''r=/data/user/0/com.droiddeck.launcher.mavk/files/mavk-guard-cse-gpu
test ! -e "$r" || { echo 'Fresh GPU output directory required'; exit 20; }
mkdir "$r"
test -n "${MAVK_ELF_LOADER:-}" && test -n "${MAVK_ELF_LIBRARIES:-}"
load_probe() { "$MAVK_ELF_LOADER" --library-path "$MAVK_ELF_LIBRARIES" "$@"; }
'''
prefix = prefix.replace('/files/mavk-guard-cse-gpu', '/files/' + a.result_name)
source = source.replace('set -eu\n', 'set -eu\n' + prefix, 1)
a.output.write_text(source, encoding='utf-8', newline='\n')
print(f'HEADLESS_REGRESSION_SCRIPT executable_sites={calls} report_paths={logs} original_checks=preserved')
