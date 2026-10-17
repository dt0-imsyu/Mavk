# Shared descriptor state — first consumer-adapter component

`include/mavk/compat/descriptors.hpp` is portable C++17, no Android/Vulkan/Windows
dependency. Both local DroidDeck and remote U-AVF adapters can use this state
after resolving their resources into native owned leases. It does not encode
handles, change a guest protocol or implement the entire Vulkan descriptor API.

## Implemented semantics

- Immutable binding layout, explicit kind/count; bounded sets/bindings/total
  descriptors. Uniform/storage buffers, combined/sampled/storage image kinds.
- **Unwritten != explicit null != bound.** Submission snapshots reject unwritten
  slots rather than inventing partially-bound support. The caller must limit
  this slice to the active layout or explicitly populate all slots. Full
  shader-usage/reflection/partially-bound semantics remain consumer-layer work.
- Checked buffer allocation/offset/alignment/range; whole-size resolves once to
  remaining bytes. Zero range is not a substitute for explicit null. Guard ABI
  currently supports32-bit byte ranges,not arbitrary64-bit range arithmetic.
- Type-compatible copies within an explicitly bounded binding range; overlapping
  source is snapshotted before mutation. No implicit spill across Vulkan bindings.
  Invalid writes/copies preserve generation/state/cache. Exact no-op writes do
  not create a new generation. Lease equality includes ownership identity,not
  just a coincidentally equal pointer address.
- Immutable per-generation snapshots retain strong resource leases. Buffer
  owner must own the actual allocation; image owner must own views/sampler and
  their image dependencies. A snapshot stays valid after state mutation or
  destruction. The submission owner releases it **after GPU completion**.
- Cached snapshots for both proven SPIRV-Tools metadata ABIs: buffer-only
  three-level lookup vs combined descriptor-index/init four-level lookup with
  array counts. Unchanged state returns the same snapshot,avoiding repeated
  packing and allocation. Sparse set/binding numbering is supported within the
  bounded CPU model; shader/layout validation must reject undeclared accesses.

Snapshot ownership does NOT own native descriptor pools/sets/layouts or permit
updating a descriptor set still in use. A real asynchronous adapter must retain
those separately and obey native descriptor update/submission synchronization.
Do not treat the metadata builder as a malicious-shader sandbox.

The shared native backend now supplies that missing native ownership layer:
[NATIVE_DESCRIPTOR_ADAPTER.md](NATIVE_DESCRIPTOR_ADAPTER.md). It materializes
immutable native generations and keeps pools/layouts/metadata/resources alive;
CPU DescriptorState itself remains independent of VkDevice and native lifetime.

## Actual acceptance

CPU test `test/descriptors.cpp`:34 assertions/25 rejected invalid operations,
both ABIs,holes,counts,null/range,whole-size,alignment,type/range errors,overlap,
partial-invalid-copy atomicity,unwritten rejection,cache reuse,resource retention
across state teardown,and shared ownership-identity changes. Not Vulkan CTS.

Native integration replaces the hand-packed numeric tables in the existing
buffer/atomic and image gates. Snapshots now drive BOTH native descriptor writes
and shader guard metadata. Real `shared_ptr<Core Buffer/SampledImage>` owners and
storage image+view RAII leases are used; null slots select valid retained native
fallbacks while guard metadata causes read-zero/write-discard. Submitted snapshot
is kept through the bounded GPU fence. These correctness gates are synchronous,
not proof of concurrent descriptor update-after-bind or sustained performance.

Use latest native report verifiers with `--shared-state` (and `--atomics` for
integer-atomic gate). This requires the new fields rather than accepting an old
manual-metadata report as evidence of integration. See STATE.md for hashes and
actual execution outcome. No `nullDescriptor`,robustness2,BC feature or DXVK
support is advertised from this component alone.

Next: connect these primitives to real consumer descriptor/image entry points,
shader-layout admission/remapping,typed null fallback cache,execution-time
snapshot retention and remaining image/array/atomic semantics. Keep WDDM and
production display outside this work.
