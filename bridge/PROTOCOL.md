# Mavk Bridge v1 — bounded vertical compute slice

This is a new protocol, not wire-compatible with Venus/gfxstream and not itself
a Vulkan ICD. Initial compute/triangle commands use trusted host shaders. The
experimental ICD offscreen command now accepts bounded guest SPIR-V; it is NOT
a semantic shader validator/security sandbox or public endpoint. Native memory
addresses, OS handles, native structure layouts and arbitrary file paths are
not accepted. Windows compute/trusted-graphics and limited standard API offscreen
render PASS; Ubuntu integration and WDDM/DWM are not yet proven.

Little-endian header, exactly32 bytes:

| Offset | Type | Value |
|---|---|---|
|0|u32|magic0x314b564d (`MVK1`) |
|4|u16|version1 |
|6|u16|header size32 |
|8|u32|opcode |
|12|u32|flags: request0, response1 |
|16|u64|nonzero request ID, strictly increasing per connection |
|24|u32|payload bytes, <=1MiB |
|28|u32|request0, response status |

All fields explicit; checked lengths before payload access. Frames are header
followed by payload. A stream adapter must bound length before allocation and
implement exact reads, partial transfers, EOF and timeout. Do not assume a socket
read equals a complete frame. No multiplexing or concurrent submissions yet.

Commands:

- Hello1: empty → u32 protocol, maxPayload, maxBuffers, maxBufferBytesTotal.
- CreateBuffer2: u32 bytes (multiple1024,1KiB..1MiB) → u64 connection-local ID.
- DestroyBuffer3: u64 ID → empty. IDs never reused within a connection.
- DispatchVerify4: u64 ID,u32 seed → empty only after bounded GPU completion.
- ReadBuffer5: u64 ID,u32 offset,u32 bytes → exact bytes after GPU→host sync.
- DrawTriangle6: u32 width,height (16..512 each) → tightly packed RGBA8 pixels.
  Trusted host-side graphics shaders, black clear, orange triangle; synchronous
  bounded diagnostic proof, not generic Vulkan commands or zero-copy/WSI.
- DeviceIdentity7: empty → u32 native API,driver version,vendor ID,device ID,
  device type, then256 bytes UTF-8/NUL name. Actual host query, no CPU fallback.
- DrawOffscreen8: u32 width,height,vertexByteCount,fragmentByteCount, then four
  float32 push constants and four float32 clear components, then vertex SPIR-V
  and fragment SPIR-V. Explicit little-endian words, shader<=64KiB each;
  SPIR-V structural/header bounds and descriptor decorations checked. Returns
  packed RGBA8. No descriptors/vertex buffers; trusted research guests only.
  DeviceIdentity reached successfully from the guest's standard loader/ICD;
  DrawOffscreen execution PASS through the Windows standard loader/ICD.
- DrawVertices10: DrawOffscreen8 prefix/shaders followed by u32 stride,
  attribute count,vertex count,vertex-data bytes; then attribute records
  (u32 location,format,offset), then interleaved vertex data. Binding0 only,
  vertex-rate,1..4 unique locations<4, R32/RG32/RGB32/RGBA32 float, aligned
  stride4..64, aligned attribute contained in stride,1..4096 vertices,
  exact bytes=stride*count. No index data, descriptors or OS addresses.
  Native Mali and Windows standard Loader/ICD hardware/pixels/SyncVal PASS
  (3330 blue pixels,one submit); not all geometry configurations or performance.
- DevelopmentBundle9: empty → one fixed build-embedded private ZIP <=1MiB.
  Development-only chunk extension: u32 offset,u32 bytes;0,0 returns u32 total
  archive size (<=4MiB). Other requests return exactly bytes (1..1MiB) from
  that fixed archive; checked offset/range. Empty legacy request returns Limit
  for archives >1MiB. Packet bound remains1MiB; no pathname or arbitrary I/O.
  Available only in explicit development builds, otherwise Unsupported. No path,
  filename or arbitrary filesystem access; not part of the application-facing ICD.
- DrawIndexed11: DrawOffscreen8 prefix/shaders, then u32 geometryBytes,
  indexType (0=UINT16,1=UINT32),indexCount,indexBytes; then exact DrawVertices10
  geometry block and little-endian indices. Up to12288 indices; every index
  must be below vertex count. Native hardware drawIndexed PASS,3330 pixels,
  one GPU submission,zero validation errors/warnings. Windows standard Loader/
  ICD indexed+staging API test also PASS (3330 pixels,EXIT0); no WSI/game claim.
- DrawIndexedPush12: u32 width,height,vertexByteCount,fragmentByteCount,
  pushStageFlags,pushBytes; four float32 clear components; pushBytes exact bytes;
  vertex/fragment SPIR-V; same indexed geometry wrapper/data as11. Push range
  starts at0,4-byte aligned,4..64 bytes,vertex and/or fragment stages only.
  Byte-preserving shader data (not interpreted as floats); finite clear only.
  Same512² image limit. Native64B nonidentity matrix PASS,3906 checked pixels,
  five malformed range/stage requests rejected,zero SyncVal errors/warnings.
  Windows matrix ICD compiled; guest runtime not yet tested. Earlier8/10/11
  wire formats unchanged. This opcode has no multiple-draw/depth/image persistence.
- DrawScene13: u32 width,height,vertexByteCount,fragmentByteCount,pushStageFlags,
  pushBytes,cullMode,frontFace,depthFormat,drawCount,indexedGeometryBytes;
  four float32 clear components and float32 clearDepth; vertex/fragment SPIR-V;
  exact DrawIndexed11 geometry block; drawCount records (u32 firstIndex,count,
  pushBytes bytes). One pipeline/geometry,1..16 draws; cumulative index limit
  remains12288. Depth format UNDEFINED or D32_SFLOAT,LESS_OR_EQUAL,test+write;
  clearDepth finite0..1. Native format support is queried before allocation.
  Cull flags0..3,front face0..1,per-draw index ranges validated. Same512² limit.
  Native three-draw depth/culling/push-snapshot3330pixel PASS; Windows recording
  implemented and CPU-tested,guest scene runtime not yet tested. No persistent
  image contents across submissions or serialized native GPU handles.
- RenderSnapshot14: same scene payload as13,dimension limit1024. Returns
  u64 snapshot ID,u32 packed RGBA8 bytes,u32 rowPitch after GPU completion.
  Retains a CPU readback,NOT a native Vulkan image; existing13 remains512².
- ReadSnapshot15: u64 ID,u32 offset,u32 bytes (1..1MiB) → exact completed bytes.
- DestroySnapshot16: u64 ID → empty. Later read/destroy reports NotFound.
  Up to4 retained snapshots/16MiB CPU pixels per connection,separate from the
  compute-buffer budget; IDs monotonic and connection-local. Disconnect frees
  all snapshots. No addresses/paths/OS handles on wire. Snapshot pixels are CPU
  readbacks; one optional uniform/sampled renderer may retain GPU resources
  independently as described below. Native1024²/four-chunk/depth/cull PASS with
  860488 checked pixels and zero validation errors/warnings;not Windows PASS.

Status0 OK,1 bad header,2 bad payload,3 unsupported opcode,4 missing ID,
5 resource limit,6 GPU failure (poisons session),7 replay/nonmonotonic ID,
8 buffer not initialized by completed dispatch. No undefined buffer contents
exposed. At most8 buffers /8MiB requested buffer bytes, plus bounded driver
allocation overhead and session objects. Session disconnect destroys resources.

Texture slice (2026-10-16, native Bridge PASS; Windows API not yet tested):

- CreateSampled19:52B: u32 width,height,mipLevels,magFilter,minFilter,mipmapMode,
  addressU,addressV,addressW;float32 bias,minLod,maxLod;u32 totalRGBABytes.
  RGBA8 tight mip chain,1..1024 each dimension,levels<=full chain; exact byte
  total validated,not truncated to packet size. Up to4 textures/16MiB per session.
  Filters nearest/linear,mip nearest/linear;repeat/mirrored-repeat/clamp-edge.
  No anisotropy,compare,unnormalized or border-clamp in this slice.
  Returns a connection-local u64 ID,not a native handle/address.
- WriteSampled20:u64 ID,u32 sequentialOffset,byteCount,exact bytes. No holes,
  rewrites or writes after commit. Entire packet remains <=1MiB.
- CommitSampled21:u64 ID. Requires complete upload; creates native image/view/
  sampler and fences its staging upload once; returns empty after completion.
  CPU staging/input freed;native sampled image retained for subsequent draws.
- DestroySampled22:u64 ID. Removes native or incomplete CPU resource. EOF also
  retires all resources after completion. NotReady8 for incomplete/repeated commit.
- RenderSampledSnapshot23:u64 committed texture ID,then existing17 UBO+scene
  payload. UBO byte count4-aligned<=4096;vertex set0/binding0,fragment set0/binding1
  sampled descriptor only. Native shader descriptors;no shader substitution.
  Returns existing14 snapshot descriptor;read15/destroy16 unchanged.

Native proof: full1024²/11mip chain5592404B over6 chunks,one upload reused for
2 indexed UBO draws,2048 verified pixels,3GPU submissions,16 invalid requests,
zero SyncVal errors/warnings and clean EOF/Core teardown. Not zero-copy,Windows
ICD/app or performance PASS. Existing13/14/17 formats/guards unchanged.

Retained renderer (2026-10-17, native descriptor gate PASS): commands17/23 use
one bounded shared OffscreenSession per connection. Exact immutable scene bytes
(including shaders, geometry, state, clear and draw ranges), opcode, texture ID
and UBO size are the key; only same-sized UBO values change without rebuilding.
A different key retires the old renderer before constructing the replacement.
Any texture destroy retires the renderer first; EOF retires it before textures.
No wire-format change or native handles exposed. Cache key <=MaxPayload+16B;
GPU allocation scope is one bounded target/readback/optional depth, geometry,
UBO and pipeline/command objects, not an unbounded per-pipeline cache.
Relevant proof: four textured UBO draws, first three share one pipeline,
fourth changes immutable scene and rebuilds;4096 verified pixels,40 commands,
16 negatives,5 GPU submissions,3 total pipeline creations (1 compute+2 graphics),
SyncVal0/0,clean EOF/Corezero. Not guest/visible FPS or zero-copy evidence.

Layer batch24 (2026-10-17, native encoded Bridge packet gate PASS):

- `RenderLayersSnapshot24`: u32 innerVersion1,u32 layerCount1..8,followed by
  exact records. Each record starts u32 recordBytes (excluding itself),then
  u32 blend (0 replace,1 straight-alpha source-over,2 pinned UI alpha),
  u32 sampledBinding0/1,u64 committed connection-local textureID (0 absent),
  u32 uniformBytes0..4096 (4-aligned),exact uniform bytes,extended scene bytes.
- Existing64B scene header, shaders and indexed geometry remain explicit.
  Extended vertex decoding additionally accepts RGBA8 UNORM attributes. Draw
  count1..64. Push bytes0..64,4-aligned; stages zero iff bytes zero, otherwise
  vertex/fragment mask. UBO occupies vertex binding0; nonempty UBO cannot use
  sampledBinding0. Fragment sampler admitted only for an actual committed ID
  at its explicitly requested binding. No extra descriptor types advertised.
- Extended draw: u32 firstIndex,indexCount,i32 vertexOffset,u32 clipEnabled0/1,
  u32 clipX,clipY,clipWidth,clipHeight,then exact push bytes. With clipping off,
  all four clip fields must be zero. Enabled clip must be nonempty and within
  target. Every used raw index +signed base vertex must fit vertexCount. Raw
  indices need not fit before applying base; no out-of-bounds access admitted.
- All records use the same extent and bit-identical finite clear color. Root
  CLEARs color; subsequent passes LOAD it. Optional D32 is local and cleared
  per pass, not preserved across layers. Blend2 uses color SRC_ALPHA and
  ONE_MINUS_SRC_ALPHA, alpha ONE_MINUS_SRC_ALPHA and ZERO (ADD), exactly the
  pinned upstream UI, not standard source-over alpha. One submit/final readback.
- Complete packet framing/schema, texture lookup, depth availability and
  structural shader binding checks occur before any packet-owned GPU pipeline.
  This is still trusted experimental SPIR-V, not a semantic shader sandbox.
  Existing snapshot quotas/read15/destroy16/descriptor response unchanged.
  Whole request <=1MiB, no paths/pointers/native handles. At most one shared
  target/readback plus <=8 local depth/pipeline/geometry sets per request;
  completed chains retired after rendering. Pipeline caching for24 not yet
  implemented; persistent font texture uses existing19..22. No FPS claim.

Evidence: actual original ImGui library/font/shaders serialized through24,
two scene/UI frames,4608 probe pixels,258 changed widget pixels,30 commands,
19 negatives,3GPU submissions (font+2frames),5pipelines (compute+4graphics),
SyncVal0/0,Corezero. Rejected packets asserted not to allocate pipelines/memory
or submit GPU work. This gate is **in-process encoded requests/responses**, not
a new FD/vsock/Windows result. Generic descriptor transport is unchanged and
its legacy texture regression remains separately required. Old13/14/17/23
formats and binding defaults are unchanged.

First transport proof is same-UID Android local socket, with peer credentials
checked, using two independent threads and actual byte-stream framing. It is
deliberately not a public network service. Later AVF/vsock adaptation must add
connection lifecycle, peer trust and backpressure appropriate to the guest.
Native Android success does not establish guest ABI, ICD, WDDM, DWM or DXVK.

Android descriptor adapter bounds this diagnostic session to128 commands and a
60s watchdog which calls shutdown on the caller-approved channel. It does not
require SO_RCVTIMEO/SO_SNDTIMEO on an AVF-provided FD. Normal completion cancels
the watchdog before returning. This bounds transport I/O where shutdown is
supported, not a hung vendor Vulkan driver: native teardown may still block in
vkDeviceWaitIdle. It is not a production long-lived GPU service policy.
