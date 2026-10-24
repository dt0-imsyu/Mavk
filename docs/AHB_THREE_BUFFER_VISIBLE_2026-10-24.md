# Visible Zink: two versus three buffers — 2026-10-24

Scope: independent glmark2 jellyfish, native ARM64 Zink/Mavk/Mali, debug Activity
output **1280x800**, FIFO. Steam/Rust never launched or switched. Installed APK32
unchanged. No U-AVF/VM/production runtime change; no commit/push.

## Evidence-driven fix

With two BGRA swapchain images (private WSI d53b291b), now-unlocked visible
rendering was29–31FPS, while previous hidden runs had reported60. Screenshot
confirmed actual jellyfish colors, not a blank surface. Android display owns a
buffer until its release fence completes; the double-buffer producer starves.

One-line A/B: experimental Wayland AHB surface capabilities minImageCount2->3.
No change to renderer, shaders, encoder, Android compositor or queue scheduling.
Normal non-AHB surface capabilities remain unchanged. This general allocation
requirement is not a jellyfish/Muck special case or an unsupported present-mode
claim. It costs one additional source/output image pair for BGRA conversion.

Private new WSI SHA256:
99a12d70e88e14babcf27ad8dcb22804334a9e769bce2dcc89c7c5646096e663.
Reproducible external-source change: experiments/droiddeck/wsi-ahb-three-images.patch.
Common BGRA blit remains one GPU copy/frame; compositor zero-copy statistics do
NOT make the complete BGRA pipeline zero-copy.

## Actual tests

- Identical10s two-image baseline: glmark29FPS/35.277ms,570draws (=285 frames),
  CoreZero/gaps0. SurfaceFlinger adjacent present interval33.358ms.
- Identical10s three-image candidate: glmark60FPS/16.828ms,1192draws
  (=596 frames), CoreZero/gaps0. Adjacent present interval16.658ms.
- --latency currently exposed only2–4 usable samples per dump. These pairs are
  corroboration, NOT sufficient alone for a sustained-FPS claim.
- Complete30s fresh-server run: glmark60FPS/16.711ms,3594draws (=1797 rendered
  frames),7189submissions,5391descriptor-template operations, gaps0, CoreZero.
- SurfaceFlinger TimeStats for game layer:1795 presented buffers. Its reported
  averageFPS62.426 uses millisecond histogram bins; do NOT quote that as real
  >60FPS. The30s rendered workload plus physical interval/other counters support
  approximately60 buffer presentations/s, not62.4.
- Two consecutive complete10s compositor windows:601/600 on-screen layer frames,
 600 incoming GPU buffers each, pool drops0, compositor copies0. This is
 601/600 frames at the compositor display boundary, not just QueuePresent calls.
- Viewed two live screenshots with correct colors and different animated poses.
  No per-frame pixel-identity audit or end-to-end latency trace was performed.
- RGBA direct and BGRA conversion allocation/acquire-fence controls at1920x1200
  still PASS on99a12d70; those are NOT1200p rendering performance tests.

An earlier30s attempt was cut short by the already-running test Xwayland's300s
lifetime. Handle/process termination confirmed before starting a fresh server.
That partial attempt is excluded from the successful30s claim above.

## Artifacts / cleanup

On C: recovery/2026-10-24/ (gitignored): zink-visible.png, zink-three.png,
zink-sustained.png; zink-two-sf.log/zink-three-sf.log;
zink-visible-three-full30.log and zink-visible-three-full30-timestats.log.
The test server :98 PID22000 stopped; parent21998/child22000 absence verified.
SurfaceFlinger TimeStats diagnostic collection disabled after the test.
Screen stays awake during debugging; PIN/manual lock preserved.

Private current library is99a12d70. Android-only rollback candidates in
files/mavk-wsi-xcb: libVkLayer-bgra-two-d53.so and libVkLayer-input-f1b9370e.so.
Original system WSI/common runtime untouched. E: remains read-only after disk153
retries; builds/source recovery on C:.

## Next

Fix debug Activity's hardcoded1280x800 selection/reattach to test1920x1200 on a
real Surface. Do not relabel the above as1200p60 or a Muck/game speedup. Preserve
three-buffer requirement, compare SHM/AHB with the same resolution/FIFO scene,
then retest games only when doing so cannot interrupt the user's Steam session.
Broader Vulkan compatibility and cross-project integration remain incomplete.
