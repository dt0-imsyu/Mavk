# AHB-native Wayland buffer + explicit-fence adapter — 2026-10-23

## Implemented, not yet a visible/game PASS

The DroidDeck fork now has an experimental banner_ahb_v1 version3 path that
creates a wl_buffer directly from a public AHardwareBuffer socket transfer.
No dma-buf fd, native_handle integer interpretation or guessed DRM modifier.
It accepts unprotected single-layer RGBA8 composer-overlay allocations only.

Requests: create_buffer, attach_explicit for fresh Android buffers,
set_acquire_fence and set_acquire_ready. Release events: fenced_release or
immediate_release. The SurfaceControl acquire dependency is forwarded as an FD;
the previous display release dependency can be forwarded back without importing
it into a guessed dma-buf. The owner object must outlive associated buffers.

Version3 is **off by default**. It is advertised only with the private process
environment DROIDDECK_EXPLICIT_AHB=1; ordinary startup still advertises version2.
No APK installed, no session started, no installed library/runtime replaced.

## Important correction to earlier audit

The common registry is named dmabuf_buffer, but android_wlegl buffers use the
same wl_buffer implementation with n_planes=0/fd=-1. Therefore the old attach
request could recognize such a buffer even though it had no dma-buf fence.
Earlier statements that the registry strictly required a dma-buf were too broad.
The real missing contract was synchronization and a proper AHB-native creator.

Legacy attach now refuses a buffer without a dma-buf FD. A failed/timed-out
implicit-fence poll no longer goes ahead with a direct SurfaceControl present.
This does NOT prove every legacy/native Vulkan fallback is synchronized; the
existing android_wlegl compositor-copy path is not converted by this patch.

Explicit buffers are fresh/layer-only: their ordinary Vulkan image is removed
before any surface references them. This prevents falling back to an unsynchronized
Vulkan read when a SurfaceControl present is unavailable. The initial experiment
is fullscreen-only, without effects/children or a general windowed-copy fallback.
SHM/ordinary WSI remains mandatory for unsupported configurations. A skipped
buffer use gets an immediate explicit release; unused acquire FDs are closed.
Destroying the protocol owner falls back to asynchronously observing the display
release fence before releasing the wl_buffer, not to declaring it ready early.
AHB receive requires a stream socket and has a bounded receive timeout.

## Evidence on actual Tab S11

- Entire droiddeckwayland native module builds/links with NDK28.2. Two existing
  unrelated unused-variable warnings remain; no new build error.
- Real candidate shared library loaded in a separate app-UID native process.
  Actual product wl_buffer factory accepts1920x1200, has no dma-buf FD, rejects
  invalid bounds and non-fresh conversion, and survives actual resource destroy.
- Real Wayland server dispatch: raw client registry/bind/create_buffer requests
  reach the candidate module. A real AHB crosses SCM_RIGHTS/public NDK socket
  transfer and is associated with the server wl_buffer. Default global version2
  and private opt-in version3 checked in separate processes. No mocked factory.
- Production ahb_swapchain.c state/FD test uses real AHB allocation/import and
  actual Wayland resources. Checks missing acquire, FD forwarding, stable repeated
  display reference, actual fenced_release ancillary FD on the Wayland wire,
  skipped-use retirement and delayed cleanup after owner/buffer destruction.
  SurfaceControl itself is MOCKED in this state test and fences are eventfds,
  not GPU SYNC_FDs. Actual GPU fence proof is the separate90-frame pool report.

These results do not prove a frame reached SurfaceControl/SurfaceFlinger, resize,
reconnect, normal game behavior, throughput or improved FPS. No Vulkan feature
bit was changed. Do not enable this adapter in the installed app yet.

## Source/artifact provenance

Fork: E:/Mavk/work/DroidDeck-mavk, files:

- app/src/main/cpp/waylandcomp/protocols/banner-ahb-v1.xml and generated bindings.
- src/ahb_swapchain.c, src/ahb_swapchain.h, src/compositor.c within waylandcomp.
- tools/tests/ahb_explicit_sync_test.c, ahb_layer_buffer_test.c.

Generated using locally extracted Ubuntu libwayland-bin1.24.0-2 (no global tool
installation). Existing configured NDK Ninja target droiddeckwayland used.

Artifacts: E:/Mavk/artifacts/ahb-wayland-explicit-20261023/ including candidate
library, helpers, state-test-final.log, protocol-v2.log and protocol-v3.log.
Candidate SHA256 aa7d84595638f916705f18c9592dfac28d4a8a2e9e513fae1f6c8bbbc86afecf.
The source capsule preserves only this change's files, not all unrelated fork work.

## Next step

Use this real buffer factory from a private glibc Wayland presentation client,
with the already-proved AHB pool/SYNC_FD resources. First actual app-owned
SurfaceControl frame, display release/resize/disconnect, then a real WSI consumer.
Account explicitly for Wine's X11/Xwayland boundary; this protocol alone does not
replace an X11 SHM swapchain. Keep mode opt-in until actual on-screen regression.

Installed APK27/commonf108/WSId07b, Steam configs and all U-AVF VMs/data unchanged.
No commit/push, root, host security/property changes or authentication workaround.

