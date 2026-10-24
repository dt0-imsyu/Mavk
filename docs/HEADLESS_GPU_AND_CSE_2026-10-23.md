# App-owned headless GPU execution / guarded CSE — 2026-10-23

## New verified execution path

The existing glibc Mavk layer and actual GPU consumers run directly under the
experimental DroidDeck app UID through its installed glibc interpreter. This
does not require Steam, Xwayland, a running desktop session, Surface or WinVM.
Android keyguard remained showing/input-restricted during the checks. No login,
keyguard bypass, new authorization mechanism, root, property/security change or
new mount/SELinux configuration was used.

The existing app-private runtime supplies libhybris and the normal stock
`/vendor/lib64/hw/vulkan.mali.so`; physical device is Mali-G925-Immortalis MC12.
This is not a replacement ICD or standalone independent GPU driver.

`run-headless-probe.ps1` provides the exact app-owned library/ICD/layer environment
and glibc loader invocation. `prepare-headless-regressions.py` reuses the existing
`nested-buffer-control.sh`: six executable-dispatch sites and68 report references
are mechanically adapted. GPU consumers, shader fixtures and verification checks
are unchanged. Fresh app-private result directories prevent stale-output PASS.
The default full-suite adapter points to the existing aggregate fixtures.

## Hardware evidence

- Direct nested-buffer GPU execution: OOB read zero, OOB write suppressed,
  all31 output sentinels and32 input words preserved, descriptor4B/allocation128B.
- Full26 existing GPU cases pass with guards on, events/diagnostic reads off,
  actual candidate path verified, gaps0 and CoreLiveZero. Existing strict verifier
  checks aggregate/copies, wide and overflowing offsets, dynamic/lifetime/layout,
  samplers, texel formats/components/atomics and image subresources.
- Source-derived compiler/static-use checks24/rejections9 run on actual ARM64.
- These are existing own GPU regressions, not CTS or a full feature-bit claim.

Candidate SHA256:
4b16fa3f680f4c5090e48503a1b4d11038e388c9167c019287976784ab22a109.
Private directory: `files/mavk-guard-cse/payload`. Installed common layer remains
f1081e2c783a4ea8256981f669cba97233383fb24470b0a3ea6a661a160d288e;
system WSI remains d07b961181b9547fd48ad9f05cc6d3b808b824676eb5a186fe3feb8caa013f19.

## Bounded optimizer experiment

Optional `MAVK_GUARD_CSE=1`: private immutable metadata marked NonWritable,
standard SPIRV-Tools inlining/redundancy elimination and cleanup after existing
instrumentation. Guards are not disabled; native capabilities are not changed.
Default OFF, not enabled in APK27. No compile-time host/core special case.

Packed UBO structure check:436 ->418 words, no remaining function calls, output
validation and cache reuse pass. Full26 GPU cases also pass with this option on.
The default/off mode passes the same fresh headless suite through the new launcher.

## Real 3D scene without a display server

Upstream glmark2 already implements `NativeStateNull`/surfaceless EGL. The existing
comparison build now optionally selects it; no new scene/renderer was written.
Actual Mesa25.1.9 Zink -> guarded Mavk -> Mali renders terrain into one1920x1080
RGBA8/D24 FBO with explicit per-frame glFinish and post-timing pixel samples.
No WSI/SF presentation; these are completed offscreen frames, NOT visible FPS.
Background app power policy and DVFS differ from a foreground game/native control;
do not compare these numbers directly to prior visible or native Android results.

| Same binary / terrain5s | FPS | ms/frame |
|---|---:|---:|
| control0 | 80 | 12.553 |
| CSE1 | 86 | 11.703 |
| reverse control0 | 84 | 11.938 |
| repeat CSE1 | 81 | 12.416 |
| repeat control0 | 84 | 11.934 |

Thermal status0/SKIN37.9C in this bounded comparison. Results do NOT establish a
reproducible speed gain: the second optimized run is slower than both warm
controls. NO-GO for changing the default. Do not spend further time polishing
this pass sequence absent a new concrete shader bottleneck.

glmark2-es2-headless SHA256:
710bb22c0e21cdd5f58e0fdfb39cdd2a956bde45524120460e1009b906911007.
Original GPL source/license and comparison harness remain available; no public
distribution was made. Partial scene scores are not official glmark2 scores.

## Artifacts / example

`E:/Mavk/artifacts/guard-cse-{frontend,layer}-20261023/`, including full GPU logs,
strict-verifier input archive, actual matching compiler/layer and242KB source
capsule. Independent headless glmark2 build is in `glmark2-20261023/`.

```powershell
& ./Mavk/experiments/droiddeck/run-headless-probe.ps1 `
  -Program /data/user/0/com.droiddeck.launcher.mavk/files/mavk-glmark2/glmark2-es2-headless `
  -Mesa -ProbeArguments @('--size','1920x1080','--off-screen=1',
  '--frame-end','finish','--benchmark','terrain:duration=5')
```

The complete guarded suite uses `-ShellSuite` with a freshly generated/staged
control script. Use a new `--result-name mavk-...` per run; do not reuse results.
Default launcher layer is this private candidate; it does not replace installed
runtime files. `-GuardCse` opts in only for a selected probe.

## Handoff / storage

Tablet desktop launch while locked reached SUSPENDED before first frame; this
was not treated as a GPU failure. That test session was stopped normally. Final
app session IDLE, no benchmark process intentionally left running. Installed
APK27 independently rehashed to f5c84d3a5b215e7d349068e0738c253ba2f0a954535caeff51fc47438fe552b6.

Disk C~50.4GiB/E~831.6GiB free. Tablet was33.4GiB free; normal Android cache trim
reclaimed~1.38GiB, leaving~34.8GiB, NOT50GiB. Apps, games, VM disks, user files and
production data were not removed. More space requires another safe identified
target; do not pretend the requested50GiB reserve was reached.

No production U-AVF/Windows/Ubuntu/P7/firmware/H264 changes, no commit/push.
Strategic goal is not complete. Practical next performance direction remains
end-to-end game render/present/readback attribution under comparable thermal and
power conditions, then a proven bottleneck fix—not another rare Vulkan feature.
