# Preserve fragment PrimitiveId through inserted cull GS

Active shared-runtime goal: PROGRESS, not completion or a blocker. Source
contract: https://docs.vulkan.org/refpages/latest/refpages/source/PrimitiveId.html
and https://docs.vulkan.org/spec/latest/chapters/geometry.html . When a GS is
present the FS reads its emitted PrimitiveId; a filtering stage must preserve
the original input ID, not renumber surviving primitives. Index resets per draw
instance. Do not infer GPU performance from this functional test.

## Implementation

Common compileCullDistance now reflects scalar32 PrimitiveId FS input and emits
GS builtin Input/Output variables. Loads input ID once, initializes output on
the entry path (including the no-emission branch), and stores it again before
every EmitVertex because output values do not persist across emissions. Both
input/output component accounting include the extra builtin for adapter limit
checks. Pipelines without FS PrimitiveId keep the previous generated program.
No private DXVK/game conditions. Full shaderCullDistance remains false.

Parser accepts a reflected standalone or block-member scalar32 integer. Actual
GPU proof below is the standard standalone signed gl_PrimitiveID input; block
member/unsigned forms are NOT separately GPU-proven. Existing GS/TES, fragment
CullDistance, PointSize, invariant user Blocks and previous extended pipeline
contracts still reject. This does not complete public feature admission.

## Actual GPU evidence

Original application VS+FS -> vkCreateGraphicsPipelines -> common inserted GS
-> stock Mali-G925, through existing APK25 loader. Independent reference has
no cull builtin and no GS. Three separated triangles, IDs0/1/2 colored R/G/B.
Reference independently checked for all three colors in their expected screen
regions (303/302/303 pixels), not just self-comparison of two wrong pipelines.

Eight cases: all visible; cull first/middle/last separately; cull all; cull on
the second plane; zero survives; restore all. Every pixel compared, with CPU
masking of the independent reference by original primitive ID. This detects
renumbering after earlier primitive rejection and verifies reset over repeated
draws. 98304 bytes/mode, four modes (descriptor-free/UBO x tracking/guarded),
393216 bytes PASS,13.571s. UBO/guarded mode traverses normal metadata pipeline
and3native-instrumented stages. Geometry-disabled negative rejects before the
native target pipeline; negative harness does not first create an invalid
geometry-capable FS reference on a device without geometryShader.
Strict verify-cull-pipeline.py --primitive-id PASS.

Previous base4 user Block scenario on SAME candidate: four modes/131072 bytes,
13.465s PASS, strict verifier/dependency rejection PASS. All26 prior GPU cases
flagOFF PASS71.234s; strict no-event-trace/no-diagnostic-read verifier PASS.
Host compiler10valid/5negative PASS. Plain standalone-interface GS still exactly
440758d4f2d26b0474e2d8bf6a2131b09374070ff670405072b5c230544ae280.
No CTS, upstream DXVK admission, full robustness/BC feature or visible-FPS claim.

## Artifacts and preserved baseline

Final layer E:/Mavk/artifacts/cull-primitive-id-final-layer-20261022/:
a4dc895986d0478b9ef22bcb57050e96d5599609c31cf380688de66eaee25723.
Final consumer E:/Mavk/artifacts/cull-primitive-id-final-consumer-20261022/:
a36620433d33c2ce099276bb1c225451c394245311bc88a57406eb143c2fdddb.
Earlier non-final build directories are not the verified candidates.
Evidence E:/Mavk/artifacts/cull-primitive-id-20261022/:
gpu-results.tar.gz 5b9ea7990d991e96e0a2308aa5acba7da81c62402e7667ebe5fb4912c75b984a;
block-regression.tar.gz b44d86de9916ab8671c936865c32bd10199b32e905573b90479eb97c14192c72;
regressions.tar.gz 1ddd55613baf7c44a8e31dc434208d3738c7e405bd5c3304ce10a313f0bcf1e3.

Installed APK25/runtime4de26844/source runtime version unchanged. No game run
on this candidate; prior candidate Muck evidence stays separate. No per-game
diagnostic override; Steam left running. Last observed battery14%, USB still
discharging, so no extra game loop. No U-AVF/Windows/Ubuntu/firmware/data changes,
root/SELinux modifications,commit or push. Artifacts on healthy E:, C53.9GiB
free/E835.7GiB. No live test/build handles left by this checkpoint.

Next: remaining consumer-relevant stage/admission semantics, not repeat this
closed primitive-ID or block-location matrix. Keep unsupported feature bits
false and experimental transformation isolated until mandatory scope is proven.
