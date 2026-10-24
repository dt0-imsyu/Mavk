# Muck GPU attribution — 2026-10-23

One known working animated menu, actual swapchain2560x1600, four images,
IMMEDIATE, x11-shm, FEX + ARM64 Wine/DXVK-Sarek, guards retained. Not a new
gameplay benchmark, not a native-power comparison with GravityMark.

## Invalid first profiler result — withdrawn

Private3be3334e reported64 samples, including impossible spans such as
1418980375344598ms and small packed-counter-looking values. Discard ALL its GPU
timing evidence, including the earlier tiny submit1 regression timings. The12
tests' checked GPU output still passed, but that is not evidence of valid query
initialization/timing. No Vulkan validation-layer/CTS pass had been claimed.

Code defect: first poll could run before queued vkCmdResetQueryPool executed;
newly allocated query availability was not initialized for host reads. Khronos
vkGetQueryPoolResults VUID-None-09401 prohibits uninitialized queries and its
description explicitly warns that queued reset can return stale availability:
https://docs.vulkan.org/refpages/latest/refpages/source/vkGetQueryPoolResults.html

## Minimal profiler fix and control

Private7d7805bf1e1fef619244d4f3b2e8f78f7a7fadfe993899657b6d9691df87f755:
use vkResetQueryPool on a newly created private pool BEFORE any host polling.
Only permitted when hostQueryReset was already enabled in application device
creation (Vulkan12 or separate host-query-reset feature struct). No new feature
enabled/advertised by Mavk; no extra CPU wait or native queue submission.
When dependency is absent, skip profiling while forwarding original rendering.

Existing nested-buffer consumer with no hostQueryReset: candidate mapped,
NESTED_BUFFER_GPU_PASS sentinel31/input32/range4/allocation128, CoreLiveZero,
MAVK_GPU_PROBE_SKIP reason=hostQueryReset-not-enabled. Saved control archive in
E:/Mavk/artifacts/gpu-profile-reset-20261023/feature-absent-control.tar.

Actual Muck has hostQueryReset enabled. Fresh private layer path and same animated
menu produce64 VkQueueSubmit2 timestamps, correct64-bit ordering, no probe error,
Mavk gap or GROUP_ERROR_FATAL in captured log. Measured batch spans:

| Original command buffers | Samples | Minimum ms | Median ms | Mean ms | Maximum ms |
|---|---:|---:|---:|---:|---:|
| 2 | 36 | 40.458540 | 41.823463 | 41.781090 | 43.641233 |
| 3 | 28 | 7.130770 | 7.259308 | 7.309044 | 7.758231 |

Independent Android sample393buffers/20.1s=19.5updates/s,modal50ms. Earlier
invalid-query run411/20.1s=20.4; this is NOT a performance improvement comparison.
Different thermal states/normal scene variation; no fix-to-FPS claim.

Spans cover prefixTOP -> suffixBOTTOM in sampled no-semaphore-wait batches.
They can include earlier-pipeline overlap, barriers and memory dependencies;
they are NOT shader-only duration, GPU busy percentage or automatically a full
frame. Do not add overlapping batches, infer their work from buffer count alone,
or compare these numbers directly against GravityMark native average41.9FPS.

## Concrete general performance candidate

Found endCommand always emits SHADER_WRITE -> HOST_READ,ALL_COMMANDS -> HOST
when guarded generations exist, even with MAVK_GUARD_REPORTS=0. In that mode the
shader compiler emits read-only guards (diagnosticWrites0), and retireCommand
never reads diagnostic buffers. The diagnostic host visibility barrier is then
unnecessary; it is not an application-requested memory dependency.

Minimal common fix: condition that barrier on guardReports, keeping guards and
the reports-enabled path unchanged. No per-game/GPU exception. Build/control
pending at time of initial write; no performance improvement claimed yet.

## Restoration

Both sampled Muck runs quit normally via Steam client; per-game settings restored
in both copies, profile-backup.active=false. No installed .so replaced; candidate
folders private per-process only. Saved logs/screens under gpu-profile-safe-
20261023 (invalid first run) and gpu-profile-reset-20261023 (corrected run).
No commit/push, U-AVF/Ubuntu/Windows/firmware/storage changes.
