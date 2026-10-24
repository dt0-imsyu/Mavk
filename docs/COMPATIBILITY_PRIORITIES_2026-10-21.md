# Practical compatibility priorities — 2026-10-21

Supersedes the old plan to exhaust one descriptor subsystem before moving on.
P0 = blocks admission/start in the NAMED target; P1 = common correctness/lifetime
risk; P2 = conditional/rare/optional. A P0 for upstream DXVK is not automatically
the next blocker in the already-working experimental Sarek path.

## Pinned sources, not capability inference from one game

- Current Sarek source actually built in sarek-native-source-20261018/source:
  src/dxvk/dxvk_device_info.cpp,dxvk_sampler.cpp. Required clip/cull/multiViewport/
  wireframe/robustBufferAccess2 differ from upstream; experimental14-format BC
  admission and null-descriptor diagnostic do NOT establish standard features.
- Original DroidDeck DXVK pin25ca63f17f34bdc05a39873ee63907d3cbbfa030 retained in
  capability-audit-20261017: seven raw-Mali mandatory gaps. Current upstream
  master source reviewed2026-10-21 still requires those seven semantics. This
  is source inspection, not a new full current-DXVK device/game execution.
  https://github.com/doitsujin/dxvk/blob/master/src/dxvk/dxvk_device_info.cpp
- Actual private Mesa25.1.9 zink_screen.c/zink_descriptors.c: bindless layouts
  have4bindings with UAB/unused-pending/partial flags; descriptor-buffer mode has
  additional requirements. Steam's existing tracking/native route does not
  prove guarded bindless implementation. It does not need to be replaced now.
- Installed common hybris SHA179870fc matches the preserved RG32F layer build;
  actual device query/consumer semantics must be distinguished from raw Mali.

## Fresh app-owned query differential (no device/workload created)

Existing real-loader consumer now has --requirements mode. Artifact
`E:/Mavk/artifacts/consumer-requirements-20261021/requirements-results.tar.gz`,
consumer SHA436ce9128d63575592f73bde41bb69b1e636d5c1c02f6697f9132345f59d2a6d.
It queries19feature fields,4extension presences and16BC format masks/2D sampled+
transfer admission rows in three SAME-runtime contexts:

| Query | ICD-only | Common default | Common HYBRIS_BC_TEXTURES=dxvk |
|---|---|---|---|
| shaderClipDistance reported |0|1|1|
| shaderCullDistance / multiViewport / fillModeNonSolid |0/0/0|0/0/0|0/0/0|
| textureCompressionBC aggregate |0|0|0|
| robustBufferAccess2 / nullDescriptor / robustness2 extension |0/0/absent|0/0/absent|0/0/absent|
| BC131..132 sampled+transfer2D admission | unsupported | admitted | unsupported |
| BC133..146 sampled+transfer2D admission | unsupported | admitted | admitted |
| Sampled-image UAB / unused-pending / partially-bound |1/1/1|1/1/1|1/1/1|

These are queried capabilities, NOT correctness/CTS PASS. In particular default
hybris BC admission differs from the D3D profile; don't inherit all16format
capability rows into that profile or call the aggregate feature implemented.
Source confirms common clip=1 is external hybris VS/FS rewriting,not new native
Mali support: spirv_clip.c injects fragment OpKill, clip_distance.c advertises the
feature/limits. No full clipping-semantic test or Mavk conformance backs it.
Preserve the working baseline while explicitly treating this as an unverified
external compatibility claim, not closing upstream's clip-distance requirement.
Fresh query makes no draw/GPU/visible-FPS claim and does not enable any feature.

## Update-after-bind decision

Sarek getFeatureList requires native descriptor-indexing subfeatures, but actual
dxvk_sampler.cpp uses a one-binding SAMPLER heap with flags7. This exact heap
already stays native in Mavk; image/buffer snapshots are separate. Do not turn
that source fact into general image-UAB support. Zink bindless is a different
path. Current Steam log confirms mode=tracking, not guarded UAB execution.

Defer broader UAB; do not remove native sampler support, spoof bits, mutate
pending snapshots, or require a large command-replay rewrite before other P1
systems receive coverage. This is prioritization, not a claim that UAB is solved.

## Cross-subsystem matrix

| Subsystem | Priority / target | Existing support and meaningful remaining gap | Next gate |
|---|---|---|---|
| Null/robustness | P0 upstream; important existing Sarek compatibility debt | Bounded guards work; whole nullDescriptor/robustBufferAccess2 incomplete; diagnostic Sarek admission remains experimental | Keep false; reopen full contract at upstream milestone or actual failed operation |
| Descriptors/lifetime | P1 current path | Dynamic/static/immutable/selection/repeat cases verified; general UAB/partial/variable still missing | Checkpoint now; no more speculative P2 expansion |
| Images/formats/BC | P0 Sarek admission; P1 game correctness |14BC format API slice and GPU decoders work. Full compressed Vulkan lifecycle is not implied; view/subresource/copy/alias semantics need focused actual-API evidence | Next package: mip/layer views, mutable UNORM/sRGB identity and compressed upload/copy execution ordering using installed runtime |
| Native image/depth layouts | P1 current path | Aspect/readable-layout correction fixed actual Muck failure. Not complete depth/stencil/attachment/view matrix | Test commonly used mip/layer/aspect combinations, not every null shape |
| Shader instrumentation | P1 current path | VS/FS/CS work; groups/no-report stage issue fixed. Pointer/image-atomic forms and wider stage interactions reject | Select consumer-relevant SPIR-V/stage operations after image package; preserve checks without unavailable stores |
| Clip/cull distance | P0 upstream; P1 dependent games | Raw Mali lacks both. External hybris source advertises clip and rewrites VS+FS; full primitive semantics NOT proved by that claim | Separate correctness audit; do not count fragment kill as full Vulkan clipping or new Mavk native support |
| MultiViewport/wireframe | P0 upstream; P1 dependent games | Native missing, no complete shared implementation. No viewport duplication/FILL substitution | Explicit large emulation backlog, not an admission-bit fix |
| Geometry/tessellation/blend/MSAA | P1 dependent games | Native G925 supports several; native support != protected pipeline proof | Prioritize ordinary stage/render-state failures from requirements, not exotic extensions |
| Synchronization/memory | P1 all real games | Native timeline + bounded submit/retention checks; independent queues/transfers/alias lifetime unproved | Common upload/readback/host visibility and actual queue boundaries before optimization |
| WSI/ABI | P0 integration; P1 stability | Current Steam/Muck display and libhybris path work; not all resize/recreate/ABI forms | Preserve runtime; classify new failures separately from shader/descriptor faults |
| Pipeline cache/compiler/transfer cost | P1 performance stage, NOT current correctness PASS | No universal measured overhead/visible-FPS result; known architecture includes copies/waits | Measure attribution after functional image/shader packages; fix proven hot path |
| YCbCr/sparse/descriptor-buffer/GPL extras | P2 current D3D11 route | Native availability varies; guarded forms incomplete | Deferred unless a selected real consumer requires them |

Raw-Mali seven gaps: robustBufferAccess2,nullDescriptor,textureCompressionBC,
shaderClipDistance,shaderCullDistance,multiViewport,fillModeNonSolid. Partial BC
decode and experimental Sarek device admission do NOT close the aggregate BC
feature. Do not interpret current upstream's required device flags as proof all
those mechanisms are exercised by every game.

## Finite package cadence

1. Descriptor checkpoint: implemented/deferred/unsupported + evidence level;
   CTS NOT RUN for Mavk, not hidden behind custom GPU PASS.
2. Images/formats/transfers: one focused P0/P1 package across actual Vulkan API.
   Distinguish common Core CPU model from installed glibc/hybris emulation.
3. Shader/pipeline correctness: select the highest consumer-relevant missing
   operation from inspected requirements or a reproducible failure.
4. Memory/synchronization and WSI dependencies as failures require; no arbitrary
   platform changes and no inference from failed present alone.
5. Systematic performance attribution; Muck retest first (real present/frametime,
   CPU/GPU/errors), HL2 only afterwards. Do not chase a Muck FPS target now.

After each shared semantic change: affected GPU gate + prior bounded regressions
and Muck integration. A test-only audit does not require reinstalling the APK or
restarting Windows. CTS use must be explicitly recorded with executable/version,
case list,actual adapter and results; native Mali CTS is not automatically Mavk
guarded compatibility CTS. No all-games/performance promise from Steam/Muck.
