# AHB in the ordinary DroidDeck session — 2026-10-24

## Practical result

The ordinary SessionActivity/SessionService/run-mode path now shows the existing
native D3D11/Sarek consumer through the bundled X11-to-AHB backend, including
1920x1200 buffers on a 2560x1600 compositor output. Basic mouse-button and key
events reach the consumer. No Steam, game, Proton, Windows VM or Ubuntu launch
was needed, so the user's competing Rust/Steam session was not interrupted.

This is NOT a Windows-game, Wine-input-coordinate, DXVK conformance, upstream
DXVK compatibility or full Vulkan feature-support claim.

Installed APK SHA-256:
`c56d55ff35c705c5c34e84d9b6777640fd9a7d6758ed03dc5cba326e63b66357`.
Native compositor:
`92bff72c4cf39701d4467bf3dda4f5a67d0b2eec1f6656ac0d01c65a9f330fb7`.
Runtime:
`9e2a00b1e303e736274c831dd233ceeb7539f63edd3d1627c0a0897af27ebd95`.
WSI unchanged: `99a12d70e88e14babcf27ad8dcb22804334a9e769bce2dcc89c7c5646096e663`.

## Two integration defects found and fixed

1. The relocated game launcher loads droiddeck-esync and droiddeck-wincomponents
   relative to its own directory. The first component package omitted both;
   the actual normal launch logged missing synchronization helper and a stock
   fallback. Packaging now requires and verifies both helper hashes, includes
   their original bytes/executable permissions, and rejects incomplete input
   before writing output. No change to their synchronization/component logic.

   Helper hashes:
   droiddeck-esync `2ee2f8b70b91d870ecb03bb1ef07bf27893d0e900e28a958c3e32ec80c285f41`;
   droiddeck-wincomponents `173ad8a194d9231f6047607ec163bc125e8405b8064c2469274e45e212c61d4d`.
   Runtime packaging still preserves all262 original members and permissions;
   only manifest changes, seven optional members are added.

2. Real normal-session rendering with AHB imports and successful Present calls
   was visibly BLACK: source1920x1200, output2560x1600. ahb_layer_only_candidate
   rejected every buffer whose size differed from the output. A/B without any
   compositor change, rendering at2560x1600 instead, immediately showed the
   animated textured/scissored scene and the SurfaceControl game layer.

   Minimal fix: retain the old exact-size case; additionally permit the existing
   topmost, childless, identity-viewport AHB-only surface when its fullscreen
   role is set and source/output aspect ratios match. Positive dimensions and
   64-bit aspect-product checks; non-fullscreen and crop/viewport restrictions
   remain. The existing sc_layer_present_ahb -> vkp_map_rect already maps the
   complete source buffer to the output destination; no extra blit/readback or
   synchronization mechanism was added. Different-aspect scaling remains outside
   this additional eligibility case.

   Canonical patch: experiments/droiddeck/droiddeck-ahb-fullscreen-scale.patch.
   apply --check against current external fork PASS. Recompiled one compositor
   object, read other cached inputs on E:, wrote build outputs on C:. One existing
   unused-variable warning. Native APK rollup verified487 other members unchanged;
   v2/v3 signatures verified, v4 disabled. No U-AVF native/runtime files changed.

## What was actually exercised

The small existing AhbSelectionCheck instrumentation now has optional `normal=1`.
After policy checks it opens the real non-exported SessionActivity inside the
target UID, with mode=run and a private shell fixture. This uses real compositor
cold-start ordering, ordinary Xwayland/openbox, binds, session setup and teardown.
It restores both Mavk preference keys afterward. No launcher architecture was
replaced; this is a bounded integration test, not a user-facing demo shell.

The native consumer itself is unchanged. Its fixture calls the installed
system_graphics_launch function on a prepare-only placeholder Proton command,
checks the resulting AHB flags and pinned WSI path, then executes only the
existing native D3D11 consumer with that environment. It never executes Proton,
Steam or a game and never touches a real compatdata prefix. The Wine-only PE
preload is not applied to the native process. Both bundled helpers load for real.

Important test correction: the first native command was not a Proton game
invocation, so system_graphics_launch correctly returned it unchanged. Its S_OK
result proved ordinary session plumbing but NOT AHB routing. That receipt is
stored as first-stock-control.log and is not counted as an AHB PASS. The revised
fixture proves selected flags plus actual AHB request/import records. Another
over-strict assertion expected a WSI_LOG_INFO message compiled out in this
build; it failed despite completed rendering. It was replaced with actual
AHB-request/import and CoreZero receipts, not a product workaround.

After the scaling fix, real source1920x1200/output2560x1600:

- Viewed live-scaled.png: expected full-height moving textured half-width
  scissor region over dark clear color, Android controls composited above it.
  The first pre-fix mismatched screenshots are black; matched-size control is
  visible. The shader draws a fullscreen triangle clipped by the moving scissor,
  not a small geometric triangle.
- Standard D3D11 device S_OK/11_0; four actual RGBA AHB imports.
- 2749 Present calls over23.005s including warmup;2393calls/20.001s measured,
  mean Present8.305ms. These are API counters, not themselves visible FPS.
- SurfaceFlinger game layer#3437 received2747 presented buffers, drops0,
  lateAcquire0. Approximately119.4 buffers/s over the whole consumer interval;
  no independent per-frame pixel-ID/end-to-end latency trace. Do not quote its
  quantized histogram average as exact FPS, or infer game speed from this scene.
- gaps0, Core liveZero1, all tracked remaining resources0, clean consumer exit.
- Instrumentation: APP_AHB_SELECTION_PASS and NORMAL_SESSION_D3D11_PASS.
- A separate same-fixture input regression: Android touch/tap produced
  WINDOW_INPUT type=mouse-button; Android key event produced WINDOW_INPUT type=key.
  This does not prove absolute-coordinate alignment, relative mouse, all keys,
  Wine forwarding, or input through every popup/window transition.
- Packaging/rollup tests15 PASS. Prior81 game-environment tests remain the
  regression evidence for unchanged launch-policy/wrapper code, not all games.

Existing Sarek diagnostic nullDescriptor/FloatControls2 admissions remain
diagnostic. No feature bit was newly advertised. No game FPS result is claimed.

## Preserved state and remaining work

C: recovery/2026-10-24/native-fullscreen-scale/ contains patched source/build,
signed candidate, live-scaled.png, session-artifacts.tar and
session-input-artifacts.tar (actual Android logs and fixture scripts).
app-selection/helpers-runtime.apk is the exact pre-native-change rollback;
original APK33 and APK32 signed rollbacks also remain on C:.
Regenerable unsigned/aligned APK intermediates (nine explicit files,1.30GB)
were removed after verification; signed APKs, source, class outputs and logs
were retained. C: free48.86GB decimal afterward, still below a strict50GB reserve;
avoid new large artifacts. No user data deleted. E: used as read inputs only.

AHB preference restored false, no native test/Xwayland/openbox/Steam processes
left, SurfaceFlinger collection disabled. Awake/mStayOn=true; PIN/manual lock
unchanged. No commit/push. Strategic goal remains active.

Next highest-value gates: actual Wine/XCB full-session game regression when safe
for the user's other Steam session, input-coordinate/fullscreen transition
correctness, then same-scene SHM/AHB comparisons. Different-aspect/non-fullscreen
AHB-only cases and renderer reconstruction remain experimental limitations.
Do not expand synthetic capability coverage in place of these integration gates.
