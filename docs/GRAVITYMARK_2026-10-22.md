# GravityMark real 3D benchmark — 2026-10-22

## COMPLETED native Android baseline — 2026-10-23

Unmodified official APK1.89, stock Mali-G925 driver49.1.0, Default raster,
Temporal AA, DefaultLOD,100000 asteroids,1920x1080. Fresh process11059; explicitly
pressed the visible Benchmark button (merely starting the Activity opens a menu).
Started07:38:54.202 and Benchmark Finished07:41:41.317 device time.

**Score7004; elapsed167.1s; benchmark average41.9FPS.** Confirmed both in native
log and result screenshot native-fhd-result.png. Steady independent20.2306109s
SurfaceFlinger sample:908 NativeActivity buffers,44.88buffer updates/s,0 reported
dropped frames. Different camera phases mean this is not the whole-run average.
No pixel-uniqueness oracle. No GROUP_ERROR_FATAL/VK_ERROR_DEVICE_LOST found in
captured native process log. Startup ASTC encoder warning still exists; not a
claim of warning-free Vulkan validation or benchmark feature conformance.

Thermal status0/skin35.4C at start;1/skin40.9C steady;2/skin43.3C after result,
so this is a sustained heating run, NOT unthrottled peak throughput. Battery70%
after run. Steam not running; idle DroidDeck desktop01 remains in background.
No Send/leaderboard action used. Saved result before normal Quit to stop GPU load.

Artifacts native-fhd-complete.log,native-fhd-result.png and both timestats files
under E:/Mavk/artifacts/gravitymark-1.89-20261022/. This is **native Android**,
not Mavk throughput. Container results remain INVALID/NO-GO as documented below;
no percentage loss or cross-platform score comparison is justified yet.

## Update — native-only layer control, 2026-10-23

Latest native run: checked previous PID23532 log and terminal Done; no result.
Fresh native process11059 was launched normally after force-stop of ONLY the
benchmark. Screenshot native-clean-start.png establishes that it was at the
INTERACTIVE CONFIGURATION MENU, not hung creating a Vulkan device. Missing
Device/Ready log before pressing Benchmark must not be labelled a GPU stall.
Selected existing Default raster mode,100000 asteroids,DefaultLOD,FHD1920x1080,
BorderlessOff. Pressed the observed Benchmark button; log now confirms actual
benchmark start at07:38:54.202 device time. No Steam; desktop01 is background.
The native startup still logs one ASTC encoder warning, but renders visibly and
does not show the Windows control's fatal GPU errors in inspected output.

Native SurfaceFlinger samples: startup1063 buffers/25.1735956s=42.23buffers/s;
steady908buffers/20.2306109s=44.88buffers/s,droppedFrames0. NativeActivity layer
only, not all-screen total911; these are new Surface buffers on moving scene,
not pixel-unique-content checks or an average completed benchmark score.
Thermal0 at start,thermal1/skin40.0–40.9C during steady sample. Raw samples at
native-fhd-timestats.txt,native-fhd-steady-timestats.txt; native-fhd-progress.png.

Bounded same-scene1280x720 fullscreen control removed ONLY VK_LAYER_MAVK_compat
from the explicit layer list, retaining VK_LAYER_window_system_integration and
VK_LAYER_HYBRIS_compat. Vulkan mode only; no feature spoofing or DXVK admission
change. No MAVK_LAYER marker in the retrieved process log. It reaches benchmark
start at3.802s, then rejects RGBAf16 environment texture derivatives at4.191s and
repeats Mali GROUP_ERROR_FATAL. Runner automatically stops this isolated Wine
server on first observed fatal; total7.988s. Thus Mavk is NOT necessary to
reproduce this specific failure. This does NOT prove libhybris/Wine is faulty:
the substituted Android assets themselves remain incompatible with this Windows
renderer, and platform build versions differ despite sharing1.89.

Retrieved full log: E:/Mavk/artifacts/gravitymark-1.89-20261022/native-layer-control.log.
No more performance claims from this invalid resource-substitution experiment.

New native Android run was started after USB briefly returned; PID23532, device
awake/unlocked, battery72%, cold thermal status0. USB disappeared during the25s
SurfaceFlinger sample, so completion and score remain UNKNOWN. Inspect the same
native activity/results after reconnection; do NOT restart solely from ADB loss.
Desktop08 was authoritatively IDLE before this run, not still READY.

User explicitly rejected the tiny D3D11 consumer as a native-power comparison.
Downloaded official GravityMark1.89 Windows MSI and Android APK from Tellusim.
Windows MSI SHA256 `38e32b475cf93eca2085334f7421fc738fb605c5b37f35e90e06d73a54169c8f`;
APK `c5b33e984367886c756c62c24401b8b36854fcefbb79b74a7131aaa2f5a83894`.
APK signature verification PASS, certificate O=Tellusim Technologies Inc.,
SHA256 `044c72ed92146686c2fa7bb1d455a9973322a533b514be3567f4031a2584214d`.
Installed package `com.tellusim.GravityMark`; no production APK changed.

## Native Android

Historical initial interrupted run (superseded by completed baseline above):
scene renders on hardware Mali-G925. Log confirms version1.89, driver49.1.0,
100000 actual asteroids, rasterization, benchmark1600x900. UI transient45.7FPS
is **not an average/unique-visible-FPS result**. Native run ended on activity
transition before completion, so there is NO completed native score yet.
Android resources use ASTC, Windows resources use BC7: versions alone do not
ensure identical resource paths/quality. No native/container performance ratio.

## Separate desktop / Wine — no Steam

User installed and started the desktop. Session `2026-10-22-08-desktop` READY,
1280x720, DISPLAY=:0 (NOT former Steam :99). Existing Wine prefix
`/root/.mavk-wine-poc` reused. Runner reads selected desktop Xwayland environment
and applies installed Mavk/Sarek environment; no game config/global overrides.
Native Windows binaries extracted from MSI, not installed on host PC. Vendor
run scripts document command-line arguments. Actual layout is bin/ + ../data/;
unpacked data.zip and copied executables to isolated bin accordingly.

Results:

1. **D3D11 NO-GO:** requires minimum feature level11_1; installed Sarek3.0.2
   offers11_0. `D3D11InternalCreateDevice: Minimum required feature level
   D3D_FEATURE_LEVEL_11_1 not supported`. No spoof/config force applied.
2. **Vulkan with Windows resources:** gets to Mali but BC7 textures unsupported
   in this native-Vulkan application path. Not a valid performance run. Existing
   Sarek BC support cannot be assumed to expose BC to arbitrary Vulkan consumers.
3. **Vulkan with Android resources, isolated attempt:** copied official Android
   data.zip to separate `mobile/`, linked data/textures -> textures.astc. No changes
   to original Windows resources. Real scene appears,100000 asteroids, but
   environment texture RGBAf16 errors and repeated Mali `GROUP_ERROR_FATAL`
   status0x2f0002c3/0x2b0002c3. Initial windowed benchmark also cancelled due to
   resolution changing1600x900 ->1286x707. Not a valid score.
4. **One fullscreen resolution control:** same separate ASTC attempt at desktop
   1280x720,fullscreen1. Repeated GROUP_ERROR_FATAL still occurs. Stopped isolated
   Wine server. No further flag sweep or driver changes.

Do NOT report the visible FPS counter65 from the cancelled/erroring ASTC run,
or compare it with the native transient45.7. Different resolution, cancelled run,
texture errors and GPU faults make this comparison INVALID.

## Remaining boundary

Feature-level source audit2026-10-23: checked Sarek D3D11DeviceFeatures::
GetMaxFeatureLevel(src/d3d11/d3d11_features.cpp199–204).11_1 requires
OutputMergerLogicOp AND vertexPipelineStoresAndAtomics. G925 profile has logicOp1
but vertexPipelineStoresAndAtomics0 (also absent in actual Muck enabled features).
Thus forcing d3d11.maxFeatureLevel11_1 would bypass a real semantic gate, not fix
an adapter-reporting typo. No override applied; new-feature implementation paused.

Full native-power comparison is NOT achieved. Concrete compatibility blockers,
not proven low GPU throughput. D3D11 lacks11_1 for this consumer. Native Vulkan
desktop resource path lacks BC7 exposure; mobile-resource substitution is not a
correct clean benchmark and also triggers GPU faults. Cannot attribute those
faults specifically to Mavk vs hybris/Wine/renderer without a controlled A/B.
Do not infer physical Mali weakness or S25-equivalent mechanism.

Next bounded diagnostic, if continuing GravityMark: same ASTC scene with Mavk
layer vs native-only layer chain (no fabricated capability bits), or select a
documented common workload supported by both platforms. Do not repeatedly run
GPU-faulting scenes as performance tests. Native baseline needs completed run at
matched settings and thermal state. New Vulkan-feature implementation remains paused.

## Saved state

### Follow-up: original Linux ARM64 build, 2026-10-23

Official https://tellusim.com/download/GravityMark_1.89_arm64.run extracted on E
without executing host installer payload. Original Linux data.zip SHA256
30236633c892e36786f0c8f7755648cb25c2ce53c4806611d042a096c05095dd.
Separate files/mavk-gravity-linux; no Windows or Android asset substitutions.
Desktop03 READY1280x720; requested benchmark1920x1080,100000asteroids,Temporal.
Bounded ARM64 runner gravitymark-linux.py uses installed Mavk aggregate plus
WSI/hybris and guards enabled, no Steam/Wine/FEX. Vulkan exits1 in initialization:
BC7 unsupported and four compute extended-stage admission gaps, CoreLiveZero1.
No actual benchmark draw/score. GL modern Mesa requires absent nullDescriptor;
existing Steam Mesa25.1.9 lazy path instead fails guarded descriptor layout
admission. Did not disable guards or invent features. Terminated both GL error
dialogs/processes. Logs linux-arm64-vulkan.log and linux-arm64-zink25.log on E.
Native Android41.9FPS cannot be divided by a nonexistent container score.
Restored Muck override before desktop switch; left desktop available. Temporary
transfer TARs removed from tablet, retained benchmark source/resources/logs.

Artifacts `E:/Mavk/artifacts/gravitymark-1.89-20261022/`:
official downloads/extracted files, wine-logs.tar, native-start.png,
wine-mobile.png, wine-fullscreen.png. Device files `files/mavk-gravitymark/`;
runner `experiments/droiddeck/gravitymark-wine.py`.

All benchmark processes stopped using only the isolated prefix's wineserver-k.
Desktop08 remains READY; Steam not running. No public feature changes, no modified
installed Mavk/WSI/APK25, no U-AVF/Windows/Ubuntu changes, no commit/push.
