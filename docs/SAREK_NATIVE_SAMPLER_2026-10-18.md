# Native sampler heap +guarded images: real D3D11 PASS

## Evidence

`E:/Mavk/artifacts/sarek-consumer-run36-20261018`: actual native ARM64 Sarek
D3D11CreateDevice S_OK/FL11_0 ->solid Draw ->Texture2D.Load ->SampleLevel with
point then bilinear samplers. All four Draw/readback stages verified16384 RGBA
components each. Filtered stages use a16-texel XY-pattern and independent CPU
nearest/bilinear reference with wrap, fractional texel coordinates, tolerance2.
Both modes differ from solid/clear/Load outputs. Process0,gaps0,CoreliveZero1,
all descriptor/resource/pipeline/command registries empty after teardown.

Layer24 SHA256714ac0fcf1acbdc80571b0287706c42ffd46ddd559013d7afdece28f4fc4f7fc.
Consumer14 SHA256576f109c27188cf61a1f92c72904e4ea74ce25c7d71da4533419f46f06e6239e.
Complete Sarek/runtime hashes and licenses are in run36/payload/SHA256SUMS.

## Reused implementation, not native-sampler emulation

Common NativeDescriptorAdapter now accepts explicitly external native set
positions in borrowed layouts. They cannot overlap logical descriptor bindings.
It allocates/updates only owned image/buffer/guard sets; external positions in
the immutable result stay NULL. The API layer substitutes original application
sampler sets at native bind time. No cached copy of the sampler heap is created.

Layer recognizes the actual sampler-only update-after-bind heap, preserves its
binding flags/exact native layout and native writes. Image/buffer generations
and metadata remain Mavk-owned/guarded, including secondary-to-primary leases.
SPIRV-Tools pass follows the IMAGE operand of OpSampledImage and ignores native
sampler loads (UniformConstant); no synthetic sampler metadata is fabricated.
The application retains its ordinary Vulkan pool/set lifetime obligations.

The actual consumer changes samplers from point to linear on the same image and
shader pipeline. This proves real selection/filtering, NOT a stress proof of
concurrent pending heap writes. Their semantics remain delegated to the driver.
Native heap copies not explicitly handled by the layer still fail closed.

## Regression and scope

Run37 uses same latest layer/common code, old Consumer11, both diagnostic flags
OFF. Existing guards/direct/indirect/boundedUBO/resubmit65536/4 PASS; one private
image initialization,diagnosticWords572,gaps0/fullcleanup/Corezero. Actual Sarek
then rejects nullDescriptor as expected (exit20). No global admission bypass.

Run36 remains explicitly DIAGNOSTIC: nullDescriptor FALSE, optional Sarek
FloatControls2 fallback selected. No complete Vulkan robustness/feature support,
validation-layer or game/performance claim. This is DroidDeck runtime-component
subset under Termux UID, NOT a visible DroidDeck/Wine-session PASS.

## Exact next step

Visible swapchain in actual DroidDeck app UID. Inspected fork commit:
0916a538aff4c08f799a4bd9fa17dd3c98b01e99; repo AGENTS read (rare factual comments).
No DroidDeck or X11-server package installed on tablet, no staged
app/src/main/assets/hybris/runtime.tzst in checkout. Existing Mali profile uses
x11-shm; do not replace stock Adreno DXVK or widen G715-specific workarounds.
Stage opt-in Mavk payload and a real native window before claiming integration.
For Wine, build matching PE Sarek DLLs: native Linux libdxvk_d3d11.so cannot be
substituted for Windows d3d11.dll. No new VM boot required for this next step.

No production U-AVF/Windows/Ubuntu/P7/H264 changes. No root/security changes,
commit or push. All test processes stopped; goal remains active/incomplete.
