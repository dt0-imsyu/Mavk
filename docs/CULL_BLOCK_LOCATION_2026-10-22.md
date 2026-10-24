# CullDistance block-variable Location support

Later update: standalone fragment PrimitiveId forwarding is implemented and
GPU-proven; see CULL_PRIMITIVE_ID_2026-10-22.md. Scope below is historical5774b8bd.

Progress toward the active shared-runtime objective, not completion or a blocker.

Common compiler reflects a block-variable Location, advances member locations
according to numeric/vector/array footprints, and preserves the same Block
layout and variable-level Location on both generated GS interfaces. It does
not flatten the block or rewrite application data. Explicit member locations
retain the prior behavior. Added bounds before location arithmetic and an
explicit empty-interface rejection.

Actual test uses a user Block at Location=4: vec4, flat int, vec2[2]. Hidden
position/cull slots follow the full occupied range; host metadata locations=11,
inputComponents=15/outputComponents=13. Application submits original VS+FS;
the Vulkan layer injects the geometry stage. Independent reference has no cull
or GS, and FS checks every payload field.

## Evidence

- Host suite 9 valid / 5 negative PASS.
- G925 automatic-hook variable-location test: 8cases/4modes,131072 bytes PASS,
  10.080s; geometry-disabled rejection PASS; strict verifier PASS.
- Prior member-location block on the SAME candidate: 8cases/4modes,131072 bytes
  PASS,10.852s; dependency negative and strict verifier PASS.
- All26 previous GPU regressions with cull flag absent PASS,63.967s; strict
  no-event-trace/no-diagnostic-read verification PASS. Times are not FPS or a
  speed comparison.
- Existing standalone-interface generated GS remains byte-identical SHA256
  440758d4f2d26b0474e2d8bf6a2131b09374070ff670405072b5c230544ae280;
  member-block generated GS also unchanged SHA256
  bf3cc85af8b8add0e6ad4027dd04f166203ab2b0e49e53e63065656f090f8e24.

Candidate E:/Mavk/artifacts/cull-block-location-layer-20261022/:
libVkLayer_mavk_compat.so SHA256
5774b8bdaffd3b2e99ffd9df858408ed504d1f02ab7c65b843ae579831c0cc2f.
Evidence E:/Mavk/artifacts/cull-block-location-20261022/:
gpu-results.tar.gz 57ff50b384f34c827bbda3bb2fe107049e11eb6b56a63287cf8a8a6f6e507a37;
member-regression.tar.gz eb2d7f4831a7b2d7b68d66404c1d6d2152c8f396909cef10083dc151c607bde1;
regressions.tar.gz 57260207064d6a75a5abe4382a3439cd82f7917c11c64d55358da93de439affb.

Muck was NOT rerun on this candidate. Previous candidate8657e17b integration
result is recorded separately; do not misattribute it to5774b8bd. No extra game
run needed to claim this narrow GPU result. Installed APK25/runtime4de26844,
source runtime assets and archive pins untouched. No per-game diagnostic
override; Steam left running. No U-AVF/Windows/Ubuntu changes,commit or push.

## Remaining admission work

Invariant user blocks need distinct input/output block types. Existing GS/TES,
PointSize, fragment CullDistance/PrimitiveId and prior extended pipeline
contracts still reject. Points/lines have compiler-only proof. These are real
semantic gaps; public shaderCullDistance remains false. No CTS, ordinary DXVK
admission, full Vulkan support or visible-FPS claim.

Do not repeat the closed block-location matrix. Select next consumer-relevant
stage contract; retain explicit limits until implemented and GPU-verified.
