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
