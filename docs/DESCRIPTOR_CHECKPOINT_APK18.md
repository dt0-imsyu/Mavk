# Descriptor package checkpoint — APK18, 2026-10-21

This is a bounded package checkpoint, NOT complete Vulkan descriptor support.
APK18 SHA23de7423a9a89bf1cce936384d93e5faba3293993726b7ddbe3639818aee0ca7;
common runtime161fe2e8, layerbc30f541. APK17/acda20c7 retained as rollback.

## Evidence classes

**CTS: NOT RUN for every row below. No Mavk feature is CTS-passed.**
CPU contracts, custom GPU/readback tests, validation and application integration
are different evidence classes. Bionic probe validation0/0 does not establish
glibc guarded-layer SyncVal or conformance. No general games/FPS claim follows.

| Area | Implemented slice | Evidence / remaining boundary |
|---|---|---|
| Writes, copies, templates | Shared transactional state, ownership, ordinary static updates | CPU contracts; generation/template GPU tests. Native heap copies/templates still separate gaps |
| Dynamic UBO/SSBO | Full native dynamic-offset order, alignment/range, immediate caller-array consumption, partial binds | Native/protected512words. Not all graphics/BindSets2/null-dynamic cases |
| Immutable samplers | Owned layout state; destination sampler retained across combined copies/templates | GPU192components; native G925 mismatch reproduced/corrected. YCbCr unsupported |
| Unused/empty bindings and sets | Entry-point/stage/call-tree selection; complete native pool/offset schema | GPU512words +1024component draw cases; unwritten USED element/set rejects. Not partial runtime arrays |
| Shader decoration groups | Shared reflection and normalization before instrumentation | Valid grouped GPU192component oracle; compiler25/9. Not every SPIR-V operation |
| Bound texel formats | RGBA8-family + scalar/vector32 metadata for real native views | CPU53/39; UNORM/UINT512components GPU. Other widths/packed/storage permutations not proved |
| Null buffers/texels | Bounded static buffer/range/integer-atomic and R32 texel slices | Custom GPU readback/guard oracles, not full robustness2/nullDescriptor |
| Null images | Tested 2D/2D-array fetch/sample, bounded3D/cube explicit-LOD and selected storage operations | Custom GPU variants. MS/depth/cube-array/queries/image-atomics/mips/filtering completeness missing |
| Separate image/sampler | Real owned samplers, tested nearest/linear with live/null images |2048components/256guards in earlier bounded probe; null samplers not valid Vulkan requirements |
| Lifetime | Recorded generations survive repeats and simultaneous pending uses, retire on reset/free |512word native/protected replays; two genuinely timeline-blocked pending submissions. Not general queue/secondary/UAB proof |
| Update-after-bind | Existing exact native sampler heap passthrough only | Used by Sarek; general guarded image/buffer late updates NOT implemented |
| Advanced descriptor models | No general variable/partial counts, inline-uniform/mutable/descriptor-buffer/acceleration descriptors in guarded frontend | Explicit unsupported boundaries, not assertions about native Mali support |

Detailed artifacts/scopes: DESCRIPTOR_COVERAGE.md, DESCRIPTOR_LIFETIME_2026-10-21.md,
SELECTED_BINDINGS_2026-10-21.md, RGBA8_TEXELS_2026-10-21.md and
NULL_DESCRIPTOR_CONTRACT_2026-10-18.md. Earlier isolated probe results are not
silently relabelled as fresh execution of every API through APK18.

## Application evidence, not universal regression freedom

APK18 normal Big Picture reaches READY and CDP identifies hardware ANGLE/Zink/
Mali. Muck uses COMMON161fe2e8 with no per-game library override,11569protected
executions/gaps0/device-lost0 at capture; earlier isolated80209 plus visible menu.
No measured FPS or all-games/conformance conclusion. Windows ICD not regressed
by these Android results; its separate coverage remains unfinished.

## Package boundary / stop expanding

General UAB stays deferred while the known Sarek sampler heap works and Steam
runs native/tracking. Keep its missing protected semantics explicit; do not
announce support because native feature queries succeed. Record-time clones
need a coherent descriptor+metadata late-update design before enabling more.
P2 YCbCr, extra null shapes and speculative permutations do not delay images/
formats/shaders. Reopen only for a concrete consumer requirement, correctness
defect or an explicitly selected upstream-DXVK milestone.

The next work is cross-subsystem P0/P1 coverage, not descriptor completeness.
COMPATIBILITY_PRIORITIES_2026-10-21.md is the active ordering. No commit/push or
production U-AVF/Windows/Ubuntu changes.
