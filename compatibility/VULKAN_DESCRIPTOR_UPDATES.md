# Vulkan descriptor update frontend (bounded, not a loader layer)

`include/mavk/compat/vulkan_descriptor_updates.hpp` accepts ordinary
VkDescriptorSetLayoutCreateInfo, VkWriteDescriptorSet and VkCopyDescriptorSet.
It feeds the shared DescriptorState and NativeDescriptorAdapter. This is a
consumer-facing building block, not exported vkUpdateDescriptorSets or a full
Vulkan ICD. Logical set handles must NEVER reach the native Vulkan driver.

Supported: static uniform/storage buffers, combined/sampled/storage images,
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
texel/dynamic descriptors, descriptor buffers/indexing/update-after-bind,
multiple public sets in the native materializer, automatic shader/layout
admission and loader dispatch. Full Vulkan command-buffer invalidation and
pending-update rules are not implemented here. Immutable backend generations
are NOT a promise of legal public Vulkan update-after-bind behavior.

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

Only descriptor-set templates and the five existing buffer/image descriptor
kinds are accepted. No push-descriptor templates,immutable samplers,dynamic or
texel buffers,inline uniform blocks,extension chains or indexing behavior added.
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
