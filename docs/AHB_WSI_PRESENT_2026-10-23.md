# Standard Vulkan AHB presentation — 2026-10-23

## What now actually works

The private existing-WSI candidate presents GPU-rendered swapchain images through
the v3 AHB protocol and DroidDeck SurfaceControl, using standard Vulkan acquire,
submit and present calls from its consumer. It no longer needs the special
AHB-aware producer. An unchanged, separately installed ARM64 `vkcube` renders a
visible rotating textured cube through this path on Mali-G925, stock Android.

This is NOT yet a Steam/Wine game backend, Windows GPU acceleration or a complete
conformant/general-purpose WSI implementation. No installed runtime was replaced.

## Evidence, with distinct builds/scopes

Artifacts: `E:/Mavk/artifacts/wsi-ahb-draw2-20261023/`.

| Consumer/build | Actual source | Evidence |
|---|---|---|
| Standard API triangle, WSI 61ac4f00 | 1920×1200, 1800 presents / 29984.699 ms | present-run.log, screen-a/b.png |
| Unchanged ARM64 vkcube, WSI cace4405 | 1280×800, 900 actual draw calls | vkcube-visible.log, vkcube-a/b.png |
| Final conservative capabilities, WSI 3b705bef | vkcube 1280×800, 60-draw regression | vkcube-final-caps.log |

Triangle Surface sample: actual `droiddeck_wayland_game#2089`, 63 valid records
over 1.033323693 s, 60.00056 buffer presentations/s. Intervals 8.327769–25.014461 ms
on the 120 Hz Android display. Two screenshots show 336-pixel horizontal movement.
Native compositor reports two complete ten-second windows of 600 zero-copy
frames each, no layer copies or pool drops. These native counters are not an
independent pixel-identity oracle or GPU-throughput benchmark.

External cube Surface sample: `droiddeck_wayland_game#2136`, 63 records over
1.033508924 s, 59.98981 buffer presentations/s. Screenshots show different cube
orientations with intact LunarG texture. Native compositor also records a complete
600-zero-copy-frame ten-second window for this run. The screenshots were inspected.
The earlier `vkcube.png` was captured after its first bounded run had exited and
is black; it is NOT evidence of a rendering failure or a visible PASS.

The cube's command requested 1920×1200 but actual Wayland configure yielded
1280×800. Do not report the external cube as 1200p. Android viewport was
2560×1496 with system bars, not pixel-exact fullscreen. Sustained unique-pixel FPS,
input-to-photon latency, peak GPU throughput, energy and game gains are unmeasured.

All successful consumers end with gaps=0, resource counters zero and CoreZero.
The descriptor-free triangle counter draws=0 is not a total draw counter; the
independent descriptor-using cube records draws=900 / submissions=2701.

## Actual synchronization implementation

`wsi-ahb-sync.hpp` performs foreign/local ownership and GENERAL/PRESENT layout
transitions. Incoming display SYNC_FD is imported into a private binary semaphore;
acquire submits the ownership barrier and exports a completion SYNC_FD into the
application's acquire semaphore/fence. It does not report an unsignalled dependency
as already complete. Present waits for application rendering at ALL_COMMANDS,
releases ownership, exports a GPU dependency and submits the AHB to SurfaceControl.

wl_buffer.release AND the explicit return dependency are required before reuse.
No CPU wait on the display fence or source pixel readback/copy is added. Private
command reuse can wait for its own prior GPU fence; caller acquire timeout is
propagated, timeout/not-ready returns the image to the free pool without faulting
the swapchain. Each frame currently entails acquire / application draw / release
submissions (three total). Their overhead is not independently measured.

Current scope: one graphics queue (best family, queue index 0), exclusive RGBA8,
single layer/mip/sample, FIFO, opaque, one eligible root surface. Unsupported
present-id/wait/timing, compression and swapchain-maintenance features are masked
off for this private mode; unsupported modes/flags/custom allocators reject.
Host submission serialization currently relies on the installed common Mavk
device mutex, which the private runner requires. Do not deploy the module alone
without auditing that contract. CTS, concurrent-queue and GPU-failure injection
tests have not run. This remains experimental.

## Consumer-driven compatibility fix

The first external vkcube launch failed before device creation because it requests
Vulkan 1.0 (4194304), while the prototype unnecessarily required 1.1. Removed that
restriction and enabled the actual AHB/external-memory/fence/semaphore/dedicated
allocation/YCbCr dependency chain. Existing instance KHR capability dependencies
are reused. No vkcube source or shaders were changed. The exact installed cube
SHA256 is `0144bade0e3c150e95c943e1f8a07eba901ab5983465ee8b7cb746c46dfb3373`.

Reference dependencies:
https://docs.vulkan.org/refpages/latest/refpages/source/VK_ANDROID_external_memory_android_hardware_buffer.html
https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_sampler_ycbcr_conversion.html

## Reproduction / preserved state

Final private WSI SHA256:
`3b705bef8842584fc333f704fc587646046dae9751475c7590116fe39f4c1561`.
Known visible baselines 61ac4f00 and cace4405 are saved as immutable binaries in
the artifact directory. Runtime/common layer hashes remain unchanged. Runner
requires `-UseInstalledLayer -PrivateAhbWsi -AhbPresent -AhbWsiHash <exact hash>`.
Only that switch selects the private WSI; normal Steam profiles were not edited.

Negative control after removing the v3 marker: native consumer fails swapchain
creation with VK_ERROR_FEATURE_NOT_PRESENT (-8), before AHB image allocation.
It does not silently fall back or present an unsynchronized buffer. Exact log:
`no-v3-negative.log`.

The shader draw helper was factored once into triangle-renderer.h and reused by
both the old private AHB producer and standard API consumer. Legacy producer was
rebuilt successfully; it was not rerun as another standalone benchmark.

All tests exited; temporary debug compositor was force-stopped and its opt-in
marker removed, so it cannot contaminate normal Steam startup with its private
runtime directory. User-requested stay-on while plugged remains enabled. No APK
install, production U-AVF/VM/data changes, root/security modifications or commit/push.

## Next useful integration

Inspect the existing Wine/Proton Wayland driver and FEX/native Vulkan route,
then run a real D3D11 consumer via this WSI in an isolated per-game configuration.
Existing Wine/X11/Xwayland SHM cannot magically become an AHB-native swapchain.
Preserve its fallback and restore any experimental per-game settings. Do not
return to triangle benchmarks or claim a game FPS gain from this milestone.
