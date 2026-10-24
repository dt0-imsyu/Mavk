# Cross-ABI AHB pool and actual GPU-fence handoff — 2026-10-23

## Result

GO for the resource/synchronization prerequisite of a private AHB-backed WSI
adapter. This is NOT a finished compositor/Surface integration, a zero-copy game
result, a CTS PASS, or evidence of improved game FPS.

Stock Tab S11 SM-X736B, Mali-G925-Immortalis MC12, Android app UID10261. Producer
is glibc through the installed guarded Mavk/common runtime and existing hybris;
consumer is a separate native Bionic process using stock Vulkan directly.
No root, security changes, VM launches, APK replacement, or game config changes.

## What now works

- Public AHB allocation/import/handle transfer, including COMPOSER_OVERLAY usage.
- Real SYNC_FD semaphore export in glibc and temporary import in native Vulkan.
- Three persistent AHB allocations and matching dedicated Vulkan images imported
  once per process, then reused for90 different color frames at1920x1200.
- Explicit FOREIGN/GENERAL ownership transfers and GPU semaphore dependencies in
  BOTH directions. Return fence gates the next writer; no guessed dma-buf fd or
  modifier and no CPU pixel payload crosses the process socket.
- Real submitted work, not just an advertised capability. Producer exported90
  pending fences; consumer imported3 while still pending. Producer imported1
  pending return fence. No blocking host fence-poll operation.
- Every frame is independently GPU-copied to a native diagnostic readback buffer
  and all pixels are verified. Producer does not read back pixels.

The private wire is same-UID, bounded, version/mode checked. Version2 is the
single-frame protocol; version3 is the three-slot session. Mixed versions fail
closed. Frame sequence is checked in every fence message; AHB identity and full
descriptor are checked at initial import. SYNC_FD fd=-1 is handled as an already
signaled fence, not as an absent dependency. Import transfers FD ownership;
exported originals are closed after send. Process-owned resources are destroyed
after completion, including intentional oracle failure.

## Measurements and controls

Final instrumented full-size run:

| Measurement | Observed |
|---|---:|
| Frames / fixed slots | 90 / 3 |
| Verified pixels | 207,360,000 |
| Native elapsed interval | 1,198.985 ms |
| CPU pixel oracle within interval | 1,183.235 ms |
| Native command-resource recycle waits | 1 / 1.306 ms |
| Producer command-resource recycle waits | 0 |
| Pending fences at export, each side | 90 |
| Pending fences at import, native / glibc | 3 / 1 |
| Pixel bytes sent over socket | 0 |

Do NOT label90/1.199 as visible FPS, or subtract the oracle and advertise a GPU
limit. There is no Surface or real shader draw here; the GPU workload is a
changing clear plus the diagnostic full-image copy. Socket request/return is
serialized, not an optimized presentation scheduler. Own command resources are
retired with a bounded fence wait if required before reset; no claim that all
CPU/GPU waits have been eliminated. Cleanup uses DeviceWaitIdle after the test.

Earlier byte-wise CPU oracle took8,003.832ms for the same90 frames. Replacing
only that test oracle with exact RGBA32 comparisons reduced overhead; it did not
optimize Mavk or any game. An intermediate final run took1,217.676ms. These are
diagnostic timings, not a causal product FPS improvement.

Controls:

-64x64/90frames passed before the full-size run.
- Intentional expected-color error at sequence7 failed on that slot's next
  retirement: actualff332218 vs expectedff332219. Both sides exited3, producer
  stopped instead of treating release-fence receipt as a pixel PASS.
- Version2 synchronous path still passes producer17/91 and native read-only91.
- Installed common nested-buffer GPU regression passed after the changes:
  guardsON, actual installed layer mapped, readZero/writeSuppressed, gaps0 and
  Core liveZero. No runtime feature bits changed.
- New private helpers do not change installed APK27/commonf108/WSId07b.
  Steam/Muck were NOT rerun for these probe-only changes; their earlier APK27
  lifecycle checks are recorded separately, not claimed as fresh game tests.

## Artifacts / reproduce

`E:/Mavk/artifacts/ahb-syncfd-20261023/` contains single-frame async logs,
pool64/full-size logs, intentional-negative logs, synchronous regression,
common-nested-regression.log, native helpers and source-capsule.tar.gz.

Final measured pair:

- Producer `E:/Mavk/artifacts/ahb-pool-observed-20261023/ahb-interop`
  SHA256 a7fca98db432ab16e93d760ed649368934d8f872cd116e43243a74e1939f0b6c.
- Consumer `ahb-syncfd-20261023/pool-receiver-observed`
  SHA256695672589bc3b66b11fa5f3b0941917685ee5b566fe927d709c16087e87954fc.
- Logs native-pool-observed.log and glibc-pool-observed.log.

Build with existing build-ahb-interop.sh using a fresh MAVK_AHB_OUTPUT and
build-ahb-native-receiver.ps1 using a fresh output file. Stage matched binaries
under app-private files/mavk-ahb, preserving earlier binaries. Start receiver
with a fresh private socket path and argument `pool`. Run producer with
run-headless-probe.ps1 -UseInstalledLayer, MAVK_AHB_COMPOSER=1,
MAVK_AHB_FULL_SIZE=1, MAVK_AHB_POOL_FRAMES=90 and matching MAVK_AHB_SOCKET.
For the negative oracle use receiver `pool-negative`; expect nonzero on both.
Do not use old V1 native binaries with the new producer.

## Next useful implementation boundary

Do not extend this headless clear benchmark. The next useful result must include
an actual app-owned Android Surface and then a real WSI consumer.

Existing banner_ahb_v1 version2 in DroidDeck requires an already valid dma-buf
wl_buffer and assumes implicit dma-buf fences. The current Mali X11 SHM WSI is
not such a buffer. Tested explicit AHB/SYNC_FD interoperability does not magically
make that protocol suitable or make the SHM image a shared gralloc allocation.

Implement an opt-in AHB-native buffer association with explicit acquire/release
fences, preserving old protocol/SHM fallback. First prove SurfaceControl import,
release, resize/disconnect on an isolated path. Then connect the real glibc WSI
source, account for the X11/Xwayland versus Wayland client boundary, and measure
actual unique Surface frames and game behavior. No native_handle fd0/modifier
assumptions. No current evidence that this fixes Windows BasicDisplay/WDDM or
AVF/GZVM memory registration. U-AVF can reuse the resource model behind its GPU
command bridge only after separate integration.

Production U-AVF, Windows, Ubuntu, CPU/firmware/storage/H.264 untouched. No
commit/push. Device /data still about35GiB free, below requested50GiB; no personal
files/games/VM disks deleted and no claim that the storage target was reached.

## Primary references

- https://developer.android.com/ndk/reference/group/a-hardware-buffer
- https://docs.vulkan.org/refpages/latest/refpages/source/VkImportSemaphoreFdInfoKHR.html
- https://docs.vulkan.org/refpages/latest/refpages/source/vkGetSemaphoreFdKHR.html

