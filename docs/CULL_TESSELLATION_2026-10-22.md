# CullDistance after tessellation — common TES->GS lowering

PROGRESS toward the active reusable Vulkan-runtime goal. Not full feature
admission, completion or a blocker. No public feature bits changed.

## Implementation

compileCullDistance has an explicit CullProducer (Vertex default or
TessellationEvaluation). The legacy result.vertex field holds the patched
producer. TES source is validated in its real execution model; ordinary
triangles/quads declared in TES are required. TES PointMode/Isolines and
inconsistent input primitive classes reject. Existing VS callers remain valid.
Unsigned PrimitiveId type reuse now also uses the existing canonical uint type
when a source producer has no uint declaration; no unsigned GPU coverage claim.

The experimental Vulkan adapter finds the last producer, keeps original VS/TCS,
transforms TES outputs, and appends GS: original4stages -> native5stages. Checks
caller-enabled geometry+tessellation, patch pipeline, geometry and TES output
limits. Existing upstream VS/TCS CullDistance interfaces reject rather than
silently passing unsupported builtins downstream. Control-stage domain
declarations, existing GS and previous extended contracts remain unadmitted.

Shared native renderer gained optional TCS/TES pair and patch state, plus UBO
stage visibility. Defaults remain VS/FS triangle list. Caller owns feature
enabling; this adds no alternate renderer/transport and changes no installed
runtime. Root test enables native tessellationShader explicitly, CullDistance0.

## Independent GPU proof

Native TCS uses equal tessellation levels2. TES computes positions and two cull
planes from barycentric coordinates; reference TES omits CullDistance and GS.
Actual selected topology has six fan triangles around the center, not four.
The initial unrun test harness's quarter-region assumption was corrected from
the specification BEFORE executing it; no product fix arose from that mistake.

https://docs.vulkan.org/spec/latest/chapters/tessellation.html
https://docs.vulkan.org/spec/latest/chapters/geometry.html

Six cases: full image; full-plane0 rejection; full-plane1 rejection; zeros
survive; partial rejection retaining two fan triangles; full restore. CPU
reference uses known center/edge coordinates and pixel centers, not the output
of the transformed shader. Reference288red pixels, surviving fan96. This tests
whole generated-primitive rejection rather than fragment clipping.

Actual G925 loader hook: four modes descriptor-free/UBO x tracking/guarded,
24576bytes/mode,98304total PASS13.366s. UBO lives in TES and traverses normal
metadata/guard path; native5stages/publicSets1,7draws/7submissions. Missing
geometry dependency rejects explicitly; strict verifier --tessellation PASS.
Host suite11valid/8negative PASS includes wrong producer/stage/topology rejects.
Triangle TES only GPU-proven; quad admission is not a separate GPU proof.

## Regressions and lifecycle

After the positive test, previous Steam session2026-10-22-05-steam exited
normally with status0 (failure=null,IDLE). Follow-up tests returned
NO_ACTIVE_SESSION, not GPU FAIL. Authoritative state checked, battery100%.
Started the existing runtime normally as2026-10-22-06-steam,READY,guestPid998.
No test/build was restarted merely because an observation timed out.

On same layer89c8c463 + new shared-renderer consumer8efd7116:
previous PrimitiveId4modes/393216bytes PASS5.700s, strict verifier PASS.
All26GPU regressions PASS32.398s, strict no-event/no-diagnostic-read gates.
Primary aggregate consumer was temporarily replaced with the newly compiled
consumer, so ordinary shared-renderer cases used the new renderer too; original
test binary restored afterward. Auxiliary specialized binaries unchanged.
These durations are not performance deltas or visible FPS.

Muck PID4118 actually mapped candidate89c8c463 from mavk-aggregate. Menu and
Settings visually PASS, tap input changed screens; no observed GAP/CULL_LINK/
shader-fail/device-loss markers, experimental cull flag absent. This is known
integration regression only, NOT evidence Muck uses tessellation/cull lowering.
Quit normally; per-game override removed, common Steam session left running.

## Artifacts

Layer E:/Mavk/artifacts/cull-tessellation-layer-20261022/:
89c8c463141ee1e5a820b7283ef9caf9f686d4dcbc6e13be7eb6f2c8ff2aad91.
Consumer E:/Mavk/artifacts/cull-tessellation-final-consumer-20261022/:
8efd7116d0778d9db2553b9641e37231bd5066af0ccb1cb59c8416e2248cd3a9.
Earlier cull-tessellation-consumer build is NOT the verified harness.
Evidence E:/Mavk/artifacts/cull-tessellation-20261022/:
gpu-results.tar.gz 41add2a900c12f1a72f174b0742ed18734ea1033845a8bddeb76468923c1f319;
primitive-id-regression.tar.gz 36a40826bee95d2717686132e8113d8d312e756f9a50f0ed55fd189ddd820b4b;
regressions.tar.gz f4f551f056690e8cf519f045c5aec5dc3a9aea0f9b0a50bef639792acc757b07;
muck-logs.tar.gz 389b805481436808dfb153c7a8a480d6b0c11cf48a300a17eaed1d2b7e8db451.
Screenshots muck-menu.png / muck-settings.png inspected.

Installed APK25/runtime4de26844/source runtime version untouched. No APK/asset
rollup, archive pin, U-AVF, Windows/Ubuntu/data/firmware changes. No root/unlock/
SELinux modification,commit/push. No live test/build handles at checkpoint.
C51GiB/E835.6GiB free; keep further large artifacts on healthy E:.

## Remaining

Already-active GS needs a different lowering than inserting a second GS; do
not advertise complete CullDistance based on VS/TES-only success. PointSize,
fragment CullDistance, invariant Blocks and other explicit contract gaps remain.
Keep public feature false. Do not repeat this closed six-case fan matrix; select
next consumer-relevant stage/admission work. No CTS/upstream DXVK PASS claim.
