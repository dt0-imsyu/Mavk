# RGBA8 bound texel-view metadata — bounded GPU PASS

Real trigger: baseline Steam reported vkCreateBufferView format37 UNORM missing
from Mavk metadata. This gap is independently reproduced, NOT established as the
cause of the separate Big Picture crash.

Shared registry now recognizes four-byte RGBA8 UNORM/SNORM/USCALED/SSCALED/UINT/
SINT/SRGB native views. It does not create views, advertise format capabilities,
add typed-null formats or broaden robustness2. Native vkCreateBufferView still
validates format features/usage/alignment. Existing32bit scalar/vector coverage,
explicit-range checks and WHOLE_SIZE trailing-texel truncation are preserved.

Source: [VkBufferViewCreateInfo valid usage and range semantics](https://docs.vulkan.org/refpages/latest/refpages/source/VkBufferViewCreateInfo.html).

## Evidence

- Host texel regression53checks/39negative cases, including all7metadata sizes,
  WHOLE_SIZE truncation and invalid explicit ranges. Nine other descriptor
  suites rebuilt/passed against new headers.
- New compact mode in EXISTING real Vulkan loader consumer, not another
  runtime: two UNORM/UINT sampled buffer views,64texels/four components each,
  nonzero aligned offset,258remaining bytes (two trailing bytes discarded).
  All512readback components and input/padding sentinels checked on Mali-G925.
  Native/protected outputs identical; guarded2draws/2submissions,gaps0/Corezero.
- Baseline common219b5bf0 rejects the SAME test at format37 with unsupported
  metadata and expected dependent tracking errors,exit1. Preserved separately;
  it is not part of positive gaps0 evidence.
- New candidate strict dynamic/selected/empty/unused/routing/reuse/in-flight
  regressions PASS, including used-unwritten and missing-used pre-submit
  rejection. Direct/grouped immutable192component regressions PASS.
- Muck PID7942 mapped isolated new library:80209protected executions,gaps0/
  device-lost0 at capture. Actual menu/world image captured in muck-screen.png;
  overlay says0fps, not a trustworthy present measurement, no FPS claim.
  Game stopped and per-game candidate override removed.

Artifacts `E:/Mavk/artifacts/layer-rgba8-consumer-20261021/` contain
texel-results.tar.gz,reg-results.tar.gz,baseline stdout/stderr, strict normalized
mode reports,game log and screenshot. Host artifacts `rgba8-host-20261021`.
Layer `mavk-rgba8-layer-20261021` SHA256
bc30f5411f6192f8f58cabf66bb620bd721b2514b9dc166729eacb1078123aec.

Runtime rollup161fe2e8331cf4da1dd88ab1a53e6b155606256e288803c055049deb0c930b6a,
only layer+manifest changed;261other runtime and485other APK members unchanged.
APK18 SHA23de7423a9a89bf1cce936384d93e5faba3293993726b7ddbe3639818aee0ca7
installed in place/same test certificate. Source fork assets match runtime161f.
COMMON native-view oracle512components passes,gaps0/Corezero. Normal Big Picture
session2026-10-21-10-steam reaches READY and keeps Steam11074 alive with actual
-gamepadui/-steamdeck args,session gaps0. CDP reports hardware ANGLE/Zink/Mali.
Earlier APK16 startup failed, but this single successful new startup is not
proof of exact crash causality or exhaustive startup stability. No random flags.
Muck PID12020 then maps COMMON161fe2e8 with no per-game layer override:
11569protected executions,gaps0/device-lost0 at capture; saved muck-common.log.
Game stopped after bounded regression, Steam left running; original suspend
preference stays absent/default manual. No visible-FPS claim.
APK17/acda20c7 retained as rollback. No production U-AVF,
Windows/Ubuntu, firmware, SMP, display or data change. No commit/push.

## Remaining boundaries

This does NOT prove GPU behavior for all7variants or storage RGBA8, nor null
RGBA8, image atomics, other texel widths/packed formats or all shader operations.
The extra7variants record byte size of successfully created NATIVE views; their
individual driver capabilities are not asserted. NULL texel backing remains the
existing bounded R32 slice. General UAB/partial arrays remain missing.
