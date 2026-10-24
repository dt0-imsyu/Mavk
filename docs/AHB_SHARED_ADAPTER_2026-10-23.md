# Shared AHB image adapter — 2026-10-23

## Actual change

The proven image import path is now `adapters/android/ahb_image.c`, with a C ABI
header usable from C++17. Existing glibc AHB producer and independent native
Android consumer both link that implementation. Their pool/fence/pixel tests
were reused, not replaced with a new synthetic performance benchmark.

The adapter checks the actual AHB description, requested image usage,
importability/extent/sample capabilities, actual buffer format features and
memory-type mask, then creates an optimal image and dedicated imported memory.
Failed allocation/binding is transactional: partially created objects are freed,
and the output stays empty. It has no implicit wait, transport or WSI policy.
Only the tested RGBA8, 2D, single-layer/mip/sample subset is supported. No Vulkan
extension/feature advertisement changed.

## Real G925 regression

Final independently linked binaries:

- glibc: `E:/Mavk/artifacts/ahb-adapter-final-20261023/ahb-interop`, SHA256
  `4a64a6d4fcd9dede0d3dd53024a9c0b30beb8a6a43ad4debcb51c92160353cf4`.
- native: `E:/Mavk/artifacts/ahb-adapter-native-final-20261023`, SHA256
  `b53ddb122b20594be9491843e3f35f03445c7e340de27834f30be4aac14e253e`.

Logs in `ahb-adapter-final-20261023/` prove three persistent 1920×1200 buffers,
90 changing frames, all 207,360,000 pixels checked by independent native GPU
readback. Producer sourceReadbacks=0, pixelPayloadBytes=0. Actual pending
SYNC_FD imports: native 3 / glibc 1; pending exports 90 per side. No host fence
poll waits. Glibc interval 1210.482 ms; native CPU oracle 1191.990 ms. These are
correctness intervals, NOT visible FPS or frame latency.

Negative control deliberately expected the wrong value: native reports
`POOL_PIXEL_FAIL submit=3 pixel=0 got=ff332218 expected=ff332219` and the glibc
wrapper reports failure 3. It does not silently report success.

Separate native mocked API test passes seven failure stages, invalid usage,
occupied output and success/idempotent cleanup. This tests ownership logic,
not Mali execution. Native warning-clean C11 compilation and C++17 public-header
syntax check passed. Wayland producer also rebuilt against the shared adapter
before the final format-feature check; its latest source requires a fresh build
and actual Surface regression before any replacement of the earlier visible
producer. No new visible run was made: Android is currently locked.

## WSI integration boundary from current source

Existing Arm WSI Wayland image factory uses DMA_BUF memory and format/modifier
selection; simply substituting a private Wayland buffer factory is insufficient.
In addition, `swapchain_base::acquire_next_image` imports an already-signalled
SYNC_FD (-1) into application acquire objects once it finds a FREE image. A new
AHB backend must represent the compositor release dependency before permitting
reuse, not treat receipt of a release event alone as GPU completion. The private
producer already imports that dependency; it must not be lost during WSI work.

Next: isolated AHB backing-memory/image-factory adaptation plus matching WSI
acquire/present dependencies. Only then run a standard Wayland Vulkan consumer.
Wine/X11 SHM remains a separate integration boundary. No claim that games are
already using this adapter or have become faster.

Installed APK28, common Mavk layer, system WSI, U-AVF/VMs and data unchanged.
No commit/push. Final tablet tests were offscreen and needed no unlock.

Reference: https://docs.vulkan.org/spec/latest/chapters/memory.html
