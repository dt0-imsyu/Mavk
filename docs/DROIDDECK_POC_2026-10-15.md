# Bounded DroidDeck / G925 libhybris PoC

## Result and scope

**Hardware Vulkan + correct offscreen graphics PASS in a minimal glibc/proot
runtime-component test. Not a complete DroidDeck session, presentation or game PASS.**

No DroidDeck package was installed on the tablet. Instead of installing Steam
or modifying another app, a fresh disposable26MiB directory under Termux's debug
UID was used. The test uses the exact proot/loader and required Linux libraries
from DroidDeck linuxfs-r9, plus the pinned libhybris component from the reviewed
system-Vulkan fork. Android context is `runas_app`, not DroidDeck's own app UID;
an actual APK/UID/session test remains necessary for product integration.

Two bounded launches within one PoC:

1. Explicit glibc loader + pinned Vulkan ICD: hardware G925,3330 pixels correct.
2. Same workload through the r9 proot binary and Android partition binds:
   hardware G925,3330 pixels correct, exit0.

Same triangle SPIR-V as Mavk's native correctness baseline; real graphics pipeline,
RGBA8 render target, image→buffer copy, explicit barrier/fence/invalidate, CPU
comparison of interior/exterior pixels. No CPU renderer fallback. Optional device
features and G715/DXVK workaround flags were not enabled. No broad emulation claim.
Process failures use OS teardown; this is a bounded diagnostic C program, not a
production renderer. The capture/readback copy is explicitly for correctness.

## Capabilities / DXVK preflight

Actual glibc device: Mali-G925-Immortalis MC12, vendor0x13b5, integrated type1,
Vulkan1.3.278, driver raw205524992. Geometry/tessellation true; multiViewport and
fillModeNonSolid false; pushConstants256. `VK_EXT_robustness2` absent.

The pinned fork's DXVK patch requires fillModeNonSolid/multiViewport unless its
experimental Mali profile matches exact identity/driver guards. This driver does
not match required0x0d803000, so even `DXVK_MALI_COMPAT=1` cannot enable that fallback.
The profile is incomplete even for its tested G715 and does not implement general
Direct3D semantics. **Do not widen the guard or fake capabilities for G925.**

D3D11/Wine was not executed: no Wine/Proton installation exists in this minimal
environment, and the current pinned DXVK feature admission already fails the
capability preflight. This is **NO-GO for the ready-made pinned DXVK profile**, not
an observed D3D HRESULT or proof that all DXVK versions can never work on G925.
Adding a full Proton environment or G925 emulation is outside this short PoC.

## Costs / Mavk Native

No FPS/zero-copy/optimality claim. Offscreen proof does not exercise X11/Wayland
swapchain presentation, gamescope, cross-process AHB or full-frame display copies.
The existing libhybris ABI path is functional; no measured bottleneck justifies
rewriting it. Its stderr includes loader lookup warnings and denied optional Mali
sysfs parameter read; neither prevented correct hardware rendering. Logs retained.

Mavk Native's Bionic binary cannot simply be dlopened by glibc. A glibc build of
Core still needs a functioning ABI/ICD boundary; a remote Bionic Core instead needs
Bridge serialization and synchronization. Neither automatically removes WSI copies
or makes unsupported Vulkan features real. Keep the working ABI proof; profile
real presentation only once an actual consumer is available. Return to U-AVF
Bridge now, rather than developing a second full compatibility project here.

## Provenance / artifacts

- Upstream DroidDeck main inspected at `6e49c0c2879abcfbbfa40b977eb63bf3d7a32f4c`.
- System-Vulkan fork inspected at `0916a538aff4c08f799a4bd9fa17dd3c98b01e99`.
- linuxfs-r9 archive790,907,606B, SHA256
  `d3240c1531531615b0dd06b6ad56e92db9015a8641eb18c3a613835ae45992e7`.
  Only required libraries and opt/android-host extracted on tablet (26MiB total).
- Fork-pinned hybris4073873 archive5,758,059B, SHA256
  `014fa3501723c6dddc50750a58170f74a8b1d1e6d558e44deec37beb72e0bbd7`.
  Upstream libhybris source `bc8cb3586b79d208fd21b925fa0209b3e432e008` plus fork
  patches, included in the component's original licenses/source. Not Mavk-owned code.
- Host-only cross headers/CRT from Ubuntu ARM64 cross packages; executable linked
  with glibc, NOT NDK/Bionic. Probe SHA256
  `e8b0c34f3f60ddfd810aa4c85c12d76cb5b71c83e7fed48c493daa9257580877`.
- Logs: `E:/Mavk/artifacts/droiddeck-poc-20261015/{run1,proot-control}/`.
- Sources/build/runner/preflight: `Mavk/experiments/droiddeck/`.

References (source, not evidence that this tablet runs games):

- https://github.com/Droid-Deck/DroidDeck
- https://github.com/wallentx/DroidDeck/pull/6
- https://github.com/wallentx/DroidDeck/blob/0916a538aff4c08f799a4bd9fa17dd3c98b01e99/app/src/main/java/com/droiddeck/launcher/gpu/SystemVulkanDriver.java
- https://github.com/wallentx/DroidDeck/blob/0916a538aff4c08f799a4bd9fa17dd3c98b01e99/tools/dxvk/mali-patches/0001-experimental-mali-capabilities-and-guards.patch
- https://github.com/Droid-Deck/DroidDeck-Components/releases/tag/linuxfs-r9

No root/unlock/flash, SELinux modification, firmware or VM changes. No commit/push.
