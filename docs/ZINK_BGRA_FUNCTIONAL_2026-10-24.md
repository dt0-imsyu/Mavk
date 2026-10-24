# Real Zink client through BGRA AHB — 2026-10-24

No Steam or Steam games started. All work uses the independent debug compositor
and a separate Xwayland display :98. User's Rust session is out of scope.
APK32, original runtime, U-AVF and VM data unchanged. No commit/push.

## Pixel and cost gate

Existing AHB receiver now also supports `--bgra-pattern-check` and
`--bgra-pattern-negative`. It uploads six changing per-pixel hash patterns in
BGRA order, uses the same blit recorder as WSI, then validates every RGBA pixel
after diagnostic readback. This is not a textured game render or display test.

Actual Mali-G925: **13,824,000 exact pixel matches** at1920x1200. Raw-copy
negative fails at frame0,x1,y0: gotffcad11e, expectedff1ed1ca, exit3.
GPU timestamps around blit:6 samples, mean0.340ms, max0.495ms, no warm-up.
This excludes upload, host validation, queue/presentation and foreign-fence cost;
it is neither frame latency nor a game FPS estimate. Normal WSI has no readback.

Receiver binary SHA256:
ffe99e63df71ddcd1f07eb629846767751d10342750f07059e45dd275c366933.
Logs: recovery/2026-10-24/bgra-pattern{,-negative}.log on C:.

## Real Zink gate

Reused upstream glmark2 jellyfish, common Mavk layer and private WSI d53b291b.
Debug Activity currently configures1280x800, so this functional integration run
is explicitly **1280x800**, not the final1200p target.

Three harness issues were exposed and corrected without changing applications:

1. Runtime/lib selected its older libvulkan.so.1.2.183 ahead of the Linux loader
   used by the known-good direct consumer. Layer receipts were absent. Choosing
   linuxfs/usr/lib first (after the selected Mesa directory) produced actual
   Vulkan Loader/Mavk logs. Loader symlinks/system files were not changed.
2. Debug Xwayland environment lacked LIBGL_KOPPER_DRI2=true, already used by the
   working Zink UI launcher. Without it, the run did not produce the requested
   WSI swapchain. The independent harness now sets it explicitly.
3. glmark2 normally calls eglSwapInterval(0). This Zink version then selects
   MAILBOX even though the experimental AHB surface advertises FIFO only;
   AHB init correctly rejects it. glmark2's existing --swap-mode fifo avoids
   requesting the unsupported mode. No fake MAILBOX implementation was added.

The initial301/315/438 benchmark numbers are INVALID for Mavk AHB/display
performance and must not be quoted as speedups. Harness now rejects a reported
success without actual Mavk-instance, AHB-request and requested BGRA-conversion
receipts. Four local gate cases pass and the actual wrong-path log is rejected.
The fake-root versus matching-UID A/B alone did NOT fix missing layer receipts;
do not assign the cause to permissions based on that test.

Final **functional PASS**, debug-fifo-bgra.log / host copy
recovery/2026-10-24/zink-bgra-fifo-functional.log:

- Real Zink/Mali renderer and common Mavk instance/device.
- FIFO, BGRA44 swapchain, two importedRGBA AHBs,1280x800.
- 1198 draws,2397 submissions,1797 descriptor-template operations.
- gaps0, remaining tracked resources0, Core liveZero1 at shutdown.
- glmark reports60FPS/16.745ms for its10s run, but see the limitation below.
- Compositor consecutive10s windows received120+479 GPU frames. No surface
  presents:0 ticks,0 on-screen frames,0 zero-copy display frames.

**Android remains deviceLocked=1 and focus NotificationShade. There is NO
visible-FPS or complete pipeline pixel PASS.** Incoming buffers and completed
present calls are not displayed unique frames. No claim of end-to-end60FPS.

Only private debug-run.py was staged; the existing glmark run.py/Steam profiles
were not replaced. Xwayland debug-run/debug-bench are explicit modes in the
existing control helper, not new session or authentication infrastructure.
The :98 test server was signalled to stop after testing; verify termination
before any future run. Do not kill a normal session Xwayland by process name.

## Next practical gate

End isolated color tests here. Correct debug Activity's output selection and
reattach dimensions, then verify visible moving Zink at1920x1200 on an unlocked
Surface. Collect actual SurfaceFlinger timestamps and compare SHM versus AHB
under identical scene/resolution/swap policy. Do not compare this FIFO run with
older unbounded or wrong-loader results. No Steam/game test while Rust is active.
