# Muck performance checkpoint — 2026-10-22

## Scope

TES->GS is closed by CULL_TESSELLATION_2026-10-22.md. New Vulkan feature work
is paused, not the strategic goal. One sequential agent, no parallel optimization.
No performance fix or 100 FPS achievement is claimed by this checkpoint.

## Actual execution and baseline

Tab S11 R52Y9072A2R, installed APK25/runtime4de26844 unchanged. Existing Steam06
session, actual ARM64 Wine/DXVK-Sarek plus FEX mappings, not Box64. Muck's normal
swapchain is 2560x1600, IMMEDIATE, four images; panel120Hz. Game FPS HUD says0
and is not a reliable measurement. Rendering is hardware Mali, not llvmpipe.

All observations below are the animated main menu, not fixed gameplay. Android
SurfaceFlinger counts new buffers, not proven unique content. Thermal state varied
2->3, SKIN up to46.5C. Do not promote these runs to a controlled optimization A/B.

| Observation | Android updates/s | Interval | Extra evidence |
| --- | ---: | ---: | --- |
| Initial common path | 11.3 | 20.1s | 228 buffers |
| Matched common CPU sample | 9.6 | 20.1s | 193 buffers; Muck71.93% of one core |
| Common layer, profiling enabled | 10.0 | 20.1s | 202 buffers; Muck69.08% of one core |
| Resolution UI applied1920x1280 | 11.7 | 20.1s | 235 buffers; screenshot/window changed, no retained independent lower-backbuffer log |
| New attribution candidate after restart | 16.5 | 20.1s | 333 buffers, transient; later steady compositor9.4–10.3 |

The last row is NOT a speedup claim. Thermal/warmup and timing are confounders.
Resolution restored to2560x1600; new launch log confirms actual buffer size.

## Attribution

- Common path Muck thread CPU: Unity graphics worker21%, dxvk-cs15–17%,
  dxvk-queue10%, main8%; whole game69–72% of ONE core. Runnable-queue delay is
  small versus elapsed time. This rules out a continuously saturated CPU thread
  in this sample, not all FEX/Wine/CPU serialization effects.
- Guard recording ~59–79ms wall per ~1s at the slow steady cadence. The previous
  label guardCpuMs was misleading: this is elapsed wall time, not a CPU clock.
- Slow candidate steady samples: fence waits ~739–872ms aggregate per interval,
  ~9–10 calls; timeline waits~889–991ms, ~36–40 calls; native submit~5–7ms,
  ~54–60 calls. Earlier faster phase fence waits~589–629ms at~20 calls/s.
  Waits can overlap across threads and must NOT be added into a frame budget.
- Native Android compositor steady menu: render_scene~2.1ms, acquire~0.14ms,
  native present~0.6ms, fence~1.1–1.25ms; no pool drops. Rendering/present may
  be included within render_scene, so these are not additive either.
- Crucial actual path: DROIDDECK_SYSTEM_VULKAN_PRESENTATION=x11-shm and
  WSI_X11_FORCE_SHM=1; WSI layer is above Mavk. Compositor records copy scenes,
  zero GPU-buffer frames, SHM window redraws. This is hardware game rendering
  with CPU-visible presentation/readback, NOT software game rendering.
- Local pinned WSI source shows SHM linear host-visible/coherent swapchain
  allocation, CPU wait before presentation and pixel copies. This is a strong
  candidate for the serialized graphics/readback cost, not yet proof that the
  memory layout/copy rather than GPU execution is the root cause.

## Implemented diagnostic correction

Mavk's old record profiler reset profileSince before present could emit, hiding
accumulated fence measurements when upper WSI consumed present. One shared
reportProfileLocked now flushes guard/fence/timeline/submit/present attribution
from any observed operation. Native waits stay outside the Mavk mutex. Added
core/KHR timeline wait hooks and timing of existing native submit calls. No new
GPU waits, query submissions, capabilities, guard bypass or feature advertising.
Profiling remains opt-in; zero Present calls here only means upper WSI consumed it.

Candidate39b0a23c00d4145958513e7f0d1d4b014903d21ae30aff84fc5c8393157cfafb
built at E:/Mavk/artifacts/muck-profile-waits-20261022. Existing12-case GPU
regression control passed11.15s (nested/dynamic/replay/inflight/empty/unused/
selected/immutable/grouped/routing/texel/images; Core liveZero). Real Muck loaded
candidate and emitted timeline/fence/submit measurements. No CTS claim.

Simpleperf under app UID cannot disable perf_harden. GPU devfreq files are
permission-denied even to adb shell. No security properties or permissions changed.
PanVK comparison has NOT been run; no substituted GPU/driver numbers.

## Artifacts and restoration

E:/Mavk/artifacts/muck-performance-20261022/performance-evidence.tar.gz contains
game/DXVK/compositor logs and both CPU JSON reports. Screenshots are alongside it.
New small scripts: performance-sample.py (read-only /proc) and
performance-profile.py (Muck-only profile toggle with guarded restoration).

Muck exited normally; profile restored0 in both config copies; candidate override
removed. Original aggregate diagnostic layer89c8c463 restored by exact hash.
Common installed layer/APK/runtime never replaced. Steam remains running.
No commit/push, U-AVF/Ubuntu/Windows/storage/CPU/firmware changes.

## Exact next step

Keep features paused. Isolate actual WSI wait-for-render vs map/copy vs X11 delivery
on the SAME scene with thermal records, then choose one safe presentation/readback
change. A linear-render-target vs optimal-target-plus-readback-copy comparison is
a possible scoped test only after confirming the installed WSI allocation/cost;
do not blindly switch DRM/DRI3/native modes on Mali without an available buffer path.
Do not optimize encoder, disable guards or call this a proven FEX/GPU bottleneck.
After an actual fix, rerun same-scene timings and fixed gameplay. Heavier game30+
and PanVK equal-workload comparison remain later goals, not achieved milestones.
