# Next bounded compatibility slice — sampled 2D texture

Predecessor: unchanged upstream windowed triangle PASS on 2026-10-16,
24 hardware frames in each IMMEDIATE/FIFO run, EXIT0 in the waited FIFO run,
RGB triangle observed on Android. No general driver/game/DWM claim.

Concrete source inspected: SaschaWillems/Vulkan commit
`4ee2501c8280cf71d1f9ce231cd8ebd47b58adde`,
`examples/texture/texture.cpp`, upstream Git blob
`ac24246770ae70ab5736f06c33d0d4c291aed76a`.

Required by this app, not speculative feature advertising:

- RGBA8 optimal sampled image, staging-buffer upload via `vkCmdCopyBufferToImage`;
  actual KTX-provided mip levels, transitions for all uploaded subresources.
- Set0 binding0 vertex UBO and binding1 fragment combined-image-sampler.
- Real native Vulkan sampler/image/view and shader descriptors on Mali;
  existing descriptor-free fragment guard must not simply be disabled globally.
- Sampler filtering/addressing/LOD state must be transmitted explicitly and
  validated. Optional anisotropy remains unadvertised until implemented/tested.
- Preserve immutable application/shader/asset bytes and licensing. Inspect exact
  KTX asset size/mip layout before choosing transfer bounds. Existing1MiB packet
  bound is NOT permission to truncate a texture or silently drop mip levels.
- Vertex pos3/uv2/normal3 stride32 is within existing vertex-layout limits;
  current UBO-only descriptors and single-mip image model are not sufficient.
- Upstream benchmark mode disables its normal overlay in the existing framework;
  do not edit the app or claim general UI/ImGui support from a bounded test.

Smallest next implementation gate: reusable native upload/sampler ownership and
actual sampled shader result, then bounded ICD upload/descriptors and unchanged
external textured app. Preserve known-good window/compute candidates; do not
rerun those gates merely to consume test time. No new VM until a new needed
slice has a compiled, finite test and explicit rollback.

Windows VM is currently stopped; carrier restored. No production changes,
commit or push. Strategic Mavk goal remains active and incomplete.
