# Mavk Compatibility

Shared, explicit compatibility implementations for native DroidDeck and U-AVF
adapters. Never expose an entire Vulkan feature from a bounded helper PASS.
No Adreno implementation is copied here. Vulkan Core and WDDM stay separate.

Null/range shader guard feasibility and explicit unsupported cases:
[NULL_RESOURCES.md](NULL_RESOURCES.md). Latest bounded native buffer proof is
not sufficient to expose nullDescriptor or robustBufferAccess2 to consumers.

## First implementation: BC1/2/3 raw block decode

`include/mavk/compat/bc.hpp`, `src/bc.cpp`: portable CPU decode to owned RGBA8,
BC1 RGB vs punch-through alpha, BC2 explicit alpha, both BC3 alpha palettes,
sub-4x4 edges and padded block rows. Explicit64MiB decoded-image budget and exact
input-range checks before allocation. No DDS/KTX parser or unchecked raw mapping.
sRGB input preserves encoded bytes and encoding metadata; it must not be passed
to an UNORM texture as if linear. Current native GPU acceptance tests UNORM only.

CPU gate: `test/bc.cpp`,862 assertions,13 invalid inputs,324 deterministic random
small images, plus independent hand-calculated color/alpha selector goldens.
Random cases test bounds/determinism, not an independent decode oracle.
Native gate: existing CoreComparisonActivity with `bc=true,validation=true`;
uses the SAME native upload/retained graphics renderer with four mip levels
per format and real GPU sampled-pixel verification. Report `core-bc.json`.

This initial slice alone is **not** Vulkan compressed-image API support,
full format/alias/copy/view semantics or DXVK PASS. BC4..7 slices follow below.
`textureCompressionBC` remains unadvertised. No per-frame CPU decode is required:
the intended integration decodes changed uploads/mips, retains native images.
Performance for large textures/streaming and implementation of an upload cache
remain unmeasured/unimplemented. CPU decode increases texture memory usage.

## BC4/BC5 channels (2026-10-17 actual bounded PASS)

`decodeBcChannels` implements BC4/5 UNORM and SNORM into float RGBA, preserving
negative values and fractional channel interpolation instead of quantizing to
8-bit. Missing channels are0; alpha1. Signed byte128 maps to-1. The spec's
ambiguous endpoint pair(-127,-128) uses canonicalized equality/4-interpolant
branch; this choice is not a promise to match another implementation for that
undefined comparison. Padded rows/partial edge blocks and64MiB output bound
are checked before allocation. BC4/5 have no sRGB variant in this API.

CPU test `test/bc-channels.cpp` checks all131072 endpoint pairs (signed+unsigned)
against an independent double-precision oracle:8421776 component comparisons,
11 invalid inputs,324 small edge/padded-row images. This is not Vulkan CTS.
Float RGBA is a correctness/reference representation, NOT an optimized memory
format:16bytes/texel. Packing native R/RG formats is a later measured step.

Native Core SampledPixels now has explicit format(default RGBA8_UNORM), bounded
RGBA32F alternative with correct mip byte offsets and runtime format/filter
admission; old Bridge wire schemas still create RGBA8 by default. No guest format
or feature exposure changes. Existing mip-layout tests and new float-layout
negative tests PASS. Native `bcChannels=true,validation=true` uses existing
renderer and remaps signed shader samples into RGBA8 readback for verification;
that GPU gate has quantized tolerance, not full-precision GPU sampling oracle.
Actual G925 gate PASS16384 pixels,16 submissions,4formats x4mips,SyncVal0/0,
Corezero. Strict verifier `verify-bc.py --channels` PASS. Old BC123 GPU gate
also passed on the same APK. Evidence `bc45-native1-20261017/`.

Algorithm/ambiguity reference:
https://registry.khronos.org/DataFormat/specs/1.3/dataformat.1.3.html#RGTC

## BC6H/BC7 (2026-10-17 actual bounded PASS)

`decodeBc6h` and `decodeBc7` use the pinned unmodified portable bcdec codec,
copyright Sergii Kudlai,MIT alternative. Original license/header and hashes
are preserved in `../third_party/bcdec/`, checked during harness builds and
license packaged into APK assets. No Adreno code. Decoder is size-oriented,
not asserted to be performance-optimized. CPU fallback/reference, not zero-copy.

BC7 outputs RGBA8 plus sRGB metadata; BC6H signed/unsigned outputs float RGBA,
preserving negative and HDR>1 values with alpha1. Bounds/padding/edge wrappers
check before allocation. Reserved modes follow codec black/transparent behavior.
Full compressed-image Vulkan APIs and format advertising remain unimplemented.

Independent **Microsoft WARP compressed-texture sampling** agrees on589824
components:BC7 all8 modes/2048 blocks,BC6H all14 modes unsigned3584 andsigned3584.
Separate3502 golden/bound assertions,18 negatives,+/-65504 HDR extrema and
reserved modes PASS. detex reference disagreed, investigated with thirdBC7
decoder and WARP, NOT accepted by relaxing comparisons. Counterexamples and
source pins: `test/BC67_ORACLES.md`. Reference tools are host-only, not in APK.

Actual Android Core upload +Mali hardware sampling PASS12288 pixels,3variants
x4mips,12submits,125 negative and197 HDR>1 input components,SyncVal0/0,Corezero.
Native HDR verification tone-maps to RGBA8 (tolerance1.1/255), not full-precision
GPU oracle. `verify-bc.py --hdr` checks report. No FPS/performance claim.
Evidence `E:/Mavk/artifacts/bc67-native1-20261017/`.

Common compressed shadow/mip/partial-update/copy/dirty-revision model and
retained native partial uploads+sRGB now have a bounded hardware PASS:
see `BC_TEXTURE_LIFECYCLE.md`. Remaining Vulkan image/view/array semantics,
execution-time ICD/ABI interception and correct null-resource semantics are
still required. Completing the codec family does NOT close DXVK's BC gate.

Format reference (algorithm, not copied implementation):
https://learn.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression

## Consumer-specific admission

See `../docs/TURNIP_DXVK_CAPABILITIES_2026-10-17.md` for pinned full Turnip/Mali
profiles and the selected upstream DXVK53 mandatory rows. The public profiles
in `profiles/` are capability observations with source/hash provenance in that
document, not compatibility-layer enablement settings.

Additional lead verified2026-10-17: zeyadadev/DXVK-Sarek Mali compatibility R1,
commit `b6653094a2b8f3b824ffcf8f50aaa3b3da41263b`.
The same actual-profile/source admission extractor finds48 mandatory rows,
46 pass on G925, two fail: **textureCompressionBC + nullDescriptor**.
It makes non-solid raster,multiViewport,clip/cull and robustBufferAccess2 optional.
This is NOT equivalent emulation of those features: its shader IR drops unsupported
clip/cull inputs/outputs. Apps relying on their semantics can render incorrectly.
Do not import that behavior as a generic Mavk feature implementation. Candidate
consumer profile must remain explicitly restricted/experimental.
Its nullDescriptor check is still mandatory; no claim that this release alone
starts on stock G925. No Sarek DLL installed, D3D11 not executed.

Source and48-row comparison: `E:/Mavk/artifacts/sarek-audit-20261017/`.
https://github.com/zeyadadev/DXVK-Sarek/releases/tag/v3.0.2-mali-compat-r1
https://github.com/zeyadadev/DXVK-Sarek/blob/b6653094a2b8f3b824ffcf8f50aaa3b3da41263b/src/dxvk/dxvk_device_info.cpp
https://github.com/zeyadadev/DXVK-Sarek/blob/b6653094a2b8f3b824ffcf8f50aaa3b3da41263b/src/dxvk/dxvk_shader_ir.cpp

Next native slices: remaining compressed-image APIs and explicit null-resource access
semantics (reads zero/writes discarded,
including sampled/typed/storage resources). Merely binding a writable dummy
buffer is NOT a generic nullDescriptor implementation. Prefer reusing a
consumer's documented existing fallback over writing a universal SPIR-V rewriter.
Do not remove admission checks as a substitute for implementation.
