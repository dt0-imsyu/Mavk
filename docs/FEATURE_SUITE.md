# Mavk Feature Suite

Native Android entry: `org.mavk.probe/.FeatureSuiteActivity` in the existing
isolated harness APK. No VM, root, system driver or new transport required.

The activity reuses existing Core comparison and sampled mip JNI workloads,
plus small native timeline, retained-renderer and raster correctness gates;
it does not implement another renderer or duplicate native tests. Open it to
read cached evidence. Press **Run Core checks** (or explicit intent `run=true`)
only when native Core changes warrant a new gate. Results are saved in the
app-private `feature-suite.json` with underlying reports and SHA-256 identities.
Fresh mode removes its previous derived reports before running, so a crash
cannot leave an old suite PASS masquerading as the new result.

Each row separates implementation and verification:

- **NATIVE**: Mavk uses the actual Android Vulkan implementation. PASS requires
  the specific existing workload's correctness, SyncVal0/0 and teardown gates.
- **EMULATED**: implemented compatibility fallback; currently the Windows WSI
  readback/blit adapter. Its gate stays separate, not falsely passed by Android.
- **UNSUPPORTED**: no supported implementation/acceptance gate in that Mavk
  layer. This does **not** mean the physical Mali driver lacks the capability.

Missing evidence is NOT_RUN, failing evidence FAIL. A bounded slice PASS is not
Vulkan CTS, arbitrary shader/format compatibility, game performance or WDDM/DWM
acceleration. Cached evidence is explicitly distinguished from a fresh run.

Timeline slice (2026-10-17): native Vulkan1.2 capability queried and enabled
by the device owner before constructing `DeviceContext(...,true)`. Core wrapper
owns the semaphore and resolves native counter/wait/signal functions; it does
not implement CPU fence polling. Four GPU submits in two fill→dependent-copy
rounds verify8192 words, with one host wait per complete pair, timeout handling,
monotonic host signal and deterministic teardown. Queue ordering/lifetime still
belongs to the caller; this is not a new asynchronous Bridge implementation.
This feature does not change Windows ICD advertising or claim Vulkan1.2 driver
conformance. Windows/Ubuntu/P7/display remain unchanged.

Retained renderer slice (2026-10-17): the shared `OffscreenSession` owns reusable
pipeline/target/descriptors/command resources.32 verified32x32 frames across
one-shot and retained paths,4 distinct mip outputs,2 negative inputs,SyncVal0/0
and Corezero. Pipeline creates16→1; no per-frame Core GPU allocations.
Preliminary sequential median5.65→0.34ms includes validation and is NOT a
desktop/guest/visible-FPS measurement. Readback remains synchronous. Evidence
is `core-reuse.json`. The shared Bridge now adopts this renderer for commands
17/23, with a separate relevant Android descriptor gate (not a Core suite row):
four textured UBO draws,4096 verified pixels,one cache hit sequence and one
immutable-state rebuild,16 invalid requests,SyncVal0/0 and clean teardown.
See `bridge-retained1-20261017/bridge-probe.json` and `verify-bridge-reuse.py`.
The production owner APK/Windows adapter has not been updated by this gate.

Raster slice (2026-10-17): opt-in native RGBA8 straight-alpha source-over
(SRC_ALPHA / ONE_MINUS_SRC_ALPHA color, ONE / ONE_MINUS_SRC_ALPHA alpha),
replacement control and dynamic target-bounded nonempty scissor. Alpha0/128/255
and full→partial→full-reset clipping verify18432 pixels over18 submissions and
six retained pipelines;30 invalid rectangles rejected before recording. Zero
per-frame Core GPU allocations, SyncVal0/0 and Corezero. Format blend capability
is checked before pipeline creation. `core-raster.json` gates the two bounded
NATIVE rows. Not arbitrary blending, premultiplied-alpha mode, UI-library,
Bridge/ICD advertising, Windows or CTS support. Existing Bridge default path
also passed its relevant descriptor regression with dynamic scissor enabled.

Optional original UI packet/layer gate is separate from the13-row aggregation:
build with pinned -UiSourceDirectory, run CoreComparisonActivity layers=true
and validation=true; core-ui-layers.json + verify-native-ui.py --layers.
It verifies native shared color LOAD across depth-scene/UI passes, exact probe
pixels and lifetime; not whole upstream app, guest API or Surface throughput.
See UI_NEXT_SLICE.md. No VM is required for this native layer gate.

Encoded Bridge slice reuses this same actual ImGui/font/shader fixture:
CoreComparisonActivity bridgeLayers=true,validation=true; core-ui-bridge.json,
verify-native-ui.py --bridge. Opcode24 performs bounded CPU admission and
ordered layered GPU rendering. Native in-process packet correctness is not a
new FD/vsock or Windows ICD PASS. Do not fold this into native-only Core rows
or claim full descriptor/command-stream support from it.

Development order: Core feature + relevant native gate first; Bridge/ICD gates
when those adapters change or after substantial common updates. Keep Windows
ICD active. WDDM remains a separate adapter/R&D project; DroidDeck and Ubuntu
consume the common Core rather than independently duplicating it.
