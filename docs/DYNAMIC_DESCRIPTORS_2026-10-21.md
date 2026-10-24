# D3 dynamic buffer descriptor slice — 2026-10-21

Native Vulkan dynamic UBO/SSBO semantics, not a new emulated feature bit.
Production U-AVF/Windows/Ubuntu unchanged. Common experimental DroidDeck APK13
includes the vetted layer; APK12/runtime9fb3 retained as rollback.

## Contract and implementation

The common descriptor state normalizes dynamic UBO/SSBO to existing buffer
metadata kinds. Original Vulkan descriptor types remain in the layout schema,
copy/update/template compatibility, native pool sizing and descriptor writes.
Borrowed native layouts carry an explicit validated set/binding list of dynamic
buffers. No new Core descriptor kind, duplicate buffer allocation, pixel copy,
hidden queue wait or offset baked into the descriptor is needed.

Binding consumes/copies caller offsets immediately, ordered by set, binding
number and array element; layout declarations are canonically sorted. Partial
binds retain other sets' offsets. Recording retains strong descriptor/resource
generations as before. Native binding uses the actual copied offsets; guard
metadata remains the static descriptor range, exactly as Vulkan requires.
Alignment and effective offset/range are validated against native device limits
and real buffer size before protected execution. WHOLE_SIZE writes normalize
at update time; their remaining-full-buffer range only permits offset zero.
Logical null dynamic buffers also require zero offset. Full nullDescriptor
remains unadvertised; dynamic null execution has NOT been GPU-verified here.

Unchanged source snapshots can reuse one materialized native generation across
different dynamic offsets. Ordinary non-dynamic execution has an immediate
validation fast path; no new per-descriptor scan. Dynamic cases skip whole
non-dynamic rows. No game-name or shader-name behavior is introduced.

## Matrix and evidence

| Slice | Classification | Evidence / boundary |
|---|---|---|
| Dynamic UBO + SSBO, arrays, multiple sets | NATIVE + guarded tracking | Real loader consumer vs direct native, seven descriptors/two sets; matched GPU output |
| Unsorted layout declarations, ordered offset arrays | NATIVE preserved | Bindings7/3 and9/2, distinct sentinel values distinguish every source |
| Partial firstSet binds, immediate caller-array reuse | NATIVE preserved | Three dispatches in one command buffer, distinct output slots; one protected generation retained |
| Base plus dynamic offset, fixed range | NATIVE preserved | Nonzero base and offsets, all unwritten output words remain sentinel |
| WHOLE_SIZE, zero offset | NATIVE preserved | Second submission after reset/update, GPU result and sentinels checked |
| Invalid alignment/range/overflow/missing/extra offsets | Rejected | Host negative contract; missing/extra count guarded recording checks. No invalid input sent to native GPU as a test |
| Templates/copies of dynamic descriptors | NATIVE + shadow | Host update/template/snapshot/lifetime tests. Dynamic template GPU execution not separately covered |
| Null dynamic buffer | Experimental/unverified GPU slice | Host null-zero contract only; no new nullDescriptor feature claim |
| BindDescriptorSets2/core+KHR, graphics dynamic buffers | Implemented, GPU coverage open | Same binding helper; GPU oracle currently compute + core BindDescriptorSets |
| Immutable samplers, descriptor indexing/UAB | PASSTHROUGH MISSING | Not enabled by this work. Submit-time late updates require a different lifetime model |

Host six existing descriptor suites rebuilt PASS: state34/25, bound18/6,
updates20/20, samplers9/7, templates13/18, texel39/32. New dynamic contract
21checks/13negative cases includes update/copy/template, snapshot independence,
WHOLE_SIZE normalization and destroyed-resource rejection. Artifacts under
`E:/Mavk/artifacts/dynamic-descriptor-host-20261021`.

Initial candidate layer SHA256
`988df082ec74b679f1c29669f330ff49e8ee075f9259025f9124d7697e9d467e`.
Original three-dispatch loader consumer native and guarded PASS256checked words;
retained initial archive `dynamic-descriptor-gpu-20261021/results.tar.gz`.
Expanded WHOLE_SIZE consumer native and guarded PASS512words/four dispatches/
two submissions. Both verify actual candidate mapping and hardware Mali-G925.
The guarded log records four executions, zero gaps and no retained resources.
`verify-layer-execution.py --dynamic` requires matching outputs and these
execution/retirement gates. These loader tests have NO CTS/SyncVal instrumentation
and are NOT FPS or full Vulkan conformance claims.

Static command-routing regression on the candidate also PASS512words/
8192components,12protected commands/12submits,gaps0/Corezero. Core/KHR tested;
AMD native extension unavailable. No availability fabricated.

Fresh Android probe built from current Core/descriptor sources. Same existing
oracles, bundled shape selector only (four build-checked asset names, no arbitrary
shader path). Fresh Core five gates/13rows PASS; 2D + live-array +3D-storage +cube
1792words each,384/384/512/512guards,SyncVal0/0. Native descriptor generations
and template updates PASS128words each/SyncVal0/0. BC123,BC45,BC67 and texture
lifecycle regressions PASS. Reports under `dynamic-descriptor-gpu-20261021`.
This is not runtime-compiler/separate-sampler/atomic revalidation: native probe
was built without runtime guard compiler; earlier bounded evidence is retained,
not relabelled as a fresh execution of those additional modes.

Common APK12 runtime now independently reached READY/firstFrame in session
2026-10-21-01-steam. Muck19893 mapped common9fb32fc7 layer; candidate20613 mapped
isolated mavk-dynamic layer, active protected rendering99072executions at capture,
zero recorded gaps. Game stopped, candidate override removed. Source/game logs
saved separately; not a gameplay/visible-FPS measurement. No need to chase Muck
FPS to proceed with D3 immutable ownership or D4 submit-time semantics.

## Remaining acceptance / rollback

Final layer SHA256 `a96aa08724694a4173ebbe4d2a05d2ad9e5ab51c35e513fb21e2ce7fec15a4a6`
skips dynamic validation scans for non-dynamic workloads. Actual final native and
guarded GPU dynamic512words/four dispatches/two submits and static routing512words/
8192components PASS; `final-results.tar.gz` and `final/` retain exact evidence.
Final consumer SHA256 `04d9c04ec0b50ab18f67731e2941f4e4f92d5245f95313702ba6638ff147e7c0`;
original dynamic shader `bf1f078c2b3ab1ae05e519e73a6749995442de2c9ef1f4145a0b2eabc8ac9085`.
Muck22274 mapped final isolated library,73039protected executions at capture,
zero recorded gaps/device-lost. Do not
replace a mapped candidate library in place. Use a fresh app-private directory.
Common APK/runtime is the fallback; `muck-use-readonly.py --common
--layer-directory <selected-test-directory>` removes only the known test layer
override from both stores. Preserve Steam/account/game data and original sleep
preference. No commit/push yet.

Installed common runtime `b047abed1abef32032569287f8dbc23393b7540d67aa0116c356a4c91fb32331`
contains the exact final layer. Only manifest+library changed;261 other members
unchanged. APK13 is signed asset-only rollup of APK12,485other members unchanged,
matching baseline test certificate; not a full Gradle rebuild. Artifact/hashes
STATE.md. Clean session2026-10-21-02-steam loaded this runtime; common payload
dynamic oracle PASS512words/four guarded commands/two submits/Corezero. Diagnostic
Muck layer overrides removed from both stores. APK12 can be installed in-place
for rollback without uninstall/data clearing. Full game/visible FPS still separate.

Normal Steam launch Muck25086 mapped common b047 payload with no per-game layer
override;98038protected executions at capture,gaps0/device-lost0. Steam's CDP
renderer remains ANGLE/Zink/Mali-G925 after update. `common13-results.tar.gz`
retains common-library oracle and new game log. Game stopped after verification;
original suspendPolicy.steam absent/default manual restored, service refreshed,
finished probe stopped. No production changes and no commit/push.

References: [binding order, offsets, range and WHOLE_SIZE valid usage](https://docs.vulkan.org/refpages/latest/refpages/source/vkCmdBindDescriptorSets.html),
[buffer descriptor range semantics](https://docs.vulkan.org/refpages/latest/refpages/source/VkDescriptorBufferInfo.html).
