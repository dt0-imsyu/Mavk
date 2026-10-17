# Next concrete compatibility consumer — native UI overlay

## 2026-10-17 encoded Bridge layer command native PASS

New bounded opcode24 serializes these actual UI draw packets and background
through shared Bridge decoding/rendering, using existing texture19..22 and
snapshot15/16 operations. Original font/shaders retained. Native in-process
request/response gate:30commands,19negatives,4608 checked pixels,258 changed
widget pixels,3GPU submits(font+2frames),5pipeline creates(compute+4graphics),
SyncVal0/0 and Corezero. Invalid packets assert no GPU memory allocation,
pipeline creation or submit. CPU tests:layers578,legacy scene630,shader12,
geometry275 rejection cases; legacy descriptor texture regression also PASS.
Evidence bridge-ui-layers1-20261017/core-ui-bridge.json SHA
a594262c969537991b5362ae5a8cd4f616019a85e35d857e32b645e92fce67ce;
verify-native-ui.py --bridge PASS. Packet layout/bounds in bridge/PROTOCOL.md.
This is NOT new FD/vsock/Windows evidence; generic stream transport unchanged.
No pipeline caching for24 yet, no performance/conformance/game claim.

Next actual consumer: guarded Windows ICD candidate must retain ordered
per-draw pipeline/descriptor/vertex/index/push/scissor snapshots. Existing
Command singleton pipeline+drew guard cannot represent the original overlay;
merely relaxing the guard would overwrite base-scene state and is wrong.
Extend only concrete state used by the pinned original app and reject
unrepresented depth/load/descriptor semantics explicitly. A substantial adapter
update warrants one Windows integration gate, not one VM per API function.

## 2026-10-17 preserved multi-pass color native PASS

`OffscreenSession` can retain a previous session and share its GPU color target.
The root pass CLEARs; followers LOAD the same color attachment, use their own
pipeline/depth state and record into one command batch. `renderLayers` accepts
per-layer push/UBO/default scissor, validates all inputs before recording and
submits once with one final readback. Maximum16 layers, serialized caller-owned
queue. Depth is cleared/local per pass, NOT preserved across layers. Following
passes do not create additional full-size color/readback allocations. Public
parent handle may be released because the child retains parent resources.

Actual gate uses the same original UI packets/shaders above a distinct
depth-enabled pipeline writing a blue background rectangle. The analytic oracle
verifies UI over blue and unchanged clear, and pixels outside the UI preserve
the first pass. Two frames:4608 verified pixels,258 changed widget pixels,
4 rejected API inputs,2passes/frame,3GPU submissions including one font upload,
4 pipelines,one shared target,SyncVal0/0,Corezero. Assertion checks actual live
image count =font+one color+one depth; parent handle is retired before rendering.
The absence of intermediate readbacks follows the shared recording path; this
is not a hardware bandwidth/profile or zero-copy presentation measurement.
Evidence `native-ui-layers1-20261017/core-ui-layers.json`, SHA
ffb1a024560423ecf02c081715b847f178ca9e633ac864602355697d45dde59e;
`verify-native-ui.py --layers` PASS. Relevant retained/single-layer Bridge
regressions PASS in the same new build. Not whole application/guest/Surface PASS.

Next practical gap: bounded Bridge/ICD command recording for multiple pipelines
and exact per-draw state. These native fields are not yet serialized or
advertised by the guest ICD. Avoid substituting renderer state or silently
discarding draw calls to get an application past initialization.

## 2026-10-17 native packet milestone PASS

Unmodified pinned ImGui1.65 library creates real font atlas512x64, text,
checkbox/button and draw lists. Original vertex/fragment shaders execute on
Mali via the shared renderer, not rewritten for Mavk. Two added analytic
probe lists exercise exact blend equation, clipping and nonzero base vertex.
Two frames:8 indexed draws,4 actual widget draws,6 nonzero-base-vertex draws,
4608 probe pixels verified,258 widget-region pixels changed,3GPU submissions,
2 pipelines,SyncVal0/0 and Corezero. `verify-native-ui.py` strict PASS.
Evidence: E:/Mavk/artifacts/native-ui1-20261017/core-ui.json, SHA
01bf53a5a3ae40e8c5baafc24805ef8caa8eee9c1d94c734016e74444137db51.
Relevant raster and Bridge descriptor regressions passed in the same build.

This is actual third-party library/shader **packet** compatibility through a
native adapter, not an unchanged whole upstream application, Windows ICD,
glyph-perfect oracle, Surface presentation, sustained FPS or 3D overlay PASS.
This earlier gate rendered on a cleared background. The later layered gate
above now proves color LOAD/preservation; guest integration remains incomplete.

Reproduction: prepare-ui-source.ps1 verifies pinned Git blob identities;
build.ps1 -UiSourceDirectory E:/Mavk/research/ui-4ee2501 links original ImGui
and shaders with their licenses; CoreComparisonActivity intent ui=true,
validation=true writes core-ui.json. For successive tests of the same top
activity, plain am start may only deliver an ignored intent; after confirming
the previous test/report is complete, use am start -S **only org.mavk.probe**
to create a fresh test process. Never force-stop a VM/production owner for this.

After upstream textured app PASS and Core native timeline/reuse/raster gates,
advance to UI rendering rather than another synthetic performance benchmark.

Local pinned upstream source already available:
`E:/Mavk/research/texture-4ee2501/base/VulkanUIOverlay.cpp`, from the same
SaschaWillems/Vulkan commit `4ee2501c8280cf71d1f9ce231cd8ebd47b58adde`.
Inspect and preserve original shaders/data/licensing before any execution.
No need for another clone or full Windows boot to identify this native slice.

Observed requirements and remaining adapter gaps, not generic ImGui support:

- Vertex stride `sizeof(ImDrawVert)`: pos float2, uv float2, color RGBA8 UNORM.
  Native graphics PASS; new24 extended geometry accepts RGBA8. Legacy wire
  commands remain float-only, Windows ICD advertising still unchanged.
- Fragment combined sampler at set0/binding0: native and new24 explicit binding
  PASS; existing23 still binds1. Windows adapter not yet updated.
- Vertex push transform; fragment multiplies texture/font and vertex color
  (verify exact original shader before allowing it through Bridge filters).
- Native viewport/scissor: full-target viewport and per-draw scissor PASS;
  arbitrary guest dynamic viewport state still needs adapter implementation.
- Indexed batches with nonzero vertexOffset; existing bounded scene format
  does not represent all these draw semantics. Native and new24 base vertex PASS.
- Actual pinned blend: color SRC_ALPHA/ONE_MINUS_SRC_ALPHA, ADD;
  alpha ONE_MINUS_SRC_ALPHA/ZERO, ADD. This is NOT the newly tested standard
  straight-alpha source-over alpha equation. Exact native UI equation now PASS;
  explicit blend2 in new24 also PASS; never silently substitute in guest adapter.
- Multiple pipelines/rendering passes preserve previous scene content: native
  shared target/color LOAD now PASS, guest command stream still incomplete.
  Do not fake an overlaid scene by discarding the background.

Smallest useful next work: native reusable graphics state/resources needed by
these exact commands, test correct color/alpha/clipping before Bridge/ICD API
advertising. Do not claim that the alpha/scissor gate alone runs upstream UI.
Keep previous immutable app and production VM/display baseline untouched.
Windows regression follows meaningful adapter changes, not every Core method.
