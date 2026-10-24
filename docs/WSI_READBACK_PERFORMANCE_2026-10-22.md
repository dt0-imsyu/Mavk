# Actual Muck WSI readback — 2026-10-22

## Result

The forced X11 SHM path is confirmed, not inferred from API names. A real-game
profiling build split GPU-ready wait, map/invalidate, pixel copy, X11 requests
and pacing. Waiting for the rendered image is the largest observed cost.
This is CPU wall time waiting for a Vulkan fence, NOT GPU timestamp duration.

An isolated cached-memory candidate works visually but does not remove the main
limitation. Keep it experimental; do not install it as an FPS solution. New Vulkan
features remain paused. The broad Mavk goal is active, not complete or blocked.

## Provenance and experiment

- Installed WSI SHA256 d07b961181b9547fd48ad9f05cc6d3b808b824676eb5a186fe3feb8caa013f19.
- Pristine source HEAD40cfec09d0e2d42c5e1f2642cf4cba11e7008112, headers
  8d6039a455a7ecc7d2a592ff97f62db4e59b70bf (Vulkan1.4.356), existing downstream
  X11 external-memory link fix. Source copy on E:, no edits to the pristine tree.
- Profile-only binary c0061893d829d6a0ad92d41f774b93a7dbe07dfcc436bc0ed9c3e56364f9105c.
- Cached/control binary 888609b2fc276857e5a5b0054d87fe5df3058824f92bbc269fdd3794d33774e1.
- Same animated menu2560x1600, guards retained, actual native ARM64 Wine/Sarek+
  FEX. No game-specific shader or quality workaround. Android60Hz observed in
  these pacing windows. Thermal3, actual SKIN around44–45C; not a cold benchmark.

| Last20 source records | Profile-only baseline | Cached candidate | Same candidate, cached disabled |
| --- | ---: | ---: | ---: |
| Interval, s | 21.186 | 21.307 | 21.762 |
| Images | 200 | 203 | 199 |
| Source presents/s | 9.440 | 9.527 | 9.144 |
| GPU-ready fence wait, ms/image | 85.7046 | 91.4192 | 88.1819 |
| Map/invalidate, ms/image | 0.0001 | 0.5338 | 0.0029 |
| Pixel copy, ms/image | 19.8867 | 12.1775 | 20.7888 |
| X sync, ms/image | 0.0470 | 0.0549 | 0.0508 |
| X queue, ms/image | 0.2583 | 0.7189 | 0.2882 |

Pacing is below0.001ms/image. Source timestamps count WSI presentations, not
unique content. Independent Android Surface windows: profile-only183buffers/
20.1s=9.1updates/s; cached191/20.2s=9.5; control181/20.1s=9.0. Histograms and
changing menu screenshots were inspected; this is not proof of unique100+FPS.
The small overall difference is insufficient for a stable FPS gain claim.

## Specific allocation issue and bounded candidate

Actual compatible memoryTypeBits=3. Old coherent requirement selects type0,
flags7=DEVICE_LOCAL|HOST_VISIBLE|HOST_COHERENT, without HOST_CACHED. Hardware
also offers type1 flags11=DEVICE_LOCAL|HOST_VISIBLE|HOST_CACHED, non-coherent.
The candidate actually selected type1; reverse control actually selected type0.

Private MAVK_WSI_CACHED_READBACK=1, additionally requiring forced SHM:

1. Prefer cached host-visible memory, retain compatible fallback.
2. Record a reusable MEMORY_WRITE -> HOST_READ barrier per presentation fence;
   include it in the EXISTING semaphore-wait/fence submission. Wait stage becomes
   ALL_COMMANDS for this barrier. No extra queue submission or CPU wait added.
3. After the existing fence wait, invalidate the whole mapped allocation before
   reading non-coherent memory. Mapping offset0/whole-size avoids atom alignment
   errors. Coherent memory skips invalidation.
4. Private barrier setup requires exactly one graphics-capable queue family,
   refuses an unknown submit pNext, and keeps command pools until their fence
   completes. This is an isolated G925 PoC, not a universal queue-family solution.

This follows the host-domain dependency/invalidation requirements in the
[Khronos memory specification](https://docs.vulkan.org/spec/latest/chapters/memory.html)
and [synchronization specification](https://docs.vulkan.org/spec/latest/chapters/synchronization.html).
Faster copy is real in these samples, but most wall time remains the GPU-ready
wait; do not assume uncached memory was the sole root cause.

Muck PID16546 mapped the diagnostic WSI and showed normal changing3D menu/UI.
Control PID17299, normal view and frames. No device loss or Mavk rejection observed.
No exact pixel oracle/CTS/validation-layer run for this WSI candidate; visual
correctness and normal exit do not establish complete presentation correctness.

## Reproduction and artifacts

- Mavk/experiments/droiddeck/wsi-profile.patch applies cleanly to pristine HEAD.
- build-wsi-profile.sh uses existing ARM64 tools/SDK on E:, exact header checkout.
- wsi-profile-select.py enables only Muck overrides in both config copies; restore
  verifies ownership of every edited key. Diagnostic runtime symlinks unmodified
  original components and owns only a replacement WSI directory.
- analyze-wsi-profile.ps1 parses actual logs and explicitly reports
  uniqueContentVerified=false. No fabricated FPS or GPU busy percentage.
- E:/Mavk/artifacts/wsi-profile-20261022 holds source/build/headers, baseline.log,
  cached.log, coherent-control.log, profile-baseline.so, and build candidate.
- Screenshots in E:/Mavk/artifacts/muck-performance-20261022: wsi-menu.png,
  cached-start.png, coherent-control.png.

## Restored state and next action

Muck quit normally after control; all WSI profiling/cache/runtime overrides were
restored in both config copies. Installed WSI exact SHA still d07b9611, APK25,
Mavk common runtime, Steam06 unchanged. No production U-AVF/Ubuntu/Windows,
firmware/CPU/storage/permissions changes. No commit/push. No running test handles.
C50.8GiB/E835.5GiB free; device battery last observed90%, USB.

Do not retest memcpy variants or treat cached readback as the 100FPS fix.
Next measure execution on the actual game's GPU command stream with timestamps
or equivalent available instrumentation to distinguish shader/render cost from
queue/presentation serialization. Fence wait alone cannot make that distinction.
Then change the largest measured cause and repeat this same scene plus gameplay.
Do not disable null/OOB guards to obtain an invalid performance comparison.
