# Standard Vulkan WSI allocation gate — 2026-10-23

## PASS scope

An ordinary Vulkan API consumer now creates a Wayland VkSurfaceKHR, a three-image
1920×1200 RGBA8 swapchain backed by imported Android hardware buffers, creates
image views, acquires the first image with a fence and waits for that fence.
It destroys the swapchain/device/instance successfully. Installed guarded Mavk
reports gaps=0, all tracked resource counts zero, Core liveZero=1.

This is an allocation/first-acquire gate, NOT a rendered or presented swapchain
frame. No pixel, repeated-acquire, display-release, performance or CTS claim.
The private candidate deliberately rejects presentation until its full acquire /
present ownership and fence contract is implemented. It is not an advertised
production Vulkan implementation and must not be selected for Steam/games yet.

Final log: `E:/Mavk/artifacts/wsi-ahb-consumer3-20261023/acquire-run.log`.
Consumer SHA256: `78c48bde7944e85d06fe47f8d49b8827766c50520fe1786d98c1f357df5637ad`.
Candidate WSI SHA256: `6111f7683cc15158eb4bf490b7665a3bf99ab9975c769d02a03b89956e7ac213`.
Actual candidate and installed common layer mappings are checked by the consumer;
the runner also checks the candidate binary hash before launch.

## Integration changes

- Isolated 1.18 MB copy of the existing Arm WSI under
  `E:/Mavk/work/wsi-ahb-20261023`; no replacement of installed WSI/APK.
- Existing image factory gains an opt-in native-bound-image creation hook.
  Default external-memory factory is unchanged. AHB backing owns memory and the
  original AHB reference; swapchain image remains responsible for VkImage lifetime.
- `experiments/droiddeck/wsi-ahb-memory.hpp` reuses the common image importer,
  public AHB allocation through existing hybris ABI, and existing WSI allocators.
- Diagnostic mode fabricates and owns its Wayland Surface handle, as the existing
  X11 backend does. It does not call Android ICD Wayland surface creation.
- AHB mode bypasses the otherwise mandatory dma-buf heap allocator. AHB allocation
  does not require opening a dma-buf heap from the app.
- Shared importer accepts both memory-properties v1 and v2 dispatch. A real
  failure established that this WSI stores only v2: ahbProperties=1, imageCaps=1,
  memoryCaps=0, import=-3. After the v2 fallback: three imports return success.
  The fallback has a mocked API/cleanup regression; real WSI gate exercises v2.
- Wayland XML build dependencies were absent from the trimmed SDK. Three public
  Wayland protocols XMLs were downloaded from wayland-mirror/wayland-protocols,
  pinned tag 1.36, into the private artifact directory. They are not device code.
  Generation now has file dependencies instead of rebuilding every protocol and
  all Wayland C++ objects on every incremental build. External SDK headers are
  system includes; project warnings remain errors.

Activation is only the explicit private runner switch `-PrivateAhbWsi`, an exact
SHA256 and `MAVK_WSI_AHB_ALLOC_ONLY=1`. No game environment/profile was changed.
Unsupported shapes/flags, mutable images and custom allocation callbacks reject.
The current gate is bounded to exclusive RGBA8, one layer/mip/sample.

## Next implementation, not another allocation benchmark

Connect private v3 AHB-backed wl_buffer/fences to this swapchain. Preserve display
release dependency in Vulkan acquire; receipt of wl_buffer.release alone is not
GPU completion. Add foreign/local queue ownership and GENERAL/PRESENT transitions
at the proper synchronization boundaries. Existing generic sync_queue_submit uses
BOTTOM_OF_PIPE for semaphore waits: do not reuse that stage blindly for commands
that access an image. Any internal submission sharing an application queue must
retain host synchronization; the installed common layer serializes its QueueSubmit
calls with the device mutex, but queue selection/ownership still needs validation.

Then run an ordinary Wayland Vulkan renderer. Wine/X11 SHM remains a separate
consumer boundary; this gate does not accelerate Muck or Windows U-AVF yet.

Production APKs, installed runtime/WSI, U-AVF, Windows/Ubuntu, firmware, CPU and
H.264 untouched. No commit/push, root or security-policy modifications.

## Regression and departure state

After the shared v2-dispatch change, both original cross-ABI pool programs were
rebuilt, not reused from an older binary. Short six-frame 1920×1200 regression
checks 13,824,000 pixels exactly; GPU SYNC_FD transfers in both directions and
CoreZero pass. This covers the legacy v1 memory-query branch while the standard
WSI allocation gate covers the v2-only branch. Logs/binaries:
`E:/Mavk/artifacts/ahb-adapter-memory2-20261023/` and adjacent native binary.
The mock API test also exercises both versions and seven failure stages.

Private consumers exited. Debug-only compositor app was force-stopped after the
tests (ordinary guest session was already IDLE), so no blank debug surface or
wrong-runtime-directory compositor is left for the next normal Steam launch.
No APK/data clear was performed. The user-requested stay-on-while-plugged setting
remains enabled and mStayOn=true. No daemon or benchmark remains running.
