"""Attribute bounded CPU wait intervals to observed submit or host signal calls.

Not a GPU timer: queued latency includes execution, dependencies and scheduling.
Only single-target successful timeline waits are classified. Unmatched producers
and incomplete edge events remain unknown, never guessed from semaphore handles.
"""
import argparse
from collections import Counter, defaultdict
import json
from pathlib import Path


def distribution(values):
    values = sorted(values)
    if not values:
        return None
    return {k: round(values[min(len(values)-1, int((len(values)-1)*q))]/1e6, 3)
            for k, q in [('minMs', 0), ('p50Ms', .5), ('p95Ms', .95), ('maxMs', 1)]}


def union_ns(intervals):
    total, end = 0, 0
    for a, b in sorted(intervals):
        total += max(0, b-max(a, end))
        end = max(end, b)
    return total


def analyze(events):
    groups = defaultdict(list)
    for event in events:
        groups[event['id']].append(event)
    successful = {i for i, es in groups.items() if any(e['kind']=='E' and e['a']==0 for e in es)}
    signals = [e for e in events if e['kind']=='G' and e['id'] in successful and e['b']>0]
    host_signals = [e for e in events if e['kind']=='H' and e['b']>0 and
                    any(x['kind']=='J' and x['a']==0 for x in groups[e['id']])]
    kinds, waits, post_submit, pre_submit = Counter(), [], [], []
    blocked_by_thread, queued_by_thread = defaultdict(list), defaultdict(list)
    examples = []
    for i, es in groups.items():
        begin = next((e for e in es if e['kind']=='B'), None)
        end = next((e for e in es if e['kind']=='R'), None)
        if not begin or not end:
            continue
        targets = [e for e in es if e['kind']=='T']
        waits.append(end['b'])
        # These APIs accept timeline semaphores only. Value zero is already
        # satisfied from creation; in WAIT_ALL it adds no dependency. WAIT_ANY
        # with a zero target cannot identify a nonzero producer.
        if begin['a'] & 1 and any(e['b']==0 for e in targets):
            kinds['wait_any_includes_initial_zero'] += 1
            continue
        targets = [e for e in targets if e['b']>0]
        if end['a'] != 0 or len(targets) != 1:
            kinds['timeout_or_multiple_targets'] += 1
            continue
        target = targets[0]
        if target['b']==0:
            kinds['initial_zero_value'] += 1
            continue
        candidates = [e for e in signals+host_signals if e['a']==target['a'] and e['b']>=target['b'] and e['time']<=end['time']]
        if not candidates:
            kinds['producer_outside_capture_or_unknown'] += 1
            continue
        source = min(candidates, key=lambda e: (e['b'], e['time']))
        duration = end['b']
        if source['kind']=='H':
            kinds['host_signal'] += 1
            continue
        producer = groups[source['id']]
        submitted = next(e for e in producer if e['kind']=='S')
        accepted = next(e for e in producer if e['kind']=='E')
        if accepted['time']>end['time']:
            kinds['submit_not_returned_before_wait'] += 1
            continue
        kinds['gpu_queue_signal'] += 1
        wait_start = end['time']-duration
        queued = max(0, end['time']-max(wait_start, accepted['time']))
        before = max(0, min(end['time'], submitted['c'])-wait_start)
        post_submit.append(queued)
        pre_submit.append(before)
        if duration >= 1000000:
            blocked_by_thread[begin['thread']].append((wait_start, end['time']))
            queued_by_thread[begin['thread']].append((max(wait_start, accepted['time']), end['time']))
            deps = [e for e in producer if e['kind']=='W']
            commands = [e for e in producer if e['kind']=='C']
            examples.append(dict(waitMs=round(duration/1e6, 3), beforeSubmitEntryMs=round(before/1e6, 3),
                                 afterSubmitReturnedMs=round(queued/1e6, 3), commandBuffers=submitted['b'],
                                 recordedPipelineBinds=sum(e['b'] for e in commands), retainedDescriptorGenerations=sum(e['c'] for e in commands),
                                 producerWaits=len(deps), zeroValueDependencies=sum(e['b']==0 for e in deps),
                                 queue=submitted['a'], targetValue=target['b']))
    submissions = [e for e in events if e['kind']=='S' and e['id'] in successful]
    span = max(e['time'] for e in events)-min(e['time'] for e in events) if events else 0
    return dict(eventCount=len(events), captureSpanMs=round(span/1e6, 3), completeWaits=len(waits), classification=dict(kinds),
                waitTime=distribution(waits), alreadySubmittedWait=distribution(post_submit), beforeSubmitEntry=distribution(pre_submit),
                submitEntryToDriver=distribution([e['time']-e['c'] for e in submissions]),
                driverSubmit=distribution([next(x['time'] for x in groups[e['id']] if x['kind']=='E')-e['time'] for e in submissions]),
                commandBuffersPerSubmit=dict(Counter(e['b'] for e in submissions)),
                queueCount=len({e['a'] for e in submissions}),
                blockingThreads=[dict(thread=t, blockedMs=round(union_ns(v)/1e6, 3),
                                      afterSubmitReturnedMs=round(union_ns(queued_by_thread[t])/1e6, 3))
                                 for t, v in blocked_by_thread.items()],
                longestWaits=sorted(examples, key=lambda x: -x['waitMs'])[:12])


def parse(text):
    captures, current = [], None
    for line in text.splitlines():
        if line.startswith('MAVK_SYNC_BEGIN '):
            fields = dict(v.split('=', 1) for v in line.split()[1:])
            current = dict(tag=fields['tag'], truncated=int(fields['truncated']), events=[])
        elif line.startswith('MAVK_SYNC ') and current is not None:
            fields = line.split()
            assert len(fields)==9, line
            e = dict(zip(('time', 'id', 'a', 'b', 'c', 'd', 'thread'), map(int, fields[2:])))
            e['kind'] = fields[1]
            current['events'].append(e)
        elif line.startswith('MAVK_SYNC_END ') and current is not None:
            captures.append(dict(tag=current['tag'], truncated=current['truncated'], **analyze(current['events'])))
            current = None
    return captures


if __name__=='__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log', type=Path)
    args = parser.parse_args()
    print(json.dumps(parse(args.log.read_text(errors='replace')), indent=2))
