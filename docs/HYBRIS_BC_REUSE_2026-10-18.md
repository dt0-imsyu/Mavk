# Existing libhybris compressed-image reuse proof

One bounded A/B, no VM launch or production change. Reuses linuxfs-r9 and
hybris4073873 from the earlier component PoC under Termux debug UID. This is not
an installed DroidDeck session, game, conformance or performance result.

Same glibc executable, shader, device and driver in both launches:

- Bare ICD +HYBRIS_BC_TEXTURES=dxvk: all16 BC format queries unsupported,
  exit2 before allocating compressed images.
- Add VK_LAYER_PATH to existing hybris/lib and
  VK_INSTANCE_LAYERS=VK_LAYER_HYBRIS_compat:14 D3D formats133..146 become available.
  Real VkCreateImage(compressed), memory binding, image view, host compressed
  block upload, CmdCopyBufferToImage, barriers, compute texelFetch and host SSBO
  readback PASS896 components. BC1 RGB131/132 remain excluded as designed.

Actual blocks: BC1/2/3 opaque red, BC4/5 maximum positive channels, BC6H zero,
valid BC7 mode6 zero RGBA. Includes signed and sRGB format identities but NOT
all selector patterns, nonlinear sRGB values, negative HDR, modes or mips.
Mavk's independent CPU/native correctness suite remains necessary. No feature
bit was spoofed; device creation did not enable textureCompressionBC.

Important remaining format blocker: BC5 UNORM141 and SNORM142 return0xc001,
missing FILTER_LINEAR0x1000. Others return0xd001. The existing fork's D3D BC
format check requires linear filtering for all14,so admission still fails.
Do not remove the requirement or claim a complete BC layer from nearest-sample
success. Pinned upstream bc_image.c maps BC5 to R16G16_UNORM/SNORM; this is a
specific backing/filter lead, not proof that changing the mapping alone fixes it.
Shader output packing, transfer/copy semantics and signed precision must agree.

Existing layer also prints HYBRIS_SCALED_VERTEX experimental fallback_mask0x1000.
We did not expand vendor guards, validate that unrelated fallback or claim its
coverage. No robustness2/nullDescriptor layer implementation was found in the
reviewed fork patches; Mavk's common guarded consumer work remains necessary.

## Reproduction and evidence

Source: experiments/droiddeck/{hybris_bc_probe.h,bc-sample.comp,bc-control.sh},
existing build-probe.ps1 and run-probe.ps1 -Script bc-control, then
-BcCompatibilityLayer. Fresh bounded directories, no overwrites.
verify-bc-probe.py requires exact14formats/896 components and bare-ICD negative
control; --require-dxvk-formats deliberately exits2 for missing BC5 filtering.

E:/Mavk/artifacts/hybris-bc-api1-20261018/{run,compat} retains all logs.
Probe SHA c26215035b04b7365208ece305bd2d432e9bb8993e2767876576a52ad93d5cbf.
Shader SHA ab7ab2423123881047e7acfc40fb76faef0541c02ce73aaa2acf6b0cc8813578.
Compat stdout SHA30a7dd447be532c6e45f9524af4735cb3624614c144f584275f030eb50bd67db;
stderr SHA038123b5bcfaec97da731ffa413d133c032f6c91bcdf058f834d005ade8e4873.
Existing component build-recipe was read on tablet and contains the reviewed
BC/vendor patches; no new libhybris build/install needed for this proof.

Pinned implementation:
https://github.com/taowen/libhybris/blob/bc8cb3586b79d208fd21b925fa0209b3e432e008/hybris/vulkan/compat/bc_image.c
Fork BC/profile patches:
https://github.com/wallentx/DroidDeck/tree/0916a538aff4c08f799a4bd9fa17dd3c98b01e99/tools/hybris/patches
The global BC feature means full family guarantees,not merely one sampled use:
https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceFeatures.html

Next: actual linear-filter-compatible BC5 backing/output candidate if a small
isolated layer build is available; otherwise preserve reuse result and continue
the shared null/image consumer adapter. No repeated bare-ICD capability sweeps.
