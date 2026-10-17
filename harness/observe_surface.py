"""Read-only presentation history for this APK only; no graphics overrides.

Launch before GraphicsActivity. Report timestamps are the SurfaceFlinger
actual-present column, not native vkQueuePresent call counts. Correlate against
the native monotonic interval separately before claiming a sustained rate.
"""
import argparse
import json
import re
import subprocess
import time
from pathlib import Path

p=argparse.ArgumentParser()
p.add_argument('--adb',required=True)
p.add_argument('--output',type=Path,required=True)
p.add_argument('--seconds',type=int,choices=(40,45),default=40)
a=p.parse_args()
assert not a.output.exists()
a.output.mkdir(parents=True)
prefix='SurfaceView[org.mavk.probe/org.mavk.probe.GraphicsActivity]'
deadline=time.monotonic()+20
layer=None
while not layer and time.monotonic()<deadline:
    listing=subprocess.check_output([a.adb,'shell','dumpsys SurfaceFlinger --list'],text=True,timeout=5)
    names=[]
    for line in listing.splitlines():
        if prefix in line and '(BLAST)#' in line:
            match=re.fullmatch(r'RequestedLayerState\{(.+?) parentId=.*\}',line)
            names.append(match[1] if match else line.strip())
    assert len(names)<=1,'Multiple probe Surfaces; ambiguous observation'
    if names:layer=names[0]
    else:time.sleep(.25)
assert layer and not any(c in layer for c in ('"','\\','\n','\r','`','$')),'No unambiguous probe Surface'
(a.output/'layer.txt').write_text(layer+'\n')
seen={}
start=time.monotonic()
with (a.output/'samples.jsonl').open('w') as log:
    while time.monotonic()-start<a.seconds:
        raw=subprocess.check_output([a.adb,'shell',f'dumpsys SurfaceFlinger --latency "{layer}"'],text=True,timeout=5)
        for line in raw.splitlines()[1:]:
            if re.fullmatch(r'\d+\s+\d+\s+\d+',line):
                triple=tuple(map(int,line.split()))
                if all(0<n<2**63-1 for n in triple):seen[triple[1]]=triple
        log.write(json.dumps({'elapsed':time.monotonic()-start,'raw':raw})+'\n');log.flush()
        time.sleep(.25)
frames=sorted(seen.values(),key=lambda v:v[1])
(a.output/'presentation-timestamps.json').write_text(json.dumps({'layer':layer,'columns':['desired_present_ns','actual_present_ns','frame_ready_ns'],'frames':frames},indent=2)+'\n')
print(json.dumps({'observed_present_timestamps':len(frames),'seconds':time.monotonic()-start,'claim':'Correlate with native report; count alone is not FPS'}))
