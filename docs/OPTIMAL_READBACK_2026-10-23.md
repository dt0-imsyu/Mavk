# Optimal render target / SHM readback PoC — 2026-10-23

Result: FUNCTIONAL, NOT A VERIFIED PERFORMANCE FIX. Do not promote to APK/runtime.

## One bounded hypothesis

The native X11 SHM WSI forces application swapchain images to LINEAR tiling.
An isolated candidate instead keeps OPTIMAL GPU render images and uses a
separate linear cached readback image. An image copy is recorded into the
EXISTING semaphore-wait/fence submit; no additional submit, queue-idle or CPU
wait. The source returns to PRESENT layout; the destination transitions to
GENERAL with TRANSFER_WRITE -> HOST_READ and noncoherent invalidation before
CPU reading. Reuse remains behind the existing per-image presentation fence.

Explicit bounds: opt-in SHM only, one graphics queue family, exclusive sharing,
one layer, four RGBA8/BGRA8 UNORM/SRGB formats, no extended image create chain.
Unsupported shapes reject rather than guessing. Default flag0 retains linear
behavior; no changes to installed WSI. This is not zero-copy.

Memory/synchronization requirements:
https://docs.vulkan.org/guide/latest/image_copies.html
https://docs.vulkan.org/spec/latest/chapters/synchronization.html

## Actual Muck A/B/A

Same animated menu,2560x1600,installed guarded Mavk/Sarek retained. Same private
WSI binary C73201c329616a38037e262d7728904e42eb88b597e2fc54980df100d44cfe8b
in all runs. CACHED_READBACK=1 throughout; only OPTIMAL_READBACK flag changes.
Actual mappings,environment and per-image extent/memory type logs inspected.
Optimal path uses type1 flags11(noncoherent cached),normal visible game frame,
no Mavk rejection/device-loss observed. No exact full-frame oracle/CTS or Vulkan
validation-layer pass claimed.

| Last20 source records | Linear control | Optimal candidate | Linear reverse |
|---|---:|---:|---:|
| interval seconds |20.571|20.867|21.118|
| source presentations/s |17.306|12.029|11.128|
| fence wait ms/frame |43.483|69.510|76.731|
| CPU copy ms/frame |13.142|12.161|12.010|
| independent Surface updates/s |17.8|13.5|11.0|

Surface windows are separate20s intervals,not exact source-record windows.
Content identity is not established by these counters. Moving menu screenshots
look normal in both modes. Temperature was already throttled(status3,SKIN45–46C).
Reverse control also remained slow: NOT evidence of a stable candidate speedup
OR proof that the candidate alone caused the decline. Do not keep iterating
copy variants on this thermally drifting scene. The test does prove the new
allocation/copy/presentation path can execute the real game on stock G925.

## Restoration and reproducibility

Muck quit normally after reverse; settings-backup.active=false in private
files/mavk-wsi-optimal. Both game-environment copies restored with ownership
checks. Original system WSI/APK25/Mavk runtime untouched; Ubuntu/Windows/H264
untouched. No commit/push.

E:/Mavk/artifacts/wsi-optimal-20261023 contains control/optimal/reverse logs,
Surface pacing JSON and screenshots. Isolated source under
E:/Mavk/work/wsi-optimal-readback-20261023. Patch in
experiments/droiddeck/wsi-optimal-readback.patch applies on top of the existing
wsi-profile.patch source; build-wsi-optimal.sh consumes its source archive.
Selector --work isolates settings backups; --optimal enables the PoC. Existing
mapped libraries are checked for equality,never overwritten during flag A/B.
