# G925 AHardwareBuffer cross-ABI / cross-process GPU proof

Result: **GO for an isolated AHB-backed WSI implementation**, not a display/FPS
or production zero-copy PASS. No installed APK, compositor or WSI changes.

## What actually works

On stock Tab S11, under the existing experimental DroidDeck app UID:

1. glibc process loads existing libhybris/stock Android Vulkan and Mali-G925.
2. Public Bionic `AHardwareBuffer_allocate` is resolved through existing hybris.
   No private gralloc layout parsing, guessed dma-buf plane or vendor workaround.
3. RGBA8 GPU buffer imports into Vulkan with dedicated memory, actual queried
   allocation size/type bits and enabled AHB/foreign-queue extensions.
4. GPU clear/transfer writes two distinct image contents; both are checked via
   GPU readback against every pixel. This is not a shader/raster draw test.
5. Ownership is released to FOREIGN in GENERAL after completed GPU work.
6. Public AHB handle transfer sends the resource to a separate Bionic executable.
   Both endpoints check same UID. IDs and full image descriptors agree.
7. The native process creates its own stock Vulkan device on Mali, imports that
   same allocation, acquires foreign ownership preserving GENERAL contents, and
   copies it to its own oracle buffer without clearing/writing the source image.
8. Every native-read pixel matches the producer's final GPU contents.

Final 1920x1200 pass uses installed common Mavk layer f1081e2c, not the private
CSE candidate. Exact mapped path is enforced in the producer. Guards on,
diagnostics off, no fake features, layer gaps0/CoreLiveZero.

## Results

| Case | Result |
|---|---|
| AHB allocation, 64x64, GPU color + sample | PASS |
| Dedicated Vulkan import + two GPU-written patterns | PASS |
| Same operations at1920x1200, all2,304,000 pixels each | PASS |
| GPU color + sample + COMPOSER_OVERLAY usage0xb00 | PASS |
| Same AHB ID / descriptor after glibc -> Bionic IPC | PASS |
| Independent native Vulkan GPU read,64x64 | PASS |
| Independent native Vulkan GPU read,1920x1200 | PASS |
| Negative: native expects red42, producer wrote91 | Expected FAIL, observed91 |
| Installed common nested-buffer guard regression afterwards | PASS |

Final large resource: AHB/Vulkan format1/37, stride1920,
allocation9,363,456B vs tightly packed oracle9,216,000B. Do not infer DRM modifier
or memory layout from that size/stride. The native driver handles import layout.

Final large buffer ID20946555502592 agrees between producer PID4877 and receiver
PID4870. Native hardware device reports Mali-G925-Immortalis MC12. Native checks
2,304,000 pixels, red91/green34/blue51/alpha255 (UNORM tolerance1), readOnly=1.
The negative native checker reads91 but expects42, emits pixel-oracle failure,
returns ACK status3 and propagates nonzero failure to the producer. No PASS is
emitted for failed native content. Oracle resources are destroyed on this expected
mismatch path before device destruction.

## What is and is not zero-copy here

The Unix channel carries AHB handle metadata/file descriptors, not a pixel array.
The application acknowledgement is32 bytes; this excludes the AHB serialization
metadata, not a measurement of total socket bytes. Shared allocation identity and
native GPU contents are independently checked.

The test deliberately DOES perform GPU-to-CPU readback on each side for its pixel
oracle. It also waits for producer completion before publication. Therefore this
is **not** a finished copy-free/fenceless presentation path. No Surface,
SurfaceFlinger presentation, actual visible FPS or game acceleration was measured.

One-shot metadata-only handoff ACK timings were8.6/13.9ms in earlier fresh native
processes; final ACK including cold native Vulkan initialization, import, full
readback and CPU verification was132.860ms. Neither is steady-state frame latency.
Allocation was5.2–8.5ms in those one-shot runs, also not a per-frame requirement.
Do not allocate, import, create VkDevices or run pixel oracles each production frame.

## Integration boundary / next implementation

The resource/ABI boundary is not blocked by Samsung policy in this app-owned
DroidDeck case. No root, unlock, flash, host SELinux/security-property changes,
unsupported Binder calls, manual physical-memory mapping or user login was used.

The existing compositor's `ahb_swapchain.c:ahb_attach` accepts AHB only when the
Wayland buffer is a recognised dma-buf. Current Mali X11-SHM output does not meet
that contract. Do NOT merely enable its zero-copy flag, assume a native-handle
fd index/modifier, or switch the production backend blindly.

Next practical work: an opt-in AHB-backed Wayland WSI/buffer adapter compatible
with the existing compositor, a registered reusable buffer pool, proper release
and acquire fences (query/test SYNC_FD support, not just an extension string),
then real moving Surface output and ordinary Wine/D3D11 integration. Preserve
current SHM fallback. This is software integration work still to be implemented.
Do not build a separate final display shell or call the diagnostics a game backend.

This does not bypass AVF/GZVM memory registration or make the existing Windows
BasicDisplay framebuffer shareable. U-AVF can potentially reuse the native AHB
target behind its GPU command bridge; guest/ICD/WDDM integration remains separate.

## Reproduction and artifacts

Source: experiments/droiddeck/ahb-interop.c,ahb-image-check.h,ahb-send-check.h,
ahb-ipc.h,ahb-native-receiver.c,ahb-native-gpu-check.h. Existing Vulkan probe
dispatch is reused; producer and receiver reuse the same image/oracle code.
Native helper uses public NDK API31 because its diagnostic identity check uses
AHardwareBuffer_getId. Older-device portability is not proved.

Build recipes: build-ahb-interop.sh (fresh MAVK_AHB_OUTPUT) and
build-ahb-native-receiver.ps1. run-headless-probe.ps1 now supports
`-UseInstalledLayer`; probe environment overrides are restricted to MAVK_AHB_*.
The socket is private, same-UID, bounded30s accept/5s I/O; the helper removes only
the socket it successfully bound. No helper is left running.

Latest artifacts:
`E:/Mavk/artifacts/ahb-cross-gpu-final-20261023/`:
native-gpu1920.log,glibc-gpu1920.log,native-negative.log,glibc-negative.log,
common-nested-regression.log and native binary.
Sender: `E:/Mavk/artifacts/ahb-cross-gpu-sender-final-20261023/ahb-interop`.

- Sender SHA256: ee9911f07fa6a67ff6d813d514316af66b706c2cd9477e29494355278f3213f3
- Native receiver SHA256: f94b29e6a85df47248ddaaa1137d40e89310379942fb7fd5b44428f02c6cb35f

Earlier prerequisite/image/metadata-only results are separately retained in
ahb-interop,ahb-image-interop,ahb-ipc-interop,ahb-composer-ipc and
ahb-installed-control artifact directories dated20261023. They are earlier
binaries; do not mix metadata-only timing with final native-GPU verification.

Installed APK27/common layer/WSI unchanged. Windows/Ubuntu/P7/H264 untouched.
No commit/push. Strategic Mavk performance/compatibility goal remains incomplete.

## Primary API references

- [Android NDK AHardwareBuffer](https://developer.android.com/ndk/reference/group/a-hardware-buffer)
- [Vulkan AHB import](https://docs.vulkan.org/refpages/latest/refpages/source/VkImportAndroidHardwareBufferInfoANDROID.html)
- [Vulkan external image layout/sharing rules](https://docs.vulkan.org/spec/latest/chapters/resources.html)
