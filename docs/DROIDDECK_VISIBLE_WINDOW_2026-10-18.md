# Actual DroidDeck app UID: visible native D3D11 window

## Result

Installed isolated debug package `com.droiddeck.launcher.mavk`. Real native
ARM64 DXVK-Sarek -> Mavk guarded layer -> pinned hybris BC layer -> stock Mali
Vulkan -> actual DroidDeck Wayland compositor -> Android Surface.

Run3: D3D11CreateDeviceAndSwapChain S_OK / FL11_0; 640x480 RGBA8, FIFO, three
swapchain images. Moving scissored bilinearly sampled texture is visible on the
tablet. 1198 Present returns in 20.009s (59.874 calls/s), process0, all owned
Core/descriptor/shader/pipeline/command registries empty. This is NOT a measured
unique visible FPS claim. Run4 repeated successfully; two screenshots show the
rectangle at different positions. Native compositor first-frame callback fired.

Evidence: `E:/Mavk/artifacts/droiddeck-window-runs-20261018/`.
run3-frame.png, run4-a.png, run4-b.png; run3 stdout/stderr, run4.full.log.
Three logged gaps are swapchain-owned render-target image views passed to the
native driver, not silently claimed as tracked Mavk resources. No validation
layer or full Vulkan/nullDescriptor conformance claimed.
Run4:1199 Present returns/20.007s (59.930 calls/s), process0. First-frame marker
`743582794635084` from actual native compositor. Default SurfaceFlinger latency
query returns only display period8333333ns: NOT per-layer visible-FPS evidence.

## Observed blocker and minimal fix

Run1 failed at guarded swapchain draw. Run2 diagnostic localized the exact
descriptor: set1/binding1, the real Sarek present shader's texture1D gamma LUT.
Existing typed fallback catalog admitted only color2D. Added bounded original
shader reflection for color1D, dimension-specific native image/view selection,
and cache keyed by (dimension,format). Initialization remains same-queue,
ordered GPU prelude, no hidden CPU wait. Color1D/2D type conflicts reject work.
The actual window uses default gamma specialization disabled: this proves
binding/materialization/presentation, not independent null-1D sampling semantics.
Full public nullDescriptor remains FALSE. No generic fallback dimension guessing.

Run38 latest-layer regression: actual clear, solid draw, Texture2D.Load, nearest
and bilinear readbacks each16384 components PASS, gaps0, cleanup/Corezero.
Run39 normal guarded buffers/images/indirect/resubmit65536/4 PASS; diagnostic
flags OFF, real Sarek still rejects nullDescriptor (expected exit20).
Existing reflection test extended with a real sampler1D GLSL declaration:
six typed texel formats, independent1D/2D maps and four malformed-input cases
PASS, actual ARM64 binary under the DroidDeck app UID. Evidence
`E:/Mavk/artifacts/image-1d-reflection2-20261018/result.txt`.

Layer26 SHA256:
`eb0faee4c6a983d7bd3b5600d5545b9daa861af2e394b9f5e6a8c5359520a29d`.

## Reproducible isolated fork/build

Fork checkout `E:/Mavk/work/DroidDeck-mavk`, exact upstream
`wallentx/DroidDeck@0916a538aff4c08f799a4bd9fa17dd3c98b01e99`.
Separate LF clone; original research checkout not normalized/overwritten.
Read root AGENTS (rare factual comments). Existing pinned runtime verification
was retained: record, verify, final APK inspection all PASS.
CI inputs staged using experiments/droiddeck/stage-ci-runtime.ps1, upstream
run38693863397/artifact11953941964; origin+per-file hashes in
`E:/Mavk/artifacts/droiddeck-ci-inputs-20261018/ci-runtime-origin.json`.

Debug-only MavkWindowActivity calls actual SystemVulkanDriver.prepare and
CompositorHost.startOrAttach, no substitute Android renderer. Optional local
Gradle SDK36/NDK28.2/WSL verification options; skipRust=true; wrapper8.10.2-bin.
Build result `app/build/outputs/apk/debug/app-debug.apk`,126181411 bytes, SHA256
`0fc46d2ff0a870dbc7383481ed199c4e1b757eeca721236e1c76dcaede3761dd`.
Samsung requested authentication for install, user confirmed on-device.
No global Play Protect/Secure Boot/SELinux/security-policy changes.

Runtime stage archives (new E only, small subset not full rootfs):
- droiddeck-window-root1-20261018/window-root.tar (~11MB).
- droiddeck-window-payload1-20261018/window-payload.tar (~28MB; initial layer24).
- then explicit local hybris manifest and layer26 replacement/checksum generation.

Controller/source: experiments/droiddeck/{sarek-d3d11-window.cpp,
build-sarek-window.sh,window-control.sh,window-stage-layer.sh,
VkLayer_hybris_compat.json}. Own app-private files/mavk-window only.
The local manifest is necessary: otherwise the upstream hybris library is
selected instead of the pinned BC implementation and Sarek rejects BC format141.

## Scope / next

This is a real native D3D11 consumer using actual DroidDeck compositor/runtime
under its own app UID. It is NOT yet a Windows PE executable under Wine,
game compatibility or a ready end-user graphics profile. Both diagnostic
Sarek admission switches remain explicit and normal native feature queries
unchanged. Keep this backend opt-in; stock Adreno DXVK unchanged.

Next: reproducibly package opt-in profile and matching ARM64 PE Sarek DLLs,
run a real Wine D3D11 window. Fix only observed consumer API blockers. Do not
repeat native draw suite or start Windows VM for each shared change.

U-AVF Windows/Ubuntu/P7/H264/storage untouched. No root/commit/push.
