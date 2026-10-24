# Optional event tracing: CPU command-recording overhead

Device policy MAVK_LAYER_TRACE=1 enables per-execute/update/template/secondary
events. Default/unset/0 skips only these event prints. Access guards, resource
retention, counters, errors, diagnosticReports and profiling retain their own
policies. Error messages and teardown summaries are never hidden by this flag.
The flag is read once at device creation, not at each command. Existing legacy
event-count controls explicitly request trace=1 to retain those test assertions.

Layer60e0a2ebe00b874a46d83e6767e5781ff846a122e256b06f7cc04fd9938e6831.
No compiler, optimizer, descriptor or shader-semantic changes from APK24.

## Controlled A/B

Existing nested-buffer consumer reused:5batches8192vkCmdDispatch recordings,
not submitted, followed by ONE submitted dispatch with GPU zero-read/suppressed
write/sentinel validation.3runs each mode, order alternated. Discard sample0
in each process;12samples per mode retained. Same candidate/libraries/device,
stderr redirected to app-private regular files. Pipeline compilation/setup/
cleanup excluded from timed recording loop. This is wall time, not CPU cycles.

| Trace | Median wall ns per recorded command | Event logs per process |
|---|---:|---:|
|0|3290.5|0|
|1|6587.3|40961|

Ratio2.002 on/off demonstrates event-print overhead for this recording workload.
It does NOT imply twice the game FPS, GPU speed or total engine throughput.
No thermal/core-pinning controls used; do not treat this as a hardware ceiling.
All6processes:guards active,40961commands,1submission,diagnosticReads0,gaps0,
CoreliveZero1,correct actual GPU result. Separate missing-used-set test with
trace0 emits the actual GAP and rejects before submission; errors remain live.
Verifier:compatibility/test/verify-trace-overhead.py.

All26 priorGPU scenarios PASS49.4sec, strict --no-diagnostic-reads and
--no-event-trace gates. This elapsed suite time is not a performance delta.
Normal candidate Muck18412 mapped the candidate,menu/Settings/input visually
PASS; no hot-event/GAP/FAIL/device-loss markers in saved log,333470bytes.
Different smoke duration means log sizes cannot be compared as a rate or FPS.
Game quit normally; per-game override removed before APK rollout.

Artifacts:E:/Mavk/artifacts/trace-policy-gpu-20261022/.
Prepared APK25 SHA044f33a9a7ddac9cd1b9491778ffb44b99ac90f6ce1c4ad1ad57a76f13a2d7d6,
runtime4de2684454d0ba9f1f80ac37e9fc9d37d2dd04be444897622ae8518fc4bb13c0.
Only layer+manifest changed in runtime,261members unchanged;485APK members
unchanged. Same dev certificate,v2/v3 signatures and16KB alignment verified.
Actual installed/common gate still to be recorded separately. APK24 rollback.
No CTS/full-feature/visible-FPS claim, commit/push or production U-AVF changes.

## Installed integration

APK25 install-r SUCCESS,Steam2026-10-22-05-steam READY/firstFrame. All26 actual
COMMON cases PASS57.043sec with exact4de26844 runtime-path, no diagnostic reads
and no hot event trace verified. Source runtime/version synchronized. Default
compiler/archive pins unchanged. Normal Muck20923 maps installed4de26844 payload,
menu/Settings/input visually PASS with trace0, no observed gap/compile/device-loss
markers or hot-event messages. Quit normally;Steam left open,no per-game override.

Archives SHA:
isolated regressions443ff8a7f3629e2d68d4da91a329873a7a2f7aa5123ac272c090b38ae57ea125;
record A/B+negative d5751a806a15520b3156a9e7ea75d6f477bd49486fbf70e1769fcf2ebc785c07;
candidate Muckb690aebb03c54640b7c37fb5f399a819fceaa9ce9338b4b2c9937167ff8962bd;
COMMON37c127457f2a74da02ec26b580c21c6fba8e04e39b39be843bc03929256c6224;
COMMON Muckc3b11c067a50cbc7eb37309d921af21f006cc7e8cacb61bee39e6b96296925c0.

This bounded overhead task is finished. Do not add more logging permutations;
return to consumer-relevant shader/render-state compatibility. Goal incomplete.
