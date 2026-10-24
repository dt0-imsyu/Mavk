# Windows PE consumers through ordinary DroidDeck AHB — 2026-10-24

## Verified

The installed experimental APKc56d55ff/runtime9e2a00b1 now has actual Wine
integration evidence, beyond native SDL/D3D11. Both existing Windows ARM64 and
AMD64 PE versions of the same D3D11 consumer render visibly through the bundled
AHB path, in the real ordinary SessionActivity/run-mode session. Source1920x1200,
compositor2560x1600; existing stock Mali-G925 and shared Mavk compatibility.

No Steam or Proton launch was executed for these consumer checks. Used the
already-created isolated /root/.mavk-wine-poc prefix and installed ARM64 Wine
binary from the existing Proton distribution, not a game compatdata prefix.
No new Wine prefix or VM clone. New test DLL links/executable copies live only
in files/mavk-selection-normal/wine-ARCH. The original benchmark files and game
DLLs/profiles were not replaced.

The first Wine invocation inherited the native SDL fixture's
DXVK_WSI_DRIVER=SDL3 and rejected WSI before device creation. This was a test
environment error: removed SDL-specific variables for Windows/Wine; default
Windows WSI then worked. No Vulkan capability or driver changed to obtain PASS.

## Actual receipts

| Consumer | Device | Present API counters | SurfaceFlinger game-layer buffers | Shutdown |
|---|---|---|---|---|
| ARM64 Win32 | S_OK,11_0 |2142/18.004s;1789/15.001s measured |2137,drops3 |exit0,CoreZero,gaps0 |
| AMD64 Win32 | S_OK,11_0 |2150/18.003s;1796/15.002s measured |2147,drops1 |exit0,CoreZero,gaps0 |

Viewed wine-ahb-live.png and wine-x64-ahb-live.png: expected textured moving
scissor stripe/clear background and Android controls. API logs identify actual
processMachine=ARM64 and AMD64 respectively; WSI requests/imports actual AHB
images. Approximately119 API calls/s in this small vsynced workload is not game
FPS, a native-power ratio, an end-to-end latency measurement or pixel-unique
presentation accounting. No FEX module-map trace captured in this run; do not
infer a new FEX implementation result from the AMD64 label alone.

Sarek's existing diagnostic nullDescriptor/FloatControls2 admissions remain
diagnostic; public feature bits unchanged. No claim of upstream DXVK readiness.

## Standard DXGI resize integration

Extended the existing shared native/Win32 consumer with off-by-default
MAVK_BENCH_RESIZE=1: at5s and10s it unbinds/releases its render target/backbuffer,
flushes context, calls standard ResizeBuffers, reacquires the RTV and sets the
new viewport. No Mavk-specific GPU commands. Ordinary default behavior remains
the prior fixed-size scene.

Compiled the two PE architectures on C: using the existing llvm-mingw inputs:
ARM64 `846dd140ec157b26c380b8620e76755f6e2e7361586e63b8a2233ebb06be6985`;
AMD64 `8fed671c3d26c9d393e302a4a4e24cedd4a0660d26d2bcfa20aa57f313220353`.
Only the new private resize fixture uses these binaries.

AMD64/Wine actual result:

- WINDOW_RESIZE_PASS width=1280 height=800, then width=1920 height=1200;
- actual new AHB imports after recreation, S_OK/11_0;
-2132calls/18.005s,1778/15.002s measured; no throughput claim from the resize run;
- remaining tracked resources0, Core liveZero1,gaps0,exit0;
- APP_AHB_SELECTION_PASS / NORMAL_SESSION_D3D11_PASS.

The first resize fixture wrapper tried to exec its non-executable copied shell
script and exited126 before starting Wine. Session log established exact cause;
the instrumentation's90s deadline expired and restored preferences. Fixed the
fixture to exec /bin/bash explicitly. This was not a driver/resize failure.

## Reproduction and preserved artifacts

The existing AhbSelectionCheck accepts `normal=1` and an explicit validated
`arch=aarch64|x86_64`, passing it as a normal SessionActivity program argument.
The private shell fixture calls ahb-normal-wine.py; ahb-normal-resize.sh is the
optional resize wrapper. It calls the existing game's environment planner but
executes Wine directly without Steam/Proton. It stops only the isolated prefix's
Wine server afterward and restores Mavk preference keys through instrumentation.

C: recovery/2026-10-24/native-fullscreen-scale/wine-{arm64,x64}-artifacts.tar and
live PNGs retain actual Android results; recovery/2026-10-24/wine-resize/ contains
PE binaries and session-artifacts.tar including both resize receipts. Source
helpers/consumer remain in experiments/droiddeck. No main APK changes in this
consumer stage, no commit/push, production U-AVF/VMs untouched.

## New authorization / next step

User subsequently explicitly allowed Steam/game launches (previous Rust-session
restriction lifted). Completed resize check first; enabled the actual AHB app
switch and invoked the normal MainActivity LAUNCH_GAME action for installed
Muck1625450. Ordinary Steam session2026-10-24-21-steam is now starting. This is
not yet a game PASS; inspect live state/logs, do not restart just for slow login.
No old per-game XCB/AHB override remains in the two checked game-environment files;
the old private profile backup is already marked restored.
