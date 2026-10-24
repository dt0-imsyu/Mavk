# Experimental BGRA to RGBA AHB conversion — 2026-10-24

Scope: independent native Vulkan consumer only. User is playing Rust through
another Steam session: do not start Steam, Muck or another Steam application.
No APK update, production runtime replacement, U-AVF/VM change or commit/push.

## Implementation

Opt-in `MAVK_WSI_AHB_BGRA_BLIT=1`. Application receives an ordinary optimal
BGRA8 swapchain image, backed by existing device-memory allocator. Presentation
blits it into a separate imported RGBA8 AHardwareBuffer before the existing
foreign-queue release and SYNC_FD export, in the same release submission.
No CPU pixel readback and no extra per-frame queue submission. This is **one
GPU image copy per frame**, not end-to-end zero-copy. RGBA remains direct.

Native BLIT_SRC/BLIT_DST support and actual AHB format properties are checked.
No undocumented AHB format numbers, fake BGRA external-memory capabilities,
channel reinterpretation or advertised Vulkan feature changes.

Common implementation: experiments/droiddeck/wsi-ahb-{conversion,memory,sync}.hpp.
External WSI integration is preserved as wsi-ahb-bgra-blit.patch, relative to
the preserved f1b source (priority-source/wsi). Request tracing is additional
diagnostic code in the recovered worktree, not required by this patch.

## Actual device observations

Galaxy Tab S11, stock Mali-G925, installed APK32 unchanged, installed common
Mavk layer unchanged. WSI candidate SHA256:
4727674c97cd5f6367c74b3cf5b1613cf542b11c587d01ef3fdb3bdb3f58befc.
Consumer SHA256:
55648123be0d72c7b0d58d75f08f3328262832fd2ac864bb42b9012d052b9722.

Initial standard Vulkan BGRA draw run completed:

```
WSI_STANDARD_FORMAT selected=44
WSI_STANDARD_DRAW_BEGIN frames=180 extent=1920x1200
WSI_STANDARD_DRAW_SUBMITTED frames=180 elapsedMs=2951.940 sourceReadbacks=0 visibleFpsUnmeasured=1
MAVK_LAYER_CORE_END liveZero=1
WSI_AHB_CONSUMER_PASS standardApi=1 images=3 extent=1920x1200 imageViews=1 firstAcquireFence=0 presentTested=1 pixelTested=0
```

Three RGBA AHB imports succeeded; conversion path selected for each. 540 queue
submissions, gaps=0 and tracked resources zero at shutdown. Approximately 61
completed present calls/s in this short consumer run is **not visible FPS**,
nor isolated GPU blit timing, pixel correctness, or a Zink performance result.

Subsequent RGBA draw control rejected `fullscreen configure` before Vulkan
creation. Marker still says 1920x1200; debug Activity had been reattached while
Android was locked. Follow-up diagnostic consumer
79c97c858abb0ad81665d7c4d565b1d8ce5a89c68b689d547961197e3592936b
confirmed three configure events at **1280x800**. MavkWindowActivity.attach()
passes exactly those hardcoded dimensions. This is a debug-output size mismatch,
not evidence of a BGRA GPU failure. Do not loosen the size check or present this
as a successful 1200p regression. Log: recovery/2026-10-24/configure-control.log.

Both BGRA and RGBA allocation/acquire-fence controls subsequently PASS on the
same candidate at 1920x1200: three images, views, first acquire fence and clean
resource teardown. They do not execute a draw/blit or validate pixels.
Logs: recovery/2026-10-24/{bgra,rgba}-allocation.log on C:.

Android trust reports deviceLocked=1. Wake-up/foreground requests do not unlock
it. No authentication bypass or repeated blind input attempted. Visible-color
and SurfaceFlinger tests remain pending; nothing was installed into Steam.

## Follow-up: pixel oracle and GPU timestamps (no Steam)

Extended the existing native AHB receiver with `--bgra-blit-check` and an
intentional `--bgra-copy-negative` control; reused the shared AHB importer,
existing pool/readback oracle and cleanup. The WSI and oracle now call the same
small `ahb-blit.h` command recorder. No duplicate renderer or transport.

Actual Mali-G925 results, six uniform 1920x1200 images, red values
0/17/91/153/224/255 with green34/blue51/alpha255:

- **13,824,000 RGBA pixels match exactly** after ordinary BGRA image -> GPU
  blit -> imported RGBA AHB -> diagnostic staging readback.
- Negative raw image copy fails on first pixel: got `ff002233`, expected
  `ff332200`, exit3 as required. Thus the oracle detects swapped channels.
- GPU timestamps around the blit only: mean0.152ms, max0.184ms, samples6,
  no warm-up. Uniform clears may compress well. These are NOT representative
  textured-game copy cost, full-frame latency, sustainable throughput or FPS.
- Oracle deliberately reads pixels on CPU; normal presentation does not.
- No visual validation of the complete WSI/compositor/display pipeline yet.
  The standalone check does not verify foreign ownership handoff or WSI fences.

Latest receiver SHA256:
2031d01962d700247ee8d86c59c9983e4d4178dcb7efc16afab9bb46e755c42f.
Logs on C: recovery/2026-10-24/bgra-pixels-timestamps.log,
bgra-copy-negative.log. Original receiver modes still build with -Wall -Wextra
-Werror; no changes to their execution path.

WSI recompiled successfully after sharing the recorder; latest private binary
and device candidate SHA256:
d53b291b8371f726ca179c974cf4f48ca2cdef42607e7ef9fa9176e6c31a7875.
Both BGRA and direct RGBA allocation/acquire controls PASS again with CoreZero
and gaps0 on this binary. Logs bgra-d53-allocation.log/rgba-d53-allocation.log.
This supersedes4727674c as the private candidate, not as the previously measured
180-frame rendering binary. No APK update or production selection change.

## Source locations and next gate

Active source and build are on C: under recovery/2026-10-24/work-wsi and
build-local/build. Recovery is intentionally gitignored; the common headers and
integration patch above preserve the implementation outside it. E: is read-only
because of observed disk153 I/O retries, including possible toolchain-read stalls.

Device private candidate: files/mavk-wsi-xcb/libVkLayer_window_system_integration.so.
Known-good rollback: same directory/libVkLayer-input-f1b9370e.so. Common runtime,
system WSI and original mavk-wsi-ahb directory are unchanged. No game/profile
selects the new conversion automatically.

Next: align the debug Activity output with the intended 1920x1200 test marker
(including reattach), then once a visible debug Surface is available obtain
correctly configured
RGBA/BGRA moving images, verify channel order against constant clear/triangle
colors, then run real independent Zink workload without launching Steam. Count
actual buffer presentations separately from calls and measure conversion cost.
Do not reuse the previous invalid 1965 glmark result or infer a game speedup.
