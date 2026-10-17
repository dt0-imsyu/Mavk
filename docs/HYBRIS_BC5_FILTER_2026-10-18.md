# BC5 linear-filter candidate — actual G925 PASS, experimental

## Change

The actual saved Mali profile supports linear filtering on RG32_SFLOAT103,
but not RG16_UNORM77/SNORM78 or RGBA16 normalized91/92. The pinned hybris layer
maps BC5 to RG16 normalized. Merely setting FILTER_LINEAR would be incorrect.

`experiments/droiddeck/bc5-rg32f.patch` changes the bounded BC5 backend to RG32F,
with matching compute output of two float32 values,8byte destination bounds,
scratch sizing and copy strides. Existing 16-bit normalized decoder precision is
preserved when converting to float; blue0/alpha1 are the RG-format defaults.
BC4 stays packed R16. No per-frame CPU decode,extra submission or host GPU wait
was introduced. Decoding remains in existing transfer/compute recordings.
The decoded BC5 backing bandwidth/storage doubles4→8bytes/texel. This is NOT a
performance PASS. The candidate remains a private explicit layer,not deployment
into DroidDeck or production U-AVF. Generic physical-device backing selection is
future integration work;this experimental patch deliberately fixes one mapping.

## Executed checks

1. Rebuilt unmodified component compatibility layer from shipped pinned source
   plus exact shipped patch. Same actual constant-block results as original
   binary,including BC5 FILTER_LINEAR absent. Bare ICD/runtime unchanged.
2. Rebuilt candidate:14 D3D BC identities133..146 report0xd001;actual compressed
   image create/bind/view/upload/compute texelFetch/readback896components PASS.
   BC1 RGB131/132 remain unsupported. Global textureCompressionBC not enabled
   or advertised by Mavk. No library overwrite:explicit manifests/directories.
3. Actual BC5 bilinear test: UNORM+SNORM,three phases each,six GPU submissions,
   all8 palette selectors across a4x4 block,endpoint-order branches,negative
   signed values and clamp-edge sampling. Shader textureLod samples halfway
   between texels;independent float CPU oracle averages four neighbours.
   PASS384components,absolute tolerance0.001. Not all modes/mips/arrays/aliasing,
   performance or validation-layer acceptance.
4. Host destination-bound unit:14checks,BC5 exact128byte output accepted,127
   rejected,offset20+128 vs147 checked,short source rejected,BC4 sizes unchanged.

The fork's required per-D3D BC flags now pass for all14 identities. Sarek still
requires the aggregate BC boolean unless adapted to real per-format checking;
nullDescriptor is still a separate mandatory blocker. Do not claim D3D11 or a
complete Vulkan BC feature. Keep common Mavk CPU codecs as the independent oracle.

## Reproduction/provenance

Everything under E:/Mavk/artifacts/hybris-bc5-layer1-20261018.
Shipped source archiveSHA14c1aaa19f0205577f4410340ae2506da6c6a14ae6e50a958d43a477bad0f6ad;
component patchSHA1ba9aecb245b9efdc48c0a9773d4fa3fde0287a8ddadfe450c5d8136e2b56136.
Both matched tablet run-as SHA before use. Windows archive extraction could not
create unrelated HWC2 symlinks; Vulkan sources extracted and full component patch
applied cleanly. This is NOT a successful whole-libhybris/autotools build.

Source pinbc8cb3586b79d208fd21b925fa0209b3e432e008,existing DroidDeck patches as
documented in HYBRIS_BC_REUSE_2026-10-18.md. Only standalone compatibility DSO
built,using existing glibc cross sysroot+NDK clang,not Android linker/ICD/WSI.
Vulkan headers pin ee2ec5fd83dafce291024683b50dc89219333076 (1.4.362),same pin found
in component validation build recipe;archiveSHAdf7a152381a7fdaf98e36de7716b8d6d6ead776764bc76d1a1aef9049d1bca89.
build-compat-layer.ps1 reads explicit upstream source list,links with no undefined
symbols;rebuild-bc-shader.ps1 uses glslc+spirv-val(vulkan1.0),regenerates own output.
Original Apache2 and bcdec shader MIT notices must accompany the DSO.

Baseline DSO SHA5dcd297cb2ed3017b402fac39144f8abd0881c0e6353368c519b06b11b44cc12.
Candidate DSO SHA179870fcf9e1cdfe0495d829df547b53bd950c63e2981ff162b53a4d02687654.
Decoder SPV SHA3c2463735518b72c5ea966042296848c9b6ee5a2a93296c19c750797d92e43a5.
Linear probe SHA8a9ac482d4525541024b45d27b9859f5096ed767ed1dde32220523adc7c0cf7e;
shader SHA7e45614e3f1e18a3b53bfe65cf0b02a8b786e765f9f7a9cdf643faec97077911.
baseline-run stdoutSHA30a7dd447be532c6e45f9524af4735cb3624614c144f584275f030eb50bd67db;
rg32f-run stdoutSHAaaaf02ab116a679f6b31052e244be87e013b21cd9ef374ff5e089ed002754739;
linear-run stdoutSHA78e25835e154763acb51e24b22918ca72ed446a7bab0024fe87836faf6dfa16c.
Strict verify-bc5-filter.py checks both actual builds/profile and six palette tests.
Stock Android,Termux debug UID subset,not full DroidDeck session. No new VM,
root,SELinux,firmware,ICD,production-library or Windows/Ubuntu/H264 changes.

Next: integrate shared guarded null/image consumer;adapt experimental Sarek BC
admission to actual14-format checks rather than spoofing global BC support.
