# Vulkan descriptor update frontend (bounded, not a loader layer)

`include/mavk/compat/vulkan_descriptor_updates.hpp` accepts ordinary
VkDescriptorSetLayoutCreateInfo, VkWriteDescriptorSet and VkCopyDescriptorSet.
It feeds the shared DescriptorState and NativeDescriptorAdapter. This is a
consumer-facing building block, not exported vkUpdateDescriptorSets or a full
Vulkan ICD. Logical set handles must NEVER reach the native Vulkan driver.

Supported: static uniform/storage buffers, combined/sampled/storage images,
R32 scalar uniform/storage texel buffer views,
arrays, consecutive binding rollover with explicit zero-count bindings,
VK_WHOLE_SIZE, ordered writes followed by ordered copies. Rejected batches leave
all logical sets unchanged. Same-set overlapping Vulkan copies reject, unlike
the broader common CPU copy helper. Copies from unwritten/destroyed descriptors
propagate unwritten state; GPU snapshot creation rejects it.

The caller scopes one registry per native device and registers trusted actual
handles, allocation sizes, usage flags and strong owners. Unregister marks the
logical resource destroyed; recorded immutable native generations retain the
physical allocation until GPU completion. Image tuples retain both view and
sampler. Even an experimental null combined-image descriptor retains and checks
its valid sampler. Repeated identical updates reuse per-slot tuples/snapshots;
there is no unbounded tuple cache. Device ownership, queue synchronization and
serialized frontend access remain the caller's responsibility.

Null writes are disabled by default. The isolated guarded correctness gates
explicitly opt in; they still use real typed fallback resources and shader
guards. This does NOT advertise nullDescriptor, robustness2 or partially-bound.

Explicitly unsupported: extension chains, immutable samplers, sampler-only,
dynamic descriptors,non-R32 texel views, descriptor buffers/indexing/update-after-bind,
automatic shader/layout
admission and loader dispatch. Full Vulkan command-buffer invalidation and
pending-update rules are not implemented here. Immutable backend generations
are NOT a promise of legal public Vulkan update-after-bind behavior.

## Bound-set composition

`snapshotBindings(logicalSets,abi)` composes registered logical sets into the
public pipeline indices implied by the vector,then generates the common guard
metadata and retains real resource leases. Each logical set's local bindings
remain unchanged. Null vector positions denote empty/unused public sets,not
null descriptors. Up to7positions are accepted; all-empty or unresolved/unwritten
sets reject. The native materializer checks the complete expected binding schema,
so omitting a required public set cannot silently produce a usable generation.

Same keys/source snapshots/ABI reuse a live composed snapshot. Cached source and
result references are weak,so the binder cannot keep retired resources alive
after logical sets and GPU snapshots are released. Resource destruction is
checked before cache reuse. Reordered or changed sets produce a different
snapshot; previous results stay immutable. Snapshot generation values are local
to the composition,not a global monotonic counter; native caches use identity.
This is not vkCmdBindDescriptorSets interception,pipeline compatibility tracking,
dynamic offsets or legal update-after-bind support.

CPU gate `test/bound-descriptors.cpp`:18checks/6rejections PASS,including source
reordering,duplicate binding,holes,logical nulls,old snapshots and weak-cache
resource retirement. Native evidence is recorded in docs/STATE.md.

CPU gate: `test/vulkan-descriptor-updates.cpp`, 16 assertions and 16 rejections.
Native lifetime gate consumes real VkWrite/VkCopy structures, then submits two
recorded generations on Mali. Strict verifier requires `--vulkan-api`.
The image gate uses this same frontend for native image/buffer writes. See
`docs/STATE.md` for executed evidence, not merely source/build availability.

## Descriptor update templates

Pinned Sarek's `src/dxvk/dxvk_cmdlist.h` invokes
`vkUpdateDescriptorSetWithTemplate`,so supporting ordinary writes alone is not
sufficient. The common frontend now compiles VkDescriptorUpdateTemplateCreateInfo
entries into an owned `UpdateTemplate`,then expands bounded application data
through the same transactional writes/resource registry. Offset/stride semantics
follow [the Khronos specification](https://docs.vulkan.org/refpages/latest/refpages/source/VkDescriptorUpdateTemplateEntry.html).

`compileTemplate(representativeLogicalSet,info)` uses that registered set's exact
layout schema,not its mutable contents. No set ownership is retained; compatible
sets can reuse the compiled template after the representative set is released.
The future loader must validate/resolve its VkDescriptorSetLayout handle to the
correct schema before calling this API. A nonzero create-info layout handle is
not independently looked up in a native device registry here. This API is not
vkCreateDescriptorUpdateTemplate/vkUpdateDescriptorSetWithTemplate interception.

Only descriptor-set templates and the seven existing buffer/image/texel descriptor
kinds are accepted. No push-descriptor templates,immutable samplers,dynamic
descriptors,non-R32 texel views,inline uniform blocks,extension chains or indexing behavior added.
Exact target schema mismatch rejects. At most4096entries/65536total descriptors,
16MiB required source extent; overflow checks precede source reads. Caller
supplies a valid pointer AND its readable byte extent,with external serialization.
Copied entries retain no application pointers. Interleaved/unaligned source
objects use memcpy; zero stride repeats the source object. Expansion owns its
temporary arrays until the staged update commits. Unknown handles,short input
and any later invalid entry leave the logical set unchanged. No GPU submission,
wait or native descriptor mutation is introduced by template expansion itself.

Actual CPU gate `test/descriptor-templates.cpp`:13checks/18rejections PASS;
ordinary frontend16/16 regression PASS. Android template generation gate:
two template updates,one descriptor copy,128verified GPU words,two recorded
generations/resources retained through completion,three invalid native resolver
cases rejected,SyncVal0/0,Core live zero. Strict acceptance uses
`harness/verify-descriptor-generations.py --vulkan-api --templates`.

Artifacts `E:/Mavk/artifacts/descriptor-templates-native1-20261018`:
APK SHA256`08f82fc7d86ce0e37907215b025e5ba8285360e3031210aad9db675967a32d5d`;
`core-descriptor-generations.json` SHA256
`4cc4a4ffb784f42ac871743a97311939619c18849845806148f28408f0f8af9e`.
This remains a bounded consumer building block,not a DXVK/D3D11/device PASS.

## Texel-view contract

`registerBufferView` records a caller-created real VkBufferView and its exact
backing buffer,format,offset/range and strong view owner. The backing resource
is retained separately. Currently only R32_UINT/R32_SINT/R32_SFLOAT views are
accepted;the caller must have checked actual native format features and native
view-offset alignment at creation. The registry validates byte bounds,4-byte
format units and required backing usage;it does not manufacture native views.
Uniform and storage usage are distinguished at descriptor writes.

Texel metadata uses element count,not byte count. Ordinary writes,ordered copies,
template VkBufferView data and bound-set composition reuse the existing common
state. Null writes stay opt-in. View/backing unregister invalidates subsequent
snapshots;old immutable generations retain their real resource ownership.
Native materialization checks actual maxTexelBufferElements and matching guard
count. Typed null fallbacks must own the actual compatible view and backing.
CPU12checks/15rejections and actual R32_UINT GPU gate are documented in
docs/TEXEL_BUFFER_CONSUMER_2026-10-18.md. No full nullDescriptor/robustness2 claim.
