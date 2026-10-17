# Mavk checkpoint — 2026-10-18

## Active development — shared Compatibility, Windows preserved

2026-10-18 Vulkan descriptor update frontend actual PASS:
compatibility/VULKAN_DESCRIPTOR_UPDATES.md documents bounded ordinary Vulkan
write/copy/layout structures -> common state -> native materializer. Trusted
native-handle registry retains buffer/image/sampler owners; transactional ordered
writes/copies,consecutive bindings/zero-count skip,whole-size and null validation.
Null combined-image retains its valid sampler;destroyed references become
undefined/reject snapshots,not stale handles. CPU16 assertions/16 negatives PASS;
existing DescriptorState34/25 regression PASS (LLVM runtime DLL directory on PATH
required for the host test executable; initial missing-DLL launch was NOT PASS).

Final APK vulkan-descriptor-api-native2-20261018 SHA256
817F41130C379ACD9A0DA32322D1051D6E967404FBF2F7EC6F3F4669498A5FF8.
Installed only org.mavk.probe. Fresh reports under same artifact directory:
- generations128words,two VkWrite structures+one VkCopy,real recorded lifetime,
  cache,3rejects:7ea67d88bfd9739255930582db1f2f51c9b71ecd99db2952136de25968d8e537;
  strict verifier --vulkan-api PASS.
- images1792words/384guards:
  58bc8fbecdeec87cd1e8947c4eeb15cfccdca510a745098e607872abbca24e09.
- atomics1536words/1024guards:
  cc761fd34892100e3cbde262e78e4a4e1eb4898ebb47a46fd53c0758df5efb64.
All SyncVal0errors/0warnings,Core live zero. No loader exports/full command-buffer
invalidation,automatic shader admission or full nullDescriptor/robustness2 claim.
No DXVK device/frame PASS. NEXT: consumer layer integration and real per-format
BC coverage;reuse existing hybris BC machinery where applicable,not duplicate it.

Reuse lead only,NOT tested BC evidence: DroidDeck fork0916a538 pins libhybris
bc8cb358 +protocolc6ae3c22;tools/hybris/patches/0001-dxvk-bc-profile.patch implements
per-format GPU BC decode. dxvk policy deliberately skips BC1RGB;0002 vendor patch
keeps full textureCompressionBC false (RGB/blit/filter guarantees incomplete).
Existing per-format DXVK patch may avoid requiring the global BC boolean ONLY
with real required-format checks. Do not widen G715 guards or claim G925 PASS.

Disk: original Windows ISO staging directory(6.755GiB) hash-verified relocated to
E:/U-AVF/inputs/win11-iso-work-20261018;old win11-iso-work path is a junction,
retired duplicate absent. ISO bytes unchanged,no VM disk/clone. C still47.71GiB
because independent installer consumption;I/O sample showed Epic helper~31MiB/s.
User will pause Epic;process untouched. Proposed old backup relocation command
was rejected before execution;backup remains on C,no claimed50GiB reserve yet.

Newest shared NativeDescriptorAdapter actual PASS. Portable native Vulkan API
materializes real immutable pool/set/metadata/log generations from CPU snapshots;
explicit shader ABI,typed resource/fallback resolver,strong leases and cached
unchanged generations. No updates to older sets,no hidden destructor queue idle.
Currently set0public+set1guard,no loader hooks/full descriptor API,see
compatibility/NATIVE_DESCRIPTOR_ADAPTER.md. Diagnostic buffers remain enabled;
changed-generation allocation/recycling and full consumer entry points OPEN.

Actual Mali lifetime gate: two ordinary compute command buffers recorded with
different generations BEFORE either executes,then shadow state/caller resource
references dropped. Both original outputs PASS128words,distinct sets,old leases
survive then expire after GPU completion. PoolCreates2/metadataUploads2/updates2,
3 invalid materializations reject before allocations,cache hit no reallocation,
SyncVal0/0,Corezero. Native adapter now used by existing buffer/atomic/image gates
instead of their own VkCreate/Allocate/Update implementations.
Latest installed APK native-descriptor-adapter3-20261018 SHA256
992BB6DA80A3042C4F549EC6E2AD302C194293A907E3E114A021ED978EB68A73.
core-descriptor-generations.json SHA256
44987046bf7c2af076b314056002fcef31558c51c512dda8d7fd4b2467a4b944.
Strict verify-descriptor-generations.py PASS. Same APK native null regressions:
atomic1536/1024guards report47c7444c5e5f19da6fb8e752c75c85f34aea58f2b43471ef67cc462d5f487e7e;
image1792/384 report58bc8fbecdeec87cd1e8947c4eeb15cfccdca510a745098e607872abbca24e09;
buffer1536/896 report90811e0f41a96376cdb1cfc1a6b749c81dc174e8c76edad6b470e1d980368068.
All SyncVal0/0,Corezero. Initial adapter2 lifetime test failed only at host
invalidate-before-map (c86cb423... report retained),fixed mapping before readback;
no platform changes. Adapter1 compile-only never deployed. No live build/test.
NEXT: real consumer Vulkan update/copy/image entry points and shader/layout
admission;complete BC/null/robustness semantics before Sarek D3D11 device/frame.
No feature bits advertised,no DXVK PASS,no Windows/Ubuntu/P7/H264/storage edits.

Space housekeeping: original LLVM archive/download pieces(16 files,1.083GiB)
hash-verified on E:/Mavk/artifacts/toolchain-download-cache-20261018,source copies
removed;actual extracted toolchain paths untouched. Historical September28 APKs
also hash-archived(8 files,2.007GiB) to E:/U-AVF/artifacts/archived-test-apks-20261018;
C51.266GiB free after relocation. Only ignored historical installers,not current APK,data or
P7 images. This is recoverable file relocation,not release/source changes.

Newest component common DescriptorState (compatibility/DESCRIPTOR_STATE.md):
portable C++17,explicit unwritten/null/bound states,checked buffer ranges/whole
size/alignment,kinds,overlap-safe copies,rejection atomicity,resource leases,
generation+immutable per-ABI cached snapshots. CPU34 assertions/25negatives PASS.
Ownership identity includes control block,not just raw address. Snapshots retain
resources across descriptor-state mutation/teardown;caller retires afterGPU.
Unwritten slots rejected,NOT silently treated partially-bound. This CPU model
does NOT own native descriptor sets/pools or allow updating in-flight sets.

Actual native gates now use that state for BOTH native VkWriteDescriptorSets
values and guard metadata,instead of hand-packed offsets. Real Core Buffer and
SampledImage owners plus complete storage-image+view leases,valid null fallback
resources retained. Synchronous fence ownership,not async update-after-bind.
shared-descriptors-native1-20261018 APK SHA256
B43E1D628C337225FEF34C0FFC8DF359B565C777D062BB7BF1C84A2C2D8E6134.
Fresh strict --shared-state gates on this APK all actual PASS:
- atomicIAdd32+scalar range/UBO:1536words/1024guards,SyncVal0/0,Corezero;
  report30515e1eed6f201a7ecdc6c670deea5af78bd975e2d5069b143b42926ba95884.
- combined/storage image:1792words/384guards,SyncVal0/0,Corezero;
  report58bc8fbecdeec87cd1e8947c4eeb15cfccdca510a745098e607872abbca24e09.
- stock buffer-only ABI:1536words/896guards,SyncVal0/0,Corezero;
  report79a03d0883ab83c9125174ccc265109daa929499b8c0f16d310d26639d846ac4.
Snapshot cache pointer reuse checked natively in each phase. Tiny GPU command
times NOT production overhead/FPS evidence. No full null/robustness2/BC feature
advertising,no DXVK/D3D11 PASS. Existing shader bytes unchanged,4a0c898 compiler
and old standalone BC/native gates preserved;no Windows/Ubuntu/H264/VM changes.
No live build/test worker. C54.9GiB,E874.1GiB,tablet54280676KiB free,all>50GiB.
NEXT: consumer Vulkan descriptor/image entry points and shader/layout admission,
typed fallback cache,remaining arrays/image/atomic/robustness semantics before
actual Sarek device/frame. No VM boot needed for every shared function.

Newest integer buffer atomic extension actual Mali PASS:32-bit atomicAdd returns
zero/no memory change for logical null; normal atomicAdd returns previous value
and applies increment. Same gate retains scalar null/range/UBO checks;1536 words,
1024guards,2submits,total GPU0.0347692ms,tiny correctness workload only.
SyncVal0errors/0warnings,callback1,Corezero. Strict --atomics verifier PASS.
Installed isolated APK null-atomics-native1-20261018 SHA256
E1DC7B0BCA8A2C8160F718E19D5307F983F2E96FF0138EDB5DF42A0AF48BD638.
core-null-atomics.json SHA256
51b7d15c9aad103d4d270f7efde6b11aced10e200292538fd8e6289d43195671.
Same APK fresh stock buffer regression PASS1536/896/SyncVal0/0,
report845265a2bc25156a8ef945a23d8e5fefe65a64b7065649f529312cc23837c836;
image regression PASS1792/384/SyncVal0/0,report98d28129... as below.

Shared configurable host shader compiler built and executed successfully from
upstream SPIRV-Tools eb0a366+headers85a1ed2 with reviewed buffer-atomics.patch.
Compiler executable SHA256
5134087A8C13CCACF722930FCFB67BF7982EECD2913790107C07A90403E39FA7.
Generated/independently validated shader SHA256
A0EF7D95782E1C7F551CF0EDF8FCA918BB5CDD8CF8F00486094ADC0C2720F381;
rebuild after flag-atomic preflight rejection emits identical shader bytes.
Patch adds core integer buffer atomic analysis+explicit type_traits include;
no Adreno source,Windows/Ubuntu/owner APK/platform/display/storage changes.
Stock legacy CLI helper still rejects ALL atomics. Combined compiler rejects
image/flag/float/pointer-arithmetic forms. All admitted opcodes are NOT certified:
only IAdd32 tested; out-of-range atomic,other atomic semantics/stages,full image
ops/dynamic arrays,robustness2 and complete BC Vulkan API remain OPEN.
No native/ICD feature advertising or DXVK/D3D11 PASS. Runtime Android shader
compiler/cache and common consumer descriptor/image adapter remain next.
No pending build/test worker. All sessions26072/35217 completed,do not resume.

2026-10-18 current extension: null sampled/storage images actual Mali PASS1792
words/384guards,4Core submits (2initial uploads+2dispatch),SyncVal0/0,Corezero.
Tests two static slots switched by metadata; RGBA8 texelFetch includes zero alpha
for null; R32_UINT imageLoad zero/imageStore discard verified by separate transfer
readback. Native app only,no VM/owner/H264/Ubuntu changes. APK null-images-native1
SHA25681F58FCCB3CB791E9EF750377863C605F5102E69316C4F96F7E49D1D7DD31D29;
report98d28129aecbfae968c4d2e3fe0053f784514e10d2a49c20cf6f10f916e5f561.
Strict verify-null-images.py PASS. Legacy instrument-null.py now has explicit
--mode buffers/images and rejects core/extension atomics;4offline parser tests
PASS,not shader/GPU conformance. nullDescriptor/robustness2 stay unadvertised.

Completed compiler preparation: configurable combined SPIRV pass+minimal integer-buffer atomic
extension. External source E:/Mavk/research/spirv-tools-v2022.4 pinnedeb0a366,
headers85a1ed2; compatibility/shaders/{CMakeLists,tool.cpp,buffer-atomics.patch}
track exact reviewable changes. Missing transitive type_traits caused first host
build failure; explicit include fixes standalone header check. Build restarted
incrementally parallel6 after stopping only verified CMake build process tree;
session26072 completed shader-tool-build1-20261018 with logbuild-fixed.
Bounded atomicAdd PASS is recorded above;do not extend it to other atomics.
Old buffer/image gate still uses stockCLI+oldmetadata;
atomics branch uses combined pass's four-level metadata. No pending native test
worker: latest atomics+buffer+image gates completed. This slice is uploaded only
to private Mavk after source checks;no public release or U-AVF push.

2026-10-18 scalar null/range extension actual Android PASS:1536 verified words,
896 guards,2submits,total GPU0.0305385ms (tiny correctness only),SyncVal0/0,
Corezero. Both metadata phases prove SSBO null reads0/writes discarded,UBO
null read0,range reads0/out-of-range stores leave allocated sentinels intact.
APK null-range-native3-20261018 SHA256
58E53BCF5AF64A04D07ED1E1AD05E808FFF61CB704655CAA59B72092584E7CC7.
core-null-buffers.json SHA256
8fd6be2fd9df016dd932e84b31297ac3642a782622e090a670c8aa062da4c961.
Strict verify-null-buffers.py PASS. First range run failed because CLI buffer
mode uses three-level metadata lookup,not four-level idx/init lookup; exact
GenDebugReadInit source+generated SPIR-V identified issue. Corrected metadata
only; failed report84ecbc0fc5395f7804b85a143cbc93aa31c3006e2c92f91e71900f244c3ed231
retained in null-range-native1-20261017. native2 diagnostic build NOT deployed.
Current common limitations: atomics/images/dynamic arrays/aggregate-pointer
access,full robustness2 and full BC Vulkan API remain OPEN. No feature bits
enabled; no DXVK/D3D11 PASS. Next common shader+descriptor/format adapter,then
actual Sarek device/frame. Baselines untouched,no Windows VM/owner change.

Newest user plan saved in ACTIVE_PLAN.md: real DroidDeck/DXVK D3D11 first,
then permanent Windows IddCx,then product Windows workspace integration.
Direct permission now allows commits/uploads ONLY to private dt0-imsyu/Mavk;
remote isPrivate=true verified,main initial547eadd includes Apache2 LICENSE,
preserved. No U-AVF commit/push/publication. Mavk local Git now tracks that main.

New null-buffer descriptor-init feasibility actual Android PASS768 verified
words/384 guarded accesses,SyncVal0/0,Corezero,2GPU submits,total GPU0.0181538ms
(tiny correctness workload,NOT performance target). Metadata switches null
slots without rebuilding shader,valid SSBO writes pass,null writes discarded,
UBO null reads zero. APK null-buffers-native1 SHA256
0362CA4CD986ACE18EA071D34E50B439C15F760A5527F0FC3FBE933AAA94B1EF.
Report SHA256 e0ceacc16af1c75a8e707b224722439bbf3158a9056b1ff5d8f9285b60aae859.
Same APK BC texture lifecycle regression PASS6144pixels/SyncVal0/0/Corezero;
report d90962435bfc8217017b08cc11527595b84aa57fff6eea9f0e9e29939cf41c20.
Uses existing NDK SPIRV-Tools guards,not dummy-storage semantic substitution.
No nullDescriptor/robustness2 advertising or DXVK PASS. Scalar range extension
being built/tested next; see compatibility/NULL_RESOURCES.md for exact gaps.
Old pass omits atomics/image-init in this mode; do NOT claim full null support.
The range build mentioned above is now completed by the2026-10-18 gate at top.
Same latest APK BC lifecycle regression strict PASS6144pixels/SyncVal0/0,
report d90962435bfc8217017b08cc11527595b84aa57fff6eea9f0e9e29939cf41c20.
No pending worker/build/test. Private snapshot includes source/checkpoints only,
not APKs,keys,VM disks or E:/ artifacts. Existing remote LICENSE unchanged.

Newest milestone common BC texture lifecycle actual PASS. BcTexture owns exact
compressed shadow+decoded cache for16 format identities; mip init masks, padded
block-region updates, same-format/overlap copies, dirty revisions and stale-ack
protection. CPU300 assertions/19 negatives PASS. Only touched blocks decoded.
Core adds explicit synchronous partial update (one staging+submit/fence per
upload, retained image/view/sampler+pipeline), native sRGB sampled format plus
default UNORM/float, runtime format checks. Native source bc_texture_comparison.
G925 PASS6144 pixels,UNORM+sRGB,12 GPU submits/2pipelines,18 decoded blocks,
256 partial-upload bytes,6 invalid native updates,SyncVal0/0,Corezero.
APK bc-texture-native1-20261017 SHA256
FC1D65B17ED84291373B81CD80173601C6278393CBA593663E459886C8090834.
core-bc-texture.json SHA256
a6f533489d4a26450c1566869bba2d0584e38d6c64ab0a46862399d2065a52ee.
No guest format/feature exposure, no Vulkan ICD interception, no full BC/API or
DXVK PASS. Complete initialized mip snapshots only;2D one layer,Core1024extent
bound. See compatibility/BC_TEXTURE_LIFECYCLE.md for scope/performance boundaries.
Next real adapters need correct execution-time upload integration, remaining
image/view/array semantics and null-resource implementation. Baselines untouched.
Same-build BC67 native regression strict PASS12288pixels/SyncVal0/0/Corezero;
retained RGBA8 regression fresh PASS32768pixels/32submits/zero per-frame GPU
allocations/SyncVal0/0/Corezero. No pending build/test process. Tablet free
54340200KiB (>50GiB), C56.9GiB/E874.8GiB. No cleanup required.

Newest milestone BC6H/BC7: bounded common wrappers use unmodified pinned bcdec
80859ed3b7afb1c527a2a99d70c61457bea72d0c, MIT alternative; notices preserved
third_party/bcdec, license included in isolated harness APK. No Adreno code.
Independent Microsoft D3D11 WARP host oracle PASS589824 components:BC7 all8modes
2048blocks,BC6H all14modes unsigned3584/signed3584. Separate3502 goldens/bounds,
18 negatives including reserved modes and+/-65504 extrema PASS. First detex
oracle disagreed; third BC7 reference and WARP support bcdec, not waived errors.
Counterexamples retained in compatibility/test/BC67_ORACLES.md +experiments.
Native G925 actual PASS12288 sampled pixels,3variants x4mips,12submits,
125negative/197HDR>1 input components,SyncVal0/0,Corezero. HDR native check uses
tone-mapped RGBA8 output, not full-precision GPU oracle. No performance claim.
bc67-native1-20261017 APK SHA256
3E60872F19E6C5AF87413A83FEDE9914FE09F6C4EB6787EC0C699FF550E1ECA4.
core-bc67.json SHA256
e30aecf1e3e6b663c29176c2666e3f0ddc70cdd97ca2beac002183640b36aa34.
Strict verify-bc.py --hdr PASS; previous BC45 same-build regression PASS.
RGBA8 retained regression from that build PASS32768 pixels,SyncVal0/0,Corezero.
Next: common compressed-image upload/mip/copy/view lifecycle+sRGB and real null
resource semantics. Codec family now present, NOT full textureCompressionBC,
Vulkan compressed API,DXVK/D3D11 or Windows acceleration PASS. Guest flags closed.
No Windows VM/owner/H264/Ubuntu changes, no commit/push.

Newest milestone: BC4/BC5 UNORM+SNORM actual CPU+native sampling PASS.
Shared decodeBcChannels preserves signed values/interpolation in float RGBA;
CPU exhaustive131072 endpoint pairs,8421776 comparisons,11 negatives,324edge
images. Native Core sampled upload now explicitly admits RGBA32F alongside
default RGBA8; format/filter query and matching mip byte layout, no wire change.
Actual G925 PASS16384 pixels,4format variants x4mips,16submits,SyncVal0/0,
Corezero. GPU verification remaps signed samples into RGBA8 output, NOT a
full-precision sampling oracle. FloatRGBA is a correctness representation,
NOT the final memory-optimized production format. BC6H/7 added by newer slice above.
bc45-native1-20261017 APK SHA256
AB37D453E3A615EA4AECC68D22BF1BB9F32AACF9C056E8A5ECA0F51D4D3570F1.
core-bc-channels.json SHA256
8e6178ea22651c23ff6aac7d23d6b81e613f22d1ad22800fc717d706a5dca09f.
Strict verifier PASS; old BC123 native sampling regression PASS on same APK.
Float mip CPU3 negatives +legacy mip9 negatives PASS. No VM/owner APK change.
Native retained RGBA8 graphics regression fresh PASS32768pixels,32submits,
zero per-frame allocations,SyncVal0/0,Corezero on same APK. No pending build/test.

User authorizes work on actual missing DXVK features; remote Mavk repository
exists at https://github.com/dt0-imsyu/Mavk (read-only HEAD checked). No commit,
push, publication or remote mutation. No Windows VM/production changes.
Full Turnip750 Mesa26.3-devel upload52003 +fresh Mali GPU-service profile saved
under compatibility/profiles; audit artifacts capability-audit-20261017.
Exact reviewed DroidDeck DXVK25ca63f:53 mandatory rows,46 pass,7 missing.
Windows guarded UI candidate remains unfinished as checkpointed below.

New Sarek lead checked at b6653094a2b8f3b824ffcf8f50aaa3b3da41263b:
48 mandatory rows,46 pass; remaining BC +nullDescriptor. Clip/cull removal is
reduced semantics, NOT emulation; don't import as generic Vulkan support.
Source/profile diff in E:/Mavk/artifacts/sarek-audit-20261017/comparison1.
First actual shared module implemented: BC1 RGB/RGBA,BC2,BC3 bounded CPU decode,
edge blocks,padded rows,alpha palettes,encoding metadata. CPU PASS862 assertions,
13 negatives,324 random small images. NOT full BC1-7/ICD/DXVK PASS.
Native existing-Core decode/upload/sampling actual PASS on Mali-G925:
bc123-native1-20261017,16384 verified pixels,4 format variants x4 mips,
16 GPU submits,SyncVal0 errors/0warnings,Corezero. Isolated harness APK
4307341CF42F763B2538C74FD458EFF8D4125FFDF0A6120B3170F0E78D515994
installed only org.mavk.probe. These tiny-texture timings are NOT performance
claims. No native Vulkan capabilities were changed or advertised.
Strict verify-bc.py PASS; core-bc.json SHA256
052dea0a01b1757dad38c751fc30dffc057a564361494d268356046608b9f400.
Existing retained-graphics regression fresh PASS on same APK:32768 pixels,
32 submits,zero retained per-frame allocations,SyncVal0/0,Corezero;
core-reuse.json SHA256 a1da9fa221f64e7477ccb1157fa55205ed549577a48a9f59c36adeb9cd584195.
Next: compressed-image lifecycle and explicit null-resource semantics. Sarek remains
experimental consumer lead, not a D3D11 PASS. No owner APK/VM/transport changes.


## Active priority — Turnip / Mali / DXVK capability audit

User-directed pivot: compare a pinned modern Turnip reference, measured G925
inventory and actual Mavk coverage; identify minimum honest DXVK/D3D11 admission
gates before more implementation. No guest boot required. Preserve Windows branch.

Windows UI ICD candidate checkpoint: guarded MAVK_EXPERIMENTAL_UI_LAYERS records
per-draw snapshots and emits opcode24 ordered groups. CPU recording test PASS
(3 snapshots/5 negatives); CPU packet test PASS (2 groups/3 snapshots/6 negatives).
These are NOT loader/GPU/guest proofs. Later sampler-binding0/font-only descriptor
factory edits are UNTESTED; pipeline factory blend/packed-color/depth-disabled
admission remains unfinished. No ARM64 candidate deployment. Preserve source,
keep feature guard off in baseline; resume compiler/regression gates first.
Last known-good Windows textured application and native Bridge24 proofs below
remain unchanged. No VM, owner APK, Windows/Ubuntu/H264 or storage changes.


## Current — bounded layers command shared Bridge actual native packet PASS

2026-10-17: previous turn actual native layered UI PASS. New opcode24
RenderLayersSnapshot implemented:inner v1 packet,1..8 exact records,explicit
blend/sampler binding/texture ID/UBO/extended scene,per-draw base vertex+scissor.
Old13/14/17/23 formats and shader-binding defaults preserved. Packed RGBA8
admitted only in new extended scene. Full packet CPU/schema/descriptor/shader
admission before any packet-owned GPU pipeline; old snapshot bounds preserved.
Pure parser test PASS578 rejects incl all truncations; legacy parser regression
running. Actual original ImGui workload extended to encode Bridge packets,
upload actual font through19/20/21,render24/read15/destroy16. Runtime gate is
in-process encoded request/response, NOT FD/vsock or Windows PASS; existing
generic FD framing is not being rewritten. Tests19 negative packet/admission
cases, asserts no new GPU allocation/pipeline/submit on rejection. Actual
PASS:30 commands,19 negatives,4608 correct pixels,258 changed widget pixels,
3GPU submissions(font+2frames),5pipelines(compute+4graphics),SyncVal0/0,
Corezero. Strict verify-native-ui.py --bridge PASS.
Evidence bridge-ui-layers1-20261017/core-ui-bridge.json SHA
a594262c969537991b5362ae5a8cd4f616019a85e35d857e32b645e92fce67ce.
Harness APKB07C7453... installed only org.mavk.probe. CPU regression PASS:
new layers578,legacy scene630,shader12,geometry275 rejection cases.
Legacy descriptor texture regression actual PASS:4096pixels,40commands,
16negatives,5GPU,3pipelines,SyncVal0/0,EOF/Corezero. Default non-UI native
translation-unit compile PASS. No pending build/test process.
Native24 currently builds/retire chains per request; no new pipeline-cache or
FPS claim. Next real consumer=Windows ICD command snapshots/multiple pipelines,
but no guest advertising until implemented and tested. Current ICD Command
still keeps one pipeline and rejects bind after drawing; implement explicit
per-draw state snapshots/ordered groups rather than overwrite those fields.
Keep a guarded candidate and preserve default ICD behavior/closed gates.
No pending VM/carrier. Tablet free52.5GiB; no unrelated cleanup needed.
No VM/production owner/Windows/Ubuntu/H264 changes, no commit/push.

## Current — shared GPU target / preserved UI layers actual PASS

2026-10-17: previous turn was actual progress (native upstream UI packet PASS).
Next gap implemented in shared renderer: reference-owned color target, root
CLEAR then following pass LOAD, separate depth per pass; same command batch,
one submit and one final GPU→CPU readback. renderLayers accepts per-layer
push/UBO/scissor inputs, validates all before recording, bound16 layers. Child
retains parent pipeline/target; no raw dangling target ownership. Existing
single-layer API defaults preserved, multi-layer input misuse rejected.
Relevant gate reuses actual ImGui test, adds distinct depth-enabled background
pipeline writing a blue rectangle different from clear, verifies preservation
under/around alpha UI, checks image sharing and parent-handle retirement.
Actual PASS:2frames,2passes/frame,1shared color target,one submit/final readback
per frame; font upload adds1GPU submit,total3.4pipelines,4608 verified pixels,
258 changed widget-region pixels,4 negative API inputs,SyncVal0/0,Corezero.
Images-count assertion proves font+one color+one depth (not per-layer color).
Parent public handle retired before render; child lifetime retained correctly.
Evidence native-ui-layers1-20261017/core-ui-layers.json SHA
ffb1a024560423ecf02c081715b847f178ca9e633ac864602355697d45dde59e.
Harness APKCF793D8B... installed only org.mavk.probe. Relevant single-layer
reuse regression PASS:32768pixels,32submits,zero per-frame GPU allocations,
SyncVal0/0,Corezero. Its new timing evidence is not a production benchmark.
Bridge regression PASS:4096pixels,40commands,16negatives,5GPU,3pipelines,
SyncVal0/0,EOF/Corezero. Default non-UI native translation unit compile PASS.
Suite strict cached aggregation PASS13 bounded NATIVE rows (not a fresh full
run); SHA dc3db2f33920c2bdf65a34fce452718bc4b7cc1cfd0675f49aa092deca242a3f.
No pending build/test/VM or carrier patch. Next practical adapter step is
bounded multi-pipeline scene/packet support in Bridge+ICD, retaining actual
native state semantics and not faking guest support. Current new layer path is
native-only. Avoid more unrelated synthetic feature/benchmark polishing.
No Windows/Ubuntu/owner APK/transport changes, no commit/push.

## Current — actual upstream UI packets native PASS (not full app/guest PASS)

2026-10-17: pinned upstream ImGui1.65 sources copied with Git-blob verification
to E:/Mavk/research/ui-4ee2501; original GLSL UI shaders fetched/pinned from
commit4ee2501c8280cf71d1f9ce231cd8ebd47b58adde (no shader rewrites).
Optional -UiSourceDirectory build compiles original3cpp and carries licenses.
Shared renderer now supports native sampler binding0, exact upstream alpha
equation, packed RGBA8 vertex attributes via native geometry, per-draw scissor
and base vertex, full-target dynamic viewport. Native scene bounds checked;
Bridge wire decoder still defaults new native fields, no ICD advertising change.
Native gate creates real font atlas/widgets +two analytic draw lists, adapts
original packets, uploads font once and renders2 frames. Includes exact probe
rectangle pixels and changed widget pixel counts, not full glyph oracle/3D
overlay/complete upstream app or guest PASS. Actual native PASS:font512x64,
2frames,8 indexed draws including4 widget and6 nonzero-base-vertex draws,
4608 analytically verified pixels,258 changed widget-region pixels,3GPU
submissions,2pipelines,SyncVal0errors/0warnings,Corezero. Strict UI verifier PASS.
Evidence E:/Mavk/artifacts/native-ui1-20261017/core-ui.json SHA
01bf53a5a3ae40e8c5baafc24805ef8caa8eee9c1d94c734016e74444137db51.
Harness APK64A3FF40... installed only org.mavk.probe. Relevant raster regression
18432pixels/30negatives and Bridge4096pixels/16negatives/40commands also PASS
same new build,SyncVal0/0 and clean teardown. No VM/production change.
No pending test/build process; no carrier patch. Existing13-row suite remains
cached bounded evidence; native UI proof is separate, NOT new guest advertising.
Next concrete gap: multiple pipeline/pass attachment preservation, then native
UI packet adapter requirements in Bridge/ICD; no fake shader substitution or
full-app/Windows claim. See UI_NEXT_SLICE.md. Goal active; no commit/push.

## Current — shared native alpha blending / dynamic clipping PASS

2026-10-17: next real-app compatibility slice, not another performance benchmark.
Shared renderer adds opt-in straight-alpha source-over and per-frame dynamic
scissor (null restores full target). Default blending remains disabled; legacy
callers still render full target. Native format blend support queried explicitly.
No Bridge wire/ICD advertising changes. Existing sampled shaders reused for
18 frames:alpha0/128/255,replace/source-over,full/partial/full-reset clipping;
CPU pixel oracle and30 rejected clipping bounds. Actual native PASS:18432
pixels,18 GPU submissions,6 pipelines,no per-frame Core GPU allocations,
SyncVal0errors/0warnings,Corezero. Evidence native-raster1-20261017/core-raster.json
SHA6db8b5eeb4e7c0d375889897a71801c9a279a9ce023ed5b64143c27a23b6b1d2.
Same new build Bridge descriptor regression actual PASS:4096 pixels,40commands,
16negative,5GPU,3pipelines,SyncVal0/0,EOF/Corezero; bridge-probe.json SHA9e4e0856...
(deterministic identical report; mtime08:14:50 after APK update08:14:15 confirms
new run, not stale collection). No VM/production owner update. Suite aggregation
of cached source reports verified:13 bounded NATIVE rows,CACHED_REPORTS,strict
verifier PASS; no false fresh-suite claim or rerunning closed gates.
Evidence feature-suite-raster1-20261017/feature-suite.json SHA
62b359323aeac7c2cca1ed05b62499beca7aeaf1b136d7a23fd22d1f8c912355.
Harness APK0E088838... installed only org.mavk.probe. No pending test process.
Next concrete consumer: pinned upstream UI overlay, see UI_NEXT_SLICE.md.
Its alpha equation is NOT identical to source-over; do not silently substitute
it or claim overlay compatibility. Core native first, periodic ICD/Bridge gate
after adapter changes, WDDM separate. Goal active and incomplete.

## Current — retained renderer in shared Bridge actual native PASS

2026-10-17 next aligned step: shared Bridge uniform/sampled snapshot commands
now use one bounded OffscreenSession keyed by exact immutable scene bytes,
opcode,texture ID and UBO size; UBO values change without rebuild. Any texture
destroy and session teardown retire renderer before textures. Wire protocol,
ICD, production owner APK and VM unchanged. No guest performance claim.
Native local descriptor test extended from existing texture gate:4 draws,
changing UBO pixels, immutable-field invalidation, explicit+disconnect cleanup.
Actual PASS:40 commands,16 invalid requests,5GPU submissions,4096 verified
pixels,5592404 uploaded bytes/11 mips,compute1+graphics2 pipeline creations.
SyncVal0errors/0warnings,clean EOF and Corezero; strict verify-bridge-reuse.py
PASS. Evidence E:/Mavk/artifacts/bridge-retained1-20261017/bridge-probe.json
SHA9e4e085602328cd05b75533a08278edc2acb821d85cb1e07f9c79346509d4e1e.
Harness APK28696C07... installed only org.mavk.probe. This is NOT a guest
performance or production owner update. Next adapter regression on its next
justified Windows gate; native-first policy remains. No pending VM/test process.

## Current — native shared renderer reuse PASS; suite integration verified

2026-10-17: relevant native A/B completed, not abandoned. `OffscreenSession`
retains pipeline/target/descriptors/buffers/fence/commands; synchronous readback
and legacy one-shot path preserved.32 verified frames,32768 pixels,4 distinct
LOD outputs,2 rejected inputs; pipeline creates16→1, Core GPU allocations64→4,
zero per-frame GPU allocations. SyncVal0errors/0warnings, teardown Corezero.
32x32 validation-enabled sequential A/B median5.64942→0.339231ms;
retained initialization2.41915ms. NOT desktop/guest/visible FPS or a statistical
performance claim. Bridge integration is now proven locally in milestone above;
production owner APK and Windows runtime remain unchanged.
Evidence E:/Mavk/artifacts/native-graphics-reuse1-20261017/core-reuse.json,
SHA08e13b0844273153ff66dfc9c9937862bc45aac3cd65dafdaea0d33f1bb460b0.
Feature Suite integration completed:11 bounded NATIVE rows,strict verifier
PASS in honest CACHED_REPORTS mode; prior gates not rerun. Source hashes and
device identity verified across all4 reports. Suite evidence
E:/Mavk/artifacts/feature-suite-reuse1-20261017/feature-suite.json,
SHA2bd843d90e4e3783a79724677d8fdc6973cca51303c7b1a6ab25c4b520448093.
Harness APKBE7D1375... installed only org.mavk.probe.
Next: apply reusable session to an actual adapter need rather than further
synthetic performance polishing; Bridge/ICD changes require their relevant
gate. Native timeline/retained graphics are NOT automatically guest features.
Windows remains active adapter, no VM needed for this completed native slice.
No owner APK/platform/Windows/Ubuntu/display changes. No commit/push.

## Latest milestone — unmodified external Windows Vulkan app PASS

2026-10-17 CURRENT Core timeline native PASS, no Windows boot this milestone.
Core TimelineSemaphore owns/query/wait/hostsignal; explicit owner-enabled
Vulkan1.2 capability, unavailable context rejected before driver calls. New
timeline feature path is opt-in; existing Root/DeviceContext defaults unchanged.
GPU fill→timeline dependency→copy→hostread in2 rounds,4GPU submits,8192 verified
words,2 final host waits (no intermediate CPU fence),timeout1,negative3,
finalcounter5,SyncVal0errors/0warnings,Corezero. Report core-timeline-build1/
core-timeline.json SHA cebec7a32d9418896d1c53f814ff6bb726648015d74ffc3456e320d74d06bced.
Feature Suite fresh common-Core regression PASS,now10 bounded NATIVE rows;
existing compute169869312/image81231872/sampled12288 passed same new build.
Suite SHA f785128adc6bf97644e1273110c3413d5714725da1f17d3fe90ddf97534a8ed3.
APK955F0F77... installed only org.mavk.probe. No Bridge/ICD capability flags
changed; native timeline proof is NOT async guest Bridge/game/performance PASS.
Next: extend reusable Core toward actual adapter needs; native relevant gate
first. Do not rerun Windows texture/compute or other closed gates without a
specific adapter/common regression reason. Goal remains active and incomplete.

2026-10-17 CURRENT Feature Suite actual Android PASS, default next work=Core.
FeatureSuiteActivity thin runner reuses2 existing native JNI tests, no renderer
duplication or VM. Fresh integration gate:169869312 compute words,81231872
image-transfer pixels,12288 sampled pixels;9 bounded NATIVE feature rows PASS,
SyncVal0/0 and Corezero. EMULATED Windows readback/blit explicitly separate gate;
UNSUPPORTED means unimplemented Mavk wrapper/adapter, NOT missing Mali hardware.
No CTS/game/generalVulkan claim. Strict verify-feature-suite.py --fresh PASS.
APK feature-suite-build3 SHA9A0889A7... installed only org.mavk.probe; evidence
feature-suite.json SHA dade19f5a0ec0fd35ee938b9db8be1b5f0625d270edd889de93ae21ffdd1b6e7.
Saved-result button tested:modeCACHED_REPORTS and source hashes unchanged,
no new Core run; cached report1c7de25a.... Screenshot suite1.png inspected.
Windows texture milestone below completed, not aborted: actual app window and
both24frame exits0. No VM active or carrier patch remains; observer48058 stopped
after normal guest shutdown. Storage C54.1GiB/E875.8GiB; no commit/push.
Next new capabilities first through Core+relevant native test; avoid rerunning
all closed gates unnecessarily. Windows ICD active adapter, WDDM separate.

2026-10-17 CURRENT Windows textured upstream app actual PASS CID2347
(mavk-windows-texture2-20261017). Original pinned texture.cpp/shaders/KTX,
no app edits. Standard loader+ICD→4057→Mali. IMMEDIATE/FIFO24frames each,
each79commands/25GPU,rx1540888/tx7376012,SyncVal0/0,EOF/Corezero. Both waited
process exits0 (UTF16 PowerShell exit files, decoded by BOM); both benchmark
CSVs24frames. Actual correct metalplate in Android texture-fifo1.png inspected.
Strict verify-texture-report.py PASS. NOT game,WDDM,DWM,60visibleFPS or CTS.
Evidence windows-texture-bundle2-20261017 host-texture{1,-fifo1}.json SHA
6c2457fea06965ce2e2141d37436453287c8928a1aee86fa9825a91c3bbad570,
guest-texture-cold/{exit2,exit3,stderr,stderr2,benchmark.csv,fifo.csv}.
CID2346 was intentionally stopped on user architecture pivot before app launch;
one justified resume used already delivered files, no repeated fetch or changes.
Normal shutdown actualVM[] confirmed, texture-carrier-rollback1 exact3381bd95
PASS; no pending carrier. Observer48058 may still be finite/read-only, VM stopped.

USER PRIORITY update: compact Android Feature Suite first, reuse native Core
tests, NATIVE/EMULATED/UNSUPPORTED classification separate from PASS/NOT_RUN.
Windows ICD remains active; test ICD/Bridge changes and periodic common updates,
not every native feature. WDDM separate. No more automatic Windows boots.
FeatureSuiteActivity source implemented, existing JNI Core comparison+sampled
reused; cached vs fresh, hashes and scoped capabilities. feature-suite-build3
in progress (61391), NOT installed/runtime PASS yet. Next build terminal check,
install only org.mavk.probe, one fresh suite integration run, verify report,
document actual remaining Core capability slice. No production changes/commit.
Earlier LIVE/rollback-pending paragraphs below superseded.

2026-10-17 LIVE test in progress: owner texture-build1 installed; new chunked
carrier applied exact57331302... at36028416. ROLLBACK PENDING untilVM[].
Approved launch evidence mavk-windows-texture1-20261017, observer44634 live,
current owner token only in that evidence/private file. Boot progressing through
driver notifications, no Vulkan consumer launched yet. Do not restart blindly.
Use new chunked-fetch-preflight1-20261017 for apply/rollback, not old fetchplan.
After launch/test shutdown actualVM[], restore original3381bd95... and coldlogs.

2026-10-17 NEXT external texture build ready, NOT yet guest PASS. Immutable
upstream texture app+KTX compiled ARM64 E4994340... from82 verified source
blobs; asset submodule a27c0e584434d59b7c7a714e9180eefca6f0ec4b, original
512 RGBA10mips1398236B KTX SHA38495B08.... Bundle1853775B EC5E66AC... at
windows-texture-bundle2-20261017. One1MiB legacy archive bound rejected it;
development op9 now additionally supports fixed archive size+bounded offset
chunks, maxarchive4MiB, unchanged1MiB packet. No arbitrary files/paths or
graphics protocol changes. Chunked fetcher ARM647680B SHA3670BBF1..., cold
preflight chunked-fetch-preflight1-20261017 candidate57331302... NOT applied.
Actual same-UID archive test PASS:10commands,6 negatives,1853775 exact bytes,
EC5E66AC... matched,EOF/Corezero;0GPU and VVL disabled (not graphics proof).
archive-local.json SHA cb1eb35bec031b7a3b4f4f876ef12ee8989df785f2bef4837273e4235902ed7e.
Native bridge-texture-delivery2 APK DCAB139D... installed only org.mavk.probe.
Owner texture-build1 terminal0 compiled, not installed yet. CurrentVM[];
tablet61%,52.6GiB free,C55.4/E876GiB. Next coldowner install+newcarrier apply,
approved profile, hash-guard rename current TEMP ZIP99267DAE... to unused
sampled0 suffix,chunkfetch,hashEC5E66AC... expand MavkTexture1,waited upstream
24frame320x240 run+hostprobe. Then normalshutdown,rollback,coldlogs and actual
image review. No raw/display/firmware/CPU/Ubuntu edits. Goal remains active.

2026-10-16 CURRENT CID2345 standard sampled texture ICD actual Windows PASS.
Standard vulkan-1.dll→candidate ICD→app-owned4057→Mali. Own consumer verified
3072 pixels in3 indexed textured renders (LOD0/1/3), full1024 RGBA11mip chain,
5592404B upload, retained native cache reuse, second content revision invalidation,
late staging/UBO writes,148B UBO descriptor. Guest EXIT0; host29commands/5GPU,
rx11193996/tx13572,cleanEOF/Corezero,SyncVal0errors/0warnings. Strict
verify-sampled-report.py PASS. NOT external texture compatibility/performance,
games,WDDM,DWM or conformance proof. Evidence windows-sampled-bundle1-20261016:
host-sampled1.json SHA b7f6141f7624f0ec21f1311dd0e4a15939b00ffc82a48bdc346339070e040847,
guest-sampled-cold/stdout.txt SHA420e77187c868ceaadef37efda52131166e5f48c0b0202d42fcb815ddbe17e90.
OwnerAPK FA11E0D6...,native4AFFBFD6...,ICD9A048F5A... retained E.
Normal shutdown actualVM[] confirmed; sampled-carrier-rollback1 exactoriginal
3381bd95... PASS. Observer71551 retained only for finite observation, VM stopped.
No Ubuntu/P7/firmware/CPU/display change, no commit/push. Next bounded gate:
unchanged pinned upstream texture example+actual KTX bytes; inspect minimal
dependency/API needs before boot. Earlier NOT-delivered candidate notes superseded.

2026-10-16 CURRENT sampled ICD build5 host gates PASS, not yet guest PASS.
Typed set0 UBO+combined sampler layout/pool/write/bind implemented behind
MAVK_EXPERIMENTAL_SAMPLED; lazy19..21 upload/cache,23 render,22 invalidation.
CPU resource11mips/5592404B/12rejects and descriptor148B/9rejects/latewrites PASS.
Standard API ARM64 consumer built:3 textured draws,mip0/1/3,2 content revisions,
3072 expected pixels. No Mavk calls in consumer. DLL9A048F5A... build5.
Default macro-off compute7/UBO12/push14/readback4MiB CPU gates PASS, ARM64/x64
builds terminal0 (default-regression2). Prepared sampled bundle558843B,
windows-sampled-bundle1-20261016, NOT delivered yet. Native delivery build
in progress bridge-sampled-delivery1-20261016. Next owner build/cold install,
guarded original carrier fetch, finite Windows test, shutdown/rollback/coldlogs.
VM[] observed, battery40%, tablet52.6GiB/C55GiB/E876GiB free. No carrier pending.
Earlier 'descriptors not implemented' partial-candidate notes below superseded.

2026-10-16 CURRENT next Windows sampled candidate: compile-time
MAVK_EXPERIMENTAL_SAMPLED only. Standard vkCreateImage/sampler/full-chain view,
full-mip vkCmdCopyBufferToImage + ordered barriers implemented; input read at
QueueSubmit. CPU test5592404B/11mips/latewrites/12invalid requests PASS.
ARM64/x64 candidate build1 terminal0, ARM64DLL
3CC53449E34FD9A42CBEB1F8D552B1C0095676FE6B354A22CF6C678141D77805,
E:/Mavk/artifacts/windows-icd-sampled-build1-20261016. NOT delivered to Windows.
Sampled format/features/descriptors not advertised/implemented in this partial
candidate yet. Existing default4MiB limit preserved; only opt-in candidate8MiB
for full mip staging, total32MiB unchanged. No device/system driver install.
Remaining exact API work: set0 binding0 UBO+binding1 combined sampler layout,
pool/update/bind; keep typed compute/old UBO cases. At draw, lazy create/chunk/
commit19..21,cache remote texture+content revision+sampler state,reuse until
mutation/destruction; prependtextureID to op23 UBO scene. Complete typed cleanup
and honest sampled capabilities only with tested implementation. Then finite
own standard API texture test, followed by unchanged external texture app.
Do not VM boot this partial candidate. Default build regression exec16397
terminal0:ARM64/x64 DLL build,compute7/UBO12/push14 rejects and4MiB readback
CPU PASS. Default ARM64DLL B15C4DE5... is a build regression,NOT new guest PASS.
Native sampled verifier Mavk/harness/verify-sampled.py gates actual saved reports.

2026-10-16 PERSISTENT TEXTURE BRIDGE PASS, not yet Windows texture API.
Create19/Write20/Commit21/Destroy22/RenderSampledSnapshot23 implemented.
1024 RGBA full11mip chain5592404B over6 bounded chunks;one real native upload
retained on GPU and reused by TWO UBO/indexed textured renders.2048 pixels
verified,16 rejected invalid requests. Native3GPU,34commands,cleanEOF/Corezero,
SyncVal0errors/0warnings. Explicit texture destroy and partial-upload disconnect
cleanup covered. Old wire13/14/17 and fragment-descriptor rejection preserved;
new sampled fragment permits ONLY set0/binding1. CPU shader guard12rejects PASS.
Evidence native-sampled-build1-20261016/bridge-sampled.json SHA
9318b59f67a12be946124185584cc49f031e7e7ddc47312c2d161bd3c9adafe9.
Isolated harness bridge-sampled-harness1 APK50AB655D... installed only org.mavk.probe.
OwnerWindows APK/profile/data unchanged; no active VM or carrier patch.
Next implement candidate standard Vulkan image upload/sampler/descriptors in
compile-time opt-in ICD, CPU recording tests,then one finite standard API guest
test before claiming Windows texture support. Native gate does NOT establish it.

2026-10-16 NEXT SLICE native texture gate PASS (not yet Windows textures).
Core SampledImage owns RGBA8/mip upload, sampler/view, staging lifetime and
explicit transfer→shader barriers. Hardware tested4mips,2contents,nearest,
trilinear mip blend and bilinear spatial sampling:12288 verified pixels/12GPU,
SyncVal0errors/0warnings,Corezero after EACH job. Evidence
E:/Mavk/artifacts/native-sampled-build1-20261016/core-sampled.json SHA
420a235431d23800fcea2196b6a7c77a489cee1180322bac3480eb047bfc3970.
CPU mip layout9rejects/full1024 chain5592404B PASS. Existing scene bridge
regression3330pixels/1GPU/8rejects/SyncVal0/0 PASS (scene-regression.json).
Isolated harnessAPK3952A6F4... only org.mavk.probe installed; owner APK unchanged.
Native helper drawOffscreen gains optional texture, old wire guards unchanged.
Next: bounded persistent texture Bridge (chunked upload under1MiB packets,
upload once and retain GPU resource), then ICD transfer/image/sampler descriptors
for pinned unchanged external texture app. No VM is active or needed for native
Bridge gate; do not rerun proven window test blindly. Goal still active.

CURRENT PASS CID2344: unchanged pinned SaschaWillems upstream triangle completed
24 hardware Mali frames in IMMEDIATE and24 in FIFO. Standard loader→ICD→4057→
Mali; each host session74commands/24GPU, cleanEOF/Corezero,SyncVal0/0.
Windows FIFO process EXIT0; both original benchmark CSVs24frames. First cmd GUI
wrapper exit.txt absent, NOT assumed0; follow-up used existing ordinary Run with
PowerShell Start-Process -Wait -PassThru and separate exit2.txt/fifo.csv.
Strict verify-triangle-report.py PASS. Android fifo1.png shows correct RGB
triangle in real upstream window through unchanged H264. Not sustained/unique
visible-FPS,zero-copy,general WSI,game,WDDM/DWM or conformance proof.
Evidence E:/Mavk/artifacts/windows-view-triangle-bundle1-20261016:
host-{triangle,fifo}.json SHA447929407f38f47257ae91181d0cabf682f917ab9316c8e540a8e4d15f35548a,
guest-triangle-cold/{benchmark.csv,fifo.csv,exit2.txt,stderr.txt},fifo1.png.
Normal shutdown/actualVM[]; view-carrier-rollback1 exact3381bd95... PASS.
Observer92166 terminal0 retains VM only until manual shutdown (now stopped).
Diagnostic ownerAPK B0D36B51...,ICD A433DF72...,native C7C231DC... retained E.
No active VM or pending carrier rollback remains. No commit/push.
Next meaningful compatibility slice: ordinary app with sampled textures;
first inspect actual upstream requirements, implement only bounded needed API
and native sampler/upload semantics. Do not repeat triangle/compute gates blindly.
Broader Mavk/Windows GPU goal remains incomplete. Historical launch notes below
are evidence, NOT instructions to resume old LIVE handles/carrier writes.

CURRENT follow-up: CID2343 upstream triangle now passes compute queue setup,
but stops at vkCreateImageView, VulkanSwapChain.cpp:353 (explicit RGBA mapping).
Guest stderr193B/no EXIT/CSV; host2commands/0GPU,cleanEOF/SyncVal0/0. Evidence
windows-compute-triangle-bundle1-20261016/guest-triangle2-cold and host-triangle2.
Normal shutdown/actualVM[] confirmed; observer25832 intentionally interrupted.
No carrier used in this boot. Minimal source fix accepts per-component explicit
R/G/B/A as identity, still rejects channel rearrangement/constants. All16 identity
combinations and3 invalid mappings pass CPU WSI test; compute/UBO regressions
CPU PASS. CandidateDLL A433DF72...; new bundle1011256B SHA
46e7121a35dd8de4eff0f8e9992ce00973e3d93692bcd26788f7883d690cd0f4,
windows-view-triangle-bundle1-20261016. Native build4623 terminal0;
owner build87144 terminal0 APK B0D36B51... installed diagnostic only.
LIVE CID2344, observer92166, mavk-windows-triangle-view1-20261016 token ONLY.
Exact view-carrier-apply1 PASS; MUST shutdown/actualVM[]/view-carrier-rollback1.
Next rename old ZIP (hash21E326...),fetch new ZIP,expandMavkTriangle3,upstream run.
Prepared short commands in new bundle directory; all<=252 characters.
Unchanged upstream application/shaders, Windows platform and production paths.

2026-10-16 STANDARD COMPUTE ICD PASS, CID2342. Standard Windows Vulkan Loader
→ application-local ICD → app-owned4057 → Mali executed TWO actual application
SPIR-V dispatches. Guest verified8192 words, late mapped writes and prefix/suffix
guards, EXIT0. Host4commands/2GPU, cleanEOF/Corezero, SyncVal0errors/0warnings.
Evidence windows-compute-triangle-bundle1-20261016/host-compute.json SHA
2cfc5b021272506a6f657232a67b7ac5590d85d5cb1de5ac99b312615e7ff697 and
guest-compute-cold; strict verify-compute-report.py PASS. Normal shutdown/VM[];
compute-carrier-rollback3 exact original3381bd95... PASS. No carrier remains.
Fetch first admission failed connect; timely retry delivered1011004 bytes.
All3 host admissions consumed, so upstream window test needs next cold boot;
already delivered ZIP remains, no carrier or another delivery needed.
Next: same approved profile, expand hash-guarded ZIP to MavkTriangle2, run
UNCHANGED pinned upstream triangle with genuine compute queue now available.
Bounded one-SSBO/1D synchronous compute only, NOT Vulkan conformance/general
compute/performance or WDDM/DWM acceleration. Below candidate notes historical.

2026-10-16 NEXT CANDIDATE: genuine standard compute ICD (not yet guest PASS).
LIVE retry3 mavk-windows-compute-icd3-20261016; token only its directory.
Attempt2 never created VM:52GiB reserve guard,actualVM[]. Carrierrollback2PASS.
Archived ONLY eight old QEMU v1-v4 img/raw files from /sdcard/Download to
E:/U-AVF/artifacts/tablet-old-qemu-images-20261016, all SHA256 verified before
removal; logs and latestv5 remain on tablet. Recovered768MiB,available52.64GiB.
No user media/workspaces/current Windows disk removed. Recoverable from E.
Compute-carrier-apply3 PASS; MUST normal shutdown/VM[]/rollback3 after test.
One set0/binding0 compute SSBO<=512KiB, literal1D local<=256/groups<=256,
push<=64B, synchronous native dispatch and readback. Opcode18 sends actual
app SPIR-V/input/push bytes. Old graphics/UBO wire unchanged. Own API consumer
uses different shader from fixed Bridge probe, two seeds +late mapped inputs,
8192 words +prefix/suffix guards. No application-specific shader substitution.
CPU compute7rejects/UBO12/push14/readback4MiB passed; ARM64/x64 DLL compiled
0ae66d62... . Native/owner build exec26172 terminal0.
Attempt1 exec22314 handle missing on resume, actualVM[]; CID2341 hit native
Windows bugcheck1DB/IPI watchdog BEFORE any fetch or ICD test. Not compute FAIL
or PASS. Original carrier restored by compute-rollback1 exact3381bd95... .
Owner report confirms display had connected; stale first error wasn't a hang.
Retry2 same frozen profile justified to execute previously unrun API workload;
no CPU/timer/GIC/firmware mutation. No random platform investigation.
Previous attempt: mavk-windows-compute-icd1-20261016, unchanged approved profile,
owner7d230c8f... and carriercompute-apply1 exact32bae66f... . Current token ONLY
that evidence directory. Bundle1010924B SHA21e32640b8db56a863c0933716c94982614b4badbe00f465d4c82c147d581911.
Own existing ZIP SHA85a682ee... must be renamed before fixed fetch. Prepared
short Run commands in E:/Mavk/artifacts/windows-compute-triangle-bundle1-20261016.
After fetch: expandMavkCompute1 +compute/hostprobe; expandMavkTriangle2 +triangle/
hostprobe only if compute succeeds. Then normal shutdown/VM[]/cold logs --compute
and --triangle --triangle-attempt2, exact compute-carrier-rollback1 REQUIRED.
Next finite boot: fetch bundle → own standard compute → upstream triangle,
max3 host sessions. Must pass actual compute before claiming queue capability.
No new production installation. Carrier rollback remains required if applied.

Triangle checkpoint CLOSED, not PASS: CID2340, unchanged8CPU profile.
Ownertriangle-build1 installed, exact guarded carriertriangle-apply1 PASS.
Normal shutdown/actualVM[] and triangle-rollback1 exact3381bd95... PASS.
Observer61293 intentionally interrupted AFTER shutdown/rollback (terminal1).
No active VM or test remains. OwnerAPK4f9d092b... isolated diagnostic only.
Bundle E:/Mavk/artifacts/windows-triangle-bundle1-20261016,880834B,
SHA85a682ee80e58f4bdc1d4afcb70391ac4507d3f817e5aeab521cb0ede9b43b01.
Unmodified pinned upstream triangle +actual framework ARM64 compiled;
46 downloaded files about2MB verified against immutable git blob tree.
build-triangle.ps1 records source manifest. No application/shader/helper edits.
Own namespaceMavkTriangle1; short Run expansion242chars/test235chars prepared.
CLI320x240 -b -bw0 -br10 -bfs24 -bf benchmark.csv (bounded24 frames,not FPS claim).
Fetch verified880914 sentbytes; API host2commands/0GPU,cleanEOF,SyncVal0/0,
Corezero. Guest cold logs confirm uncaught std::runtime_error:
"Could not find a matching queue family index". No exit marker/benchmark CSV:
do NOT count as success or infer EXIT0. Source VulkanDevice.cpp:208-210 requests
COMPUTE in its default logical-device setup; current ICD advertises GRAPHICS|
TRANSFER only. This is a concrete ICD API coverage blocker, NOT GZVM/Mali failure.
Do not add a fake COMPUTE queue flag just to get past the framework. Next slice
must actually implement standard compute pipeline/dispatch/resource semantics,
or choose an independently useful graphics-only app without changing its source.
No need to repeat this launch before that concrete capability exists.
Evidence host-triangle.json SHA9bf40b105831eeb52ccaf4e83b0f1331d290bff55503a96157d2e02356814b89,
guest-triangle-cold/{stdout,stderr,extraction}; fetch and screenshots preserved.
Required upstream two command buffers/reset now added; CPU12rejects PASS,
optional WSI build ARM64 F5B6B087... . Reset/multi-buffer entry CPU-only PASS;
upstream stopped before executing these entries. Do not claim guest coverage.
Old UBO/headless PASS preserved. This test's report must be judged fresh.

2026-10-16 UBO milestone PASS: CID2339, unchanged8CPU/platform/display.
Standard Windows Vulkan Loader → app-local ICD → app-owned4057 → Mali:
192B vertex UBO/set0/binding0, descriptor offset32, mapped writes AFTER command
recording, nonidentity P*V*M, indexed staging. Guest verified3906 pixels,EXIT0.
Host5commands/1GPU,2512rx/16852tx,SyncVal0errors/0warnings,cleanEOF/Corezero.
Evidence E:/Mavk/artifacts/windows-uniform-bundle1-20261016:
host-uniform-retry.json SHA098273666b7085d228a733210cf41e39776490bf883559792d2cba0937cec33c,
guest-uniform-retry-cold; strict verify-uniform-report.py PASS.
First CID2338 rejected beforeGPU: old shaderWords guard disallowed descriptor
decorations. Fixed ONLY opcode17 vertex set0/binding0 allowance; fragment and
old graphics descriptor rejection retained. Actual filter CPU unit8rejects PASS.
Native deliverybuild2 and ownerbuild2 compiled; installed owner7f871a01... .
Header extraction after build is equivalent guard logic, not yet rebuilt APK.
CID2338 normal shutdown/VM[] + exact carrier rollback3381bd95... PASS.
CID2339 reused own already delivered bundle (no carrier/disk mutation), normal
shutdown +actualVM[] confirmed, cold read-only logs. Observer47628 intentionally
interrupted after shutdown (terminal1, not boot failure). No live GPU test remains.
This is own standard API UBO consumer, NOT third-party WSI/texture/perf/WDDM
or Vulkan conformance PASS. Next concrete target: unchanged external triangle
window using UBO; inspect/build only required upstream framework dependencies,
then fix only observed API gaps. Do not rerun proven headless/UBO tests blindly.

2026-10-16 current work: next upstream triangle requires vertex UBO192B/set0
binding0. Added bounded descriptor pool/set/update/bind path and Bridge17
uniform snapshot: actual native Vulkan UBO/descriptor, not shader rewriting.
Uniform bytes copied at submit (late mapped writes supported),range<=4096B,
single vertex binding,no textures/dynamic offsets. Old13/14 wire unchanged.
CPU uniform state10rejects +existing push14/readback4194304B PASS; scene parser
630 rejects PASS. Candidate windows-icd-uniform-build3 DLL305fdf89... compiled.
NOT native/Windows GPU PASS yet. Owneruniform-build1 SHAb3ca9526... installed,
ZIP544584B SHA1d9de8df...; prior WSI report exact56e0ccb9... preserved.
LIVE: approved unchanged profile mavk-windows-uniform1-20261016; carrier
uniform-apply1 exact32bae66f... . MUST shutdown/VM[]/uniform-rollback1 afterward.

Windows ARM64 CID2336: pinned SaschaWillems renderheadless4ee2501 runs unchanged
through standard Vulkan Loader → app-local Mavk ICD → app-owned AVF4057 → Mali.
1024×1024, three indexed RGB triangles, 64B MVP pushes, depth/culling, staging
uploads and separate linear-image readback. EXIT0; strict independent analytic
verifier PASS3624 samples (triangles279/823/292,background2230). PPM3145745B
SHA089e42548bbaa606feead8fb1509d99e6c9eee2b70a91cf7d906466ccbae69c1.
Host8commands/1GPU,2444rx/4194868tx,SyncVal0errors/0warnings,cleanEOF/Corezero.
Host SHA405c5aa9748b0a8d5bc34d9c396b3259e5984f8f7a5f6cc60b855767bac75a53.
Evidence E:/Mavk/artifacts/windows-headless-bundle1-20261016:
host-headless.json,guest-headless-cold/{stdout,stderr,exit,headless.ppm,extraction}.
Normal guest shutdown,actualVM[],carrier rollback2 exact3381bd95... PASS.
Observer28051 intentionally interrupted AFTER shutdown/rollback (terminal1,
not boot failure). No live VM/test left. No commit/push or production changes.
This proves ONE external offscreen application, NOT conformance, WSI, games,
DWM acceleration, sustained FPS or a high-performance universal Vulkan driver.
Next: extend concrete real-app compatibility and measure persistent-resource
costs; do not repeat completed offscreen/first-window milestones or redesign
manual control.

WSI candidate (opt-in compile macro only): Win32 surface/swapchain, two owned
images, synchronous binary acquire/submit/present, guest CPU blit of genuine
Mali readback, FIFO DwmFlush pacing, resize/lost-window handling. Not zero-copy
or general asynchronous semaphore/WSI conformance. Host hidden-window state
unit PASS and default offscreen CPU regressions PASS. ARM64 WSI DLL2dbca5e8...
in windows-icd-wsi-build3-20261016; original proven offscreen artifacts intact.
Standard API-only own window consumer320×240/12 changing draws compiled;
no third-party WSI/runtime PASS yet. Ownerwsi-build1 SHA3716a63d... installed.
Old host PASS report preserved before-wsi; fresh report absent before launch.
Current live test: mavk-windows-wsi1-20261016, guarded wsi-carrier-apply1PASS.
MUST guest shutdown/actualVM[] and wsi-carrier-rollback1 exact readback after test.

WSI runtime update: CID2337 FUNCTIONAL PASS through standard loader Win32
surface/swapchain/acquire/binary semaphores/submit/present. Own API-only consumer
rendered12 moving images320×240 on Mali;12 distinct guest-window sampled hashes,
blue center checked each frame,EXIT0. Fresh Android current-window1.png visibly
shows Vulkan window via unchanged H264. NOT Android visible-FPS measurement.
Host14commands (Hello+Identity+12draws;close is EOF,NOT a15th opcode),12GPU,
23056rx/3687140tx,SyncVal0errors/0warnings,cleanEOF/Corezero. Strict window
verifier corrected its initial counting expectation from15 to14 using actual
protocol (no bye opcode); all pixel/submit/validation gates unchanged.
HostSHA56e0ccb93a5c33643f7ce8a33eb199ae6269728b985bd1b9c5fd88d6b641fccd.
Evidence E:/Mavk/artifacts/windows-wsi-bundle1-20261016:
host-window.json,guest-window-cold/{stdout,stderr,exit,extraction},current-window1.png.
Normal shutdown,actualVM[],wsi-carrier-rollback1 exact3381bd95... PASS.
Observer13625 interrupted only AFTER shutdown/rollback; no active VM/test.
WSI remains compile-time opt-in and CPU readback/blit, not third-party WSI,
zero-copy,games,DWM,conformance or production performance. Keep native/Mali
renderer and all golden boot/display/storage baselines unchanged.

## Решение

Рабочее имя Mavk; отдельная соседняя папка, будущий отдельный репозиторий.
Главный приоритет: native Android Vulkan proof → небольшое общее ядро →
Windows GPU → DroidDeck → Ubuntu Vulkan. Не продолжать старые FPS-прогоны
по инерции. Не commit/push/publish, не запускать новую VM для записи плана.

## Что есть и чего пока нет

- Изолированный native harness реализован/собран/установлен: org.mavk.probe,
  без разрешений/VM/system changes. Три отдельных запуска процесса: аппаратный
  compute PASS, по8,388,608 проверенных uint32 значений (итого25,165,824).
  Device Mali-G925-Immortalis MC12, device API1.3.278, loader1.4.0,152 extensions.
  Доказательства/тайминги/ограничения: NATIVE_BASELINE_2026-10-15.md.
- Direct graphics/offscreen: 3330 пикселей проверены; движущийся треугольник
  1920×1200. Независимые SurfaceFlinger actual-present timestamps: 1678 за
  фиксированные28.0036с =59.9208/с; max gap33.326мс. Это простой native workload,
  не предел GPU, не доказательство уникальности каждого изображения/guest GPU.
- Минимальный Core выделен в core/include + core/src (без JNI/Android),
  отдельная статическая библиотека: buffers/images/shaders/pipelines/fences/
  command batches/submission. Device/queue пока caller-owned; семантика
  Vulkan-команд остаётся явной, никакого полного ICD/compatibility обещания.
- Compute/image A/B на одном device/queue, ABBA на каждом размере:
  3 отдельных процесса, 509,607,936 uint32 и243,695,616 image pixels без ошибок.
  Buffer sizes4096/65536/262144 words; images64²/640×360/1920×1200.
  Все Core live counters после каждой фазы нулевые. Это не независимый leak
  detector и не validation-layer PASS.
- Core graphics/Surface A/B:1920×1200,30с,3330 offscreen pixels PASS в обоих
  путях; Core59.9559 vs direct59.9667 actual-present timestamps/s за фиксированные
  ~28с. Один APK, существующий renderer, без замены WSI. Core live counters
  после реального teardown нулевые. Graphics GPU timestamps пока не измерены.
  Отчёт: CORE_AB_2026-10-15.md; raw в E:/Mavk/artifacts/core-ab-build2-20261015.
- Overhead record+submit: медиана run-level delta Core-direct = -5.308,
  +5.307, +1.847мкс для трёх размеров. Малые величины/разброс/DVFS не позволяют
  заявлять причинный speedup или нулевой overhead. Сырые samples сохранены.
- Ограниченный guest Vulkan доказан (latest milestone выше); WDDM/DWM и
  DXVK compatibility пока НЕ доказаны.
- Bounded Core gate завершён: Khronos validation1.4.363.0 включён app-local,
  validate_sync активен (намеренный записанный WAW обнаружен, никогда не submit).
  Compute/image ABBA169,869,312 words +81,231,872 pixels;0 errors/0 warnings.
  Две потери активного Surface1920×1200→1280×800→1920×1200, без перекрытия
  native owners; последний цикл30с.0 errors/0 warnings, Core live zero после
  каждого teardown. Это bounded gate, не исчерпывающая проверка Vulkan.
  E:/Mavk/artifacts/core-validation-build1-20261015/validation-lifecycle-summary.json.
- Bridge v1 local proof PASS:32-byte LE framing, connection-local resource IDs,
  bounded allocations, trusted compute dispatch/readback.49 commands/10 negative
  cases/16 actual GPU submissions,557056 words correct,0 validation errors/warnings,
  live resources zero after disconnect (including resource intentionally left alive).
  Same-UID AF_UNIX stream/separate threads/fragmented writes control.
  E:/Mavk/artifacts/bridge-build1-20261015/bridge-run1.json.
- Короткий DroidDeck PoC выполнен: exact r9 glibc/libs/proot + pinned fork hybris,
  hardware G925/3330 pixels PASS direct loader и proot. Только minimal subset под
  Termux debug UID/runas_app, не полный DroidDeck app/session/WSI/game PASS.
  DXVK pinned profile preflight NO-GO: missing native multiViewport/nonSolid,
  exact G715 driver guard не совпадает. D3D11 не запускался, guard не расширен.
  DROIDDECK_POC_2026-10-15.md и E:/Mavk/artifacts/droiddeck-poc-20261015.
- Bridge descriptor adapter проверен на реальном PFD:49 commands/16 GPU jobs,
  557056 words,0 validation errors/warnings,clean EOF,live zero. Optional isolated
  U-AVF diagnostic APK с port4057/явным opt-in и same-UID/token admission собран
  и установлен в isolated package (не production). Старые4055/4054 policy unchanged;
  84 assertions PASS.
- Windows ARM64 → app-owned AVF4057 → Mavk Core → hardware G925 compute PASS.
  CID2328/run6:23 commands,8 GPU submissions,131072 uint32 проверены гостем,
  process EXIT=0; host1000 rx/525056 tx bytes,clean EOF,live zero,
  sync-validation enabled,0 errors/warnings. Отчёты обеих сторон сверены строгим
  verifier; не считать это graphics/Vulkan ICD/WDDM/DWM PASS.
  E:/Mavk/artifacts/windows-bridge-run6-20261015/{host-report.json,guest-cold/}.
  Clean shutdown VM[]; single8192B diagnostic carrier exact rollback PASS.
- Graphics Bridge PASS, CID2329: Windows command→real Mali graphics pipeline→
  guest3330 verified RGBA8 pixels,process EXIT=0. Host3 commands/1 GPU submission,
  112 rx/16496 tx bytes,clean EOF/live zero,SyncVal0 errors/warnings.
  E:/Mavk/artifacts/windows-graphics-run1-20261015/{host-report.json,guest-cold/}.
  Strict verify-report.py --graphics PASS. Это trusted offscreen slice, не ICD.
- По накопленной диагностике U-AVF: Ubuntu аппаратный OpenGL через VirGL/Mali
  работает; Windows BasicDisplay/WARP/IddCx не являются аппаратным 3D.
- Vendor Venus backend не имел Vulkan support; gfxstream branch упиралась
  в GZVM register_memory EFAULT. Это исторические ограничения конкретных путей,
  а не доказательство работоспособности нового guest/host bridge.
- Общий backend не должен обещать исправление всех недостатков vendor driver,
  всех Vulkan features, WDDM или производительность Snapdragon.

## Сохранённый Windows checkpoint

2026-10-14, CID2320: реальный Explorer/primary Idd1920×1200, source/encode
56.233 FPS за30с; Android render callbacks56.131 FPS, pending<=1,
receive→render6.942ms. Это не доказательство физических60 FPS или аппаратного3D.

CID2321: повторная топология/restore и native children0/0; контрольное движение
попало после возврата на исходный монитор. Не считать провалом производительности
или успешным60FPS тестом. 2026-10-15 проверены VM[] и отчёты отката обоих display
helpers + ALL112 original codec blocks, unknown0.

Недоставленный host-only workload helper9728B не влезает в старый8192B slot;
не ставить его и не переразмечать ESP ради замороженного FPS-теста.
Артефакты сохранены в E:/U-AVF/artifacts/idd-long-workload-* и
idd-continuous-workload-*; вывод Ubuntu/Windows сейчас не изменять.

## Текущее состояние / следующий этап

2026-10-15 21:30: compute + trusted graphics Bridge milestones PASS.
CID2328 и2329 завершены; VM[] подтверждено. Graphics carrier exact rollback:
E:/Mavk/artifacts/windows-graphics-carrier-rollback1-20261015/report.json.
Observer exec66641 остановлен только после clean guest shutdown/VM[];
outer exit1 — intentional observer cleanup, не GPU или Windows boot failure.
Ручной input/UAC/AVF метод сохраняется; не автоматизировать login/launch infrastructure.

Предыдущий compute shutdown:
VM[] подтверждено. Observer exec41211 завершился0 после clean shutdown;
его final KD timeout произошёл после остановки гостя, не Windows boot failure.
Временный carrier восстановлен byte-for-byte только на холодном диске:
E:/Mavk/artifacts/windows-bridge-carrier-rollback6-20261015/report.json.
Оригинальный8192B cluster SHA3381bd9574ddecd6a6674d99b5b5be81e506393746c66460082e2bde35f4320d.
Никаких изменений firmware/CPU/SMP/Ubuntu/production H264.

В isolated package com.example.uavfwinpetest остаётся graphics owner build1:
E:/U-AVF/artifacts/mavk-owner-graphics-build1-20261015/U-AVF-Windows-diagnostic.apk,
SHA5b103c1c1705ded684ba325dd1518561d0cebf7e019d91a9030790e632438d43.
Baseline APK сохранён в E:/U-AVF/artifacts/mavk-owner-installed-before-20261015/.
Bridge требует явного opt-in; нет автоматически открытого GPU endpoint.
Следующая загрузка не нужна для повторения уже доказанного compute slice.

Закрытые причины подготовительных неудач (не повторять как platform A/B):
- Run1: standard user не читает ESP; input в elevated console не доставлялся.
  Использован finite script до обычного UAC, не обход прав/защиты.
- Run2: accept(NULL,NULL) падал в guest WSP; исправлены valid peer outputs.
- Run3: обязательный setsockopt на AVF FD падал до Core. Заменён whole-session
  transport watchdog60s; локальный PFD control повторно PASS.
- Run4: guest deadline истёк до подключения; ограниченная координация admission.
- Run5: команда ушла в Search из-за потерянного focus/modifier state, клиент
  не запускался. Обычный modifier-up + наблюдаемый console focus исправили.
- Run6: обычный UAC подтверждён; фактический Windows client/GPU PASS.
Каждый carrier был откатан; подробные неудачные отчёты сохранены на E,
не превращать их в новую серию повторных boots.

Не повторять пройденные compute/trusted-graphics тесты. Новый фокус: стандартный
Windows Vulkan Loader→ограниченный Mavk ICD→обычное offscreen API приложение.
Swapchain/окно не нужны для первого PASS; никакого Vulkan1.3/conformance/WDDM claim.

Реализованы и собраны icd/windows/{icd.cpp,transport.hpp,build.ps1,mavk_icd.json}
и независимый Vulkan API consumer icd/test/offscreen.cpp (без Mavk API/protocol).
App shader СИНИЙ, trusted host triangle был ОРАНЖЕВЫЙ; это необходимый контроль
реальной передачи guest shader вместо подмены. Есть bounded logical buffers/images,
memory/map, shaders/pipeline, renderpass/draw/copy, synchronous queue/fence.
Scope/known incomplete semantics: icd/README.md. НЕ production/conformant driver.

Последний полный guest DLL/app build7:
E:/Mavk/artifacts/windows-icd-build7-20261015/{aarch64,x86_64}/
ARM64 DLL SHA6a9d3d1f14df3582d1bc86f4f574ebb6beb20a3bcc181e8d5eb61c1b41e02362.
Стандартный HOST X64 Windows loader находит JSON, загружает DLL, вызывает ICD
CreateInstance; concrete pNext47 loader-private rejection исправлен (device48 тоже).
Fresh host-loader-report.json PASS только manifest/entry boundary; отсутствующий
на ПК Viosock даёт ожидаемый init failure. Это НЕ ARM64 enumeration/render PASS.

Host DeviceIdentity7/DrawOffscreen8 source compiled in bridge-icd-build1-20261015,
native bundle SHAf2922363c860d8238399242a680f43e74d49f28d341ca6f6b708c2524a0ef018.
Там реальные app SPIR-V/clear/push parameters, не fixed trusted shaders;
structural checks не semantic/security validator. На устройстве ещё НЕ проверено,
этот bundle/ICD ещё НЕ установлены. Старый tested graphics APK сохранён.

Точный следующий шаг: доставить ARM64 DLL+JSON+independent app и стандартный
ARM64 Vulkan Loader (сначала проверить наличие) в отдельный guest TEMP directory,
без registry/system install. Не перезаписывать display/codec helpers ради доставки.
Existing single-cluster carrier7168B не вмещает DLL622592B/app430080B; сеть текущего
profile networkSupported=false. Не считать это тупиком GPU. Минимальная допустимая
доставка — ограниченный known-bundle по уже app-owned4057, с явным hash/size и
manual запуском через существующий PowerShell/UAC, не новый input/auth channel.
Не запускать новую VM, пока payload/loader/test/rollback не подготовлены целиком.
После guest loader discovery сразу API offscreen blue-pixel test+host SyncVal report.
Goal активна; enumeration и реальный ICD app PASS пока не доказаны.

2026-10-15 continuation: прошлый turn — progress, не wait/no-progress:
compute/graphics runtime PASS, ICD source/build и host loader discovery entry.
Git session10329 завершилась0: Loader commit00893b9a…/Headers217e93c6… скачаны.
ARM64 standard Khronos Loader1.3.275 собран в
E:/Mavk/artifacts/vulkan-loader-arm64-build1-20261015/loader/loader/vulkan-1.dll
(600064B,ARM64 PE,только OS/UCRT imports). C fallback unknown-extension
trampolines: test не использует unknown extensions, не считать release-certified.
ICD source после transport serialization rebuilt8, ARM64 SHA
a1454b6057be6a1d286d80e2745d5489eecde98d6fe4cc395db01b02aaa59d29.
HOST x64 loader discovery/entry smoke повторно PASS; не guest PASS.

Private bundle1 готов:527965B SHA
c478ce445c160c912006071854e55ef7fb15fb955700b2df3102e030f68d279c,
E:/Mavk/artifacts/windows-icd-bundle1-20261015/mavk-icd-development.zip.
Включены upstream licenses. Fixed readonly artifact opcode9 compiled only in
explicit development build, no path selector/arbitrary filesystem service.
Это доставка test files по тому же4057, не новый input/auth channel.
Guest finite fetch7168B SHA8574b919… пишет CREATE_NEW TEMP/MavkIcdDevelopment1.zip;
не выполняет bundle. Manual API command checks exact zip hash, expands only into
fresh TEMP/MavkIcdApi1, sets VK_DRIVER_FILES for ordinary test process,60s deadline.

Ready-to-test owner APK0b68c2caa47d60e11eccc5fcc7010a987ae1bc096f434b642f9272630b6bfc50:
E:/U-AVF/artifacts/mavk-owner-icd-bundle-build1-20261015/U-AVF-Windows-diagnostic.apk.
Native bundle201c7985… in bridge-icd-bundle-build1-20261015. NOT installed yet
at this checkpoint. Cold fetch preflight ready, plan
E:/Mavk/artifacts/windows-icd-fetch-preflight1-20261015,cluster3182/offset36028416.
Candidate SHA32bae66f…; original3381bd95… . If applied, MANDATORY exact cold rollback
with this plan after VM[]. No firmware/display/CPU/Ubuntu changes.
Next one boot: fetch once(manual UAC/Probe7)→collect host transfer report→ordinary
API launch(guest-api-command.txt)→same existing mavk_probe admission→collect actual
host report and own API stdout/stderr/exit after clean shutdown, then rollback.
No repeated trusted GPU tests or input/control infrastructure refactoring.

ICD test1 historical: CID2330/UID10042/PID15822, observer exec97740 terminal0.
APK bundle0b68… installed; fetch carrier applied EXACT READBACK PASS, journal
E:/Mavk/artifacts/windows-icd-fetch-apply1-20261015/report.json.
Mandatory rollback plan windows-icd-fetch-preflight1-20261015 after actual VM[].
Current token only under EvidenceName mavk-windows-icd-api-20261015; never print.
Fetch Probe7 completed: fixed bundle527965B delivered,clean EOF/2 commands/0 GPU.
API ordinary-user call reached native DeviceIdentity (Hello+Identity2 commands,
356 tx,0 GPU), then standard ARM64 loader rejected missing mandatory
vkGetPhysicalDeviceSparseImageFormatProperties. Concrete guest stderr preserved;
not hardware blocker, not GPU render PASS. Correct zero sparse-format enumeration
implemented + lookup export in build10; all12 required instance entrypoints checked
against pinned loader source. Need test this fix, not platform mutations.

Before first API execution, app shaders corrected to SPIR-V1.0 for declared API1.0:
exact two-byte update of OWN disposable app matched complete build9 recompilation
SHA18ea17ea… . No Windows/system/signed binary modified. New bundle uses correctly
compiled app directly (no patch). Standard Start-Process.ExitCode was blank;
new prepared command uses direct .NET Process/captured streams,60s deadline.

After API ended/idle, screen became black while input channel wrote; did not
debug/change production display or type blind console commands. Stopped isolated
VM with its observed app Stop button; VM[] confirmed by cold extraction guards.
Own guest API/fetch logs in E:/Mavk/artifacts/windows-icd-api-run1-20261015/
{guest-api-cold,guest-fetch-cold}. Exact carrier rollback1 PASS.
Old TEMP/MavkIcdDevelopment1.zip still exists and must be renamed with exact hash
check before next fetch (CREATE_NEW). No current VM/observer alive.

Next test2 fully prepared bundle528005B in windows-icd-bundle2-20261015,
ARM64 DLL build10 SHA8cb52934… with sparse entrypoint fix, app SPIR-V1.0.
Native APK bundle rebuild running exec58424; verify same handle, no duplicate build.
Then build isolated owner/install cold/apply same fetch preflight1 with new journal,
one known-profile boot. First normal console rename old OWN zip after exact
c478ce44… hash check (preserve .before-sparse-fix). Fetch finite Probe9, then
guest-api-command.txt from bundle2 creates fresh TEMP/MavkIcdApi2. Ordinary user,
same input/UAC mechanism. Collect --api --api-attempt2 after stop and rollback.
Do not repeat compute/trusted graphics or claim full Vulkan conformance.

Test2 launch now in progress, observer exec86919 (not58424, which completed0).
Owner bundle2 APK SHA9ff151d71ab07087549630f315ec0720f3b8487f03e14c26cd6e1240d5e03b0e
installed cold; native bundleb54b5a76… . Fetch carrier apply2 exact readback PASS,
same preflight1 plan/mandatory cold rollback with fresh rollback2 journal.
EvidenceName mavk-windows-icd-api-sparsefix-20261015. Do not duplicate launch.
Before fetch: preserve old own TEMP zip1 by exact c478ce44… check + rename.
Then finite Fetch Probe9 command from windows-icd-fetch-build1, same client hash.
API2 command from windows-icd-bundle2 is1340-ish chars, parser checked; no
SPIR-V binary patch required, shaders compiled for Vulkan1.0. Bundle2 SHA
3d8a53ad28b9ded8a58a593675065b5a458c76b23eb4ecd40727378db9150f91.

DroidDeck bounded PoC закрыт: functional glibc/proot G925 PASS, но не WSI/zero-copy/
performance PASS. Не переносить G715 guards и не переписывать libhybris без
измеренного bottleneck. Pinned DXVK profile — preflight NO-GO, не общий запрет
DXVK. Mavk Native не устраняет glibc/Bionic ABI или WSI costs автоматически.
Не расширять короткий PoC до установки игр; вернулись к U-AVF Bridge.

Сырые Core measurements:
E:/Mavk/artifacts/core-ab-build1-20261015/core-ab-summary.json;
E:/Mavk/artifacts/native-graphics-build5-20261015/correlated-graphics-summary.json.
Отсутствующие robustness2/descriptor_buffer — unsupported, не включать фиктивно.
Стратегическая цель активна и не завершена; никаких commit/push/publication.

2026-10-15 LATEST AUTHORITATIVE CHECKPOINT — ICD API OFFSCREEN PASS.
Resumed CID2331 without a new VM boot. Previous UAC expired/cancelled while idle;
same compact encoded finite fetch was entered again through existing manual UIN1.
Observed ordinary UAC approved; bundle2 transferred:2 commands/0 GPU/528085 tx.
Then ordinary-user API2 command (1186 chars) ran successfully through standard
ARM64 Vulkan Loader and application-local manifest; no special Mavk app API.
Windows stdout: STANDARD_VULKAN_BLUE_PIXELS=3330,
STANDARD_VULKAN_OFFSCREEN_PASS,device Mavk experimental / Mali-G925-Immortalis MC12.
External EXIT=0. Host:3 commands,1980 rx/16772 tx,1 real GPU submission,
SyncVal enabled,0 validation errors/warnings,clean EOF,Core live resources zero.
Strict verify-api-report.py PASS after cold read-only extraction of own logs.
Evidence E:/Mavk/artifacts/windows-icd-api-run2-20261015/
{host-api-report.json,guest-api-cold/,api-guest-verdict.png}.
Windows shutdown.exe /s /t0 completed; actual VM[] confirmed. Observer86919
already terminal0; no live observer remains. Carrier rollback2 exact readback PASS:
E:/Mavk/artifacts/windows-icd-fetch-rollback2-20261015/report.json.
Original cluster3381bd95… restored; firmware/8CPU/H264/Ubuntu/P7 unchanged.
No commit/push. This is a limited, non-conformant offscreen ICD PASS, not a
third-party game, swapchain, WDDM, DWM or Vulkan1.3/conformance PASS.
Next: select one concrete independent application's API requirements and extend
only the smallest required slice. Do not repeat compute/trusted graphics/API PASS
or rebuild VM-control infrastructure. Overall strategic goal remains incomplete.

2026-10-15 NEXT SLICE — VERTEX DATA (progress, not another VM baseline).
External target inspected: SaschaWillems/Vulkan examples/renderheadless/
renderheadless.cpp (upstream master, not vendored/pinned yet). It needs real
vertex/index buffers, depth, pipeline cache and matrix push constants; current
ICD cannot run it. Do not modify that app into the old VertexIndex test or claim
third-party compatibility. First implemented prerequisite: vertex buffers.
DrawVertices10 adds bounded explicit stride/attributes/count/bytes; host creates
and flushes a real vertex buffer and binds it to native Mali pipeline. Existing
DrawOffscreen8 remains unchanged wire contract. ICD now supports one binding,
4 float attributes,stride<=64,up to4096 vertices; binds from mapped guest data
at submit (including buffer and attribute offsets). Index buffers remain absent.
CPU wire test rejected129 malformed/truncated/overflow/duplicate-location cases.
ARM64/x64 ICD/API app builds PASS in windows-icd-vertex-build3-20261015,
ARM64 DLL SHA12384031ed058809dd155d5cd1d506a6b31055766857d46b4e54cbdd4c60d273.
Native DEVICE TEST PASS: bridge-vertex-build2-20261015/vertex-gpu-report.json:
Mali-G925,3330 verified blue pixels,1 GPU submission,2 commands,SyncVal enabled,
zero errors/warnings,clean EOF,Core live zero. APK org.mavk.probe only updated;
diagnostic U-AVF APK and production Windows/Ubuntu/H264 untouched,VM remains[].
Separate API-only vertex-buffer consumer compiled (padded stride16,attribute
offset4,binding offset16) but NOT run in Windows yet. Same standard Vulkan API,
no Mavk app calls. Verifier --vertices rejects prior VertexIndex PASS markers.
Private vertex bundle prepared (not delivered):530820B,
E:/Mavk/artifacts/windows-icd-vertex-bundle1-20261015/mavk-icd-development.zip.
Next exact step: embed this bundle into isolated diagnostic APK (same existing
development delivery),prepare API attempt3 and own-zip exact-hash preservation,
then ONE known-profile boot/fetch/API vertex test/host+guest verify/cold rollback.
No reason to rerun trusted compute/old graphics or alter manual input machinery.
After Windows vertex PASS, implement index-buffer prerequisite for external
headless sample; full external sample and performance remain unproven.

LIVE vertex API test: observer exec74284, EvidenceName
mavk-windows-icd-vertices-20261015, CID2332. Same known-profile boot; no duplicate.
Owner APK19345d294082a49367429ccde1fc29a4276422146c63e5e9781840e33c5889cc;
native development bundle5a38e89c…; private ZIP530820B SHA
be5b1e5d3d6b03f4298d47b0b21384d0fb93b4980db39bbca7e3f241622b8280.
Carrier applied exact preimage/readback under windows-icd-vertex-carrier-apply1;
MANDATORY cold rollback preflight1→windows-icd-vertex-carrier-rollback1 after VM[].
Finite fetch command10-encoded prepared; API attempt3 prepared in vertex-bundle1.
Must first preserve exact old own zipSHA3d8a53ad… before CREATE_NEW fetch.
Current owner token under new EvidenceName only; never print. Do not repeat boot.

2026-10-16 interruption audit: previous VM2332 really terminal, actual VM[],
owner PID absent,observer74284 missing; events end OBSERVER_STOP ADB runner closed.
No fetch Probe10 logs exist in guest (cold read-only extraction). UAC was never
approved before interruption. Old zip2 preservation WAS observed before stop.
host-fetch-report.json in vertex-bundle1 is STALE (same hash9875c021… as prior
API2,3commands/1GPU), NOT new fetch or vertex PASS; admission failed with no owner.
Mandatory carrier rollback1-20261016 exact readback PASS. No newGPU conclusion.
Safe next: same prepared installedAPK/bundle,one repeat of interrupted test only;
new evidence/token required. Existing own zip is already .before-vertices;
do not repeat its rename blindly. Fetch10 then API3 remain unexecuted.

LIVE resumed test CID2333/UID10042/PID12663,observer exec20286, EvidenceName
mavk-windows-icd-vertices-20261016. Same installed owner APK19345d29… . Carrier
apply2-20261016 exact PASS; mandatory rollback2-20261016 after actual VM[].
Old own host report9875c021… preserved by same-UID rename to
files/mavk-bridge-report.before-vertices-20261016.json before new admission;
no stale report can now be collected as a fresh result. Input token new directory.

External sample pinned (source+MIT license only,not built/not modified) under
E:/Mavk/research/renderheadless-4ee2501; upstream commit
4ee2501c8280cf71d1f9ce231cd8ebd47b58adde,sourceSHA
3353eaf467e223bbc34eb6a9d41d16a0d02cb7dac17d8d6909d6219f0e810d64.
Concrete remaining requirements beyond vertex upload: staging CopyBuffer into
device-local vertex/index memory;uint32 index draw;64B vertex-only matrix push;
depth/culling;3 draws in one render pass;separate render/transfer submissions;
1024x1024 image (>1MiB reply,requires bounded chunk readback);linear-image copy/
MapMemory/GetImageSubresourceLayout;empty descriptor layout and pipeline cache.
Do not silently shrink/disable these in upstream to call it application PASS.

2026-10-16 LATEST AUTHORITATIVE — WINDOWS ICD VERTEX BUFFER PASS.
CID2333 same frozen Windows8CPU/H264 profile completed new API test. Standard
ARM64 Loader→ICD→DrawVertices10→Mali:3commands/1GPU,1788rx/16772tx,SyncVal enabled,
0 errors/warnings,clean EOF,Core live zero. Own guest API3 stdout verifies3330
blue pixels with padded stride16,attribute offset4,vertex binding offset16.
EXIT=0. Strict verify-api-report.py --vertices PASS after cold extraction.
Evidence E:/Mavk/artifacts/windows-icd-vertex-bundle1-20261015/
{host-api-20261016.json,guest-api3-cold-20261016/}. No external app/performance PASS.
Windows shutdown.exe /s /t0 completed,actualVM[]; observer20286 already terminal0
at its normal bound (retained VM then stopped by normal guest shutdown).
Carrier rollback2-20261016 EXACT READBACK PASS,original3381bd95… restored.
No firmware/SMP/storage layout/Ubuntu/production display/input/security changes.

Delivery lessons — do NOT repeat today's UI detour:
- Windows Terminal launch stalled,classic conhost PowerShell did open. Existing
  Unicode input_text did not enter text in classic console,ordinary keys did.
- Run dialog truncated long commands (1638B encoded fetch never createdProbe10;
  raw fetch11 returned MissingEndCurlyBrace). This is NOT GPU/ICD failure.
- Short conhost native launch of existing exact carrier worked,normal Run UAC:
  conhost.exe "\\?\Volume{7430910d-646b-494d-8ba4-7c4c7e462fa9}\IDDWORK\idd-motion-source.exe"
  Volume was observed via Win32_Volume (FAT32,not C:); re-observe on future boots,
  do not assume GUID permanent. No mapping/formatting/install/task service added.
- Wait for visible LISTEN_READY4057 before host admission; first host attempt
  failed before native listener,second fetchPASS2commands/530900tx,third API PASS.
- Own zip2 had already been preserved before overnight interruption. New ZIP
  exactSHAbe5b1e5d… checked; 252char Run Expand-Archive command succeeded.
- Short173char Run command launched conhost cmd /d /v:on /c,cd ownTEMP API3,
  child-local VK_DRIVER_FILES=.\mavk_icd.json,redirected stdout/stderr and recorded
  delayed !errorlevel! into exit.txt. No global environment/registry setting.
  The long .NET60s-deadline launcher was NOT used in this run; manual observation
  plus host transport watchdog,normal completion,not production cancellation proof.
- Read-only own fetch status confirmedProbe10 absent; don't count parser errors
  as GPU failures. host-fetch-20261016.json is failed admission; later fetch2 and
  host-api are separate preserved actual reports. Old stale report preserved.

Next meaningful step: index buffers and staging CopyBuffer/device-local vertex
upload for pinned independent headless sample; do not rerun this vertex PASS.
Its depth/matrix/multiple-draw/1024²/linear-readback requirements remain open.
Also current ICD format query still lacks VERTEX_BUFFER bufferFeatures for newly
accepted float formats: reconcile with actual host capability before external app.
Strategic goal ACTIVE/incomplete; no commit/push/publication.

2026-10-16 INDEXED/STAGING CHECKPOINT (after context recovery).
Native indexed Bridge opcode11 hardware PASS:3330 checked pixels,1GPU,
SyncVal enabled,0errors/0warnings,clean EOF,Core live zero. Nonsequential
indices3,1,2 and unused junk vertex0 prevent ordinary nonindexed draw passing.
Evidence E:/Mavk/artifacts/bridge-index-build1-20261016/indexed-gpu-report.json
SHA69b265b21f9ec3fc512d7038d84c948da7da1510b4e3e5cf3fd9b6675d289f55.
CPU wire-parser 275 rejection checks PASS. ARM64/x64 ICD and indexed API test
compile PASS in windows-icd-index-build2-20261016. New test uses actual queried
device-local memory type,host-coherent staging CopyBuffer,nonzero index binding
offset and firstIndex. CopyBuffer currently synchronous guest shadow-memory
emulation; NOT native GPU transfer/zero-copy/performance PASS.
Windows indexed API runtime NOT YET RUN. Preparing fixed private development
bundle + diagnostic owner APK; preserve original carrier/readback rollback.
Current VM[]; original carrier restored. No baseline or production changes.

LIVE INDEX TEST CID2334/UID10042/PID21345,observer exec98841. Diagnostic APK
E:/U-AVF/artifacts/mavk-owner-index-build1-20261016/U-AVF-Windows-diagnostic.apk
SHA4bffe6668ae2131fc49aa815b1218907180ee82c300a61ca5ebf0369d6d28619.
Private ZIP534010B SHAa98a26fb294662384d35ca60228e058fbd72198fd0ab57170b36c6bbc4f912c2.
Carrier apply1-20261016 exact readback PASS; MUST rollback after actualVM[].
EvidenceName mavk-windows-icd-index-20261016; new token in that evidence dir.
At08:05 Windows driver init observed,no display connection yet; API not started.
Old ownTEMP ZIPbe5b1e5d… remains:preserve exact hash before next CREATE_NEW fetch.
New API directory4; short checked Run commands in new bundle dir. No rerunAPI3.
Verifier --indexed correctly rejects earlier vertex PASS; --vertices still PASS.

2026-10-16 LATEST AUTHORITATIVE — WINDOWS INDEXED/STAGING ICD PASS.
CID2334 same frozen Windows8CPU/H264 profile:standard Loader→ICD→DrawIndexed11
on Mali-G925. Own API4 stdout:STANDARD_VULKAN_INDEXED_STAGING_PIXELS=3330,
STANDARD_VULKAN_INDEXED_STAGING_PASS,EXIT=0. Strict --indexed verifier PASS.
Host3commands/1GPU,1832rx/16772tx,SyncVal enabled,0errors/0warnings,clean EOF,
Core live zero. Host-reportSHA94987af3ff695247d424bd3ddf943cd0d37b3cdb0b8b5bbda6a92b335a7560df.
Evidence E:/Mavk/artifacts/windows-icd-index-bundle1-20261016/{host-api.json,
guest-api4-cold/}. Fetch separate2commands/0GPU,534090tx; not render evidence.
Existing old ZIPbe5b1e5d… preserved as .before-index; newZIPa98a26fb… verified
before expansion,ownTEMP/MavkIcdApi4. Same finite short manual Run/UAC channel.
Windows normal shutdown completed,actualVM[]. Carrier rollback1-20261016 exact
readback PASS(original3381bd95… restored). No production baseline changes.
Staging remains synchronous guest shadow copy,lazy host upload at draw; NOT GPU
transfer/zero-copy/performance/conformance/external-app/WDDM/DWM PASS.
Next: pinned renderheadless requires64B matrix push,depth+culling,multiple draws,
1024²/chunked image readback,linear-image copy/layout,empty descriptor layout/
pipeline cache. Do not repeat passed index test; implement next required slice.
Observer exec98841 intentionally interrupted only AFTER normal guest shutdown,
cold log extraction and carrier rollback; terminal1 due Ctrl+C,not boot failure.
No observer/VM remains. PC reserve C56GiB,E878GiB; tablet last check52GiB.

2026-10-16 MATRIX IMPLEMENTATION — NEXT REQUIRED EXTERNAL-APP SLICE.
Bridge12 carries explicit push bytes/stages (4..64B,vertex/fragment),indexed
geometry and shaders. Original8/10/11 unchanged. Native hardware nonidentity
matrix(scale0.5,translate0.25/-0.125) PASS3906 analytic blue/background pixels,
five bad range/stage requests rejected,1GPU/7commands,SyncVal0errors/0warnings,
clean EOF/Core live zero. Evidence bridge-matrix-build1-20261016/matrix-gpu-report.json
SHAbfcbcbc28d484b8dde37d75080bffa02e64c9b0af43832c175f14db93e67d8b0.
Windows ICD supports64B vertex-only push range,aligned partial writes,exact
byte preservation; snapshots compatible pipeline range. Explicitly rejects
extended pushes without indexed geometry in current slice. Empty descriptor
layout creation/destruction and empty pipeline cache lifecycle added. Cache
does not cache/serialize anything; initial data/bindings rejected,not ignored.
ARM64/x64 builds PASS windows-icd-matrix-build2-20261016,DLLSHA
bfc55e4e9874f892b46153214158666d31d75a1ecc7472c6dfb128a4d5e1fb93.
Host internal CPU push-state test PASS13 rejects,partial64B/NaN-bit exactcopy,
unchanged state on failed writes and empty-object lifecycle. NOT guest/GPU test.
Matrix API consumer built with two32B push updates and3906pixel check. Windows
matrix runtime NOT TESTED; installed diagnostic owner APK still INDEX build1.
No VM boot/mutation this stage; native probe APK only org.mavk.probe updated.
Before next Windows run combine remaining meaningful requirements as practical:
depth/backface culling,multiple draws in one pass,1024² bounded chunk readback,
separate render/transfer submissions and linear image copy/layout. Pinned source
GLSL default is Vulkan1.0;1.1/SPIRV1.4 extensions only Slang,not default blocker.
Do not claim unmodified upstream renderheadless works before actual execution.
Strategic goal active/incomplete,no commit/push.

2026-10-16 SCENE DEPTH/CULL/MULTIDRAW IMPLEMENTATION.
Bridge13 bounded one-pipeline scene:up to16 indexed draws with independent push
snapshots,clockwise/counterclockwise front face,cull flags,D32_SFLOAT depth
test+write LESS_OR_EQUAL. Actual native depth-format support queried before use.
One renderpass/submission,not CPU compositing of separate rendered triangles.
Native PASS3330 pixels:near blue survives later far red,near reversed green
is culled. Last draw has scale0.5; earlier pushes retain identity.8 malformed
scene/range/state requests rejected,1GPU/10commands,SyncVal0errors/0warnings,
clean EOF/Core live zero. Evidence bridge-scene-build1-20261016/scene-gpu-report.json
SHAd68fa1c91082c120ffba16a223bf2c819c9de61ed318dbbc7245b4fa14ba6362.
CPU scene framing/parser PASS418rejects incl all truncations/range overflow;
output unchanged on fail. WINDOWS SCENE RUNTIME NOT TESTED.
ICD D32 image/view/renderpass/framebuffer/pipeline recording implemented;
per-draw matrices snapshotted,all requested indices bounded/concatenated at
submit,Bridge13 emitted for depth/cull/multidraw. Old8/10/11/12 path retained.
ARM64/x64 scene-build1 compile PASS,DLLSHA
be5d65ddf1af38ee57737460be54c3aa5ef3af9e07b3956eb07032c7d6141358.
Host CPU recording unit PASS14rejects verifies independent push snapshots and
draw16 bound. Unit executable push-scene-state-test.exe; not GPU/guest evidence.
No VM launched this stage; installed diagnostic owner still INDEX build1.
Next meaningful work before guest run:1024²/chunked readback and independent
render/transfer submission+linear-image copy/layout for pinned upstream sample.
Do not shrink upstream1024² or remove depth/draws to call it third-party PASS.
Shared renderer regression:scene-build1 native opcode12 matrix rerun PASS3906,
1GPU/7commands,SyncVal0errors/0warnings,Corelivezero. Report transitioned from
scene d68fa1c9… to matrix bfcbcbc2… after own probe force-stop/new Activity;
deterministic matrix hash matches earlier run,not a stale scene result.
Evidence bridge-scene-build1-20261016/matrix-regression-report.json.

2026-10-16 1024² CHUNKED READBACK + SEPARATE SUBMISSIONS.
Native RenderSnapshot14/Read15/Destroy16 PASS:1024² depth/cull/three-draw scene,
4MiB pixel readback downloaded in4x1MiB chunks,860488 analytic pixels checked,
11 malformed/bounds/retired-ID rejects,1GPU/18commands,SyncVal0errors/0warnings,
clean EOF/Core live zero. Retained object is CPU pixels,not a persistent GPU
resource or zero-copy. Bound4objects/16MiB per connection;old13 remains512².
Evidence bridge-snapshot-build1-20261016/snapshot-1024-report.json
SHA293042fc5a4ee9a187ce9a6c7cb9f6b0ce9d818df2ed3fa6661a36e1854676cf.
ICD now supports1024² indexed scene renders via chunked snapshot readback,
stores completed colour image bytes/layout in existing guest shadow allocation,
then separate command-buffer/submission CopyImage to host-visible linear image,
deferred layout transitions and GetImageSubresourceLayout,rowPitch=width*4.
RenderPass final TRANSFER_SRC accepted. CopyImage executes CPU memcpy of actual
completed GPU pixels,NOT a host GPU copy. Virtual memory budget32MiB/max4MiB
per allocation. No performance/conformance claim; no physical shared mapping.
ARM64/x64 snapshot-build1 compile PASS,DLLSHA
6bd7c37099e6c8b21f2c95f8f338268253b86d841de34d228b6f41a4b6e36d89.
CPU linear-readback unit PASS full4194304bytes,rowPitch4096,map/lifecycle;
artificial input bytes,not guest GPU evidence. Windows new runtime NOT TESTED.

PINNED THIRD-PARTY APP PREPARED,NOT EXECUTED:
Unmodified SaschaWillems renderheadless4ee2501 +same pinned helpers/shaders
and actual repo GLM submodule1ad55c5016339b83b7eec98c31007e0aee57d2bf.
GLM archiveSHAcec07d19a13df44030d0a3fac920303aa3bb72add52a3b80bfeb3187e71d7015;
only headers and two upstream licenses extracted (copying.txt absent; initial
tar command partial-success retained headers,then exact archive license entries
extracted; no source changed). No full repository/assets clone.
build-renderheadless.ps1 verifies pinned source/helper/shader hashes,builds C++20
release ARM64 with standard loader import library and local ./shaders/ build
configuration; application/shaders/1024² workload unchanged. Build2 PASS,
exeSHA13f5919677d60e6fc2fe79dcbaa0bf1c3ff26c60c725b1ee3f9d0b5a5e0ef4d2.
Private bundle1 ZIP763739B SHAe90890d36f55a34882ce949a5bbf1cd0d440ccd2d8c58dc661660afccad2f6b0.
Includes loader/ICD/upstream exe (mapped to existing vulkan-offscreen.exe),
SPV nested shaders and third-party licenses. Bundle1 predates optional
UPSTREAM-BUILD.json addition; source/build-report preserved on PC.
Native delivery bridge-headless-delivery-build1 compiled with exact ZIP.
Next: build/install isolated owner APK,COLD guarded existing carrier fetch,
ownTEMP/MavkHeadless1. Preserve old ZIPa98a26fb… before CREATE_NEW fetch.
Run existing short cmd launcher with child-local VK_DRIVER_FILES and <NUL
to supply ordinary stdin EOF to upstream final std::cin.get (no source edit,
no new guest control infrastructure). Record stdout/stderr/EXIT. After normal
shutdown,cold collector --headless --headless-attempt1 includes only own logs
and bounded <=4MiB headless.ppm. verify-headless-report.py requires host8cmd/
1GPU,clean validation/EOF,upstream stdout/EXIT0 and analytic RGB/depth samples
covering all3triangles/background. Verifier syntax checked,NO runtime PASS yet.
No VM boot/Windows mutation this stage;installed owner still INDEX build1.
Goal active/incomplete,no commit/push;keep all established baselines.
Owner headless-build1 compiled READY NOT INSTALLED:
E:/U-AVF/artifacts/mavk-owner-headless-build1-20261016/U-AVF-Windows-diagnostic.apk
SHA25aa3297ed61c065cd843c934ee536818d4210ee6b879549999a58515d6699a5.
All build handles terminal;VM[]. Scene parser updated420 rejects includes
1024 only on retained path/1025 rejection. Verifier analytic coverage3624
points(classes279/823/292/2230) precomputed ONLY,not pixel/runtime evidence.
Short Run expansion/launch commands preserved in windows-headless-bundle1;
guard length<=252 before input. Current PC reserve C55.3GiB,E878.5GiB.

LIVE UPSTREAM TEST:owner headless-build1 exact25aa3297… installed,cold carrier
headless-apply1-20261016 exact readback PASS32bae66f… . Observer exec87033,
EvidenceName mavk-windows-upstream-headless-20261016,new private token there.
MUST normal shutdown/actualVM[] and carrier rollback before cold-read conclusion.
No new GPU result yet; one unchanged8CPU/frozen platform/H264 boot started.

Recovery checkpoint: prior observer87033 ended at its bound before guest fetch
or upstream app execution. Actual VM[]; no external-app result, not a GPU FAIL.
Carrier rollback1 exact original3381bd95... PASS. Reapplied same guarded slot
headless-apply2 exact32bae66f... PASS; unchanged approved profile started as
mavk-windows-upstream-headless2-20261016. No new platform/driver experiment.
Installed owner remains exact headless-build1. Must rollback after VM[].
