# Quick D3D11 benchmark — 2026-10-22

User requested a short benchmark instead of more game-specific investigation;
reported Muck/Crab Game also running at 10–20 FPS on Snapdragon S25. That is
user-reported external evidence, not a controlled same-scene/device comparison.

## Method

Reused `sarek-d3d11-window.cpp`, the existing textured-triangle consumer. Built
ARM64 and AMD64 PE binaries using the existing build script. Both execute through
the current ARM64 Wine, installed DXVK-Sarek 3.0.2, Mavk guards, libhybris and
Mali-G925. AMD64 additionally exercises FEX. Existing isolated `.mavk-wine-poc`
prefix reused; no new prefix/container, game config override or runtime replacement.
DLL symlinks in the benchmark's own directories select installed Mavk Sarek PE
payload, not the system runtime's different upstream DXVK payload.

15-second measured interval after 3-second warmup; Present(0), four swapchain
images, IMMEDIATE, forced X11 SHM retained. Public capability bits unchanged.
These are **Present-return throughput**, not GPU-only time or unique visible FPS.

| Actual swapchain | ARM64 Present/s | AMD64 Present/s | Mean Present ms, ARM64 / AMD64 |
|---|---:|---:|---:|
| 1280×800 | 59.769 | 59.857 | 16.577 / 16.571 |
| 2560×1600 | 45.469 | 46.419 | 21.850 / 21.395 |

All four sizes verified in DXVK's actual swapchain
log. No device creation/present failure in these runs; D3D11 FL11_0, S_OK.
GPU rendering correctness is visual only here, not a new pixel oracle/CTS result.

## Invalid preliminary measurements — do not use

First version still created a 660×520 decorated window. ARM64's actual swapchain
was 652×486, whereas AMD64's was later resized to 2560×1578 by the window manager.
Its apparent ARM64 advantage was therefore INVALID, not evidence of FEX overhead.
Fixed creation to a borderless window with requested dimensions, rebuilt both
binaries and repeated the matrix above. All quoted results use corrected builds.

The first attempt selected system `dxvk-mali` DLLs instead of Mavk's actual Sarek
payload: upstream version rejected `fillModeNonSolid`, as expected. Corrected only
the benchmark's DLL symlinks. This was harness selection error, not a new Mavk bug.

## Interpretation and limitations

- FEX is not the dominant limit for this tiny workload: ARM64/AMD64 full-resolution
  throughput differs by only about 2%, within uncontrolled-run variability.
- There is no universal 10–20 FPS ceiling in this D3D11 path. This does NOT predict
  Muck performance: its CPU simulation, draw volume and shaders are absent here.
- Higher resolution costs throughput even for this simple scene. Existing forced
  SHM/readback/presentation path remains a candidate, not newly proven cause.
- Thermal status3, SKIN45.0–46.2°C: not a cold-device peak performance result.
- Android buffer samples were not reliable unique-frame measurements: one crossed
  teardown/refresh changes; another exceeded producer rate due to other composition
  updates. Neither is presented as the benchmark's visible FPS.
- No GPU timestamps in this benchmark, no native-Vulkan equal-workload comparison,
  no S25 measurements. The opt-in GPU timestamp layer was built but NOT staged or
  validated; do not describe it as proven profiling evidence.

## Artifacts / next

Corrected binaries: `E:/Mavk/artifacts/d3d11-quick-bench-fixed-20261022`:
ARM64 `cbc3e84817888ec176f553aa386cf3a42c268fb1e4acb6174cf591878b556195`,
AMD64 `90381c7f1d293396534b81633a36d2b9b747c3f18314a63565d18fa14ed7cefa`.
Device logs: `files/mavk-quick-bench/{aarch64,x86_64}-{1280,2560}.{log,stderr.log}`.
Runner: `experiments/droiddeck/quick-d3d11-bench.py`.

Do not keep cycling games or extend feature coverage for this result. Next useful
performance work is bounded GPU timestamps vs the already measured WSI fence wait
in the actual workload, with thermal state controlled. Preserve original APK25,
installed runtime/WSI and Steam session; no commit/push.
