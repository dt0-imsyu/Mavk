# D3 immutable sampler package — 2026-10-21

Experimental protected Vulkan layer, not full Vulkan conformance or full
nullDescriptor/robustness2 admission. No game-specific branch. Windows/Ubuntu,
P7/firmware/CPU/H.264/production U-AVF remain unchanged.

## Shared implementation

`ImmutableSamplerBindings` copies sampler handles and retains native resource
leases. Layouts and update templates own those arrays rather than caller memory;
native layout owner retains sampler leases through destruction. Empty immutable
state is shared: ordinary layouts/sets do not allocate another empty map object.

- Standalone immutable samplers initialize at set allocation. Protected native
  materialization never writes those sampler descriptors again.
- Combined-image updates ignore the supplied sampler, including a null/irrelevant
  handle, and retain the layout's sampler while replacing only the image.
- Combined-image copies into an immutable destination preserve its sampler,
  including null image copies in the experimental CPU shadow. Mutable targets
  can inherit samplers copied from immutable sources.
- Writes/copies into standalone immutable sampler descriptors reject, as Vulkan
  requires. Consecutive binding spans must agree on immutable-sampler presence.
- Templates own the immutable definition and require matching sampler references
  on the target layout. Pipeline compatibility compares owned references, not
  caller pointer addresses; declaration order is canonicalized.
- Immutable YCbCr conversion metadata remains explicitly UNSUPPORTED. Dynamic
  sampler heaps/update-after-bind retain their existing narrow native path.

No pixel copy, GPU readback, queue wait, shader rewrite or feature-bit addition
is introduced by this descriptor handling. Existing guarded snapshots/metadata
and resource leases are reused. This does not fix their separate D4 late-update
limitations and does not implement empty descriptor layouts.

## Independently reproduced native copy issue

Same GLSL/GPU oracle in glibc consumer and native Android harness. Four UV
coordinates outside [0,1] distinguish CLAMP vs REPEAT samplers; distinct RGBA
texels distinguish images. Three phases exercise writes, mutable-to-immutable
combined-image copy, and template update. Two combined samplers + two standalone
immutable samplers,192components,compatible separately-created layouts; caller
sampler-handle array overwritten immediately after layout creation.

Native ARM Android Vulkan with NO Mavk/glibc/libhybris reproduces:
`phase=1 component=0 actual=108 expected=18`. Native copy's result uses the
source REPEAT sampling behavior instead of the destination CLAMP behavior.
Khronos validation + synchronization validation:0errors/0warnings,callback1,
Core resources0. This is not inferred from a Muck artifact or Present count.
With source samplers matching destination immutable samplers, the native
control passes all192components with the same0/0 validation gate.

The glibc path reproduces with Mavk disabled and without HYBRIS compatibility
layer as well. Therefore this specific discrepancy is not caused by Mavk,
Wine/FEX,DXVK,WSI or glibc ABI. The observed failure is in the native G925 Vulkan
path; this does not claim all Mali models/driver versions have the same issue.
Do not reinterpret native control PASS as full native copy correctness.

Protected Mavk's common shadow preserves destination immutable semantics and
materializes legal native descriptors; GPU result passes all192components,
three dispatches/five total submits including two one-time image uploads,
gaps0 and Core/resources0. Native and guarded are deliberately NOT labelled
identical PASS here: the failure/control/fix are separate recorded outcomes.

Native Android reports `native-android.json` and `native-android-control.json`
under `E:/Mavk/artifacts/immutable-descriptor-gpu-20261021`; raw glibc controls
in native-proof.tar.gz. Shared oracle `compatibility/test/layer-immutable.hpp`
is reused by native harness and standard-loader consumer, not two renderers.
`verify-immutable-samplers.py` checks failure/control and corrected GPU evidence;
the guarded glibc run itself has NO SyncVal/CTS instrumentation.

## Matrix / regression boundary

| Case | Implementation / evidence |
|---|---|
| Standalone immutable sampler arrays | NATIVE initialization + owned tracking; actual separate-image/sampler GPU reads |
| Combined immutable arrays, ignored update sampler | NATIVE writes + shadow;192RGBA component oracle across all phases |
| Combined copies preserving immutable destination | VERIFIED COMPATIBILITY correction for reproduced native issue; no GPU data copy |
| Template updates and identical layout compatibility | NATIVE + owned shadow, actual GPU execution; host template survives source-set destruction |
| Retained leases / caller-array lifetime | Host18checks/8negative cases; GPU caller handle-array overwrite and cleanup |
| Null combined image with immutable sampler | Host shadow tested only; no new GPU null/feature claim |
| YCbCr conversion / extended descriptor layouts | UNSUPPORTED by guarded slice; reject rather than fake |
| Empty layouts and general late updates | Still PASSTHROUGH MISSING / D4 architecture work |

Final layer `8b72f0d28c7df99a7d0bee1f56feb7232b39faf147a879110c5ecfb31d344184`:
immutable192components PASS; dynamic buffers512words/four dispatches/two submits
PASS; protected command routing512words/8192components PASS. Logs final-results.
Initial layer ebb689be50406a827ab2e1d950196fb11b92b54a9d9cd42359b260ddf8214779
has same GPU semantics; final only avoids allocating empty immutable state.

Host eight suites rebuilt PASS: immutable18/8,descriptor34/25,bound18/6,
updates20/20,sampler9/7,templates13/18,texel39/32,dynamic21/13. Retained in
`E:/Mavk/artifacts/immutable-descriptor-host-final-20261021`.
Fresh Android Core five gates/13rows, descriptor generations/templates128words,
null2D/live-array/3D-storage/cube1792words each, BC123/45/67/texture regressions
PASS with their own validation/SyncVal gates. Probe hashes identify the exact
pre-allocation-cache build; final layer regressions above cover that last
allocation-only change. Runtime guard compiler/separate sampler/atomic modes
were not rerun by this probe build; earlier evidence is not relabelled as fresh.

Muck30257 mapped isolated final layer;47466protected executions at capture,
no recorded gap/device-lost. Only startup/render-log regression, not new
gameplay/visible FPS proof. Game stopped; its layer override removed from both
stores. Logs in game-regression.tar.gz. Common runtime rollup/proof in STATE.md.

Common runtime `1f3566b1426d26fc065c452fb1950201ba0cfdbc0b3515cf90f561f2048727f5`
installed in signed asset-only APK14, same baseline certificate; only layer+
runtime manifest changed (261other runtime members unchanged,485other APK members
unchanged). APK13/commonb047 preserved for in-place rollback; no data clear.
Common-library oracle PASS192components/3dispatches/5total submits/Corezero.
Normal Steam Muck406 mapped common1f35 with no per-game override and recorded
55084protected executions at capture,gaps0/device-lost0. Steam retained
ANGLE/Zink/Mali hardware renderer. Common proof in common14-results.tar.gz.
Stopped test game/probe, restored absent/default-manual sleep preference;
Steam/data preserved. Production U-AVF unchanged. No commit/push.

## References

- [Immutable sampler initialization, writes and lifetime](https://docs.vulkan.org/refpages/latest/refpages/source/VkDescriptorSetLayoutBinding.html)
- [Copy valid usage, including immutable standalone destination prohibition](https://docs.vulkan.org/refpages/latest/refpages/source/VkCopyDescriptorSet.html)
- [Identically defined objects](https://docs.vulkan.org/spec/latest/appendices/glossary.html#glossary-identically-defined)

No CTS or full compatibility claim. Next D3 dependency: empty layouts and unused
sets; then D4 submit-time/version-aware descriptor lifetime. Do not accept
update-after-bind flags by removing checks from record-time snapshots.
