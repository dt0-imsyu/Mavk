# Native subgroup-size stage passthrough — 2026-10-23

## Consumer failure and bounded fix

Official GravityMark1.89 Linux ARM64, original assets, through installed guarded
Mavk rejected four compute pipelines with `extended shader stage unsupported`.
Private diagnostic build82a64114 identified sType1000225001: required subgroup
size. G925 reports subgroupSizeControl1,min/max16,compute admitted. This is a
native capability, not a new emulation or feature-advertising opportunity.

Added shader_stage_chain.hpp: accept the single required-subgroup-size structure
on a module-backed stage and preserve its exact caller pointer when replacing
the shader module with instrumented SPIR-V. Flags, specialization, entrypoint and
stage remain unchanged; driver still validates enabled features and limits.
Existing inline-module path consumes the inline structure as before. Unknown,
multiple/extended chains and unsupported inline combinations remain fail-closed.
CullDistance-linked pipelines explicitly reject this new combination because the
linker replaces the producer stage, so must not silently discard its request.

Specification:
https://docs.vulkan.org/refpages/latest/refpages/source/VkPipelineShaderStageRequiredSubgroupSizeCreateInfo.html
https://docs.vulkan.org/refpages/latest/refpages/source/VkPipelineShaderStageCreateInfo.html

## Evidence

- Pure stage-rewrite helper executed on ARM64:11 checks PASS. Preservation of
  caller data/pointers, replacement module, inline removal, unknown/tail rejection.
  This checks rewrite policy, not GPU execution.
- Same new execution consumer, subgroupSizeControl explicitly enabled only after
  native feature/property/extension checks. Existing nested-buffer GPU workload.
  Old aggregate89c8: exits1 at stage admission. Candidate253758f4: exits0,
  readZero/writeSuppressed,31 sentinel words and32 input words unchanged.
  Candidate mapped-path checked, CoreLiveZero, guards remain enabled.
- Candidate253758f4:12 existing actual GPU regression modes PASS (nested,dynamic,
  resubmit,inflight,empty,unused,selected,immutable,grouped,routing,texel,images).
- Actual GravityMark candidate: no MAVK_LAYER_GAP; SceneManager/PrefixScan no
  longer fails and reaches Creating RenderManager at7.349s. Still BC7 format
  failures; bounded runner terminates at10.564s, not a valid benchmark result.
- Final candidate49369e395def798ab89840a71e8da6cfeb88d978631c7cead34c0ca2b06cda8b
  additionally rejects unverified subgroup+cull-link combination. Full ARM64
  build PASS; repeated old/new subgroup GPU A/B and12GPUregressions PASS.
  Existing guarded cull-link descriptor/no-descriptor2controls also PASS.
  Final real GravityMark control also has gaps0, reaches RenderManager2.352s;
  terminated at10.491s for unsupported BC7, no benchmark score claimed.

Not Vulkan conformance/CTS, no full subgroup API claim, no measured FPS gain.
Original aggregate89c8 and WSI d07b hashes remain unchanged. No APK install,
production runtime change, new feature bit, U-AVF/Windows/Ubuntu change or commit.

## Artifacts / next

E:/Mavk/artifacts/stage-chain-diagnostic-20261023/ (exact sType failure),
stage-subgroup-passthrough-20261023/ (first functional candidate and consumer log),
stage-chain-test-20261023/ (rewrite-policy executable),
stage-subgroup-consumer-20261023/ (GPU consumer),
stage-subgroup-final-20261023/ (final isolated build).
Guest files/mavk-stage-subgroup holds private payload/logs; no installed file
replacement. Keep desktop03 available; no ongoing GravityMark process.
Final logs in files/mavk-stage-subgroup-final; results archived as
stage-subgroup-final-20261023/gpu-results.tar.gz and gravitymark-control.log on E.
Next real benchmark boundary is original BC7 resource admission, not the already
fixed required-subgroup stage request. Do not reintroduce invalid ASTC substitution,
disable guards or fabricate textureCompressionBC to get an apparent score.
