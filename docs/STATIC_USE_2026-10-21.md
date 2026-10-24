# Selected entry points and unused descriptor sets — 2026-10-21

## Contract and bounded coverage

Static use follows the selected SPIR-V entry point's call tree and ID operands,
not every descriptor declaration or entry-point interface variable.
Source: https://docs.vulkan.org/spec/latest/chapters/shaders.html#shaders-staticuse

The shared shader frontend validates the bounded original module, resolves
entry point by name AND execution model, traverses called functions, distinguishes
ID operands from literals, and resolves direct/grouped descriptor decorations.
No specialization-dependent dead-code claim. Unreachable functions do not add use.

| Combination | Classification / outcome |
|---|---|
| Entire unused nonempty set, no allocation/binding/write | Native semantics + verified protected tracking |
| Unused dynamic-buffer set between used sets0/2 | Native/protected GPU512words match; omitted offsets do not shift used sets |
| Three unused nonempty layouts, descriptor-free draw | Native/protected GPU1024components match,72red/184black pixels |
| Allocated but unwritten/unbound unused set | Accepted in the descriptor-free GPU test; not converted into null resources |
| Missing statically used set2 | Guarded rejection before submit; expected single compatibility gap, submissions0 |
| Unused bindings inside a USED set | Still missing: current snapshots require all bindings/elements of that set |
| Partially bound arrays / general update-after-bind | Not admitted; require separate initialization/lifetime/submit-time semantics |
| Unsupported stages, malformed modules, absent/wrong-stage entry | Reject; no fallback declaration of support |

Pipeline preparation records required set indices once, not at every draw.
Unused whole sets are omitted from snapshots and native allocation; original
layout indices remain unchanged. Native binding spans skip holes and retain
dynamic-offset order and the private guard set. No framebuffer copies, queue
waits or game-name conditions added. Performance benefit is NOT measured.

## Evidence

- Shared host analyzer:14checks/7rejections; includes multiple same-name entries
  in different stages, helper call, unreachable function, group decorations,
  literal/ID collision, SPIR-V1.6 interfaces and descriptor-free selected entry.
- ARM64 glibc analyzer inside DroidDeck:8checks/7rejections (earlier fixture;
  the later SPIR-V1.6/interface additions ran on host, not recompiled on ARM64).
- Existing guard compiler/cache regression21assertions/9rejections PASS.
- Nine existing host descriptor suites rerun PASS; shared descriptor headers
  are unchanged by this package, so these reused their previous build.
- Real loader native/protected unused-set oracle PASS. Regressions: ordinary
  dynamic512words, empty-layout512words/1024components, immutable192components,
  core/KHR routing512words/8192components PASS.
- Known native G925 immutable-copy108vs18 mismatch reproduces; matching native
  sampler control passes, protected copy still preserves the correct sampler.
  Native failure is not relabelled PASS.
- Fresh existing Android Core suite5gates/13rows PASS. Null2D/array/volume/cube,
  generation/templates,BC123/45/67/texture regressions PASS with their existing
  bounded validation gates. These are Core/harness regressions, NOT validation
  of the glibc guarded layer itself.
- Muck15053 mapped the candidate;220054protected executions at capture,
  no Mavk gaps/device-lost in the captured log. No visible-FPS/gameplay claim.
  Test game stopped; Muck-only candidate override removed afterwards.

No CTS or full nullDescriptor/robustness2/indexing declaration. No Windows ICD
coverage inherited, no production U-AVF/Windows/Ubuntu modifications.

## Reproducible artifacts and rollback

- Frontend archive `E:/Mavk/artifacts/static-use-frontend-20261021/`
  SHA09554659ff4ea9bc6da01de69be1b1416d3e47f409376556c225be0819a69117.
  Uses current frontend sources and pinned existing SPIRV-Tools static libraries.
- Layer `E:/Mavk/artifacts/mavk-unused-candidate-20261021/`
  SHAf9b2960e25c8c12b936b480eea3ab60b7f542ddb34478e115f25eb1b0de5d530.
- Final consumer `E:/Mavk/artifacts/layer-unused-final-consumer-20261021/`
  SHAae89643ca32fa5bb0686188cbf786ee014bcaacce85730257eb1e6dbe0324325.
- Evidence `E:/Mavk/artifacts/unused-descriptor-gpu-20261021/`.
  `results.tar.gz`: initial positive/regression runs, including EXPECTED native
  immutable failure. `final-results.tar.gz`: final-consumer positives, expected
  missing-used-set negative, game mapping/log. Strict positive verifier uses
  `final/` with `--unused`; never apply the gaps0 gate to the negative test.
- Runtime d1ad4522b4e40e3b107c2076541e607ecfee55e9e1172a4531f449a4f6a18527;
 261other archive members unchanged. APK16 installed in place, no data clear.
- APK `E:/Mavk/artifacts/droiddeck-unused-apk16-20261021/DroidDeck-Mavk-unused-20261021.apk`
 SHA5097806c4b470dcb020b90e64fb963ec2e90fdd4e4850a03c9fc186b1117b0bb.
 Asset-only signed rollup, not a full Gradle rebuild;485other APK members
 unchanged. Existing dev signing certificate verified. Source fork runtime.zip
 and version.txt match the packaged runtime. Rollup unit tests8/8 PASS.
- Session2026-10-21-05-steam: installed COMMON library hash/mapping verified;
 unused-set oracle512words/1024components/gaps0/Corezero PASS. Muck18364 maps
 COMMONd1ad runtime,113581protected executions at capture,gaps0/device-lost0.
 No per-game layer override. Steam remains hardware ANGLE/Zink/Mali-G925.
 `common16-results.tar.gz` retains this independent installed-common evidence.
 Muck/probe stopped, Steam data retained, original suspend preference removed
 back to absent/default manual. No visible-FPS/performance claim.
- APK15/common24b402d3 remains the known-good in-place rollback. No uninstall or
  data clearing required; no commit/push made.

Next: binding-level static-use snapshots, separately accounting full native
layout pool budgets and dynamic offsets. Then D4 submit-time lifetime model,
before admitting general indexing/UAB. Do not globally accept uninitialized data.
