# Experimental frame-loop lifecycle fix — 2026-10-23

`CompositorHost.pauseVsync()` previously only cleared a boolean, leaving the
Choreographer callback queued. A detach/attach before that callback executes
posts a second callback; both then repost indefinitely. The extracted small
FrameLoop cancels the pending callback on stop, ignores a stopped callback and
only reposts while running. Rendering/pace/native backend are unchanged.

Actual compiled Kotlin checks: six JVM cases, including100 rapid stop/start
cycles, repeated start, stopped callback, stop during render and restart. PASS.
The Android Gradle unit task was blocked by dependency TLS `Tag mismatch`, so it
is NOT claimed as passing. The separate JVM check uses the compiled product class,
not a Python model. Source JUnit tests are retained for normal CI.

APK27: `E:/Mavk/artifacts/droiddeck-lifecycle-apk27-20261023/`
`DroidDeck-Mavk-lifecycle-20261023.apk`, SHA256
f5c84d3a5b215e7d349068e0738c253ba2f0a954535caeff51fc47438fe552b6.

DEX-only rollup from exact APK26 baseline,472 non-DEX members unchanged,
four expected FrameLoop/CompositorHost classes added,none removed. v2/v3 signer
matches the installed experimental app;16KiB alignment checked; install-r PASS.
No native library/runtime/manifest/resource changes. APK26/25 remain available
for normal update rollback, no uninstall or data clear required.

## Hardware regression

Actual Tab S11: normal Steam and Muck reach their rendered interfaces. Project
guest input opens Muck Settings, then returns to menu. Images are retained in the
artifact directory. This is an actual input check, unlike the earlier APK26
Android-coordinate tap that did not open Settings.

Muck menu Surface buffer cadence before background19.3/s; after a normal explicit
resume19.7/s. Existing manual-pause policy intentionally displays the paused
overlay; it was resumed normally, not treated as display loss. Three additional
background/foreground/resume cycles have matching suspend/resume lifecycle events
and live output afterwards. After those cycles cadence was11.8/s and thermal
status3/SKIN45.0–45.2C. No causal FPS gain/regression claimed from hot results.
The test's unsupported uppercase wait commands failed; the lifecycle events and
actual successful resume commands, not those waits, prove the transitions.

This prevents a concrete queued-callback multiplication defect. It does NOT
establish the cause of every historic frozen display or prove all suspend races
fixed. Native guards/WSI remain unchanged. A later observation found guest exit0
and session stopped normally; the tablet had since rebooted. No evidence ties
that external-state change to this patch and no live test was silently restarted.

## CPU attribution and limits

Read-only ten-second `/proc` sample of actual Muck menu:10.39 CPU seconds in10.005
seconds (~104% of one core total), main thread12.2%, UnityGfxDeviceW29.9%,
dxvk-cs26.8%,dxvk-queue12.2%. Allowed CPUs0–7; no accidental single-core affinity.
These are CPU-time counters, not stack samples or proof of a particular GPU
bottleneck. Native Android simpleperf denied profiling unless perf_harden changes;
no security property was changed and that route was stopped.

No U-AVF/Windows/Ubuntu/P7/H264 changes. No commit/push. Temporary desktop display
preference removed; per-game diagnostic profile backups inactive before launch.
