# Direct AHB presentation checkpoint — 2026-10-23

## Proven result

Real glibc Vulkan graphics through the installed Mavk/hybris runtime rendered a
moving triangle directly into three persistent 1920×1200 Android hardware buffers.
The private Wayland v3 client transferred handles and explicit acquire/release
SYNC_FDs to the experimental DroidDeck compositor, which submitted the same AHBs
to Android SurfaceControl. No source CPU readback or full-frame pixel transport
was used. This is not yet a standard Vulkan WSI implementation or a game path.

Evidence: `E:/Mavk/artifacts/ahb-wayland-draw-20261023/`:

- `draw1200-run.log`: 360 submitted frames / 360 callbacks in 5984.517 ms;
  358 fenced returns, zero immediate returns, real shader draw, no source readback.
- `draw-surface-latency.txt`: actual `droiddeck_wayland_game#1843` layer,
  63 valid records over 1.033316308 s, approximately 60.001 buffer presentations/s;
  median interval 16.664539 ms, maximum 16.840616 ms.
- `draw-a.png`, `draw-b.png`: visible triangle, sampled horizontal displacement
  264 display pixels between screenshots. Android viewport was 2560×1496, scaled
  from the 1920×1200 source, not pixel-exact fullscreen.

The Surface sample is approximately one second, not proof of sustained unique
pixel FPS for the whole six-second run. No end-to-end latency, game FPS benefit,
HWC internal copy count, CTS result or peak GPU throughput is claimed.
Mavk guarded graphics ran with gaps=0 and Core liveZero=1. Descriptor-free draw
instrumentation counters are not a total rasterized draw count.

## Two actual end-to-end fixes

1. App receiver `setsockopt(SO_RCVTIMEO)` failed with errno 13 on the transferred
   socket, although earlier same-context helper tests passed. Producer now sets
   the bounded two-second receive timeout before transfer; receiver checks it
   before attempting mutation. No security policy was changed. Exact syscall
   failure is established; its kernel/vendor policy origin is not established.
2. Layer-only buffers intentionally have no compositor Vulkan image. The old
   `has_content = image != nullptr` test therefore kept the window unmapped.
   AHB association now also constitutes content. The earlier black-screen run
   with callbacks and immediate releases was NOT a visible PASS.

## Candidate and rollback

Installed only experimental `com.droiddeck.launcher.mavk` APK28:

`E:/Mavk/artifacts/droiddeck-ahb-apk28-20261023/DroidDeck-Mavk-AHB-full1200.apk`

SHA256: `7430fd64b1db7e1d3871c38c6b03e44e7981791c00c9f45ec4a5889c433f6a12`.
Native module: `2cbb58a9b6de0eb17392c4eb760ab73374620dcffa19dfc24ec2685cc5fd16ce`.
Only libdroiddeckwayland.so replaced; 486 other APK members identical to APK27.
Signing identity matches the baseline. No production U-AVF or VM data changed.

Rollback: install-r, without uninstall/data clear:
`E:/Mavk/artifacts/droiddeck-lifecycle-apk27-20261023/DroidDeck-Mavk-lifecycle-20261023.apk`.
SHA256: `f5c84d3a5b215e7d349068e0738c253ba2f0a954535caeff51fc47438fe552b6`.

## Regression and final state

Actual device state tests passed acquire-required, acquire forwarding, stable
repeat, release-FD wire transfer, skipped-use retirement and destroyed-owner
release wait. Surface is mocked in that test, not in the moving-triangle run.
Actual native factory/Wayland wire tests passed default v2 and opt-in v3 AHB
import. Their factory tests themselves do not exercise Surface.

Private debug marker was removed. Normal startup confirmed protocol v2 and
zero-copy off. Normal Steam session `2026-10-23-13-steam` launched Muck; actual
menu rendered and project input opened Settings, then returned without changing
settings. Agent launch focus wait timed out (focusedApp metadata absent), so the
command itself is NOT a PASS. Screenshots prove rendering/input, not performance.
Session stopped normally: IDLE, no guest PID, artifactsComplete=true.

Installed common runtime/layer and X11 WSI were not replaced. No Vulkan feature
bits changed. No commit or push. Tablet free space remains about 34 GiB, below
the requested 50 GiB; no games, VM disks or personal data deleted for this test.

## Next actual consumer

Integrate the proven buffer/fence contract into a private standard Vulkan
Wayland swapchain, preserving existing fallback. Audit acquire/release ownership
and queue-family/layout semantics before reuse. Current Wine games use X11 /
Xwayland SHM; a Wayland Vulkan swapchain does not automatically accelerate that
path. A native Wayland consumer is the first integration gate, followed by a
supported game/runtime route. Do not expand triangle benchmarks or advertise
this as completed game zero-copy.
