"""Read-only game thread CPU attribution without perf-event/security changes."""
import argparse
import json
import os
from pathlib import Path
import time

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('pid', type=int)
p.add_argument('--seconds', type=float, default=10)
a = p.parse_args()
assert a.pid > 1 and 1 <= a.seconds <= 30
root = Path('/proc') / str(a.pid)
hz = os.sysconf('SC_CLK_TCK')


def read_stat(path):
    raw = path.read_text()
    end = raw.rindex(')')
    fields = raw[end + 2:].split()
    return dict(name=raw[raw.index('(') + 1:end],
                ticks=int(fields[11]) + int(fields[12]), start=int(fields[19]))


def snapshot():
    result = {}
    for directory in (root / 'task').iterdir():
        try:
            result[directory.name] = read_stat(directory / 'stat')
        except (FileNotFoundError, ProcessLookupError):
            pass
    return result


process_before = read_stat(root / 'stat')
before = snapshot()
start = time.monotonic()
time.sleep(a.seconds)
after = snapshot()
process_after = read_stat(root / 'stat')
elapsed = time.monotonic() - start
assert process_before['start'] == process_after['start'], 'PID was reused'
rows = []
for tid, end in after.items():
    old = before.get(tid)
    if not old or old['start'] != end['start']:
        continue
    seconds = (end['ticks'] - old['ticks']) / hz
    rows.append(dict(tid=int(tid), name=end['name'], cpuSeconds=seconds,
                     percentOneCore=round(100 * seconds / elapsed, 2)))
rows.sort(key=lambda row: row['cpuSeconds'], reverse=True)
total = (process_after['ticks'] - process_before['ticks']) / hz
print(json.dumps(dict(pid=a.pid, process=process_before['name'], elapsed=elapsed,
                     processCpuSeconds=total, percentOneCore=100 * total / elapsed,
                     threads=rows, countsUniqueFrames=False, samplesCallStacks=False), indent=2))
