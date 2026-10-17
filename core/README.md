# Mavk Core — first native resource layer

`include/mavk/core.hpp` / `src/core.cpp` build separately as `libmavk_core.a`.
No JNI, Activity, Android Surface or AVF dependencies in this library. The test
harness links it next to the unwrapped Vulkan control path.

Current responsibilities:

- Buffer/image allocation, binding, mapped-memory flush/invalidate.
- SPIR-V shader modules and compute/graphics pipeline ownership.
- Command-batch and fence lifetime, explicit submission and host-read barrier.
- Explicitly enabled native timeline semaphores; RGBA mip upload/sampler ownership.
- Live-resource accounting for harness assertions, not an independent leak detector.

The caller owns the Vulkan instance, physical device, device and queues, as well
as descriptor layouts/sets, pipeline layouts, renderpasses, image views and WSI.
Keep `DeviceContext` alive longer than its resources and keep `VkDevice` alive
longer than the context. Externally synchronize queue access. Complete GPU work
before destroying referenced objects. Destructors do not hide a `vkDeviceWaitIdle`
per resource. Context counters are single-threaded / externally synchronized.

Core does not change supported features or shader semantics. It does not emulate
missing Vulkan extensions, implement an ICD, serialize commands, provide a shader
compiler, or provide a Windows platform GPU driver. The separate experimental
Windows ICD/Bridge already consumes Core for bounded standard Vulkan apps;
that is not WDDM/DWM acceleration. Direct/Core comparison workloads retain
identical GPU commands.

Tests are in `../harness`: `CoreComparisonActivity` runs same-device ABBA compute
and clear/copy/readback tests. `GraphicsActivity --ez core true` uses Core-owned
resources for the existing graphics workload; `false` retains the direct control.
Pixel verification, fences and retirement are real. App-local validation and
synchronization validation are exercised by bounded Core/graphics/lifecycle
gates. See `../docs/FEATURE_SUITE.md` for exact coverage and source reports;
this is not universal Vulkan/CTS or application compatibility coverage.
