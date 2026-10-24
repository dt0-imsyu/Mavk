# Real DXVK-Sarek D3D11 device and shader draw: diagnostic milestone

## Result

Run26 returned **S_OK**, `FEATURE_LEVEL=0xb000`, non-null D3D11 device/context.
Ordinary `CreateTexture2D -> CreateRenderTargetView -> ClearRenderTargetView ->
CopyResource -> Map` verified **16384 RGBA components** on Mali-G925.
Process exit0; no production or VM changes. This is a clear/readback workload,
NOT a shader draw, visible frame, game or complete DroidDeck/Wine-session PASS.

Context: real pinned Sarek b6653094 native ARM64 ELF, actual DroidDeck linuxfs-r9
subset/hybris, isolated Termux debug UID. Common Mavk layer and native Mali.

## Important diagnostic limitation

User explicitly authorized local admission to find actual consumer blockers.
`MAVK_SAREK_NULL_PROBE=1` makes only Sarek's nullDescriptor **requirement** optional.
Its feature table continues copying actual supported bits; native queries and
enabled nullDescriptor remain FALSE. The flag defaults OFF and is unset before
each test. It is not a conformant full-feature implementation and must not be
shipped as such. No fake Vulkan extension/feature or production toggle added.

## Observed blocker and narrow fix

Run25 passed old admission, then failed creating sampler descriptor heap.
Sarek uses a sampler-only2048-entry layout, UPDATE_AFTER_BIND pool/layout plus
UPDATE_AFTER_BIND / UPDATE_UNUSED_WHILE_PENDING / PARTIALLY_BOUND binding flags.
Native Mali had accepted it; Mavk incorrectly rejected the extended layout.

Layer18 classifies this exact non-nullable native heap, preserves native
allocation/valid non-null writes, without immutable snapshot conversion.
Unknown mixed heap copies and guarded pipeline use remain rejected. No claim of
complete sampler-heap integration yet. Null sampler handles remain forbidden.

Run26 then created D3D11 device and executed clear/copy/map successfully.
One gap remained: Sarek view uses VkImageViewUsageCreateInfo. Layer19 source
adds only that recognized chain, stores narrowed view usage, rejects unknown
chains. Next consumer adds actual HLSL VS/PS Draw with full pixel verification.

## Evidence and rollback

- `E:/Mavk/artifacts/sarek-consumer-run25-20261018`: original layer17 blocker.
- `E:/Mavk/artifacts/sarek-consumer-run26-20261018`: device/clear PASS, layer18.
- Original binaries: `E:/Mavk/artifacts/sarek-pre-null-probe-20261018` and run24.
- Reversible source patch: experiments/droiddeck/sarek-null-diagnostic.patch
  (applies after existing BC admission patch; reverse-check passed).
- Fresh per-run payload SHA256SUMS records all binaries.

Layer18 SHA256 d303215366bfafb3eac1bbb2b85e4bdf9eb3cbfb9b249659975883d92857f281.
Diagnostic Sarek d3d11 98543e80bf5fd50aa9a84f08a5b07fc7c65f75c66546ca6f34af08a58535a37d.
Diagnostic Sarek dxgi 29e93b0c02dac3a7da056fc0b36f4ef77e6e3cf27e92b335548cece189f2e2e7.

No commit/push/root/SELinux changes. Full support/visible DroidDeck frame OPEN.

## Actual shader-draw progression

Consumer12 adds ordinary fxc-compiled HLSL VS/PS, SV_VertexID fullscreen triangle,
Draw(3,0), D3D11 CopyResource/Map and16384-component expected-color comparison.
No Mavk API or special Vulkan commands are used by this D3D workload.

- Run27: real Sarek reached Draw; Mavk rejected zero-set pipeline layout.
  Layer20 adds descriptor-free native execution only when shaders declare no
  DescriptorSet decorations. No guard set or descriptor emulation is needed.
- Run28: advanced to secondary command-buffer allocation, previously rejected
  wholesale by Mavk. Layer21 adds bounded descriptor-free secondary execution
  with native inheritance preserved and retained pipeline owners. Nullable
  descriptor-bearing secondary pipelines remain explicitly rejected.
- Both failed on actual observed operations, not theoretical feature cases.
  Both retained D3D device/clear PASS; shader draw still unproven at these runs.
- Sarek CS-thread failure can leave Map waiting; timeout now has a5second kill
  grace. Run27 test PID was stopped explicitly after verifying its slot; no VM
  or user process stopped. Run28 timeout cleaned its isolated process group.
- Run29: secondary allocation/execution advances; graphics pipeline fails
  because real Sarek supplies inline VkShaderModuleCreateInfo in stage.pNext
  instead of pre-created VkShaderModule. Pixel check correctly fails against
  unchanged clear color (exit35), not a false Draw PASS.
- Layer22 source accepts only this bounded inline module chain, copies original
  SPIR-V, uses the same compiler path when guards are needed, and materializes
  a real native module. Stage entry/specialization are retained; unknown chains
  and identifier-only shaders still reject.

## First actual D3D11 shader Draw PASS

Run30 / Layer22 / Consumer12: actual CreateVertexShader/CreatePixelShader ->
Draw(3,0) -> CopyResource/Map verified16384 components (64x64 RGBA) against the
shader color, deliberately different from prior clear. Process0, layer gaps0,
all resource maps empty, CoreliveZero1. Real Sarek generated SPIR-V from DXBC;
Mali executed both stages. This is **solid-color offscreen geometry**, not a
textured workload, visible swapchain or performance claim.

Evidence `E:/Mavk/artifacts/sarek-consumer-run30-20261018` includes complete
payload/hashes/logs and diagnostic-admission.json. Native queries/feature bits
remain unchanged. Full nullDescriptor is still NOT claimed.

Run31 regression with diagnostic admission OFF and previous Consumer11:
guarded boundedUBO/direct/indirect/resubmit65536 components/4submissions PASS,
private image initialization once, diagnosticWords572, gaps0/fullcleanup.
Actual Sarek then correctly rejects nullDescriptor (exit20 expected).
This confirms no accidental global admission/feature override was introduced.

## Textured D3D11 progression (actual consumer)

Consumer13 adds a real immutable4x4 RGBA texture, SRV, HLSL Texture2D.Load,
fullscreen triangle and full64x64 repeated-pattern readback. This tests texel
fetch, not filtered sampler semantics. Each of16texels has independent XY color.

Run32: actual Sarek shader selects native FloatControls2 capability6029;
SPIRV-Tools2022.4 used by current common guards rejects it before compilation.
This is **PASSTHROUGH MISSING in Mavk's compiler dependency**, not missing Mali
hardware. No unknown instruction/capability is stripped or falsely accepted.

For the short consumer diagnostic only, explicit MAVK_SAREK_LEGACY_SPIRV=1
requests Sarek's existing optional-feature fallback. It leaves queried native
capabilities unchanged, does not request/enable FloatControls2, and is restricted
to the explicit null-admission experiment. Defaults OFF. Full float-controls
passthrough requires a future compiler update; do not call this implemented.
Patch: experiments/droiddeck/sarek-legacy-spirv-diagnostic.patch (reverse checked).

Run33: the fallback successfully compiles the actual texture pipeline. Next
observed blocker is layer22's descriptor-free-only secondary binding restriction.
Layer23 transfers immutable secondary descriptor generations/pipeline leases
into the executing primary and emits diagnostic host barrier only at primary
end, outside the inherited render pass. Unknown/mixed sampler heaps still fail
closed. Result pending, not yet a textured PASS.
Run33's proot wrapper returned0 even though the explicitly stopped child had
signal9. This is NOT an overall PASS: textured readback token is absent. Runner
now normalizes that child-signal case and reports clear/solid/texture stages
separately, so a later failure cannot erase an earlier proof or fake completion.

## Textured Draw PASS — run34

Layer23 + Consumer13 + explicit null-admission/legacy-SPIRV diagnostic flags:

- D3D11CreateDevice S_OK, FL11_0.
- Clear/readback16384 components PASS.
- Solid VS/PS Draw/readback16384 components PASS.
- **Textured Draw/readback16384 components PASS**, all16 source texels checked
  across64x64 output. Ordinary D3D11 immutable upload/SRV/Texture2D.Load/Draw/Map.
- Actual guarded texture pipeline, template sourceBytes24, secondary execution
  retains1descriptor generation in primary. No direct Mavk API in D3D consumer.
- Process0, gaps0, all maps empty/CoreliveZero1. No active test processes remain.

Evidence: `E:/Mavk/artifacts/sarek-consumer-run34-20261018`.
SHA256:

```
layer23 cc8781735b1c3c867f6a1c9f1afb8c5286add91a5454922f43ecb69c024c525a
consumer13 102c0425f2d320a01348cd9fd3f64e47105c01ce7bcaf5d1dee0c4cb5b742211
Sarek d3d11 42d3e4827459d959bbb0bfa94ba203a2b6fd660e47d8607aaae588b0f157404a
Sarek dxgi b2155710f0850680a7bef39b3c65deb6145af03343b881694a07ffb7da916da9
```

Run35 latest layer23/old Consumer11 normal guarded regression:65536/4 PASS,
one private initialization, diagnostics572, gaps0/allcleanup. Both diagnostic
flags OFF; real Sarek still rejects nullDescriptor (exit20 expected). This
separately validates preserved baseline and absence of accidental feature lies.

Next: filtered sampling uses Sarek's native sampler heap in pipeline layouts;
current layer still rejects that mixed path. Then actual visible swapchain in
DroidDeck. This proof is NOT filtered sampling, window present, performance,
full nullDescriptor, FloatControls2 passthrough, game compatibility or upstream
DXVK PASS. No production U-AVF/Windows/Ubuntu changes, commit or push.
