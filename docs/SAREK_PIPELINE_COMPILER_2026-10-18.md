# Standard resource/pipeline API +common glibc compiler — 2026-10-18

Status:real layer routing and guard compilation PASS;NO guarded GPU execution,
NO D3D11 device/frame and NO full robustness2 declaration.

## Real consumer evidence

E:/Mavk/artifacts/sarek-consumer-run7-20261018 adds standard resource API coverage:
SSBO,sampled image,separate sampler,combined sampler,storage image,uniform/storage
R32 texel views. One update with7 writes/7 copies plus template update reaches
the existing shared frontend. Template creation precedes any native set allocation;
private temporary schema key is never sent to a Vulkan driver. Bounded source
offset/stride expansion uses the existing common implementation. Core/KHR aliases
are exposed only when the next chain exposes that command.

Run8 at E:/Mavk/artifacts/sarek-consumer-run8-20261018 also creates shader modules
and a native graphics pipeline from the ACTUAL pinned Sarek3.0.2 video-blit
vertex/fragment arrays. The standard pipeline hook uses its real descriptor layout
to choose the private guard index and calls common GuardCompiler in glibc.

```
MAVK_LAYER_UPDATE common=1 writes=7 copies=7
MAVK_LAYER_TEMPLATE common=1 sourceBytes=136
MAVK_LAYER_PIPELINE guards=compiled-only native=unmodified stages=2 publicSets=1 target=Vulkan13
MAVK_LAYER_DEVICE_END commonWrites=7 commonCopies=7 gaps=0 remainingSets=0 remainingBuffers=0 templates=1 remainingImages=0 remainingViews=0 remainingSamplers=0 remainingTexels=0 remainingTemplates=0
MAVK_LAYER_PIPELINE_END compiledOnly=1 remainingShaders=0 remainingPipelineLayouts=0 remainingPipelines=0 shaderBytes=0 pipelineBytes=0
```

Mavk forwards ORIGINAL shaders/layout to the native chain. Its compiled guarded
variants are retained as bounded shadows,NOT executed. Downstream hybris still
applies its existing shader policy (log:unused builtins removed in vertex input).
No draw or pixel result is claimed for this lifecycle/compilation regression.

Afterward real D3D11CreateDevice still returns0x80004005 at required nullDescriptor.
The native Vulkan startup succeeds;the D3D11 milestone remains INCOMPLETE.
No ordinary DXVK/Game compatibility claim follows from these results.

Run8 SHA256:
- Layer84fb89f7d2974da0999a4dbad57606c3985b96daee4a1142326d5e779c8d32b5.
- Consumere9bc1568a684b24a8eab2bc8efff26f8bfda150e672c50a71912531edaa4488f.
- Manifest0c1a48e54a4f75d0104b2a30e79e40ac45a48390f4e914ba15d23cc14d99dbf2.
Builds:mavk-glibc-layer5-20261018,sarek-native-consumer5-20261018 under E artifacts.

## Concrete compiler fix

Real Sarek fragment input is SPIR-V1.6 (generated header SHA
9e7b50475f2eb00d12bf4f28e41d77769b08d326055a93e0a24cac06e5e3fdaa).
Default Vulkan1.1 correctly rejected it. Added explicit immutable GuardTarget::Vulkan13
using SPIRV-Tools Vulkan1.3 environment for both input/output validation and passes.
No opcode guard exclusions weakened;Vulkan11 remains the default. Caches remain
per compiler/options instance,not mixed between environments.

E:/Mavk/artifacts/sarek-shader-consumer1-20261018 runs the SAME shared compiler
as an ARM64 glibc executable inside the DroidDeck library runtime:
- Original fragment6984bytes,SHA3701701db3df9375ee4035d3795cfa518a057aa601eb78d0f11807ef2c68a158.
- Guarded12320bytes,SHAd862ca21c2d27b660bf2f1de738dd52949e244d21b85acc6af1c67915a8a07e4.
- Vulkan11 negative rejected without writing output;Vulkan13 validated PASS.
- Existing common cache/bounds tests on ARM64/glibc:14assertions/9rejections PASS.

Compiler output path:shader-guards-glibc1-20261018. CMake/GCC downloaded/extracted
to E only,no apt package installation. Original pinned/modified SPIRV-Tools and
headers reused,licenses preserved. Static archive symbols hidden from layer exports.

## Necessary default-path regression

Rebuilt Android compiler archive after shared interface/implementation change;
previous archive saved at guard-compiler-pre-vulkan13-20261018. Existing native
GPU gate rerun,not a new Feature Suite case. Fresh report captured only after
renaming the previous app-owned report out of the way.

compiler-target13-regression1-20261018 APK SHA
582aeca9b122a9b27289c53f7224b4f4e284e7f38d4796a23ae6b8f5a60661fd.
Report core-separate-samplers.fresh.json SHA
8fea318408d27751d52d5d84e25a6c039b73f3659bf3db5fcb3d5a6fe410cd1b:
2048components/256guards,Corezero,SyncVal0/0 PASS. Probe stopped.
glibc run itself did NOT use an ARM64 glibc validation layer.

## Exact next integration

Implement actual augmented pipeline layouts and instrumented shader modules,
using exact public layout stage/binding/push-constant compatibility,not a generic
incompatible replacement. Bind immutable metadata/native descriptor generations
when recording draws/dispatches and retain them for command-buffer executable/
pending lifetimes (including resubmission),not merely until first fence completion.
Add command pool/reset/free and submission completion handling. Reuse common
NativeDescriptorAdapter;resource and shader lifetime must remain real.

Keep nullDescriptor false until required semantics work;current image/atomic/
layout support is bounded. Do not bypass Sarek admission to manufacture a frame.
Target this same real consumer,then actual D3D11 frame and opt-in DroidDeck fork.
Stock Adreno DXVK,production U-AVF/Windows/Ubuntu/H264 unchanged. No commit/push.
