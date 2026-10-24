"""Read-only process/thread CPU attribution; not GPU time or frame counting."""
import argparse
import json
import os
from pathlib import Path
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--pid", type=int, required=True)
parser.add_argument("--seconds", type=float, default=20)
parser.add_argument("--output", type=Path, required=True)
args = parser.parse_args()
assert 1 <= args.seconds <= 60
assert args.output.parent == Path('/data/user/0/com.droiddeck.launcher.mavk/files/mavk-perf')
assert not args.output.exists(), 'Preserve previous measurements'
hz = os.sysconf('SC_CLK_TCK')


def stat(path):
    raw = path.read_text()
    left, right = raw.index('('), raw.rindex(')')
    fields = raw[right + 2:].split()
    return dict(name=raw[left + 1:right], state=fields[0],
                ticks=int(fields[11]) + int(fields[12]), start=fields[19],
                lastCpu=int(fields[36]))


def threads():
    result = {}
    for folder in Path(f'/proc/{args.pid}/task').iterdir():
        try:
            row = stat(folder / 'stat')
            try:
                run, wait, switches = map(int, (folder / 'schedstat').read_text().split())
                row.update(runNs=run, runqueueNs=wait, timeslices=switches)
            except (OSError, ValueError):
                pass
            try:
                row['wchan'] = (folder / 'wchan').read_text().strip()
            except OSError:
                pass
            result[int(folder.name)] = row
        except (OSError, ValueError, IndexError):
            pass
    return result


def processes():
    result = {}
    for folder in Path('/proc').iterdir():
        if not folder.name.isdecimal():
            continue
        try:
            row = stat(folder / 'stat')
            name = row['name'].lower()
            if int(folder.name) == args.pid or any(key in name for key in ('gamescope', 'xwayland', 'wineserver', 'steamwebhelper')):
                result[int(folder.name)] = row
        except (OSError, ValueError, IndexError):
            pass
    return result


def delta(before, after, elapsed):
    rows = []
    for pid, current in after.items():
        previous = before.get(pid)
        if not previous or previous['start'] != current['start']:
            continue
        row = dict(id=pid, name=current['name'],
                   cpuPercent=round(100 * (current['ticks'] - previous['ticks']) / hz / elapsed, 2),
                   stateAtEnd=current['state'], lastCpu=current['lastCpu'])
        for key in ('runNs', 'runqueueNs', 'timeslices'):
            if key in current and key in previous:
                row[key] = current[key] - previous[key]
        if 'wchan' in current:
            row['wchanAtEnd'] = current['wchan']
        rows.append(row)
    return sorted(rows, key=lambda item: item['cpuPercent'], reverse=True)


start = time.monotonic()
before_processes, before_threads = processes(), threads()
time.sleep(args.seconds)
after_threads, after_processes = threads(), processes()
elapsed = time.monotonic() - start
assert args.pid in after_processes and args.pid in before_processes, 'Target exited'
report = dict(pid=args.pid, seconds=round(elapsed, 3), hz=hz,
              oneBusyCorePercent=100, cpuCount=os.cpu_count(),
              processes=delta(before_processes, after_processes, elapsed),
              threads=delta(before_threads, after_threads, elapsed),
              limitation='CPU time of surviving tasks; schedstat/wchan may be unavailable. No GPU or unique-frame measurement.')
args.output.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps({**report, 'threads': report['threads'][:12]}, indent=2))
