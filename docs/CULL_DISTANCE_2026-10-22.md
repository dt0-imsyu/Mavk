# First whole-primitive CullDistance mechanism

Update:experimental automatic pipeline-hook integration verified,including
UBO+guarded mode. CULL_PIPELINE_LINK_2026-10-22.md supersedes initial
"no automatic interception" status below. Full public feature remains false.

This is implementation + GPU execution,not feature admission or upstreamDXVK
PASS. APK25/runtime4de26844 stays installed,unchanged. No new APK/default layer,
firmware,U-AVF,Windows,Ubuntu or user-data changes. No commit/push.

## Why this P0

Pinned ordinary DXVK requires shaderCullDistance;raw G925 reports0 while
geometryShader=1. Vulkan requires culling a whole primitive when one cull plane
is negative at every vertex. Per-fragment discard cannot substitute this rule.
References:
https://docs.vulkan.org/refpages/latest/refpages/source/CullDistance.html
https://docs.vulkan.org/spec/latest/chapters/vertexpostproc.html
https://docs.vulkan.org/spec/latest/chapters/interfaces.html

## Common implementation

compatibility/shaders/cull_distance.cpp and public compat/cull_distance.hpp:
validated VS/FS SPIR-V input,reflect output types/locations/cull arrays,replace
unsupported VS builtins with internal varyings,and generate native GS that
tests all vertices per plane before emitting anything. Restore Position
builtin identity at the final native stage;pass supported user
outputs without shader source recompilation. No fragment kill is injected.
No shaderCullDistance request reaches the native device.

VS output blocks must be mirrored as GS input blocks. Initial separate inputs
produced no pixels. A GLSL pass-through with same separate interface also
produced none;constant-output GLSL GS rendered the288reference pixels. Mirroring
the actual block interface corrected the real shader output. This localizes
the required linking representation;it does NOT prove a general Mali GS bug.

Separate static library mavk-cull-distance uses existing SPIRV-Tools validator/
assembler. Existing guard compiler/archive pins unaffected. Native shared
OffscreenSession RasterOptions gained an optional geometryShader vector,empty
by default. Caller must enable geometryShader and check actual native limits.

## Proof

Host:6valid builds,float+flat signed/unsigned32varyings across points/lines/
triangles,5negative contracts. Only triangle/float subset has actual GPU proof.
glslc itself rejects9cull planes;this is not counted as a Mavk negative test.

Actual Mali-G925,existing app-owned DroidDeck runtime/loader,geometryShader
enabled and shaderCullDistance reported0/requested0.8triangle cases,2planes:
all-positive,mixed signs,all-negative on either plane,zero,negative zero,
different-negative-vertex per plane,restore after cull. Surviving images equal
the independent no-GS/no-cull VS render byte-for-byte;culled images are clear.
Reference coverage288red pixels. This rules out partial fragment clipping in
the mixed-sign cases.32768bytes each with layer guards0 and1,65536total PASS.
Ordinary routing+16image-subresource cases using newly compiled shared native
renderer also PASS;CoreliveZero1,gaps0. No CTS/validation-layer/performance/FPS
claim. Exact final compiler outputs match the GPU-tested shader bytes.

Artifacts:E:/Mavk/artifacts/cull-distance-{compiler,gpu,detail-consumer}-20261022/.
GS SHA440758d4f2d26b0474e2d8bf6a2131b09374070ff670405072b5c230544ae280;
VS SHA1179a6c74721b8c57a55d18d0eff5c039fa910d1d41a3da3ff2e5c8141ea9606;
GPU archive2ee8181ecfc75428f7a2e88e06ffa2e921509c90ce3492a980521b75d5d3da8b.
Verifier compatibility/test/verify-primitive-cull.py.

## Explicit remaining scope / next action

No automatic vkCreateGraphicsPipelines interception,device-feature mediation,
Bridge/ICD linkage or ordinary DXVK cull execution yet. This is a reusable
compiler API executed through standard native Vulkan,not a complete driver.
Points/lines and integer varyings validated on host only;not GPU-proven.
Rejects fragment CullDistance/PrimitiveId inputs,clip-distance combinations,
XFB/multiview,user output blocks lacking linked forwarding,group decorations,
unrecognized widths/types/decorations and multiple entry points. Existing GS/
tessellation paths are not transformed. Full feature stays false.

Next:explicit pipeline-link integration and dependency/limit admission for
supported VS->GS->FS contracts,then extend actual consumer-needed semantics.
Do not advertise all shaderCullDistance because eight triangle cases pass.
Do not go back to descriptor/logging microtests or a new benchmark framework.
