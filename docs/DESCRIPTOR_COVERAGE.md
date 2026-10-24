# Descriptor compatibility coverage — 2026-10-20

Active package boundary: DESCRIPTOR_CHECKPOINT_APK18.md. General UAB and P2
descriptor expansion are deferred; COMPATIBILITY_PRIORITIES_2026-10-21.md now
orders P0/P1 across subsystems. No CTS case has been run for Mavk. Historical
sections below retain their original evidence dates and narrower scopes.

This is the shared Core/layer contract, not a Muck-specific checklist. It
supersedes obsolete consumer-state conclusions in the 2026-10-17 capability
audit; the raw native profile and seven upstream DXVK gaps remain evidence.
Windows Bridge/ICD coverage is separate and must not inherit Android results.

## Admission rule

Keep native feature queries unchanged. `nullDescriptor` remains false. The
experimental Sarek diagnostic admission is not a conformant driver feature.
NATIVE denotes a driver operation, not proof that guarded Mavk preserves it.
EMULATED below always means only the explicitly tested slice. Creation,
reflection, GPU execution, and real-consumer results are distinct gates.

| Priority / class | Native vs guarded Mavk | Current evidence / gap |
|---|---|---|
| P0 null sampled 2D-array | EMULATED bounded fetch/sample slice | All-null fetch/sample1792words/512guards. Live/null two-layer fetch and explicit-LOD sample1792words/384guards each, view baseLayer1, SyncVal0/0. Actual Muck world renders. Interpolation/mip/implicit derivatives/query/atomic coverage still missing |
| P0 null static UBO/SSBO | EMULATED slice | Existing null/range/integer-atomic GPU tests; dynamic offsets and all instruction forms not covered |
| P0 null sampled/storage color2D | EMULATED slice | Existing 1792-word/384-guard two-phase GPU oracle; texelFetch and R32 image load/store, not all sampling/query/atomic operations |
| P0 null color1D/1D-array | EMULATION NEEDED beyond evidence | 1D consumer/reflection history; new generic array candidate not a GPU PASS for 1D-array |
| P0 null color3D | EMULATED bounded sampled/storage slice | GPU1792words/512guards for sampled explicit LOD; second GPU1792/512 for sampled fetch + live/null R32 storage3D, SyncVal0/0. Actual3D backing/view, not2D substitution. No live sampled volume/mip/query/atomic claim |
| P0 null sampled cube | EMULATED bounded explicit-LOD slice | GPU1792words/512guards, six sampled directions, all6 backing faces initialized/read back in each phase, SyncVal0/0. No live cube/filter/mip/query/atomic/full feature claim |
| P0 null cube-array/MS/depth | UNSUPPORTED in guarded slice | Native hardware has image capabilities; correct null semantics and compatible backing/views unimplemented, reject explicitly. Storage cube also rejected |
| P0 null texel buffers | EMULATED R32 slice | Existing GPU R32 oracle; scalar signed/float reflection. Typed-null format/atomic coverage incomplete |
| P0 bound texel formats | NATIVE + metadata | RGBA8 family plus scalar/vector32 registry53/39; actual GPU native/protected UNORM/UINT512components,64texels/nonzero offset/WHOLE_SIZE two-byte tail. RGBA32_SFLOAT prior consumer views/elements6. Metadata not native capability/typed-null advertisement; storage RGBA8,other widths/packed formats remain unproven/missing. RGBA8_TEXELS_2026-10-21.md |
| P0 sampled depth/stencil layout | NATIVE + metadata | Shared aspect/layout policy, descriptor regression20/20; real lobby/UI advanced. Not a complete mip/layer/view GPU matrix |
| P0 vertex instrumentation | EMULATED protection slice | No-report compiler retains checks without unavailable vertex writes. Compiler25/9 and actual vertex pipelines; no native atomics feature claimed |
| P0 static writes/copies/templates | NATIVE + shadow | Existing descriptor/template/generation tests; immutable generation leases; full update-after-bind semantics do not follow from snapshot tests |
| P1 sampler heap late updates | NATIVE narrow passthrough | Exact one-binding non-null sampler heap is kept native; not general image/buffer update-after-bind support |
| P1 indexing / partial / variable counts | PASSTHROUGH MISSING in guarded path | Driver supports subfeatures; layer rejects most extended layouts. Snapshot-at-record cannot implement arbitrary late updates |
| P1 dynamic buffer descriptors | NATIVE + protected tracking, bounded GPU PASS | Seven UBO/SSBO dynamic descriptors/two sets, partial binds, immediate caller offset-array reuse, nonzero base+offset and WHOLE_SIZE/zero offset. Native/guarded output matched512words/four dispatches. Graphics/BindSets2/null-dynamic GPU cases remain open; DYNAMIC_DESCRIPTORS_2026-10-21.md |
| P1 immutable samplers | NATIVE + owned tracking; VERIFIED COPY COMPATIBILITY | Standalone initialization, combined writes/copies/templates and layout compatibility GPU192components PASS. Native G925 copy mismatch independently reproduced in Android with SyncVal0/0; common shadow preserves destination sampler. YCbCr conversion remains explicitly unsupported; IMMUTABLE_SAMPLERS_2026-10-21.md |
| P1 empty layouts / unused empty slots | NATIVE + protected tracking, bounded GPU PASS | Mixed sets0/2 with unbound empty set1:512words; three all-empty layouts and descriptor-free rendering:1024components match native. Registered-empty vs absent-placeholder contract retained; EMPTY_LAYOUTS_2026-10-21.md |
| P1 selected entry points / whole unused NONEMPTY sets | NATIVE + protected tracking, bounded GPU PASS | Name+stage/call-tree/ID-aware analysis, group decorations. Unbound dynamic set1 between used0/2:512words; three unused nonempty layouts:1024components native/guarded match. Missing used set rejects before submit. Partial arrays remain missing; STATIC_USE_2026-10-21.md |
| P1 unused BINDINGS inside USED sets | NATIVE + protected tracking, bounded GPU PASS in common APK17 | Seven unwritten unused bindings/eleven dynamic offsets,512words native/protected match. Used-array-element unwritten rejects before submit. Host12/16 + nine rebuilt suites; prior layer and fresh Core/image/descriptor/BC regressions PASS. No partial arrays/general UAB admission; SELECTED_BINDINGS_2026-10-21.md |
| P1 SPIR-V grouped descriptor decorations | Common reflection + compiler normalization, bounded GPU PASS | Shared reflection resolves groups; guarded compiler strips original debug and flattens groups before instrumentation. Valid grouped immutable oracle192components matches control; no new Vulkan feature advertised |
| P1 null vertex / conditional null index | EMULATION NEEDED | Existing isolated work is not full enabled-feature contract; do not advertise nullDescriptor |
| P1 barriers / lifetime / resubmission | PARTIAL, bounded repeat/pending GPU PASS | Same-queue image initialization, leases, submit1/2 pre-driver rollback tests exist. New native/protected512word oracle: two fence-completed replays and two timeline-blocked SIMULTANEOUS_USE submissions, reset after completion,gaps0/Corezero. Ownership/multiple queues/late updates remain open; DESCRIPTOR_LIFETIME_2026-10-21.md |
| P1 other shader stages / image atomics | UNSUPPORTED forms | Compiler explicitly rejects OpImageTexelPointer and selected pointer/atomic forms. Geometry/tessellation native availability is not guarded correctness proof |
| P0 execution command routing | NATIVE + shared guard preparation, bounded GPU PASS | DrawIndirectCount/DrawIndexedIndirectCount and DispatchBase core/KHR now use prepareExecution. Actual-loader native/guarded comparison512words/8192components PASS. AMD wrappers implemented but native extension unavailable/unexecuted. Other extension draw families remain unaudited |

## Small parameterized GPU suite, not a second infrastructure

Reuse `null-images.comp`, `null_images_comparison.hpp`, existing descriptor
tests, shader compiler, validation capture and report verifier. The current
array fixture is a parameter of the existing two-phase oracle, not a new demo.
Each implemented cell must assert GPU results, live-resource behavior, null
reads, discarded writes, zero resource leaks and validation/synchronization
results. A skipped/unsupported cell is never PASS.

Matrix axes: image dimension/view (1D,2D,3D,array,cube,MS,depth); scalar kind and
native format features; mip/layer/subrange; operation (fetch,sample,query,load,
store,atomic); descriptor state (live,null,unwritten,updated,copied); shader stage;
record/update/submit ordering. Generate only legal combinations from the actual
enabled device features. Null samplers/input attachments are not invented
requirements; consult their own valid usage. Bound and null resources have
different metadata/fallback responsibilities.

Next after array GPU + consumer: cover image shape/operation cells first, then
descriptor lifetime/late updates. Keep unsupported shapes visible, eliminate
duplicated admission gates, and avoid enabling any whole feature from a partial
test. Run the real consumer after each shared semantic change, not each matrix
cell. Preserve the working Steam/native sampler path.

## Descriptor audit closure and implementation batches

Audit means the implementation boundary is identified, NOT that every cell is
implemented. Source reviewed: `vulkan_descriptor_updates.hpp`, `descriptors.hpp`,
`native_descriptors.hpp`, image/texel backing, shader reflection/compiler, layer
layout/bind/update/record/submit paths. The guarded path currently accepts only
seven kinds: UBO, SSBO, uniform/storage texel, sampled/storage image, combined
sampler. Standalone samplers have a separate narrow native heap path. Other
legal Vulkan descriptor kinds must not be inferred from those seven.

| Batch / dependency | Legal combinations to preserve | Remaining boundary / acceptance |
|---|---|---|
| D1 metadata/static updates | Native-supported buffer-view format/usage, whole vs explicit range; compatible image usage/aspect/layout; writes/copies/templates across compatible consecutive bindings | RGBA8 plus scalar/vector32 metadata, host53/39 and9other suites; bounded native/protected texel512components. No GPU proof for every format/layout |
| D2 null image operations (D1 + compiler) | Separate dimension/view, scalar type, fetch vs sample vs query, mip/layer, storage reads/writes; multisampling/depth only when enabled and correctly represented | 2D live/null fetch,2D-array live/null fetch/explicit LOD, null3D sampled fetch/sample and live/null R32 storage3D tested. Array view baseLayer1/two distinct layers. No cube/MS/depth or query/atomic claim. Never substitute a 2D backing for a different declared shape |
| D3 descriptor binding model (D1) | Dynamic UBO/SSBO offsets with device alignment/range; immutable samplers; empty sets; separate image/sampler; compatible pipeline layouts | Dynamic buffers, ordinary immutable samplers, empty/unused sets and selected bindings GPU PASS, no buffer copy. YCbCr and general indexing remain missing; native sampler heap exception is not general UAB support |
| D4 late updates/lifetime (D1,D3) | Enabled indexing flags, partial/variable counts; update-after-bind vs ordinary update; reset/free/resource destruction; primary/secondary and repeated submits | Record-time immutable snapshot is insufficient for legal late updates. Requires submit-time/version-aware model, with no mutation of in-flight generations. Do not enable by removing admission checks |
| D5 robustness/stages (D2,D4) | Null reads return specified values; discarded stores/atomics; buffer OOB and image OOB distinguished; vertex/fragment/compute and other enabled stages | Read-only instrumentation preserves checks without diagnostic storage writes. Image atomics/pointer forms still rejected. Full robustness2/nullDescriptor remain unadvertised |

Non-null samplers and input attachments remain subject to their own Vulkan
valid usage; nullDescriptor is not permission to invent null sampler/input
attachment behavior. Inline uniform blocks, acceleration structures, mutable
descriptor types and descriptor-buffer paths are unsupported by guarded Mavk,
not universally unsupported by Mali. They are not D3D11/Sarek-first priorities.

After descriptors: formats/image views/transfers (including BC), then memory
visibility/barriers/ownership and timeline/fence semantics, then pipeline/SPIR-V
and required raster features, finally WSI/lifecycle. Each batch reuses the
existing GPU oracles and regresses previous batches. DXVK upstream's seven
mandatory gaps remain separate acceptance gates; do not advertise them from a
Sarek run. Modern Zink and Gamescope requirements need pinned-source validation
before claiming broad compatibility, rather than extrapolation from Steam UI.

### Verified package checkpoint, 2026-10-20

Host tests in `E:/Mavk/artifacts/descriptor-package-20261020`:
descriptor state34/25, bound18/6, updates20/20, sampler9/7, templates13/18,
texel39/32 (assertions/negative cases). These are CPU contract tests, not GPU
conformance. Existing GPU null-array fetch, baseline2D, quiet-array and new
explicit-LOD array sample each verify1792 words, zero live resources and
SyncVal0/0. Reported guard counts512/384/0/512 respectively; quiet oracle checks
expected accesses through pixel/write outcomes, not absent report counters.

Filtered variant uses `MAVK_FILTERED_IMAGE` + `MAVK_ARRAY_IMAGE` in the existing
shader and the same two-phase storage oracle. Both array sampled slots are null;
this is NOT live-array/multilayer/linear-filter interpolation coverage. The
historical harness JSON scope string still says texelFetch; exact shader hashes
and `array.filtered.*.spv` artifacts define that older operation, not that label.
The new harness emits `sample_operation`, actual guard counts, diagnostic policy
and live-layer/base-layer fields, fixing that reporting ambiguity.

### D2 live-array checkpoint — 2026-10-20 20:26 Moscow

Existing oracle now accepts `--ez liveArrays true`, ONLY with an array shader.
Two real RGBA8 images have three physical layers each: sentinel201 at layer0,
two distinct constant colors at layers1/2. View selects baseLayer1/count2 and
shader alternates view layers0/1. Across two phases the live/null descriptor
slots swap; exact colors must survive and null slots must read zero. Existing
R32 storage live/null write-discard and fallback readback checks remain intact.
This verifies subresource selection and descriptor generation replacement, not
all sampling semantics. No Core/production layer feature queries changed.

Results under `E:/Mavk/artifacts/null-array-gpu-20261020`:

- `live-layers-result.json`: explicit LOD,1792words/384reported guards, SyncVal0/0.
- `live-layers-fetch-result.json`: texelFetch,same1792/384 and SyncVal0/0.
- `layers-all-null-regression.json`: same new shader/all-null mode1792/512,
  including access to layer1; SyncVal0/0.
- Filtered guarded shader SHAfe08de046f62a6b2e16dab11206cf4799a11ef1334d58b5c11d5321c1885247f;
  fetch guarded SHA4cb76a69bffcf4b37830405424e15bec36a1c9124ebd322f8eb2970417eb242f.

Fresh five-gate/13-row Core regression and GPU descriptor generations/templates
(128words,2generations,3negative cases each), BC123/45/67 and BC texture lifecycle
all PASS. Reports: `E:/Mavk/artifacts/descriptor-layers-regression-20261020`.
GPU report modification times were checked against device time; these are new
runs, not old archived verifications. Host six-suite regression also PASS.
Installed test APK6fba8c9802a9d7a4f8f30ab9a31c0c144497061d913d7f8ff9c6999f52276ddd
uses the same new native binary with fetch shader asset; no DroidDeck APK change.
No new Muck run: changes were scoped to the parameterized harness/oracle, not
shared runtime semantics. Existing real-game evidence remains baseline.

### D2 3D implementation — 2026-10-20 20:36 Moscow

Common reflection now retains non-array color3D shape. Typed null resource cache
creates an actual VkImageType3D/VkImageViewType3D after native format/usage query;
layer passes the reflected type instead of treating all non1D as2D. Existing
initialization/prelude/lifetime model reused, no new transport or feature bit.
Cube/depth remain omitted and nonexistent3D-array declarations are not admitted.
Reflection fixture now checks those distinctions and all prior scalar formats.

GPU artifacts `E:/Mavk/artifacts/null-volume-gpu-20261020`:

- `volume-result.json`: null sampled3D explicit LOD + existing R32 storage2D
  controls,1792words/512guards, SyncVal0/0.
- `volume-storage-result.json`: null sampled3D fetch + live/null R32 storage3D
  load/store,1792/512, SyncVal0/0. Actual SPIR-V `OpTypeImage float3D Sampled1`
  and `uint3D Sampled2 R32ui` inspected in `volume.storage.guarded.spvasm`.
  Report has no storage-dimension field; that scope is tied to this exact shader
  and APK, not inferred from generic report labels. Storage extent8x8x1 only.
- Guarded shader hashes b97ee1e2d0e1b7e328e5f3965c6281661b78e0868dab5016c6cde27571a18930
  (sample) and4ac3efb65d2309e675e8f59e9ca7e7c232422d63940f02e51e585df23e16c5e9(storage).
- `baseline2d-regression.json` and `array-regression.json`: same newly compiled
  native code with previous shader assets,1792/384each, SyncVal0/0. Core5gates,
  descriptor generations/templates and BC sets also freshly rerun; six host
  tests rebuilt/run against current headers. Reports in
  `E:/Mavk/artifacts/descriptor-volume-regression-20261020`.

Isolated layer02be618634697bb34cb2a3db0bf30e9b24bc75cc6d6e221abff6ac6dae9ae185
deployed in fresh `files/mavk-null-volume`, old libraries preserved. Muck actual
process8057 mapped it, reached correct animated3D menu; two observed captures
and logs archived (`muck-regression*.png`,`muck-regression-logs.tar.gz`). No
Mavk gap/device-lost seen. Menu regression only, not gameplay/FPS/whole-game PASS.
Game stopped afterwards, Steam session retained. Profiling remains off.

Still missing: sampled live volume/depth slices, storage depth>1, full numeric
format/operation matrix, mip/filter/query/atomic semantics, and combinations of
same-format dimensional fallbacks in one submission. Partial3D evidence must
not become a full nullDescriptor advertisement. No game-specific rule added.

### D2 sampled cube — 2026-10-20 20:48 Moscow

Real cube view over square2D image, CUBE_COMPATIBLE flag and6layers. Native
format/usage query includes that flag; sampled/transfer usage only (storage cube
explicitly rejected). Initialization barriers/clear cover all6faces, not just
layer0. Common reflection admits sampled non-array cube; depth/cube-array remain
excluded. No native feature/extension query changed.

`E:/Mavk/artifacts/null-cube-gpu-20261020/cube-result.json`:1792words,512reported
guards,14fallback readback words (6cube faces+1storage texel in each of2phases),
SyncVal0/0, Corezero. Shader cycles +/-X,+/-Y,+/-Z directions; sampled slots are
both null, not live cube correctness. Guarded shader SHA
af777e1a77053e510d9a34bca52c3a24004cc1e9e936a996d485146e8b18095c.
Oracle reports exact cube face count, storage dimension and fallback coverage.

Six rebuilt host suites + shape reflection PASS. Same new native binary with
2D/array/3D shader assets PASS, as do fresh Core5gates, GPU descriptor generations/
templates and BC123/45/67/texture lifecycle. Reports
`E:/Mavk/artifacts/descriptor-cube-regression-20261020`.

Real Muck process12280 maps `files/mavk-null-cube`, layer SHA
e87b886e7e1a2e9ff764281f3c89d14de02fc438198845acb07b573eedbe10f9;
correct3D menu, no observed layer gap/device-lost. Captures/logs archived in
cube GPU artifact directory. Menu-only regression, not gameplay/FPS claim.
Game stopped, Steam session2026-10-20-01-steam suspended/preserved. No commit/push.

Next descriptor integration priority: intercept native indirect-count draws and
base dispatches, preserving aliases/arguments/feature availability. They currently
bypass guarded descriptor preparation; test actual GPU work at nonzero offsets/
counts, not just that a function pointer exists. Native untouched pipeline path
must remain unchanged. Cube-array/depth/MS and broader null semantics stay open.

### Protected execution routing — 2026-10-20 21:14 Moscow

Added core/KHR DispatchBase and core/KHR/AMD indirect-count draw wrappers using
the existing per-command preparation and mutex. Exact spelling forwarded to
the downstream driver, all offsets/counts/base axes preserved. Existing next-
function availability checks still gate GetProcAddr: no new advertised features
or fabricated extension entry points. No special consumer/game branch.

`compatibility/test/layer-execution.cpp` is a standard Vulkan loader consumer,
reusing Core + shared offscreen renderer. Test-only linker wrappers select
indirect commands in that renderer; they do not replace Mavk's command hooks.
Input3D base(2,3,1), counts(2,2,1) must write only4expected words in256; indirect
arguments at offset16, count at offset12, count0/1 must produce clear/texture
respectively. Indexed and nonindexed compared with direct hardware reference;
reference itself checked against exact RGBA(23,47,91,255), not arbitrary pixels.

Native and guarded runs through the SAME candidate library each PASS:
512words,8192color components,2dispatch cases,8draw cases; guarded layer logs
12prepared executions/12submits,gaps0,remaining resources0,Corezero. Core+KHR
aliases executed; AMD extension unavailable and explicitly NOT a GPU PASS.
`verify-layer-execution.py` checks both archived results and exact equality.
This run has NO CTS/SyncVal instrumentation and makes no performance claim.
Not coverage of stride>one command,negative base vertex,all indirect limits or
every extension draw family. Do not inflate this to full Vulkan conformance.

Artifacts `E:/Mavk/artifacts/layer-execution-consumer-final-20261020`:
native/guarded stdout+stderr,gpu-results.tar.gz,Muck startup log archive.
Candidate mapped path `files/mavk-execution/libVkLayer_mavk_compat.so`, SHA
f614df284058d4f7941539e130b372fe2923a01c5e9b4ea663d159e405d4dab7.
Consumer9d62f6f2856388ca4affb2c017be913a9cc7ec0e7e440385d2965d7afa7dfc91.
Muck16983 mapped candidate and recorded real guarded draw work with no observed
gap/device-lost. Android keyguard prevented a visible Surface/menu check; only
background startup/render-log regression claimed. Game stopped afterwards.

Screen-off/manual policy suspended DroidDeck before early test commands could
start (no native output files then). Temporarily used EXISTING app preference
suspendPolicy.steam=never via project agent; reloaded existing service action
as same app UID/current user0. No OS security/root/SELinux/lockscreen change.
Original preference was absent; removed it after tests and refreshed service,
returning default manual policy and preserving/suspending Steam session.

Host six-suite regression remains PASS. Attempted fresh Android Core Suite
did not produce even first report; process17421/worker17464 remained alive in
top-sleeping screen-off state, logs included ART userfaultfd timeout. Normal
wake did not finish it. Cancelled org.mavk.probe only; no root cause assigned.
NOT a fresh Core PASS and not evidence of new routing failure (this standalone
Bionic harness does not load that glibc layer). Failed report collection renamed
core-regression.incomplete.tar.gz; own-process journal under
`E:/Mavk/artifacts/layer-execution-regression-20261020/probe-pending.log`.
Previous verified Core/BC/image evidence retained, not relabelled as this run.

Follow-up: repeat standalone suite when normally usable, promote vetted layer
into common experimental DroidDeck runtime bundle rather than leave only Muck
override, then proceed with D3 dynamic/immutable descriptor semantics and other
missing execution commands gated by actual native capabilities. Production
U-AVF untouched; no commit/push.

CTS has NOT run. Existing validation/SyncVal and bounded result checks are not
CTS or Vulkan conformance. A useful CTS run must target the implemented feature
slice without forcing unsupported feature bits; current native CTS would test
Mali, not automatically exercise guarded Mavk's experimental null emulation.

## Primary references / reproducibility

- [Khronos descriptor sets](https://docs.vulkan.org/spec/latest/chapters/descriptorsets.html)
- [Khronos robustness2 feature definition](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceRobustness2FeaturesKHR.html)
- `NULL_DESCRIPTOR_CONTRACT_2026-10-18.md`: detailed scoped requirements.
- `TURNIP_DXVK_CAPABILITIES_2026-10-17.md`: pinned DXVK requirements/native profile.
- `MUCK_2026-10-20.md`: consumer checkpoints, hashes and rollback.

Zink/Steam hardware UI is proven on the retained private Mesa build, not evidence
of modern Zink's complete nullDescriptor requirements. Gamescope composition is
running but its broad compatibility requirements are not yet audited as a suite.
Upstream DXVK still needs the seven explicitly recorded missing mandatory
features; Sarek device/scene progress must not be substituted for that gate.
