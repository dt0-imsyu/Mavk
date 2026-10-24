# Vertex robustness: bounded native semantic result

Goal: avoid implementing a new vertex-fetch emulation merely because the Mali
driver does not advertise robustness2. Descriptor guards do not intercept
fixed-function vertex fetching. This is one required part of the upstream
DXVK P0 contract,not a conformance claim.

## Executed workload

Native robustBufferAccess is queried and explicitly enabled;no unsupported
feature is enabled. Same existing Core/offscreen renderer,not a new runtime.
Two formats:R32_SFLOAT andR32G32B32A32_SFLOAT,3vertices with actual buffer
sizes12/48bytes. Fullscreen triangle positions use gl_VertexIndex%3;firstVertex
0(valid),3(outside),3000000(far outside). Six rendered images,1536components.
Valid nonzero input has a positive pixel oracle. Outside reads must yield zero
raw components;R32 format substitutes absent alpha1,RGBA32 keeps raw alpha0.
UNORM readback allows1unit conversion tolerance.

## Result

PASS on Mali-G925 with candidatec41b2f2d AND control without Mavk (hybris-only).
Thus no new Mavk emulation was required for these tested native paths.
Protected logs:descriptor-free stages2/native1;pipelines6/submissions6;gaps0,
CoreliveZero1. Guarded draw counter0 is expected for descriptor-free pipelines;
it is not a missing draw or a performance metric.

Consumer SHA2562751fae2bcce2b0ae579428c17b621656d4cf66611834c4fb3d04262d7de80db.
E:/Mavk/artifacts/vertex-robust-consumer-20261021/results.tar.gz contains both
native/protected stdout/stderr. Sources:layer-vertex-robust.hpp and
vertex-robust.vert/.frag;control:vertex-robust-control.sh.

## Still not proved

Partial attributes,packed/integer formats,instancing/divisors,indexed fetch,
dynamic binding sizes/strides and complete implementation limits. No CTS run.
No robustBufferAccess2 feature enabled/advertised,no all-vertex-support claim.
Do not replace known native paths with buffer copies based on speculation.
InstalledAPK19,Steam and production U-AVF unchanged.

Required semantic source:
https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_robustness2.html
