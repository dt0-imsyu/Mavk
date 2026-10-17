# Null resources / range guards — feasibility, NOT feature support

2026-10-17: stock Mali-G925 does not implement nullDescriptor. Binding a real
sentinel resource is legal but **not** sufficient: null writes must be discarded,
not stored in a dummy buffer. Native Vulkan feature bits remain unchanged.

## Implemented bounded experiment

`experiments/instrument-null.py` uses the existing Android NDK28.2.13676358
SPIRV-Tools **v2022.4 ndk-r27-beta1**, `--inst-buff-oob-check`. No upstream source
is copied/modified. Shader compiler/optimizer/validator are external build tools.
The pass inserts conditional accesses, zero result on invalid reads and skips
invalid stores. Its validation record output is retained to prove guard firing.
Khronos source references: `source/opt/inst_bindless_check_pass.cpp`,
`include/spirv-tools/instrument.hpp` in the NDK's SPIRV-Tools source distribution.

The CLI reserves descriptor set7, outside G925's7-set limit. This experiment
accepts original set0 only and relocates **exactly the two generated** set7
bindings0/1 into set1; collisions/other original sets/malformed SPIR-V are
rejected. Rewritten code must pass spirv-val for Vulkan1.1. This is deliberately
not a general shader-layout remapper or production API.

Important upstream ABI difference: descriptor-init/index mode reads
`Data[i + Data[b + Data[s + Data[0]]]]`; buffer-only mode disables descriptor
index validation and instead reads `Data[i + Data[b + Data[s+1]]]`.
The first range gate failed because it reused the former table with the latter
pass. The generated SPIR-V and GenDebugReadInit source establish this difference;
the range harness now explicitly packs the three-level layout. Failed evidence
is preserved in `null-range-native1-20261017`, not relabelled as GPU failure/PASS.

Native gate uses existing Core buffers/shader/pipeline/command/fence plus native
descriptor metadata, not a replacement renderer/ICD. All real descriptors are
valid. Logical nullness/ranges live in metadata and are switched between two
dispatches without rebuilding the shader. Static descriptor-array slots0/1
cover SSBO loads/stores and UBO loads; an allocated-but-unexposed second half of
each buffer is a sentinel for range read-zero/write-discard verification.
There is no arbitrary invalid mapping or GPU write into another allocation.

Initial descriptor-init-only Android gate PASS768 words/384 rejected accesses,
SyncVal0/0,Corezero. Evidence `null-buffers-native1-20261017`; original report
SHA256 e0ceacc16af1c75a8e707b224722439bbf3158a9056b1ff5d8f9285b60aae859.
It established functional null-buffer semantics, not robustness2.
Scalar range gate subsequently PASS1536 words/896 rejected accesses,2 submits,
SyncVal0/0,Corezero; tiny total GPU command time0.0305385ms,NOT a performance
claim. APK null-range-native3-20261018; report SHA256
8fd6be2fd9df016dd932e84b31297ac3642a782622e090a670c8aa062da4c961.
Current verifier accepts this extended gate,not the earlier init-only report.

## Explicit remaining gaps

- Stock upstream pass does **not** guard buffer atomics. The new configurable
  compiler at `shaders/` applies a reviewed small extension to the same analysis;
  actual Mali32-bit buffer atomicAdd null/valid-return+side-effect gate now PASS.
  This does not imply all atomics,64-bit atomics or image atomics are supported.
- Current `--inst-buff-oob-check` invocation does not enable image initialization
  checks. Combined guards require an explicit pass configuration, not arbitrary
  stacking that might instrument the generated debug resources themselves.
- Dynamic/nonuniform descriptor arrays, full image/sample/texel-buffer operations,
  all atomic forms/results, aggregate/pointer access forms and required
  robustness2 alignments/partial-vector semantics are not accepted by this gate.
- Metadata layout is test-only; no runtime descriptor update/copy/lifetime API,
  execution-time snapshot, shader cache key or consumer ICD integration yet.
- Every invalid access writes a diagnostic record. This overhead must not be
  mistaken for an efficient production compatibility implementation.

Therefore neither `nullDescriptor` nor `robustBufferAccess2` is advertised, and
there is no D3D11/DXVK PASS. Next implement the covered semantics in the common
shader/descriptor adapter and expand only on real consumer requirements.

Build: compile `harness/shaders/null-buffers.comp` with NDK glslc Vulkan1.1,
run `instrument-null.py --tools <NDK shader-tools> input.spv output.spv`, then
`harness/build.ps1 -NullBufferShader output.spv -ValidationLibrary <app-local SO>
-OutputDirectory <fresh>`. Start CoreComparisonActivity with `nullBuffers=true`
and `validation=true`; collect `core-null-buffers.json` using collect_report.ps1.
Strict acceptance: verify-null-buffers.py. Do not replay on a running worker.

## Subsequent image and integer atomic slices (actual2026-10-18)

Image mode uses stock descriptor-init/index pass with four-level metadata.
Both static combined sampler slots perform RGBA8 texelFetch; null returns all
zero including alpha,valid pixels unchanged. R32_UINT storage images perform
load/store; transfer readback proves null store left the sentinel intact and
valid store wrote expected data. Two metadata phases swap null/live slots.
PASS1792 values/384 guards,SyncVal0/0,Corezero. verify-null-images.py is strict.
NOT filtered sampling/all image formats/ops/samplers/descriptor arrays support.

Combined custom compiler enables index/init/buffer/texel guards in one pass and
adds integer buffer atomic recognition without copying shader operations into
a demonstration-specific implementation. Invalid atomicAdd returns0,no side
effect; valid atomicAdd returns the old value and increments exact expected
word. Actual PASS1536 values/1024 total guards,2GPU submits,SyncVal0/0,Corezero;
tiny GPU command total0.0347692ms is NOT production overhead/FPS evidence.
Strict verifier: verify-null-buffers.py --atomics. Atomics shader inherits scalar
OOB load/store tests,not yet an out-of-range atomic test. Other atomic opcodes
admitted by the patch still need actual semantic gates before feature exposure.

Host compiler source/pins and full limitations: shaders/README.md. Core shader
metadata is still experiment-owned,not a complete consumer Vulkan descriptor
adapter. Runtime Android compilation/cache integration remains open. No feature
advertising or DXVK PASS follows from these bounded gates.
