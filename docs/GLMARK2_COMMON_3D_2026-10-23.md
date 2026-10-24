# Common real 3D workload on G925 — 2026-10-23

## Scope

Official glmark2 2023.01 source, commit
22c527cb0556f3a1ac4445aaa52cc532760928d5. Three actual upstream scenes:
jellyfish, terrain, shadow, ten seconds each, 1920x1080. Not the full standard
benchmark; do not publish its partial score as an official glmark2 result.

Android control: GLES directly to the stock Mali driver. Container: ARM64 GLES
through Mesa Zink 25.1.9, guarded Mavk and libhybris to the same hardware driver.
There is no FEX/Wine/DXVK in this particular comparison. Results therefore do not
explain all Muck costs and are not a pure Mavk Core overhead measurement.

## Visible output

Fresh DroidDeck desktop session, actual output/fullscreen 1920x1080, three scenes
complete at approximately 60/59/60 benchmark FPS. Independent Surface pacing:
713 updates/12.1 seconds = 58.8 updates/s. Terrain screenshot visibly correct.
Surface updates are not a unique-image identity oracle. The native direct GLES
visible benchmark exceeds display refresh, so its FPS is not actual visible FPS.

An earlier resumed desktop was frozen despite benchmark progress; exclude those
109–115 numbers. A subsequent normal background/resume control worked, so there
is no demonstrated root cause for that earlier freeze.

## Completion-rate comparison without the 60 Hz presentation cap

Both modes render one offscreen FBO: RGBA8, D24, stencil0, no MSAA; explicit
glFinish after every frame. This measures completed-frame cadence, not visible
FPS, maximal asynchronous GPU throughput or game performance.

Android's upstream canvas did not implement offscreen/frame-end options. The
bounded comparison harness implements the above FBO, completeness checks and
finish, without changing scene/shader code. Java GLSurfaceView still swaps its
otherwise blank window after each callback, an unmatched control overhead.

| Scene | Native GLES FPS / ms | Zink + Mavk FPS / ms |
|---|---|---|
| jellyfish | 366 / 2.732 | 392 / 2.555 |
| terrain | 130 / 7.692 | 96 / 10.507 |
| shadow | 401 / 2.494 | 349 / 2.865 |

These sequential runs are not thermal-normalized. A subsequent cold terrain
control reached113 FPS/8.919ms, followed by95 FPS read-only metadata and95 FPS
reverse control. This does not establish a read-only metadata gain/regression.
Native logging floors FPS; Linux rounds up from measured average frame time.
Do not derive a single percentage of native GPU power from this table.

Additional Linux run with post-timing pixel samples: 388/96/340 FPS,
2.579/10.494/2.949ms. All27 sampled pixels were read successfully; shadow samples
include the same ground/shadow/horse values as native. Animated scenes are not
bitwise compared because their sampled final times differ. This is an output
sanity check, not full image conformance. Layer guards on, gaps0, CoreLiveZero.

The existing optional GPU batch profiler was enabled for one terrain control,
but yielded no eligible no-wait-semaphore batches. No GPU timestamp conclusion
is drawn from that run. CPU statistics under glFinish include driver wait/spin
costs; they do not establish shader-only GPU time or the game bottleneck.

## Reproducibility

Artifacts: `E:/Mavk/artifacts/glmark2-20261023/`:
native-offscreen.log, linux-offscreen-three-scenes.log, offscreen-pixels-cpu.log,
terrain-profile-control.log, terrain-readonly.log, terrain-reverse-control.log.

Source and original GPL license: `E:/Mavk/research/glmark2/`. Isolated Android
test package org.linaro.glmark2 (target36, no storage permission), not a production
DroidDeck/U-AVF replacement. New test APK SHA256
2c4db0ffa1c5db81b03e0655ba422490286b41c66721669f25336ccfcb86d082.

The Linux runner requires an explicit layer root in desktop mode; normal Steam
uses its installed common runtime. No robustness guards disabled, no fabricated
features, no layer gaps. Temporary desktop full-panel preference must be restored
before handoff. Benchmark executables are independent of Steam/game data.

## Consequence

Guarded hardware rendering can complete hundreds of frames/s in two real scenes;
the old approximately60 visible benchmark numbers were presentation-limited.
This rules out treating those numbers as the total hardware throughput ceiling.
It does NOT prove that FEX, Wine, DXVK, Mavk instrumentation or SHM presentation
is the main remaining game bottleneck. Prefer actual game CPU/GPU/frame pacing
attribution next, not more rare Vulkan features or blind shader-cache tuning.
