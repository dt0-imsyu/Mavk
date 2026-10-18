# Texel buffer consumer slice — actual Mali PASS

Pinned Sarek b6653094 uses UNIFORM_TEXEL_BUFFER and STORAGE_TEXEL_BUFFER in
dxvk_context.cpp (e.g.315,752/756,7106). A layer using the previous frontend would
reject those ordinary operations. This slice adds the actual resource types,
not a fake extension/feature bit or a relaxed Sarek admission check.

Common DescriptorState has UniformTexelBuffer/StorageTexelBuffer and explicit
element-count metadata. Registry records real R32 scalar views+backing leases,
bounds and usage. VkWrite/VkCopy,template VkBufferView payloads,bound-set cache
and native VkWriteDescriptorSet materialization share those implementations.
Typed null fallbacks remain explicit and owned;real native descriptors never
receive null handles. View/backing unregister invalidates future snapshots.

Only R32_UINT/SINT/SFLOAT registrations accepted;actual GPU semantics tested
here only for **R32_UINT scalar x components**,static two-slot arrays,compute
texelFetch/imageLoad/imageStore and positive out-of-range coordinates. This is
not all texel formats/components/ops,negative coordinates,dynamic indexing,
image atomics,complete nullDescriptor/robustness2,performance or DXVK PASS.
Native format/alignment admission is the real view creator's responsibility;
registry does not replace native vkCreateBufferView validation. No loader layer
was delivered in this turn;the existing hybris layer was inspected,not replaced.

## Executed proof

Artifacts `E:/Mavk/artifacts/texel-descriptors-native1-20261018`.
APK SHA256`ce10cc0b6fe611fd4b552c5109e84c10a5c35eacd0557711414272b110626268`.
Only org.mavk.probe installed. Windows/Ubuntu/P7/H264/storage unchanged.

Original SPIR-V compiled by the shared runtime compiler on Android. Real
R32_UINT format buffer features checked before native view creation. Each view
contains64elements while its backing contains128elements. Two metadata phases
swap null/live views. GPU verifies2304values,including ordinary reads,valid
stores,discarded null/OOB stores,and unchanged backing sentinel region beyond
the view.1152actual diagnostic records:768null,128uniform-texel OOB,256storage-
texel OOB. Two submits,Core live zero,SyncVal0errors/0warnings. Strict checker:
`harness/verify-null-texels.py`.

`core-null-texels.json` SHA256
`abb49fa96726534cd8ea201e5eb9bc3e66e02a27a539a4a739cfaf1f5e729f7c`.
Same APK combined/storage image regression1792values/384guards PASS,SyncVal0/0;
`core-null-images.json` SHA256
`58bc8fbecdeec87cd1e8947c4eeb15cfccdca510a745098e607872abbca24e09`.

CPU test `compatibility/test/texel-descriptors.cpp`:12checks/15negative cases PASS.
It covers registration bounds,usage mismatches,templates/copies,logical nulls,
bound snapshots,cache identity,view/backing retirement and old resource leases.
Existing shared state34/25,templates13/18,bound composition18/6 regressions PASS.
No new Windows VM boot was needed.
