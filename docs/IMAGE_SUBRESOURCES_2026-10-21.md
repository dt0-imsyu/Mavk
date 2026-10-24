# Images/formats P1 checkpoint — actual API, unchanged APK18

User priority: finite practical P0/P1 packages,not exhaustive descriptors.
Reuses existing real-loader consumer/Core; one new mode,not another renderer.
No runtime implementation,APK,feature bit or production change was needed.

## What actually executed

Installed COMMON161fe2e8/layerbc30f541 + preserved hybris179870fc, Tab S11 G925,
actual D3D profile HYBRIS_BC_TEXTURES=dxvk. Vulkan calls create two BC1_RGBA
images with3mips/2layers and mutable UNORM/sRGB view lists. Staging upload uses
padded row/layer strides. Staging contents are filled AFTER command recording,
flushed before queue submission: readback must observe execution-time contents.
Then vkCmdCopyImage copies mip1/both layers to a second image. Six source
subresources +two copied subresources are each sampled through UNORM and sRGB
views:16actual views,4samples/four channels each,256components checked.

Each mip/layer uses a different BC endpoint. Row/layer padding contains0xCD,
not a valid duplicate of expected texels,so wrong source strides select different
content. Shader uses explicitLOD0 relative to the selected baseMip/baseLayer.
CPU reference expands known RGB565 endpoints; sRGB transfer uses the standard
piecewise function. UNORM tolerance0; sRGB tolerance1 in quantized8bit output.
This is not a general BC decoder test or an independent oracle for all blocks.

## Actual result

Native/tracking and protected Mavk outputs identical. Strict verifier --images:
views16/components256,mips3/layers2,mutableSrgb,paddedRows,afterRecordHostWrite,
compressedCopy all asserted. Protected draws16/submissions18,gaps0,all registries
empty/Corezero. No fresh guarded SyncVal, CTS,visible-FPS or broad-format claim.

`E:/Mavk/artifacts/image-subresources-consumer-20261021/` contains
images-results.tar.gz,normalized native/guarded stdout+stderr. Consumer SHA256
9aa33bd9bf0302c27042e2e58f581e594f5aaaa17549599fb198f55f0fc898ab.
Shader in image-subresources-shader-20261021,validated as Vulkan1.1 SPIR-V.
Source: compatibility/test/layer-images.hpp,image-subresources.comp and existing
layer-execution.cpp/verify-layer-execution.py. Device files app-private
files/mavk-images; no Steam data change or VM launch.

## Package conclusion / next subsystem

This important mip/layer/view/copy path is already working: do not rewrite it
or add a per-game fix. No need to reinstall unchanged APK18 or repeat the same
Muck regression solely for a test-only addition. Existing APK18 Muck/Steam
integration remains the recorded baseline,not an all-games claim.

Open image cases remain explicit: BC2..7 complex blocks through THIS multi-view
oracle,depth/stencil and attachments,packed/other widths,uncompressed↔compressed
reinterpret copies,aliasing/ownership,async retirement and other image usages.
They are not all automatic P0s; reopen from consumer requirement or high-value
package. CTS has NOT run. Full textureCompressionBC remains false.

Next shader audit identified a concrete common boundary: guarded compiler
rejects OpImageTexelPointer/image atomics and selected pointer/float-atomic
forms. Native image atomic capability alone is not guard/null correctness.
Pinned pass handles integer BUFFER atomics but deliberately excludes image
pointers. Do not remove that reject or advertise support without guarding the
atomic operation (not producing an invalid guarded/null image pointer).
Implement consumer-relevant semantics as a shared shader/Compatibility change,
not more null-shape permutations. External hybris clip rewriting remains a
separate unverified full-semantics claim documented in the priority report.

No commit/push; Windows/Ubuntu/P7/H264 untouched. C: old diagnostic
live-name-fix-candidate.wim recoverably moved to
E:/Mavk/archives/windows-diagnostic-intermediates-20261021/ (hash-verified CSV),
not the production image; C: restored above50GB decimal.
