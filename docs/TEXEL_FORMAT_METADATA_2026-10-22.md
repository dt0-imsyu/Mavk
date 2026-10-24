# Runtime texel-format metadata — APK23 verified

## Final result

Normal layer9c00e7d440269fd3484a0811b9b62c50857ddcda73824ec0ac05b8cb733593cd
uses frontend9d34b1547a2f51a24b56e8cc4eb7cb613f61931ae4b59cc0055e73b95d66924f
and optimizer1f1f274af886d448621d70696dbf0d42bc756d1a62efe10ba129cd77d9858401.
The diagnostic count-output probe is NOT included. Both normal and reverse
format order PASS; all26 isolated GPU cases and6aggregate forms PASS.

APK23 installed with existing data intact at09:34:44,2026-10-22. Runtime
62fcf6348a0c179854dc3f03e065343ef929f962dd40c0b294c3cb98b1868207.
All26 cases passed through this actual COMMON runtime in28.708sec; strict
verifier --texel-view-components --common --runtime-sha256 PASS. This elapsed
time is suite duration, not rendering performance. New view test verifies
six formats/18phases/1872words using two shader pipelines without recompiling
per view. Existing known-format/null/atomic and previous regressions remain PASS.

Normal Muck11336 maps the62fcf634 runtime, no individual candidate override.
Actual menu and Settings/input visually verified; game quit normally through
Steam. Saved log contains245188 guard-log records; no observed Mavk gap,
shader compile-failure or device-loss markers. This is a menu smoke test,
not full game completion or measured FPS. Steam remains running.

APK SHA83561ddb443edc2cfa3275da87b44e61272973a361e0c960746f41249db0b475.
Evidence:E:/Mavk/artifacts/droiddeck-format-apk23-20261022/.
common-results.tar.gz SHA3612e0ec8fbea827d9ac0a260c22c9068efbfab81acf8a25e59b7ee407301fe6;
muck-format-common.tar.gz SHA7e3e529edf3dc147e41f68b49b9d7d04a6dbf0f46644883e65f8516c6a16277d.
Source fork runtime/version and default matching archive pins synchronized.
APK22 retained for rollback. No CTS, broad feature claims, commit/push or U-AVF
changes. Earlier observations below are historical, not current rollout status.

## Shared contract

`IndexInitFormats` is a new explicit ABI,not reinterpretation of IndexAndInit.
For S=highest selected set+1,word0=S+1;length roots are at1+set,initialization/
range roots atS+1+set,format roots at2*S+1+set. Each format root addresses a
binding table;each texel binding addresses per-element component counts1..4.
Null entries contain0 and are excluded by the outer initialization guard.
Range entries retain bytes for buffers,elements for texel views. Legacy ABIs
and default GuardCompiler callers keep their previous contract.

Native resource registration derives component counts from actual admitted
VkBufferView formats. Copies,templates,selected-set snapshots and old-generation
retention carry them. Bound unknown-count metadata explicitly rejects. New
cache identity includes component count and ABI;95CPUchecks/41negatives PASS,
including scalar->RGBA->scalar rebind and immutable old snapshots. Existing
Vulkan descriptor update gate20checks/20negatives PASS.

Same private metadata buffer and two existing guard bindings;no additional GPU
buffer/submit/wait or per-format pipeline compilation is introduced. Metadata
size grows for the header and texel binding tables. Actual performance unmeasured.

`TexelComponents` runs after instrumentation only when texelFormatMetadata=true.
It recognizes Buffer/Unknown image reads/fetches,requires a proven inner OOB
Phi/size-compare branch,recovers the actual descriptor set/binding/index,and
reads component count there. Outer null result remains allzero. Ambiguous
pointer/merge/width forms fail closed. New layer explicitly pairs this compiler
mode with IndexInitFormats;old standalone tools/harnesses remain legacy unless
--texel-formats is specified. Do NOT pair new layer source with old default
atomic frontend8b1d0db1:explicit matching frontend required during this WIP.

## Historical initial failure and localization

New test reuses layer-execution/Core and two shader pipelines across six actual
native view formats:R32/RG32/RGBA32 UINT and SFLOAT;valid/OOB/negative/null/
restored data plus sentinels. Target18dispatches/1872words;not yet all completed.
Old installed layer FAIL reproduced on format98,phase0,word15,got0 expected1.
Initial candidatea5a36255 passes first R32/RG32 UINT phases but fails RGBA32 UINT
word15,got1 expected0. Bounded actual layer metadata trace shows correct count4.
Reversed format order still fails RGBA32 on the FIRST dispatch,so not explained
by prior pipeline/view rebinding alone. Host/ARM compiler outputs differ only
in two entry-point interface IDs' order;logic agrees and both validate.

Isolated compile-time MAVK_TEST_TEXEL_COMPONENT_PROBE emits actual loaded count
as OOB alpha instead of the required substitution. With RGBA32 first,it returns
4 while host expects0:diagnostic evidence that the GPU receives count4,NOT a
compatibility PASS. Probe frontend839434c3 must NEVER be packaged as runtime.
No conclusion that Mali generally miscompiles comparisons is established.

Next candidate uses alpha=1-(components>>2),equivalent to components<4 for the
validated count range1..4,then converts to original scalar type. This avoids
comparison/select without changing the required result. Normal frontend9d34b154
built;layer build/actual same GPU gate pending. Known-format optimizer1f1f274a
is shared with the previous isolated fix. Do not declare success until GPU PASS.

Artifacts rooted E:/Mavk/artifacts/texel-format-*20261022/.
Source tests:texel-descriptors.cpp,texel-view-components.comp,
layer-texel-components.hpp;controltexel-format-control.sh. Bounded trace flag
MAVK_TEST_TEXEL_METADATA is off by default. No CTS/feature advertisement changes.
