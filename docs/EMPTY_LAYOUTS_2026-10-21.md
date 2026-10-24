# D3 empty descriptor layouts — 2026-10-21

Native Vulkan semantics with shared protected tracking; no new feature bit.
Production U-AVF, Windows/Ubuntu, P7/firmware, storage and H.264 untouched.

## Implementation boundary

- DescriptorState can represent a genuinely empty declaration: zero bindings,
  zero values and a minimal CPU-only metadata header. NativeDescriptorAdapter
  still explicitly requires resource bindings; no empty-vector back() access
  or meaningless GPU metadata allocation is permitted.
- VulkanDescriptorUpdates registers empty layouts and zero-count reserved
  bindings. Their nonexistent elements cannot be written/read. A bound snapshot
  containing only absent placeholder handles still rejects, preserving the old
  negative contract; actual registered empty sets can be represented.
- Layer creation accepts bindingCount0 without pointer arithmetic on a null
  pBindings. An empty public slot stays at its original set number. Mixed
  schemas skip its binding requirement and snapshot rows, but retain its native
  layout in the allocated/bound generation. It is NOT a logical null descriptor.
- All-empty layouts with descriptor-free shaders bypass metadata adapter and
  guard compilation. No dummy resource or private metadata upload is invented.
  Descriptor-free native observation also skips the compiler: this fixes the
  previously logged guard-cache/set-limit gap at publicSets0 in native mode.

Scope is EMPTY slots, not arbitrary statically-unused nonempty sets. Nonempty
sets still need the existing tracked/initialized resource contract. Shaders with
unused descriptor declarations are not admitted by the descriptor-free branch,
which conservatively rejects descriptor decorations. Extended flags/pNext,
independent sets and general UAB/partial binding remain separate work.
Existing bounded set counts/private guard reservation remain; no broad Vulkan
conformance claim follows from this package.

## Matrix and real GPU evidence

| Case | Evidence |
|---|---|
| Empty slot between nonempty sets0/2 | Existing dynamic oracle parameterized by OUTPUT_SET2; seven dynamic UBO/SSBO descriptors, four dispatches/two submits,512words including sentinels |
| Empty slot left unbound | Native/guarded results identical; real sets0/2 bound independently, no fabricated set1 descriptor |
| Three all-empty public slots | Descriptor-free shared renderer, real VkPipelineLayout injection verified by call counter,1024color components match descriptorless reference |
| Empty set allocation with no pool descriptors | Native and guarded allocation/cleanup succeed; allocated empty set intentionally not bound |
| Zero-count reserved binding17 | One all-empty layout includes reserved storage binding, no resource or metadata row created |
| Shader/render correctness | 16x16 target,72exact red/184black pixels,alpha255; no sampler/descriptor reads, actual Mali rendering |
| Missing/unwritten real resources | Old bound6negative cases retained, new empty9checks/7negative cases PASS |
| Nonempty unused layouts, partially-bound indexing/UAB | Not newly supported; requires static-use/descriptor-lifetime work |

Final layer SHA256
`dd714682398f7b3224711daa27926d36cc495a36ce0dcaa2cc72c0a00a130167`.
Native/guarded complete stdout identical, gaps0/Corezero and all resources retired.
Strict verifier `verify-layer-execution.py --empty` passes `verified/` reports
under `E:/Mavk/artifacts/empty-descriptor-gpu-20261021`.
This loader consumer has NO CTS/SyncVal/FPS instrumentation. Offscreen readback
correctness is NOT Android visible frame-rate evidence.

Initial results.tar.gz retains the rejected comparison: pixels correct but
native observer logged one guard-cache/set-limit gap. The verifier failed that
run; it was not weakened. General descriptor-free handling fixed the source,
and verified-results.tar.gz holds the new passing result. Earlier candidate
e47d1e61dd63b7f500250694b8282557e273469be27a731ac8fa31f11dc431f5 is not final.

Host nine suites rebuilt PASS: empty9/7,immutable18/8,descriptor34/25,bound18/6,
updates20/20,sampler9/7,template13/18,texel39/32,dynamic21/13. Artifacts
`E:/Mavk/artifacts/empty-descriptor-host-final-20261021`.
Final layer's ordinary dynamic512word,immutable192component and protected
routing512word/8192component regressions PASS; backend-regressions.tar.gz retains
the actual stderr/retirement gates. Fresh standalone Core five gates/13rows,
null2D/live-array/3D/cube and descriptor generations/templates PASS with their
own validation/SyncVal gates. Native probe built before the final layer-only
descriptor-free branch fix, not evidence that the glibc layer has SyncVal.
Additional BC results and consumer rollout are recorded in STATE.md.
BC123/45/67/texture lifecycle regressions subsequently PASS as well. Probe build
41d8dc2c09288ecd647560c0ba5eab89ef0729f5ee562e888cb8f7a116a4db25.

Muck7524 mapped final isolated layer and recorded54168protected executions at
capture,gaps0/device-lost0. Startup/render-log regression only, not new gameplay
or FPS proof. Game stopped and diagnostic override removed from both stores.
No game rules, display transport or CPU/platform changes introduced.

Promoted into common runtime
`24b402d336a21afda3f2bd212037396689e335789d3ec704173ffd07f5f2d455`, installed
asset-only signed experimental APK15. Same cert,261other runtime members and
485other APK members unchanged; not a full Gradle rebuild. APK14/common1f35
retained for in-place rollback without clearing data. Installed common oracle
PASS512words/1024components,gaps0/Corezero. Normal Muck11382 mapped common24b4,
400444protected executions at capture,gaps0/device-lost0; Steam CDP remains
ANGLE/Zink/Mali-G925. Evidence common15-results.tar.gz. Test game/probe stopped,
original absent/default-manual sleep preference restored, Steam/data preserved.
No commit/push. Exact APK/hash/path STATE.md.

Reference: [Vulkan binding and statically used set requirements](https://docs.vulkan.org/refpages/latest/refpages/source/vkCmdBindDescriptorSets.html).
No CTS, full descriptor indexing, robustness2 or nullDescriptor admission.
Next: static-use-aware nonempty sets, then D4 submit-time/version-aware lifetime;
do not claim general late updates from immutable record-time snapshots.
