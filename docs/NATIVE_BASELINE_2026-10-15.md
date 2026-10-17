# M0 native compute baseline — 2026-10-15

## Verified

An independent debug APK `org.mavk.probe` was built with local NDK28.2.13676358,
installed normally, and run in the Android application sandbox. No permissions,
VM, system properties, injected global layers or vendor/crosvm changes.
Three distinct app process runs returned `NATIVE_COMPUTE_CORRECTNESS_PASS`.
Each checked128 rounds ×65536 uint32 outputs =8,388,608 values with changing
shader push-constant seeds. Total across three reports:25,165,824 checked words.

Compute is dispatched through the stock Vulkan device. The harness explicitly
rejects a CPU device or non-ARM vendor instead of selecting a software fallback.
It uses shader-write→host-read barriers, fences, invalidation where necessary,
and destroys its Vulkan objects/device/instance before returning the report.
No validation layer was enabled, so this is not a validation/conformance PASS.

APK SHA256: `2ADEE8CCFC8A4D7B86F4CD37FD10B92E337289133C2E562171D06C0DBEB4C05C`.
Raw reports/build: `E:/Mavk/artifacts/native-compute-build1-20261015/`.
`device-native-compute-run1.json`, `run2.json`, `run3.json` are separate snapshots.

## Actual inventory

| Property | Observed |
|---|---|
| Device | Mali-G925-Immortalis MC12 |
| Vendor / type | 0x13B5 / integrated GPU |
| Loader / physical-device API | 1.4.0 / 1.3.278 |
| Driver version | raw205524992, not decoded to a marketing version |
| Advertised device extensions |152 |
| Queue family0 | flags23,2 queues,64 valid timestamp bits |
| Reported Vulkan12 features | timelineSemaphore, bufferDeviceAddress, descriptorIndexing |
| Reported Vulkan13 features | dynamicRendering, synchronization2 |
| Other queried features | shaderInt64 yes; shaderFloat64 and sparseBinding no |

These features were queried, **not enabled/exercised by this minimal shader**.
No claim of feature emulation or full descriptor-indexing compatibility.

Selected extension inventory, not a functional import/export test:

- Present: Android HardwareBuffer external memory, external_memory_fd,
  external_semaphore_fd, external_fence_fd, swapchain, transform_feedback,
  maintenance5, dynamic_rendering, synchronization2.
- Absent: external_memory_host, descriptor_buffer, robustness2.

External-memory extensions do not establish cross-VM/GZVM mapping support.
Missing robustness2/descriptor_buffer must be evaluated against exact consumer
requirements; do not advertise them or promise universal DXVK compatibility.

## Timing (run1)

Warm sample set: rounds16..127; all128 raw samples also retained.

| Operation | p50 ms | p95 ms | p99 ms |
|---|---:|---:|---:|
| CPU command recording |0.010846|0.021538|0.022154|
| vkQueueSubmit host call |0.021847|0.043077|0.062770|
| CPU fence wait |0.873538|3.832920|5.219770|
| CPU verification/readback + timestamp retrieval |0.551077|4.557150|5.349770|
| GPU timestamp command span |0.020846|0.034385|0.036923|

This is a **tiny compute correctness workload**, not a game/render benchmark.
Fence wakeup, CPU scheduling, GPU startup and verification have different costs.
No conclusion about60FPS, game performance, Turnip parity or Mavk overhead.
This section is the original direct-only checkpoint. Subsequent Core A/B is
recorded in STATE.md and CORE_AB_2026-10-15.md; these original timings are not
paired measurements and must not be used alone to infer Core overhead.

## Next gates

1. Graphics/offscreen correctness and moving Android Surface workload.
2. In-process allocation/teardown and Surface recreation stress; app-local
   validation, without modifying global Android graphics configuration.
3. Extract the minimal reusable context/resources/submission core; compare the
   same workload direct vs core, with raw timings and equivalent settings.
4. Only then guest ICD/command-bridge and DroidDeck ABI proofs. WDDM remains a
   separate feasibility gate; this native PASS does not accelerate Windows DWM.
