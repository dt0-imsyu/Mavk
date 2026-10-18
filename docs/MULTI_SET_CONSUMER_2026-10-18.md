# Multi-set consumer prerequisites — actual Android PASS

Pinned Sarek b6653094 `src/dxvk/dxvk_pipelayout.h:67` explicitly separates
graphics views(set0) and buffers(set1). Mavk's fixed private metadata set1
conflicted with that ordinary consumer arrangement. No Sarek requirement was
removed and no Vulkan capability was falsely enabled to get around it.

## Implementation

- GuardCompilerOptions.publicSetCount is immutable per compiler/cache;1..7,
  private set index equals that count. Original public decorations outside that
  range/private collisions reject. Vulkan1.1 target and existing opcode limits
  remain;this is not exhaustive shader admission/reflection.
- NativeDescriptorAdapter matches configurable public count+guard,including
  empty intermediate/trailing public layouts,and actual maxBoundDescriptorSets.
  Descriptor writes address each binding's real set,not always set0. `sets()`
  returns a vector;default one-public-set callers retain the same2-set behavior.
- VulkanDescriptorUpdates.snapshotBindings composes ordinary per-logical-set
  writes/copies/templates into one immutable common snapshot. Weak source/result
  caches avoid retaining retired resource allocations. Target layout still
  verifies the complete expected schema. No dynamic offsets,partial pipeline
  compatibility tracking,loader hooks or update-after-bind support added.

## Actual proof

Final artifact directory `E:/Mavk/artifacts/multiset-guards-native2-20261018`.
APK SHA256`83e6a9fa0cac991f9dec8d89efa8f9254494fe37a0c63434ab9c7d59ce5006f1`.
Only org.mavk.probe updated;Windows/Ubuntu/P7/H264 and their disks unchanged.

`runtimeMultiSets` compiles the original3232B SPIR-V on device into9372B:
SSBO arrays/output in public set0,UBO array in public set1,empty public set2,
metadata/log private set3. Ordinary VkWriteDescriptorSet structs populate the
two logical sets;common binder/cache/native materializer supplies GPU sets.
Two metadata phases swap null and valid resources. Actual Mali-G925 verifies
1536words/1024guards,valid+null IAdd32/load/store/UBO/range side effects,Core live
zero and SyncVal0errors/0warnings. One cold compile9.86923ms,one exact cache hit
0.000846ms;single samples,not general throughput/overhead measurements.

Strict verifier:
`harness/verify-null-buffers.py --atomics --shared-state --runtime-compiler --multi-sets`.
Report `core-null-atomics.json` SHA256
`66705259441325d467f7685388f84eb0dea5d8e9b3d2ad0fbc2e5b737c8ba2a3`.

Same APK regressions,each with SyncVal0/0:

- Default single public set +private1,1536words/1024guards PASS. Report
  `core-null-atomics-single-set.json` SHA256
  `7face7c4d05562f389de8129b4551327dac546a570f4141e9ed3c9c433530fe0`.
- Two-recorded native generations through descriptor templates128words PASS;
  report `core-descriptor-generations.json` SHA256
  `4cc4a4ffb784f42ac871743a97311939619c18849845806148f28408f0f8af9e`.

Host gates:compiler14assertions/9rejections;bound-set composition18checks/6
rejections;existing descriptor templates13checks/18rejections. Cache identity,
source reordering,holes,old resource leases,weak retirement and rejected private
set collisions covered. These narrow gates do not prove full Vulkan semantics.

Next remains the real loader/consumer shader+layout+command integration and
missing descriptor/shader operations. Full nullDescriptor/robustness2 and an
actual DXVK D3D11 device/frame are NOT yet achieved. Windows is still preserved,
not used as a per-feature test harness.
