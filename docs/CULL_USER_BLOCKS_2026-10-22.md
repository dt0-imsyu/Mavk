# CullDistance user interface blocks — isolated GPU proof

Later update: variable-level block Location is also implemented and GPU-proven;
see CULL_BLOCK_LOCATION_2026-10-22.md. Scope below describes candidate8657e17b.

Common compiler now preserves user output Blocks through the inserted geometry
stage, rather than flattening their outputs. Member locations, interpolation,
numeric scalars/vectors and fixed arrays are forwarded. Builtin-only blocks
retain the previous handling. No app/game special cases.

New fixture carries vec4 color, flat signed integer and vec2[2] through a user
Block to the fragment stage. The fragment stage checks every payload field;
incorrect data produces green instead of the expected red. Independent
reference uses the same interface without CullDistance or a geometry stage.

Actual G925 loader hook PASS: original application VS/FS, automatic insertion,
8 cull cases x 4 modes (descriptor-free/UBO x tracking/guarded), 131072 bytes
compared. Geometry-disabled negative rejects. Strict verifier PASS. Host
compiler 8 valid / 5 negative PASS. Unmodified standalone interface generates
the exact previous geometry binary (SHA256 440758d4f2d26b0474e2d8bf6a2131b09374070ff670405072b5c230544ae280).

All 26 previous GPU regressions PASS, 28.486 s, with experimental cull disabled;
strict no-event-trace and no-diagnostic-read verifier PASS. Duration is not a
performance comparison. No CTS, full-feature, upstream DXVK or visible FPS claim.

Candidate layer SHA256:
8657e17bf1e2558cfd0356a60416d38b2e939d7930ac79aea3f8636b49ce08f9.
Artifacts E:/Mavk/artifacts/cull-user-block-20261022/:
gpu-results.tar.gz aacbbdb14536b88a00284c6d5f2a2076f909bdc9cbcaa0dd8f8fbaf438d9b7ac;
regressions.tar.gz 89a707ccaf951a4f4667a826b082771ed8a4736566c6cd5d19220e51102133ca.
Candidate binary E:/Mavk/artifacts/cull-user-block-layer-20261022/.

Scope: explicit per-member locations only. Block-variable Location, mixed
builtin/user blocks and invariant user blocks fail closed. Existing GS/TES,
PointSize, fragment CullDistance/PrimitiveId, specialization and the other
previously unadmitted contracts remain unimplemented. This does not complete
shaderCullDistance: public feature stays false, private diagnostic flag only.
APK25/runtime/source runtime assets unchanged. No U-AVF/Windows/Ubuntu changes,
commit or push. Candidate staged separately; previous candidates retained on E:.

Integration regression: Muck PID5722 mapped the candidate from mavk-aggregate;
experimental cull flag absent. Menu and Settings rendered correctly and tap
input changed screens. Checked session log had no GAP/CULL_LINK/device-loss/
shader-failure markers. This does NOT prove Muck exercises the new feature.
Game quit normally; only per-game diagnostic layer override removed; Steam
left running on common APK25 runtime. Screenshots and muck-logs.tar.gz retained;
log archive SHA256 382a8716d8afc3344e3f60f311548a7534fedb75baa4397a3146e456f202d306.
