# Consumer-driven layout admission — 2026-10-23

## Corrected diagnosis

GravityMark Linux ARM64 through the existing Steam Mesa25 Zink failed at
vkCreateDescriptorSetLayout. Private diagnostic reports flags2,bindings4,
sType1000161000,allocator0. Flags2 is UPDATE_AFTER_BIND_POOL, NOT push descriptors.
The first interpretation as push descriptors was wrong. Zink source
zink_screen.c:init_layouts creates the four-binding bindless heap eagerly.
Native extension enumeration in this runtime has143 entries and no
VK_KHR_push_descriptor even in the original layer. An experimental push-filter
candidate712fb0f4 was unnecessary; its tracking-control assertion failed because
it wrongly expected that extension. ALL filtering/feature-query changes were
removed from source; that candidate was not installed or promoted.

BC audit: shared hybris per-format decoder intentionally leaves the full
textureCompressionBC feature false (BC1 RGB and full blit/filter guarantees are
incomplete). Sarek uses a separate14-format admission path, not a full BC feature.
Do not enable the aggregate bit or substitute Android assets for a benchmark.

## Implementation

- Accept a single descriptor-binding-flags chain whose flags are all zero, or
  bindingCount0. This is the same ordinary layout semantics as no chain. Preserve
  caller/native parameters, capture the existing owned schema normally.
- Retain native update-after-bind layout definitions without immutable sampler
  pointers as deferred layouts. They can be created, allocated, retained in
  pipeline schemas and bound while unused. No snapshot/nullable emulation claim.
- VkCreateGraphics/ComputePipelines rejects any statically referenced deferred
  set before native pipeline creation. Unknown shader patterns still fail closed.
  Same-owner compatibility is required for deferred layouts; no guessed extended
  flag compatibility. Layout/resource ownership remains retained.
- Updates/templates for these opaque sets remain unsupported. This is NOT full
  bindless/update-after-bind support. Existing native sampler path is unchanged.
- No Vulkan feature/extension bits changed, guards remain enabled. No per-game
  or vendor-specific exception. Expanded failure diagnostic records shape only.

Zero-chain semantics:
https://docs.vulkan.org/refpages/latest/refpages/source/VkDescriptorSetLayoutBindingFlagsCreateInfo.html

## Evidence

Final isolated layer SHA256
f1951495bd8ecb8ffda97314936b956f015163a7f2ea66a3dbe23ddf4bf69e3c.

- Pure policy helper21checks PASS on ARM64 (not GPU/CTS).
- Original89c8 rejects zero-flags-chain nested-buffer consumer; candidate passes
  GPU zero-read/write-suppression/sentinels with subgroup16 enabled, guards on.
- Old/new A/B for an unused sampled-image UAB layout: native feature booleans
  checked/enabled normally; old layout creation fails, candidate executes existing
  dynamic compute and descriptor-free graphics oracles successfully.512words and
  1024color components checked by the reused consumers.
- Referenced deferred SSBO layout negative: actual pipeline creation explicitly
  fails, draws0/submissions0/CoreLiveZero. No GPU dispatch with uninitialized data.
-12 existing GPU regression modes +2guarded cull-link controls PASS. API extension
  count/partial enumeration remains143, pushDescriptor0 in old/new/tracking.
  This is NOT CTS or broad game regression/performance proof.
- Actual GravityMark GL now gets beyond failed layout initialization, but exits
  with GLXBadFBConfig. Same-environment glxinfo exits0 and identifies hardware
  Zink Vulkan1.3/Mali-G925, GL core/compat3.2, GLES2.0. GravityMark desktop build
  has gl45/gles32 paths. No version spoof attempted. NO benchmark score/FPS gain.

## Persistence / next

Source: descriptor_layout_shape.hpp,layer.cpp,commands.hpp and reused execution
fixtures/control. Logs/builds E:/Mavk/artifacts/layout-deferred-native-20261023/
(gpu-results.tar.gz,gravitymark-gl-after.log,glxinfo-after.log), consumer2 and policy
in adjacent dated folders. Guest files/mavk-layout-deferred/payload only.
Original aggregate89c8 and WSI d07b hashes verified unchanged. Muck override
inactive; desktop03 remains available, no GravityMark process. No APK update,
production U-AVF/Windows/Ubuntu changes, commit or push.

Do not expand all Vulkan/OpenGL features merely to force GravityMark. Its Vulkan
BC resource admission and desktop GL context requirements are concrete remaining
compatibility gates, not measured low GPU throughput. Select a proper common 3D
workload supported by both native Android and this runtime for the performance
comparison; keep existing native GravityMark41.9FPS as its own result only.
