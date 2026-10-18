# Native descriptor adapter (bounded backend API)

`include/mavk/compat/native_descriptors.hpp` consumes the common immutable CPU
DescriptorSnapshot and creates **real Vulkan** layouts,pools,sets and guard
metadata. Native header uses existing Core allocations; no guest/Windows API,
Android-specific lifecycle,ICD loader hook or driver replacement is introduced.

## Contract

- Public sets followed by one guard set,no dynamic descriptors,push constants,
  immutable sampler bindings,update-after-bind,variable descriptor count or full
  VkDescriptor API. Kind/stage visibility and explicit shader metadata ABI are
  fixed at adapter construction. Descriptor layout mismatch/ABI mismatch rejects
  before allocations. Native Vulkan limits still apply. Public count is inferred
  from the highest populated set unless explicitly supplied as the fifth
  constructor argument. It must be1..7 and include all bindings; public count+1
  must fit actual maxBoundDescriptorSets. Holes/trailing public sets get empty
  native layouts. `guardSetIndex()` must match GuardCompilerOptions.
  Snapshot `sets()` now returns a vector of all public sets plus the guard set;
  callers bind its actual size,not a hard-coded2. Default set0 remains2total.
- Caller supplies a resolver for actual owned resources and **typed** null
  fallbacks. There is no universal dummy image: shader format/dimension/sample
  admission/reflection and fallback cache remain higher-level work. Every
  resolved descriptor must have a real resource lease and nonnull native handle.
  Buffer offsets/ranges/alignments and native type limits are checked before
  native allocations; bound buffer metadata must match the native descriptor.
- Each changed CPU snapshot materializes an independent descriptor pool,public
  sets,guard set,immutable metadata allocation and diagnostic log. It never
  mutates sets belonging to an older snapshot. Resources/fallback leases,CPU
  metadata and native layouts remain owned by that native snapshot.
- Repeated materialization of the cached CPU generation returns the same native
  snapshot,without pool creation,metadata upload or descriptor update. Cache is
  weak and does not retain retired snapshots/resources. Resolver mappings must
  be stable; explicitly invalidateResolverCache if replacing fallback mappings.
- Serialized caller/external queue synchronization. Submission owner retains
  each native snapshot until **all** GPU uses complete. Device/DeviceContext
  must outlive snapshots. Destructors do not call queue/device idle; no hidden
  hot-path stall. On fence timeout/error caller must drain work or handle device
  loss before retiring resources. Diagnostic reads also require GPU completion;
  repeated cached dispatches accumulate debug records unless caller explicitly
  manages completed diagnostic state. No production overhead claim.

The native buffer/atomic/image correctness gates now consume this same adapter
instead of implementing pool/set/update logic themselves. Image resolver owns
SampledImage or storage-image+view RAII bundles; buffer resolver owns Core Buffer.
They verify cache reuse does not increment Core allocations/native update counts.

## Independent lifetime acceptance

`descriptor_generation_comparison.hpp` records two ordinary Vulkan compute
command buffers with different descriptor generations **before submitting either**.
It mutates shadow state and drops caller resource/CPU snapshot ownership,then
executes both generations on the actual Mali. Each output must still match its
own original input. Distinct native sets are required,old resource leases must
survive recording/state mutation,and release after the completed GPU fence.
Three invalid materializations must reject before allocations. Cached native
generation must reuse its objects. Strict verifier:
`harness/verify-descriptor-generations.py`. See STATE.md for actual result;
source or a successful build alone is not PASS.

Changed-generation path still allocates a small pool/metadata/log. Recycling,
batching,diagnostic-free production mode,multiple public sets and a consumer
Vulkan entry-point layer remain open. Do not confuse this reusable backend API
with already working DXVK or guest ICD feature support.
