# Common BC texture storage / upload integration

`BcTexture` owns authoritative compressed bytes and a decoded upload cache.
All16 BC format identities route to the existing BC1..7 decoders, with sRGB
metadata and signed/HDR float outputs preserved. This is an immediate CPU
resource model, NOT a Vulkan command recorder or a complete VkImage substitute.
Guest adapters must apply updates at the correct execution/synchronization
boundary, not snapshot future mapped-buffer contents at command recording.

Implemented:

- 2D,one array layer,explicit mip count; block-aligned offsets and edge extents;
- padded source rows, exact byte bounds before decoding, source snapshot for
  alias safety, validate/decode before authoritative state mutation;
- compressed/decoded/init-mask payload budget(default64MiB,caller can request
  up to512MiB total; individual decode output still bounded64MiB);
- initialized-block tracking; no fabricated readable content for untouched mips;
- same-format region copies, including overlapping self-copy through a snapshot;
- decode changed blocks only, cumulative decoded-block counters;
- coalesced per-mip dirty rectangle and revision-aware acknowledgement: old
  completed uploads cannot erase newer damage;
- explicit native snapshot adapter to existing Core upload. It currently
  requires initialized complete mip chain and existing Core1024 extent bounds.
  Snapshot is for upload, NOT rebuilt each render frame;
- Core sRGB sampled image format and existing float/UNORM formats; actual
  physical-device sampled/filter format checks remain mandatory;
- explicit synchronous native partial image update, retained image/view/sampler
  and render pipeline. Caller serializes queue/readers; upload owns staging,
  records per-mip read→transfer→read barriers, waits fence before retiring staging
  and acknowledging revision. GPU failure prevents further image updates.

Not implemented/advertised:

arbitrary Vulkan copies/format-compatible reinterpretation views, arrays/cubes/
3D,sparse resources,device-memory binding/aliasing,compressed-data readback APIs,
asynchronous/batched upload scheduling or ICD interception. No
textureCompressionBC enablement or DXVK/D3D11 PASS. A CPU codec family plus this
bounded lifecycle does not make a conformant compressed-image Vulkan driver.

Performance boundaries: partial decode avoids a full-texture decode/compare.
Native partial updates currently allocate one staging buffer and submit/wait
per explicit update; this is a correctness integration, not the final streaming
implementation. Float RGBA remains a reference representation(16bytes/texel).
Next optimize/batch only after actual consumer integration and measurements.

CPU test `test/texture.cpp`:300 assertions,19 negatives,all16 format routes,
odd mip edges,padded uploads,uninitialized-read rejection,source-preserving
overlap copy,stale acknowledgements,unchanged state on invalid input.
CPU PASS artifact E:/Mavk/artifacts/bc-texture-20261017/texture-test.exe.
Native gate source harness/native/bc_texture_comparison.hpp tests UNORM+sRGB,
partial patch and cross-mip copy,retained GPU image/pipeline and invalid updates.
Actual G925 gate PASS6144pixels,UNORM+sRGB,12submits,2retained pipelines,
18decoded blocks (7initial+1patch+1copy per variant),256partial upload bytes,
6invalid native update rejects,SyncVal0/0,Corezero. Strict verifier
`harness/verify-bc-texture.py` checks core-bc-texture.json. Evidence
E:/Mavk/artifacts/bc-texture-native1-20261017/.
