## 2026-10-26 — clean developer APK / ordinary game route

Clean checkout APK installed (SHA256 36d5efc4749ba6ee22726d6be26f9be04f757a24d2ef8f1d4fa4ed7caf59c6f0).
Packaged runtime8213 selected by normal Steam launch; HL2 menu/train and Muck
menu visible without private PE/layer overrides. AHB copy0 intervals confirmed;
SHM restart produced the Muck loading image, copy counters and zero-copy0.
AHB restored. Full HD user gameplay estimates: HL2 usually20–30 near maximum,
30–50 low; not an independent benchmark or a60FPS claim. Core independently
builds from a fresh checkout. Local commits only, source/binary license gate
and broad Windows-host unit failures explicitly open. See ACTIVE_PLAN.md and
the fork BUILD_CHECKPOINT_2026-10-26.md; artifacts-local/release-20261026.

2026-10-26 CURRENT: user-confirmed HL2 Full HD gameplay, usually20–30FPS near
maximum graphics and30–50 low; rare character/audio repeats. In-game resize can
crash; cold launch1920x1080 verified. Do not interrupt recording. New normal-
button runtime8213c670 builds but is not installed. Clean build/commits/final
ordinary-button device checks pending. Muck FPS research CLOSED. See ACTIVE_PLAN
and DEVELOPER_REVIEW. Prior checkpoints below are historical.

# Mavk checkpoint — 2026-10-23

## LATEST — uniform metadata / native public sets, 2026-10-25

Resumed GLM's UBO work; corrected packed std140 metadata passes Mali GPU oracles
but same creative scene12.1 vs control12.2 buffers/s: no gain. Do NOT repeat this
as a promising60FPS path. New optional native-public-set passthrough mirrors
validated ordinary/template writes at the app update point, seals them after
recording, preserves typed-null/copy/extended-update fallback.14 actual GPU runs
PASS (including retention/replay/dynamic/inflight/immutable/images), CoreZero;
ARM64 template callback test19checks/19negatives PASS. Layer68586264ff98...
actually bypasses public duplication for~99% of Muck draws. Menu35.2/s, same
forward creative scene13.0/s; control12.2/s had different thermals, so no proven
FPS gain.60FPS still NOT reached; diagonal artifact cause still OPEN.
Private profile restored/inactive and Muck quit, Steam retained. Installed APK
6c8680f5/runtime3bd128 unchanged. See UNIFORM_METADATA_AND_NATIVE_SETS_2026-10-25.md
for exact hashes/evidence/failures. Stop descriptor microtuning; isolate dominant
GPU shader/driver versus engine cost next. GPU probe/CSE remain OFF.

## LATEST — portable FEX boundary / APK checkpoint, 2026-10-25

Installed source APK6c8680f5 (fex-boundary-demo), runtime3bd128 unchanged. Steam
normal startup and Muck AHB menu smoke PASS/errors0, hot20.4buffers/s is not a
performance A/B. Portable-FEX child environment fix advances native Linux DDNet
past socket failure; even x86-64 true then fails in loader with sbrk()/exit127.
No DDNet graphics claim; HL2 not installed.37 FEX +20 packaging tests PASS.
Steam minimization lowers helper CPU to~2% total; menu~35/s, not60. Grass-disabled
world still~13/s; saved checkbox restored, no valid geometry on/off A/B claim.
Detailed follow-up in MUCK_GPU_PROFILE_AND_ARTIFACTS_2026-10-25.md and fork's
docs/MAVK_NATIVE_FEX_2026-10-25.md. CSE/GPU probe off; artifact root cause OPEN.

## LATEST — live Muck profiling / artifact rollback, 2026-10-25

MUCK_GPU_PROFILE_AND_ARTIFACTS_2026-10-25.md supersedes Steam reservation below.
Steam/game launch authorized. Installed notices APK443c300b/runtime3bd128,
executable payload unchanged; source/legal completeness still open. Real same-view
1600p/1200p AHB both9.1 displayed buffers/s; no resolution-only performance gain.
GPU batch spans24.9/55.7ms, CPU not eight-core-saturated; no60FPS claim. GPU probe
run showed black tiles with Mali errors; probe disabled. User recurrence
also reported during actual CSE candidate with errors0. CSE/performance profile now
RESTORED, normal metadataReadOnly0, three screenshots captured (two inspected clean).
Not a permanent artifact fix. Thermal Status3/SKIN45C invalidates cold-vs-hot FPS
comparisons. Muck quit for cooldown, Steam remains open. Next same-scene bounded
artifact/performance localization or second available game. No push/production change.

## LATEST — source-built APK / Muck saved pacing, 2026-10-25

SOURCE_APK_AND_MUCK_PACING_2026-10-25.md: actual Gradle APK675e9cb6 installed,
runtime-input APK attestation PASS; AMD64 Wine AHB1920→1280→1920 resize passes,
CoreZero/gaps0. Seed70inputs100MB were double-pinned; not a full dependency/Rust
rebuild. Read-only Muck IL and saved XML now prove144FPS setting/VSync0 and its
assignment path, not an explicit saved30cap. Does not localize live~30FPS root
cause. Steam reserved; no game launch. Local commits only, no production changes.

## LATEST — developer demo preparation, 2026-10-25

DEMO_PREPARATION_2026-10-25.md is authoritative. C: fork DroidDeck-Mavk/mavk-demo;
portable WSIb1bad07f and runtime33c0; installed experimental APK360c4765. Actual
scaled pointer bug fixed: fullscreen AHB takes pointer input before underlying
desktop; verified2000,1200→1820,1108/mask0 on real X server. Normal native D3D11
AHB stream and policy fallback PASS, CoreZero/gaps0. Kotlin compile and native
recompile PASS;17+81+30packaging/environment/controller tests. Not a clean full
source release build or gameplay60FPS. Steam reserved; no game launches this turn.
Historical Muck menu29.65/29.94 at1200/1600, root cause unresolved. HL2/gameplay
and relative captured input remain gates. Muck override and private fixture restored.
Local commits authorized now; no push/publication. U-AVF/VMs unchanged.

## LATEST — GLM AHB resize recovery, 2026-10-24

See AHB_RESIZE_RECOVERY_2026-10-24.md. Actual session27 Muck26064/private
d0ea715a verified1920x1200 AHB after ordinary mode toggle, live menu and resize
back/up work.349GPU=349zero-copy=349on-screen/10s,copy0. RGBA/BGRA allocation+
acquire regressions PASS CoreZero/gaps0. BGRA display usage inheritance fixed;
forced2560 marker clamp removed; tiny XCB windows keep X11. Bundled99a12 restored,
Muck-only candidate reversible. Fair SF menu samples29.65/29.94 at1200/1600,
no resolution speedup/60FPS claim. Absolute input scaling still incomplete.
Next input + actual frame-pipeline profile; no separate Steam CLI experiments.

## LATEST — Wine/AMD64 AHB + resize, real Muck launch authorized, 2026-10-24

AHB_WINE_CONSUMER_2026-10-24.md: actual ARM64 and AMD64 Win32 PE consumers
visibly render via ordinary-session bundled AHB. AMD64 ResizeBuffers down/up
passes,CoreZero/gaps0. Existing APKc56d/runtime9e2 unchanged. No gaming FPS claim.
User now permits Steam/game launches; enabled app AHB toggle, requested normal
Muck1625450 launch. Session2026-10-24-21-steam starting; inspect live process/logs
next. Earlier Rust-session restriction no longer active. No commit/push.

## LATEST — normal-session AHB black-screen fix, 2026-10-24

AHB_NORMAL_SESSION_2026-10-24.md: installedc56d55ff, native92bff72c,
runtime9e2a00b1. Relocated launcher dependencies now included/pinned. Real
SessionActivity/run-mode + Xwayland/Openbox + bundled AHB renders1920x1200 on
2560x1600 output after minimal same-aspect fullscreen eligibility fix. Matched
size control localized prior black output; actual screenshot now visible.
SF2747buffers/drops0 over~23s, native mouse/key receipts, CoreZero. No Steam,
Proton or games executed; gameFPS/Wine mapping still pending.15package tests
PASS. AHB restoredoff, signed rollbacks/logs on C:, no commit/push. Strategic
goal active; next actual game/input regression without interrupting user's Rust.

## LATEST — optional runtime artifact + launch hook, 2026-10-24

AHB_RUNTIME_PACKAGE_2026-10-24.md:19.26MB runtimec2f107a4 adds AHB99a12d70,
receipt/upstream license and tested policy5cd0a6da;262 old members identical,
onlymanifest changed.11 packaging/rollup regressions PASS. Existing generated
Valve/extra launchers support quoted explicit policy override with original
default preserved;81 existing tests incl alternative-path cases PASS. Source/
artifact only, not active APK/UI/session selection. No Steam or VM changes.
Next app preference + BEFORE-start compositor marker ordering and packaged paths.

## LATEST — launch-selector candidate/preflight PASS, 2026-10-24

AHB_LAUNCH_SELECTION_2026-10-24.md: explicit ahb-bgra/profile/x11-shm selector
in isolated existing launch helper; binary/receipt/compositor gates, SHM reversal
clears stale AHB flags.81 tests PASS + actual Android prepare-only check PASS.
No Steam/Proton/game executed, active launcher and game profiles unchanged.
Canonical code/tests patch saved; actual private candidate5cd0a6da. This is not
yet normal-user UI integration or a game performance PASS. Next runtime package/
fork selection plumbing, then actual Wine/game regression when Rust is not active.

## LATEST — real visible1920x1200 ~59.5FPS, 2026-10-24

ZINK_1200_VISIBLE_2026-10-24.md: APK33 debug-only existing FIFO output command,
486 other APK members unchanged, C: verified APK32 rollback. Common WSI99a12d70
three buffers + BGRA blit:30s jellyfish,1786 SF-presented buffers, full compositor
windows59.6/59.9FPS,drops0,CoreZero/gaps0, actual visible screenshot. Native
D3D11/Sarek1200p regression S_OK11_0,clean shutdown (visibleFPS unmeasured for
that separate regression). Steam/Rust untouched. No game/full-Vulkan claim.
Stop synthetic FPS refinement; next reversible experimental backend integration.

## LATEST — visible Zink30->60 by three-buffer A/B, 2026-10-24

AHB_THREE_BUFFER_VISIBLE_2026-10-24.md: unblocked Android, real moving jellyfish
at1280x800. Two-buffer d53 baseline29–31FPS; minImageCount3-only99a12d70 reaches
~60 over complete30s:1797 rendered,1795 SF-presented buffers, two full compositor
windows60.1/60.0FPS, drops0,CoreZero/gaps0. Correct live screenshots. SF histogram
62.426 figure is quantized, not >60 claim. One GPU BGRA blit/frame still remains.
RGBA/BGRA1920x1200 allocation regression PASS, not1200p render. Need debug output
selection fix before1200p measurement. Steam/Rust untouched. No commit/push.

## LATEST — real Zink/BGRA functional PASS, 2026-10-24

ZINK_BGRA_FUNCTIONAL_2026-10-24.md: changing1200p pattern oracle13,824,000 exact
pixels; isolated blit0.340ms mean/0.495max. Real glmark/Zink now reaches private
BGRA-AHB at1280x800,FIFO:1198 draws,2397 submits,CoreZero/gaps0; compositor599
incoming GPU buffers but ZERO on-screen presents (Android locked). No visible
FPS claim. Corrected independent harness loader order,Kopper flag,FIFO selection
and receipt gates; invalid301/315/438 numbers excluded. Steam untouched. Stop
color-test expansion; next full visible1200p Surface gate, then real comparison.

## LATEST — BGRA pixel oracle PASS, no Steam, 2026-10-24

BGRA_AHB_CONVERSION_2026-10-24.md follow-up: shared blit recorder used by WSI
and existing native AHB receiver; six uniform1200p images,13,824,000 exact pixel
matches. Negative raw copy catches swapped R/B. Isolated GPU timestamp mean
0.152ms/max0.184ms (6 uniform frames, not game/frame latency). Latest private
WSI d53b291b rebuild + BGRA/RGBA allocation regressions PASS. No feature/APK/
production changes. Full WSI colors and visibleFPS still pending locked screen
and debugActivity1280x800 config; no Steam/Rust interaction.

## CURRENT — opt-in BGRA GPU conversion, 2026-10-24

BGRA_AHB_CONVERSION_2026-10-24.md: implemented and compiled ordinary BGRA source
to RGBA AHB GPU blit, same existing release submission, no CPU pixel readback.
Initial real GPU draw180 frames/2951.940ms, CoreZero/gaps0. NOT visible FPS or
pixel PASS. Both format allocation/fence controls PASS. Subsequent draw control
rejected fullscreen configure while Android locked; exact configure diagnostic
added. APK32/common runtime unchanged. Steam/Rust explicitly must not be started
or interrupted. Private candidate4727674c, f1b rollback preserved. C: recovery
contains current external source; reproducible integration patch in experiments.

## CURRENT — keyboard PASS, Zink BGRA/AHB boundary, 2026-10-24

KEYBOARD_ZINK_AHB_2026-10-24.md: APK32 + WSI f1b9370e now delivers mouse+A+
Shift+B to actual original XCB D3D11 consumer, CoreZero/gaps0. No actual Muck
input retest (competing Rust). Zink requests format44; stock Mali AHB optimal
BGRA8 unsupported even minimalusage, RGBA8 supported. Need proper negotiation or
explicit GPU conversion, not false zero-copy. Invalid1965 FPS rejected by fixed
harness. Disk1/E: real event153 retries; new artifacts/source protection on C:
recovery/2026-10-24. See document before touching older E worktree. No commit/push.

## CURRENT — real Muck through XCB/AHB, 2026-10-24

XCB_AHB_GAME_2026-10-24.md: actual Muck.exe/Wine/Sarek reaches moving visible
menu through private AHB WSI proxy; 2560x1600, two 10s windows189/186 zero-copy
frames, Surface short sample18.56 buffer presents/s. No matched gain claim.
APK31 input-transparent proxy: native mouse-button reaches original XCB app,
keyboard unproven; first APK30 tap did not reach Settings. Muck retest stopped at
Steam's competing-Rust-session warning, not approved. Per-game/script rollback
completed, own session marker removed, tests stopped. Original WSI/runtime
untouched. Linux16/16 launch tests, native explicit-sync mock PASS. Scope/limits
and prior one-off Steam exit139 documented. No commit/push; strategic goal active.

## CURRENT — real D3D11/DXVK visibly on the AHB zero-copy layer, 2026-10-23

SAREK_AHB_VISIBLE_2026-10-23.md: black-screen root cause found and fixed. SDL3
always installs wp_viewport with identity destination; compositor's
ahb_layer_only_candidate rejected any dst_set window, so DXVK presents fell to
the copy path that cannot import gralloc AHB -> black at 60/s. Fix admits
identity viewports only. APK29 native rollup d63e36e2 installed (library
2fe2d1a7, 486 members unchanged, APK28 rollback kept). Actual DXVK-Sarek run:
219/220 frames zero-copy on droiddeck_wayland_game, layer shown, gaps=0, and
VISIBLE movement 62832/256000 sampled px, X 36..1648. draw2 zero-copy
regression PASS. Steam/X11 regression NOT RUN yet - do before further runtime
changes. Diagnostic triangle, not game FPS/latency. No commit/push.

## CURRENT — actual standard Vulkan Present + external vkcube, 2026-10-23

AHB_WSI_PRESENT_2026-10-23.md: private existing WSI now implements AHB acquire /
present GPU fences and ownership transitions. Standard API triangle 1920x1200,
1800 frames/~30s, actual moving Surface; independent short Surface sample ~60.
Unchanged ARM64 vkcube also visibly rotates textured cube, actual 1280x800 (NOT
requested1200p), Surface sample ~59.99. Consumer-driven Vulkan1.0 dependency fix.
Final conservative-capability candidate3b705bef passes cube regression, CoreZero.
No source readback or app-layer image copy; not peak GPU/game FPS or CTS proof.
Private fullscreen/queue0/FIFO subset, not shipped; default WSI/common/APK unchanged.
v3-absent negative fails-8; marker removed and debug app stopped. Next actual
Wine Wayland/FEX/D3D11 consumer, preserve X11 fallback. No commit/push; goal active.

## CURRENT — standard Vulkan AHB swapchain allocation/first acquire, 2026-10-23

AHB_WSI_ALLOCATION_2026-10-23.md: isolated existing WSI + shared AHB importer now
passes CreateWaylandSurface/CreateSwapchain/GetSwapchainImages/CreateImageView
and first AcquireNextImage fence on G925, three 1920x1200 images. Actual mappings
and hash checked; destruction gaps0/CoreZero. Proven WSI v2-only memory-properties
dispatch mismatch fixed in common importer. Allocation-only diagnostic: no rendered
or presented swapchain frame, no FPS/game-speed claim. Present intentionally rejects.
Next real v3 buffer/fence + ownership/acquire/present integration, not more allocation
tests. Installed APK28/common/WSI and production VMs preserved; no commit/push.

## CURRENT — shared Android/glibc AHB image adapter, 2026-10-23

AHB_SHARED_ADAPTER_2026-10-23.md: actual imported-image code extracted into
adapters/android/ahb_image.c, linked by both existing cross-ABI pool consumers.
Final real G925 regression: 90x1920x1200, 207.36M pixels exact, explicit GPU
fences both ways; wrong-pixel negative fails. Seven mocked API failure stages
also verify transactional cleanup. No new visible FPS measurement (locked
Android), APK/runtime/feature bits unchanged. WSI integration must preserve
display-release dependencies in acquire, not import an already-signalled FD
merely because a buffer release event arrived. Standard WSI still incomplete.

## CURRENT — real moving GPU buffers on Android Surface, 2026-10-23

AHB_DIRECT_SURFACE_2026-10-23.md supersedes the no-Surface checkpoint below.
Actual glibc/Mavk graphics -> three persistent 1920x1200 AHBs -> private Wayland
v3 explicit fences -> Android SurfaceControl: visible moving triangle PASS.
360 frames/callbacks in 5984.517 ms, zero source readbacks; Surface layer sample
63 records / 1.0333 s ~60 buffer presents/s. NOT sustained unique game FPS or
latency proof. Actual app socket-timeout ownership and layer-only window mapping
fixed. Native-only experimental APK28 installed; normal default-v2 Steam/Muck
render and Settings input checked, session stopped IDLE. Common layer/WSI and
production VMs unchanged. Private marker removed; rollback APK27 retained on E.
Next standard Vulkan Wayland WSI consumer; Wine/X11 boundary remains unresolved.
No new feature bits, commit or push. Strategic goal remains incomplete.

## CURRENT — private AHB-native Wayland protocol implemented, 2026-10-23

AHB_WAYLAND_EXPLICIT_2026-10-23.md: DroidDeck native candidate now creates an
AHB-backed wl_buffer directly via public socket transfer, explicit acquire and
release FDs. Version3 gated DROIDDECK_EXPLICIT_AHB=1; default stillv2. Layer-only
fullscreen experiment; no unsynchronized Vulkan-copy fallback. Native module
build and actual candidate factory+Wayland wire/realAHB import PASS onG925.
FD/lifetime state test PASS but Surface mocked; NOT visibleSurface/game/FPS PASS.
Old audit correction: common dmabuf registry also accepts android_wlegl/noFD;
the missing contract was synchronization,not strictly resource registration.
No APK installed/runtime replaced. Next actual private glibc client+SurfaceControl
using tested pool/fences; then WSI/X11 boundary integration. No commit/push.

## CURRENT — reusable AHB pool and bidirectional GPU SYNC_FD, 2026-10-23

AHB_SYNCFD_POOL_2026-10-23.md supersedes the synchronous prerequisite below.
StockG925/glibc installedcommon -> independent nativeVulkan: three persistent
AHBs,90changing1920x1200frames,207.36M pixels exact PASS. Explicit GPU-fences in
bothdirections,including actual pendingimports(3native/1glibc),90pendingexports
eachside; no producerreadback/pixelpayload. Interval1198.985ms includes1183.235ms
CPUoracle,NOT visibleFPS; nativeone1.306ms commandrecyclewait,producerzero.
Negativeoracle correctlyfailsbothsides; legacy sync +common nested GPU PASS.
Next actualSurface/WSI adapter,not more clear benchmarks. Existing bannerAHB
requiresdma-buf wl_buffer;currentX11SHMdoesnotqualify. No finalzero-copy/gamegain
claim or newfeaturebits. Privateprobesonly,APK27/commonf108/WSId07b/VMs preserved.
No commit/push; strategicgoal incomplete. See detailed report for hashes/limits.

## CURRENT — full-size AHB across glibc and independent native GPU, 2026-10-23

AHB_CROSS_ABI_GPU_2026-10-23.md: stockG925/appUID,existing hybris allocatesRGBA8
AHB1920x1200 withCOLOR|SAMPLED|COMPOSER_OVERLAY. Existing installedcommonf108
imports dedicated memory, GPU writes17then91, all2.304M pixels/frame checked.
Same allocation is passed as AHB handle to independent Bionic process; matching
ID/descriptor, nativeMali Vulkan imports and GPU-reads all2.304M pixels correctly
WITHOUT writing source. Expected42/actual91 negative correctly fails and status
propagates. Installed common nested guarded GPU regression afterwards PASS.
GO for isolated shared AHB-backed WSI integration; NOT Surface/FPS/finishedzero-copy
PASS. Oracles read back to CPU; producer waits for completion. Pool+SYNC_FD+
actual moving Surface/game integration still needed. Existing hostAHB protocol
requiresdma-buf wl_buffer, currentX11SHMdoesnotqualify;do notjusttoggleflag or
guessnativeHandleFD0/modifier. NativeGPU cold ACK132.86ms includesdevice creation,
import,readback+CPUoracle,notframe latency. Metadata-only coldACK8–14ms separate.
Only private probes/tools added. APK27/commonf108/WSId07b/productionVMs untouched,
no commit/push. Source/binary/log provenance on E; app session remains IDLE.

## CURRENT — hardware GPU suite and real3D run without a desktop, 2026-10-23

HEADLESS_GPU_AND_CSE_2026-10-23.md: app-owned direct glibc loader + existing
hybris/Mali executes actual compute/graphics consumers without Steam/Xwayland/
Surface or Android unlock. Native24compiler checks/9rejects and26existing GPU
cases PASS,strict verifier with actual private4b16fa3f path,guards1,gaps0/CoreZero.
Fresh report directories; original fixtures/consumer/checks reused. Generic small
run-headless-probe.ps1 and generated-control adapter,not another test framework.
Default/off headless full-suite launcher also exercised independently.

Upstream glmark2 surfaceless NativeStateNull now builds and renders actual1080p
terrain with pixel samples. CSE opt-in preserves tested guards but paired scene
results80/86/84/81/84 show no reproducible speed gain; defaultOFF,NOT rolled into
APK27. Installed commonf108/systemWSId07b unchanged;source/candidate capsule on E.
APK27 rehashed f5c84d3a;app sessionIDLE after locked-desktop test stopped normally.
No commit/push/production changes. C50.4/E831.6GiBfree;tablet cache trim reclaimed
~1.38GiB,remaining~34.8GiB,NOT50;no games/VM/user data removed.

## CURRENT — installed common APK26, real 3D comparison, lifecycle candidate, 2026-10-23

COMMON_RUNTIME_APK26_2026-10-23.md: checked subgroup/extended-layout/standard-UBO
fixes rolled into experimental DroidDeck common runtime7549290e, layerf1081e2c.
Installed APK26 and actual normal Steam/Muck menu PASS;26 fresh guarded GPU cases
PASS with exact installed runtime hash,no diagnostic reads/events,gaps0/CoreZero.
Runner stale nested-output verification hole fixed. No new public feature bits.
Read-only metadata26GPU PASS but no verified speed benefit,defaultOFF. Compiler
cache tuning not pursued:~1s guard compilation out of~76s Muck startup.

GLMARK2_COMMON_3D_2026-10-23.md: independent real3D scenes outside Steam at1080p,
matched RGBA8/D24 offscreen+glFinish. Native366/130/401 completionFPS vs guarded
Zink/Mavk392/96/349; sequential/thermal/API/harness differences,NOT pure overhead
or visibleFPS. Fresh actual visible run~60FPS/58.8Surface updates/s.27 post-timing
pixel samples successful;no layer gaps. Thermal control terrain113then95/95.

OPTIMAL_READBACK_2026-10-23.md: isolated optimal-render/linear-readback WSI works
but no measured gain;reverse control also slower when hot. NOT shipped. Muck
diagnostic profiles restored;system WSId07b unchanged. Temporary lxqt display
preference removed before APK27 launch.

Separate lifecycle candidate: FrameLoop removes queued Choreographer callback on
detach and ignores stopped callback,prevents rapid detach/attach multiplication.
Actual compiled Kotlin checked by six JVM cases/100 cycles. Android unit-test
Gradle task was blocked by dependency TLS Tag mismatch,NOT claimed PASS. DEX-only
APK27 signed/installed,472non-DEX members unchanged,4expected new classes,none
removed. Hardware Steam/Muck Settings input and live frames after explicit resume
PASS,19.3before/19.7after Surface updates/s. Three more paired lifecycle cycles
PASS;hot11.8fps atthermal3/45C is not a causal performance claim. See
DROIDDECK_LIFECYCLE_2026-10-23.md;APK26/25 rollbacks on E. Later tablet rebooted,
currentsessionIDLE;previous session guest exit0,not a proven app crash.
Production U-AVF/Windows/Ubuntu/P7/H264 untouched. No commit/push.

## CURRENT — enabled standard UBO validation fixed, 2026-10-23

UNIFORM_LAYOUT_2026-10-23.md: real glmark2/Zink exposed false rejection of enabled
uniformBufferStandardLayout. Native enabled policy now reaches static-use and
guard optimizer/input/output validation; strict feature-off remains. Private
83e4a6d9 passes actual Mali valid/OOB packed-UBO A/B,12GPU regressions,19compiler
checks/9rejects. Real fullscreen1920x1080 three glmark2 scenes complete,gaps0,
roughly60 benchmarkFPS and independent58.8 Surface updates/s. Not native-power
ratio or game improvement. First resumed/frozen and720p windowed runs excluded.
Desktop temporary resolution key restored; installedAPK25/common runtime intact.
Next: isolated optimal-render+linear-readback WSI A/B on actual Muck; no new
Vulkan feature expansion. Prepared source E:/Mavk/work/wsi-optimal-readback-20261023,
private wsi-optimal artifacts; experimental Muck-only profile must be restored.

## CURRENT — unused extended layouts / Zink initialization fixed, 2026-10-23

UNUSED_EXTENDED_LAYOUTS_2026-10-23.md: actual layout flags2=UAB_POOL, not push.
The push-filter interpretation was wrong and that experimental code was removed;
native/current extension count143,pushDescriptor0 unchanged. New shared admission
accepts semantically zero binding-flags chains and defers native extended UAB
layouts while statically unused. Actual shader reference explicitly rejected at
pipeline creation, draws0/submissions0/CoreLiveZero. NOT full bindless or opaque
updates/templates support. Guards on/no feature bits changed.
Private f1951495bd8ecb8ffda97314936b956f015163a7f2ea66a3dbe23ddf4bf69e3c passes
old/new zero-chain nested GPU A/B,unused-UAB compute+graphics A/B,referenced-set
negative,21policychecks,12GPUregressions,2cull-link controls. No CTS/FPS claim.
GravityMark GL progresses beyond init_layouts, then GLXBadFBConfig. Matching
glxinfo succeeds with hardwareZink/Mali,GL3.2/GLES2.0,vs benchmarkgl45/gles32.
No API spoof. Native GravityMark41.9FPS remains separate; no container score.
Artifacts E:/Mavk/artifacts/layout-deferred-native-20261023/; private guest folder
files/mavk-layout-deferred. Installed89c8/WSId07b hashes unchanged,overrideinactive,
desktop03 retained,no benchmark process,APK/production changes,commit or push.
Next performance work: supported common 3D benchmark, not more API expansion to
force GravityMark. Keep all existing common Vulkan and Windows branches intact.

## CURRENT — native subgroup stage compatibility fixed, 2026-10-23

SUBGROUP_STAGE_PASSTHROUGH_2026-10-23.md: diagnostic82a64114 proved actual
GravityMark pNext sType1000225001(requiredSubgroupSize). Shared layer now preserves
that native stage request through shader instrumentation; unknown/tail chains
remain fail-closed, cull-link combination explicitly not admitted. Guards remain
enabled; no feature changes. Final private49369e395def798ab89840a71e8da6cfeb88d978631c7cead34c0ca2b06cda8b
real Mali A/B oldaggregate fails/new candidate passes existing nested-buffer
read-zero/write-suppression/input-sentinel checks.12GPUregressions +2guarded
cull-link descriptor/no-descriptor controls PASS,CoreLiveZero. Helper11checksPASS.
First candidate253758f4 GravityMark no longer fails PrefixScan, reaches
RenderManager7.349s,gaps0. BC7 still unsupported, stop10s,no FPS/native ratio.
Final GravityMark control also gaps0,reaches RenderManager2.352s; stops10.491s
for unsupported BC7,no benchmark score. Logs/results archived on E. Candidate deployed only
private files/mavk-stage-subgroup-final/payload. Installed aggregate89c8 andWSId07b
hashes unchanged, Muck overrides inactive, desktop03 running,no APK/production
mutation,commit or push. Next actual benchmark blocker: BC7 admission, not
subgroup request; no ASTC substitution/fake textureCompressionBC.

## CURRENT — official Linux ARM64 benchmark attempt, 2026-10-23

User requested benchmark on installed desktop. Actual initial session was Steam02
with Muck foreground. Quit game normally, restored Muck diagnostic overrides
(profile active=false), then started isolated desktop03 READY1280x720.
Official GravityMark1.89 Linux ARM64 extracted on E, original Linux data.zip,
no Windows/Wine/x86 translation, no mobile asset substitution. New bounded runner
experiments/droiddeck/gravitymark-linux.py; installed aggregate layer unchanged.
Vulkan1920x1080/Temporal/100000asteroids exits1 before benchmark: BC7 unsupported
and four compute pipeline `extended shader stage unsupported` gaps. CoreLiveZero.
GL first fails modern Zink nullDescriptor requirement; existing Steam Mesa25.1.9
lazy path instead reaches Mavk but rejects guarded descriptor layout/allocator.
No fake features or guard disabling. Both GL initialization-error processes
terminated, no ongoing benchmark GPU load. Original native Android result41.9FPS
remains valid; NO container score or native-power ratio. Logs on E in
gravitymark-1.89-20261022/linux-arm64-{vulkan,zink25}.log. Desktop03 left READY.
Removed ~389MiB temporary transfer TAR duplicates from tablet; benchmark files
retained for reproduction. No APK/production/feature changes, commit or push.
Prior no-host-barrier candidate correctness passes remain; performance A/B
unproven due focus/thermal confounders, not an improvement/regression claim.

## CURRENT — real native baseline + GPU profiler regression, 2026-10-23

GravityMark official Android1.89 full run COMPLETE: score7004, average41.9FPS,
167.1s,1920x1080,Temporal,100000asteroids,DefaultLOD,Mali-G92549.1.0. Native
only, NOT Mavk. Independent steady44.88Surfacebuffer/s20.23s. Thermal0->1->2;
no GPU fatal found in captured native log. Result screenshot/native log saved.
Previous missing startup logs were an INTERACTIVE MENU, not a native driver hang.
Normal Quit after capturing results; no Send/publish. No valid container ratio.

ADB host PnP healthy but repeated read failures; one supported host-server restart
restored current USB. Desktop2026-10-23-01-desktop started READY1280x720.
Private GPU profile3be3334e staged ONLY files/mavk-gpu-check/payload. Existing12
GPU regression modes PASS11.139s,11 real submit1 timestamp reports,allCoreLiveZero,
no probe error/gap/fatal. Not submit2 GPU coverage or Muck FPS/overhead proof.
Aggregate89c8 and installedWSId07b hashes verified unchanged; Muck profile inactive.
No APK/runtime/game-config changes or commit/push. Local builds terminal; goal
active/progress. Next real-workload GPU attribution, not new feature-matrix loops;
do not rerun known-invalid GravityMark ASTC substitution or fake capability bits.

## CURRENT — bounded A/B and profiler safety, 2026-10-23

Native-only WSI+hybris GravityMark control (without Mavk) repeats the same
RGBAf16/ASTC-resource errors and GROUP_ERROR_FATAL. Retrieved log saved; Mavk
not necessary for failure, but asset substitution remains invalid, not proof of
Wine/hybris bug. No benchmark/native-power ratio. Do not repeat ASTC variants.

GPU_PROFILE_SAFETY_2026-10-23.md: fixed std::bad_alloc in prepared private GPU
probe decoration so original queue submission is preserved. Real helper host
test33checks PASS, fullARM64 layer build PASS3be3334e. NOT staged/GPU tested;
installed APK25 unchanged. No new Vulkan features or actual FPS improvement.

USB briefly returned then disappeared. Native GravityMark PID23532 started;
completion/result UNKNOWN, inspect SAME run after reconnect, no blind restart.
Desktop08 was IDLE, battery72%, native-start thermal0. Native25s FPS capture
failed on ADB loss. No live local build handles; goal active, progress, no commit.

## CURRENT — real GravityMark launch, no valid comparison, 2026-10-22

GRAVITYMARK_2026-10-22.md has reproduction/boundaries. Official1.89 Android
hardware scene works, no completed score. Separate Wine desktop08: D3D11
needs FL11_1 vs Sarek11_0; Vulkan desktop assets require BC7 absent in that path.
Separate Android-ASTC asset experiment shows scene but RGBAf16 errors and Mali
GROUP_ERROR_FATAL; one fullscreen control repeats faults. Stopped all isolated
Wine benchmark processes. Desktop08 READY1280x720; no production changes.
Do not quote cancelled65FPS or native transient45.7 as a valid comparison.
Artifacts E:/Mavk/artifacts/gravitymark-1.89-20261022/wine-logs.tar plus images.

## CURRENT — short D3D11 benchmark, 2026-10-22

QUICK_D3D11_BENCH_2026-10-22.md: existing Wine/Sarek/Mavk hardware path,
same tiny scene ARM64 vs AMD64, corrected actual2560x1600 45.47/46.42
Present-return/s; AMD64 actual1280x800 59.86. Not unique visible FPS or GPU-time.
Initial window-size mismatch invalidated apparent FEX disparity; fixed/retested.
Thermal3. No game overrides/runtime/APK changes. Isolated old Wine prefix reused.
GPU timestamp layer built in muck-gpu-timestamps-20261022, NOT tested/staged;
opt-in code needs regression before use. New-feature pause/no commit still apply.

## CURRENT — WSI cost split and cached-memory A/B, 2026-10-22

WSI_READBACK_PERFORMANCE_2026-10-22.md records real game measurements. Largest
cost is rendered-image fence wait~86–91ms, NOT a measured GPU timestamp. Copy
~20ms, X/map/pacing small. Actual compatible types0flags7(coherent uncached)
vs1flags11(cached noncoherent). Isolated cached candidate888609b2 uses HOST_READ
barrier in existing fence submit + invalidate; copy12.18ms vs reversecontrol20.79.
Android9.1->9.5->9.0 buffer updates/s,thermal3: NO stable overall FPS gain claim.
Normal changing Muck menu/control, no observed loss/rejection;not CTS/pixeloracle.
Do not promote candidate or repeat copy variants. Need real GPU execution
timestamps vs queue serialization before next main-bottleneck change.
All per-game overrides restored,Muck quit,installedWSI d07b9611 exact,APK25
unchanged,Steam06 stays. New-feature pause still applies. Goal active,PROGRESS.
No running handles,C50.8/E835.5GiB. Artifact root E:/Mavk/artifacts/wsi-profile-20261022.

## CURRENT — real-game attribution, not an FPS fix, 2026-10-22

See MUCK_PERFORMANCE_2026-10-22.md. Common menu9.6–11.3 Android buffer updates/s;
FEX+ARM64 Wine/DXVK,not Box64; Muck~69–72% of one CPU core. Thermal3/skin46.5C
confounds performance. Guard recording~60–79ms wall/s; native submit~5–7ms/s;
long fences/timeline waits overlap and cannot be summed. Actual game output is
X11 SHM (forced),not direct GPU buffers; final Android composition~2.1ms/frame.
Next isolate render-ready vs readback/map/copy in WSI,not new Vulkan features.
Opt-in profiler fixed shared-report reset and now measures timeline/core+KHR,
fences and submits even when WSI consumes Present. Candidate39b0a23c,12GPU
regressions PASS11.15s,real Muck measurements. NOT a performance improvement.
Muck quit normally,profile0 restored,override removed,aggregate89c8 hash restored.
APK25/common runtime unchanged,Steam06 stays. No live handles,goal active.
Evidence E:/Mavk/artifacts/muck-performance-20261022/performance-evidence.tar.gz.

## CURRENT PRIORITY — Muck performance, 2026-10-22

User paused NEW Vulkan feature work after completed TES->GS gate. Do not start
the discussed existing-GS point compiler. Shared-runtime strategic goal remains
active. Sequential real-game performance work, no parallel agents/optimization.
Initial installed APK25 normal menu:2560x1600 IMMEDIATE swapchain4images;
Surface20.1s228buffers11.3updates/s,120Hz panel. This is not unique-content FPS
or an optimized result. Actual nativeARM64 Wine/DXVK+FEX,not Box64.
Warm thermalstatus2/skin~42C noted; matched measurements required.
Simpleperf under app UID denied perf_harden; DO NOT change the suggested security
property. Read-only /proc CPU attribution + existing opt-in layer profiler next.

## CURRENT — CullDistance after TES PASS, 2026-10-22

Shared compiler supports explicit TES producer; experimental adapter original
VS/TCS/TES/FS ->5native stages. Actual tessellation level2 fan,partial cull:
288reference/96surviving pixels,98304bytes4modes PASS. Host11valid/8negative;
previous PrimitiveId4modes+26GPU regressions PASS. Shared renderer TCS/TES path
added, ordinary new-renderer cases included. Layer89c8c463/consumer8efd7116;
Muck4118 menu/Settings/input PASS. CULL_TESSELLATION_2026-10-22.md has scope.
Steam05 ended normally0; resumed existing runtime Steam06 READY,guest998.
Battery100% freshly observed (older14% historical). Game quit,override removed.
APK25/runtime4de26844 unchanged; public cullfalse,existing-GS scope remains.
No live handles. C51GiB/E835.6GiB free. Goal active,PROGRESS,not blocked.

## CURRENT — CullDistance preserves FS PrimitiveId, 2026-10-22

Common compiler forwards original GS input ID before every emission,including
proper component-limit accounting. Actual original-VS/FS loader hook with
three colored primitives/culling holes/repeated draws:393216bytes4modes PASS.
Prior base4 Block4modes +26GPU regressions PASS;host10valid/5negative PASS.
Final layera4dc8959/consumera3662043;details CULL_PRIMITIVE_ID_2026-10-22.md.
No game rerun on this candidate. APK25/runtime4de26844 unchanged,no override.
Public cullfalse,remaining GS/TES/other admission gaps explicit. Goal active,
current/previous turn PROGRESS,not blocked. Last battery14%,USB discharging;
do not confuse old80s% historical value with current state. No live handles.

## CURRENT — CullDistance block-variable Location PASS, 2026-10-22

Shared compiler now preserves ordinary whole-Block Location,including nonzero
base4 and implicit array/member footprints. Candidate5774b8bd: original API
hook4modes/131072bytes PASS; previous member-block4modes/131072bytes PASS;
host9valid/5negative and26priorGPU regressions PASS. No Muck rerun for this
candidate; previous8657 integration proof is separate. APK25 unchanged,cull
publicfalse,no override. See CULL_BLOCK_LOCATION_2026-10-22.md. Goal active,
previous/current turns PROGRESS; stage/admission work remains,not blocked.

## CURRENT — CullDistance user Blocks PASS, 2026-10-22

Common compiler now preserves member-located user Blocks into/out of inserted
GS: vec4 + flat int + vec2 array checked by real FS. Original API-hook pipeline
4 modes/8 cases/131072 bytes PASS; host8valid/5negative and all26 prior GPU
regressions PASS. Candidate Muck5722 menu/Settings/input PASS; quit normally,
override removed, Steam left. Candidate8657e17b; APK25 unchanged, public cull remainsfalse.
See CULL_USER_BLOCKS_2026-10-22.md. Remaining block-variable Location/invariant
and stage contracts are explicit rejects, not advertised support.

## CURRENT — experimental CullDistance pipeline hook PASS, 2026-10-22

Candidate layer47de8943 automatically transforms original2stage VS/FS at
vkCreateGraphicsPipelines into3native stages using common compiler. FourGPU
modes descriptor-free/UBO x tracking/guarded:131072bytes PASS. UBO+guarded uses
normal metadata/guard pipeline;native geometry disabled explicitly rejects
before generated driver pipeline. Host7valid/5negative PASS. All26 previousGPU
cases flagOFF PASS,visible candidate Muck30516 menu/Settings/input PASS,no
observed gap/compile/device-loss;game quit normally,override removed,Steam left.
APK25/runtime4de26844/source assets/archive pins unchanged. No new APK,commit/
push or U-AVF changes. Private flag only;public shaderCullDistance still0.
This diagnostic original-builtin admission is NOT public Vulkan conformance
or ordinary DXVK PASS. CULL_PIPELINE_LINK_2026-10-22.md has scope/artifacts/gaps.
Next real linked-interface/stage contracts;do not repeat closed eight triangles.
Current/previous turns PROGRESS;goal incomplete,not blocked. Older entries historical.

## CURRENT — whole-primitive CullDistance implemented/proven slice, 2026-10-22

New common compileCullDistance API transforms VS outputs and generates native
GS whole-primitive culling,not fragment discard. Real G925 via existing APK25
loader:8cases/2planes/32768bytes each tracking+guarded,65536total PASS against
independent no-GS reference;zero and mixed signs preserve full image,negative
whole plane culls,restore PASS. Native shaderCullDistance0/requested0,geometry1.
Initial split interface failed;mirroring actual VS output Block into GS input
fixed it. No general vendor-bug claim. Final generated bytes equal tested GS/VS.

Host6valid/5negative PASS;newly compiled shared renderer default routing+image
GPU regressions PASS. Optional RasterOptions.geometryShader empty by default.
APK25/runtime4de26844 remains installed unchanged;no new layer/archive pins or
production U-AVF changes,commit/push. No public feature bit enabled. Full
GS/TES/FS-cull/point/line/user-block/Bridge/ICD/pipeline-hook admission incomplete.
CULL_DISTANCE_2026-10-22.md records scope/artifacts/remaining integration.
Next pipeline-link integration for actual consumer,not more abstract Suite.
Current/previous goal turns PROGRESS;goal incomplete,not blocked.
Older CURRENT entries below are historical snapshots.

## CURRENT — APK25 optional trace and measured recording overhead, 2026-10-22

Installed APK044f33a9/runtime4de26844/layer60e0a2eb. Hot execute/update/template/
secondary messages off by default;MAVK_LAYER_TRACE=1 restores them. Errors and
summary counters always retained; guardReports/profile independent. No shader
semantics/compiler/feature changes. Six controlled consumer runs,12measured
samples/mode:median3290.5ns/command trace0 vs6587.3 trace1,ratio2.002. Record-only
wall time, NOT visible-FPS/GPU/game-speed claim. Each process submits one real
nestedGPU guard test with correct output; missing-used-set rejects with visible
GAP even trace0. Full26 isolated and26COMMON GPU gates PASS; source assets synced.

Normal Muck20923 maps4de26844,visible menu/Settings/input PASS,no observed gaps/
compile/device-loss or hot-event messages in saved logs. Quit normally;Steam05
left running,no per-game override. APK24 rollback retained. TRACE_POLICY_2026-10-22.md
has exact artifacts and scope. No CTS/full-feature claim,commit/push or U-AVF
changes. Current and previous turns PROGRESS;goal incomplete,not blocked.
Bounded trace task finished;next actual consumer P0/P1,not more trace tests.
Older CURRENT sections historical.

## CURRENT — APK24 diagnostic overhead fix verified, 2026-10-22

APKbe073d18/runtime0d2d96b4/layerda2ac31c installed-r SUCCESS. Reports disabled
now skips diagnostic invalidate/map/read and per-generation stderr; access
guards remain active. Enabled reporting still reads22OOB diagnostic words on
existing nestedGPU workload. Baseline9c00e7d4 reproduces unwanted log with0.
All26 isolated GPU cases PASS65.722sec; all26COMMON cases PASS61.695sec with
strict mapped-runtime and --no-diagnostic-reads gates. No performance claim
from suite duration. Old APK23 archived results fail the new log gate as expected.

Normal Muck16415 mapped0d2d96b4 payload, actual menu/Settings/input visually
verified, no GUARD_LOG or observed Mavk-gap/compile/device-loss markers in
saved log. Quit through Steam normally; session2026-10-22-04-steam left open.
Per-game candidate override removed, source runtime/version synchronized.
APK23 rollback preserved. DIAGNOSTIC_RETIREMENT_2026-10-22.md records evidence.
No CTS/visible-FPS/full-game/full-feature claims; no commit/push or production
U-AVF changes. Goal incomplete, previous/current turns PROGRESS, not blocked.
Older CURRENT headings below are historical snapshots.

## CURRENT — APK23 common/runtime and visible integration PASS, 2026-10-22

Installed APK83561ddb/runtime62fcf634/layer9c00e7d4. Actual common-runtime26GPU
cases PASS28.708sec with strict mapped-runtime verifier. Native R/RG/RGBA UINT
and SFLOAT view metadata, null/restored cases PASS18phases/1872words; prior
regressions preserved. Isolated6aggregate forms also PASS. Normal Muck11336
mapped62fcf634 payload, menu/Settings/input visually PASS;245188 guard-log
records, no observed Mavk-gap/compile/device-loss markers. Quit normally;
Steam running session2026-10-22-03-steam, account intact, no per-game override.

Source runtime assets/version synchronized. Default frontend9d34b154 and
optimizer1f1f274a match new IndexInitFormats ABI. APK22 retained for rollback.
TEXEL_FORMAT_METADATA_2026-10-22.md has hashes/evidence and earlier failed
candidate/probe history. Probe839434c3 is diagnostic ONLY, never package it.
No full robustness2/nullDescriptor/BC feature claim, CTS or measured FPS.
No commit/push or production U-AVF modifications. Goal incomplete, progress.
Older CURRENT sections below are historical snapshots, superseded here.

## Latest isolated fix — texel component substitution, 2026-10-22

APK22 remains installed/runtime1e859af6;default still uses verified atomic
optimizer1040dcb5 + frontend8b1d0db1. Do not confuse candidate
with installed archive pins. New isolated layer8470144c uses optimizer1f1f274a.
Actual APK22 FAIL-before:OOB R32_UINT imageLoad alpha0 instead of1. Candidate
GPU PASS192words,valid/OOB/negative/null/restored;no CPU pixel-copy/waits added.
All25GPU cases strict verifier PASS61.834sec;6forms1152words PASS15.229sec.
Muck1096 actualcandidate mapping/menu/Settings/input PASS;176678guarded commands,
no observed gap/compile/device-loss errors. Log archive3bc01f24ccff191d0fa0a39a769f574ae3c41e629f1b33b6b9e4df78aee3053f.
Artifacts:E:/Mavk/artifacts/texel-components-gpu-20261022/.
Game quit normally,per-game override removed;Steam left open. No active build/
test handles. No new APK/default/source-runtime replacement,commit/push.

Scope:known non-alpha storage-texel formats use typed(0,0,0,1) on OOB only;
null/atomic results retain zero. Unknown formats including samplerBuffer still
need runtime view-format metadata/specialization;full feature bits/CTS unproved.
TEXEL_COMPONENT_SUBSTITUTION_2026-10-22.md records patches/hash/prechange backup.
Do not rerun closed DeepSeek atomics or grow rare atomic variants. Independent
copy report reviewed with actual hashes/dumps,not blindly accepted or merged;
INDEPENDENT_REPORT_REVIEW_2026-10-22.md records D-verifier and scope caveats.
DeepSeek build-host generated files moved toE:/Mavk/archives/deepseek-builds-20261022/
with original C:path junction preserved;no deletion,source/test data unchanged.
C:~50GiB free. Prior turn and this turn PROGRESS,not blocked;goal incomplete.

## CURRENT installed baseline — APK22, 2026-10-22

APK6525353cd72c22cf750705a7ca2c1407c58f190944fbd57876c633e24c042dfd
installed-r SUCCESS with existing data/Steam account intact. Runtime
1e859af69e957fc6e5f59f7b1a5785b8037fc1cd8684a56b5c22797873d659ca,
layerca87ea54. All24 COMMON GPU scenarios PASS using that actual runtime mapping
(42.437sec); strict --storage-atomic --common --runtime-sha256 verifier PASS.
Six aggregate forms/1152words passed on identical candidate before rollup.
Source runtime assets and default frontend/optimizer pins now synchronized.
Same dev certificate,16KB alignment/signatures PASS,only2APK assets changed;
485other members identical. Runtime only layer+manifest changed,261unchanged.

Normal Muck30542 mapped1e859af6 payload,visible menu/Settings/input PASS;
225068guarded commands,no observed Mavk-gap/compile/device-loss errors in saved
game log. Log c06ec080aaefde118493fb7e024e48bc6f155fb93c01d30c5728173c309f906b;
common GPU archive513a1be8efdba085a5734261094d2631ad6eba0f9c3186cbbf008f42624900b9.
Artifacts:E:/Mavk/artifacts/droiddeck-atomic-apk22-20261022/.
Muck quit normally,Steam left running session2026-10-22-02-steam;no per-game
layer override. No active build/test handles. APK21 rollback preserved.
This turn is PROGRESS,not blocked. Goal incomplete. No commit/push.
No unique-FPS/full-game/ordinary-DXVK/CTS/fullrobustness2 claims.
Next:stop broadening atomic variants;select next concrete P0/P1 beyond this
finite shader/buffer package from actual consumer requirements,not rare cases.
Do not rerun already-passed native/atomic/common rollup gates without changes.

## Latest isolated candidate — guarded texel atomics PASS

Layer ca87ea54 / frontend8b1d0db1 / optimizer1040dcb5: real R32_UINT compute
atomics, five phases/320 words PASS; prior23 GPU cases PASS, gaps0/CoreliveZero1.
Host atomic structure22 assertions and existing compiler gates PASS. Native
scope/semantics retained, OOB/null zero without side effects, restored valid PASS.
Only the bounded tested slice is proven; no full feature advertisement, CTS NOT
RUN. Installed APK21 unchanged. Remaining candidate integration gate: visible
Muck with candidate, then restore per-game selection. Details and artifacts:
STORAGE_ATOMICS_NEXT_2026-10-21.md. Older 'opcode60 rejected' notes are historical.

## CURRENT installed baseline — APK21 (supersedes all older checkpoints)

Next-operation prerequisite now hardwarePASS:native R32_UINT texel atomic
add/exchange/compare-swap/read,64words correct,hybris-only with LD_DEBUG proving
Mavk layer not loaded. Consumer48329a97/artifactsstorage-atomic-native-20261021.
STORAGE_ATOMICS_NEXT_2026-10-21.md records exact next proper reference/clone
implementation. Opcode60 remains rejected;no image-atomic compatibility claim.
No active build/test handles remain;Steam left running,per-game overrides absent.

APKc5f68760/runtime8de92183/layerc2a12a21 installed-r;23COMMON GPU cases PASS.
Visible Steam hardwareANGLE/Zink/Mali and ordinary Muck26715 menu/Settings/input
PASS with actual8de92183 payload mapping;816873guarded commands,no observed
Mavk errors. Game quit via client,Steam session2026-10-21-15-steam stays running
READY/firstFrame=true. No per-game candidate override. Source DroidDeck assets
and default layer build now use8de92183/frontendf0a0c057/optimizer0d8a0607.
APK20 rollback preserved. STORAGE_TEXEL_2026-10-21.md and
BUFFER_PACKAGE_CHECKPOINT_2026-10-21.md record finite scope and remaining gaps.

Both latest turns are PROGRESS:actual shared shader fixes,hardware validation,
visible real-consumer checks and normal APK integration. No blocking condition.
Strategic goal remains incomplete;do not mark complete. Full robustness2,
nullDescriptor,BC feature contract and ordinary upstreamDXVK still unproved.
CTS NOT RUN. Next concrete missing operation:guarded storage-texel/image
integer atomics (OpImageTexelPointer currently explicit reject). Do not deepen
equivalent descriptor/array cases. First isolate nativeR32 atomic capability,
then extend proper reference/clone/bounds analysis;don't spoof feature bits or
replace atomics with non-atomic reads/writes. No root/production/commit/push.

## New installed baseline — APK20, supersedes checkpoints below

APK20 f186cb72 installed-r;data preserved. Runtimebfc10a4c/layer743d9d6c:
22common GPU cases PASS,visible Steam library+hardware Zink/Mali,ordinary Muck
launch with NO per-game layer override mapped new runtime and showed working
menu/Settings/input. RollbackAPK19 retained. Source DroidDeck assets nowbfc10a4c;
common builder defaults copied-pointer frontend872c57ec + optimizer0d8a0607.
No CTS/fullrobustness2/unique-FPS claim. SurfaceFlinger menu baseline13.3 vs
candidate12.8screenupdates/s is NOT controlled unique-game-FPS measurement.

Next actual bug found by storage-texel read/write fixture:signed index operands
in generated uint offset math caused invalid UMulExtended (uint4,int0). Shared
saturation pass now bitcasts same-width operands to unsigned before extended
math,preserving original bits/application math. HostFAIL before/PASS after;
storage compiler5checks,texel5,copy6/1,nested4,static14/7,prior25/9 PASS.
Frontendf0a0c057 built;storage-texel-layer candidate building. Not inAPK20.
GPU test controlled by MAVK_STORAGE_TEXEL=1;don't expect APK20 to pass new case.
Do not publish/commit or change production U-AVF/VMs.

## Current checkpoint — 2026-10-21, supersedes historical entries below

Installed baseline remains APK19/runtime73ec2df4 (nested/overflow/wide-index
guards),17common GPU cases PASS. Aggregate/plain-CopyMemory candidateb46cfb43
has19GPU cases plus960aggregate words PASS and real Muck headless regression:
D3D11device/swapchain/202826guarded commands,no observed Mavk gaps/device loss.
Game stopped and per-game override removed. No visible-frame/FPS claim.

Vulkan1.2 admission/static-use target and CopyMemory distinct source/target
common memory operands now Mali PASS:c41b2f2d/frontend9339061f,20cases plus
6forms/1152words. Bounded native vertex gate PASS in protected and hybris-only
controls:2formats/3firstVertices/1536components. No new vertex emulation needed
for tested paths;not full contract. VERTEX_ROBUSTNESS_2026-10-21.md.
Next actual bug:buffer-pointer OpCopyObject bypassed bounds analysis.Host
fixture FAIL before normalization/PASS after;4checks,prior25/9,copy6/1,static14/7
PASS. Layer804d9271/frontend872c57ec:21GPU cases+6forms/1152words PASS. Real
Muck9528 loaded candidate,D3D11 FL11_0/81534guarded commands,no observed Mavk
errors. Headless only;game stopped/override removed. COPIED_POINTERS_2026-10-21.md.
No APK20/CTS/feature-bit claim. Remaining full buffer/texel/vertex contract is
still incomplete;do not equate these fixes with ordinary DXVK compatibility.
Do not deepen P2 descriptors. Continue concrete upstream P0 robustness contract;
full vertex/texel semantics remain open. Android keyguard is not GPU failure.
No production/Windows/Ubuntu changes or commit/push.

## Latest candidate — aggregate buffers, not installed

CopyMemory GPU result now confirmed:candidateb46cfb43/frontend6e330b19.19cases
PASS plus5aggregateforms/960words,gaps0/Corezero. AGGREGATE_BUFFERS_2026-10-21.md
contains exact artifacts/limitations. Candidate not installed; commonAPK19 stays
73ec2df4. Next application regression requires actual visible Android session;
offscreen Core development can continue independently. No CTS/fullfeature claim.

Shared aggregate lowering implemented. Struct/array/column-major matrix/common
memory flags PASS on Mali,768words; basiccase+17priorGPU regressionsPASS. Layer
9c9d6680/frontend62cb42d1. AGGREGATE_BUFFERS_2026-10-21.md records scope.
Plain CopyMemory lowering now implemented/hostvalidated; its GPU build pending.
InstalledAPK19 remainswide guardbaseline73ec2df4. Steam's hardware ANGLE/Zink/
Mali queryPASS,but visible frames blocked by Android keyguard,not a GPU PASS.
Temporary power/suspend settings restored. No commit/push/fullfeature claim.

## Latest — APK19 buffer-guard runtime installed

Nested chains,32-bit generated offset overflow and64-bit index narrowing fixed
in shared compiler. Mali wide unsigned/signed/valid controlsPASS; all17 GPU
casesPASS isolated AND installed common73ec2df4. Layer9bab9c9c,APK0f77d072,
rollbackAPK18. Source assets and common build archive updated. No robustness2
advertising/CTS/FPS claim; aggregate/vertex/texel contract stillincomplete.
BUFFER_ROBUSTNESS_2026-10-21.md records scope/artifacts. Steam13 currently
repairing client/download; UI/Muck underAPK19 not yet verified. No commit/push.

## Latest shader/P0 implementation — 2026-10-21

Second proven gap fixed: generated32-bit byte-offset arithmetic overflow could
wrap huge indices into accepted ranges. Shared compiler saturates generated
multiply/add offsets without changing application math. Mali multiply/add cases
PASS +12 existing scenarios PASS,gaps0/Corezero. Candidate7f6aa9f3/frontend6bb1b750;
BUFFER_ROBUSTNESS_2026-10-21.md. Common build default updated,installedAPK18 unchanged.
64-bit truncation/aggregate/full vertex/texel contracts stillopen; featurefalse.

Short audit selected ordinary-DXVK robustBufferAccess2. Proven nested buffer
AccessChains bypassed existing range instrumentation. Shared compiler now
canonicalizes chains before guarding. Mali read-zero/suppressed-write/sentinels
PASS,11 prior GPU scenarios PASS,gaps0/Corezero. Host25/9+new4+static14/7 PASS.
BUFFER_ROBUSTNESS_2026-10-21.md records artifacts and remaining contract; no
robustness2 advertisement/CTS claim. APK18 common baseline unchanged.
This supersedes the image-atomic-next suggestion below; that P1 remains deferred.

## Latest image P1 evidence — unchanged APK18

Standard Vulkan BC1RGBA path:3mips/2layers,16UNORM/sRGB views,256readback
components,padded upload/after-record host writes/compressed image copy match
native/protected. Guarded16draws/18submits,gaps0/Corezero. Consumer9aa33bd9;
IMAGE_SUBRESOURCES_2026-10-21.md. No CTS/SyncVal/FPS/generalBC claim or runtime
modification. Descriptor checkpoint remains finite; next operation-level shader
audit targets explicit image-atomic rejection rather than more descriptor P2s.
Existing common Muck/Steam baseline unchanged,not universal regression freedom.

## Priority checkpoint — cross-subsystem P0/P1, not exhaustive descriptors

Broader UAB work deferred per user; no implementation/advertisement was added.
Actual Sarek source uses existing native sampler heap; Steam session10 logs
confirm tracking mode. Separate general image/buffer UAB and bindless Zink remain
unimplemented in guarded path. DESCRIPTOR_CHECKPOINT_APK18.md summarizes bounded
support and CTS NOT RUN. COMPATIBILITY_PRIORITIES_2026-10-21.md records current
Sarek/upstream/Zink requirements, P0/P1/P2 across systems and next image/format
package. Muck regression does not prove no regressions in other games. Future
performance sequence is Muck then HL2. Windows/Ubuntu remain untouched.

Fresh read-only app-owned requirements differential completed using existing
consumer mode (no VkDevice/GPU work): ICD-only vs common default vs D3D BC profile.
Raw clip0 becomes1 from inherited hybris rewriting,not native hardware proof;
BC aggregate stays0,default16format admission differs from D3D14format admission.
Native UAB subfeatures1 do not prove protected UAB support. Query evidence under
consumer-requirements-20261021; priority doc records exact scope. Existing common
APK18/libraries unchanged. Next actual-API image/subresource/format correctness
package selected; no more descriptor deepening or new game launch now.

## Latest — 2026-10-21: real RGBA8 texel gap corrected

Shared native-view byte metadata now covers RGBA8 family,not new native feature
or typed-null support. Old common219b5bf0 fails SAME format37 real-loader test;
candidatebc30f541 native/protected UNORM/UINT512components PASS,WHOLE_SIZE with
nonzero offset/two-byte tail, padding unchanged. Host53/39+9descriptor suites;
all previous layer gates including pending/reusable and immutable/groups PASS.
Muck7942 maps candidate,80209protected/gaps0/device-lost0; actual menu captured.
Override removed/game stopped. APK18 SHA23de7423/runtime161fe2e8 installed,
asset-only/same certificate, source fork assets match. APK17 rollback retained.
COMMON texel oracle512components PASS/Corezero. Big Picture session10 READY,
Steam11074 has gamepadui/steamdeck args,sessiongaps0,hardware ANGLE/Zink/Mali.
One successful startup,not proof exact old crash cause or exhaustive stability.
Muck12020 maps COMMON161fe2e8:11569protected/gaps0/device-lost0 at capture;
game stopped, Steam running, no per-game override or suspend preference change.
RGBA8_TEXELS_2026-10-21.md scopes evidence and remaining unsupported cells.
No commit/push or production U-AVF/Windows/Ubuntu changes.

## Latest — 2026-10-21: selected bindings and grouped SPIR-V GPU PASS

Device returned. Final isolated layer219b5bf0 passes strict native/guarded
selected512words (seven unwritten UNUSED bindings/eleven dynamic offsets),
used-unwritten rejection before submit, dynamic/empty/unused/routing regressions.
Grouped-decoration reflection and guard compiler bugs reproduced and fixed
generically: resolve descriptor groups, then strip debug/flatten groups before
the pinned bindless pass. Strip debug avoids a proven pinned flatten-pass
OpMemberName infinite loop. Grouped immutable sampler control and protected
copy/template/layout oracle both pass192components, gaps0/Corezero.
Host compiler25/9 and reflection formats6/negatives10/groups11 PASS. Fresh probe
Core5gates/13rows, image variants, generations/templates and BC regressions PASS.
No CTS/general feature/visible-FPS claim. Muck isolated141349protected executions,
gaps0/device-lost0; override removed. APK17 SHA1c5fa1ce/runtimeacda20c7 installed,
asset-only update; common oracle512words PASS, Muck12065 maps COMMON with25059
protected executions/gaps0/device-lost0. APK16/d1ad rollback kept. Steam Big
Picture crashed on unchanged APK16; ordinary desktop Steam hardware ANGLE/Zink/
Mali works. Game stopped; original suspend preference restored. No auth changes.
Evidence and exact hashes: SELECTED_BINDINGS_2026-10-21.md. No commit/push or
production U-AVF/Windows/Ubuntu changes.

D4A subsequent test-only checkpoint: native/protected reusable commands and
two truly pending SIMULTANEOUS_USE submissions behind native timeline gate PASS,
matching512words, reset after both completions,gaps0/Corezero. Existing layer
retention sufficient; no runtime mutation/late-update flag widening. Details
DESCRIPTOR_LIFETIME_2026-10-21.md. General UAB/partial arrays still missing.

## Historical preparation — used-binding source candidate

Implemented selection-aware snapshots + cache/ownership checks, separate full
native allocation schema, complete dynamic-offset slots and selected-stage image
reflection. New host12checks/16negatives and nine rebuilt descriptor suites PASS.
Layer0b32e738, final consumer2e18fb21, probe0688126f compile; prepared isolated
payload under E:/Mavk/artifacts/selected-bindings-gpu-ready-20261021.
GPU/SyncVal/CTS/game/performance NOT RUN for candidate: ADB lists no devices.
Installed experimental APK16/commond1ad is unchanged and remains known-good.
No promotion/feature flags/commit/push; production Windows/Ubuntu untouched.
SELECTED_BINDINGS_2026-10-21.md records matrix, hashes, exact next device gates,
independent D4 audit and recoverable C:-to-E: intermediate-artifact offload.

## Latest — 2026-10-21 08:53 Moscow: unused nonempty sets

Shared validated SPIR-V static-use analyzer selects entry name+execution model,
walks reachable calls/ID operands, resolves decoration groups. Whole unused
nonempty sets now skip snapshot/allocation/binding without renumbering or dummy
resources. GPU native/protected512words+1024components PASS; missing used set2
rejects pre-submit. Host14/7, ARM64 earlier fixture8/7. Prior layer regressions,
fresh Core/image/generation/template/BC bounded gates PASS. No CTS/full feature
or Windows ICD claim; UNUSED BINDINGS inside USED sets remain missing.

Layer f9b2960e, final consumer ae89643c, common runtime d1ad4522 installed.
Muck15053 mapped candidate;220054protected executions at capture/gaps0/device
lost0, no visible-FPS claim. Game stopped and per-game path removed. APK16
SHA5097806c installed in place, asset-only signed rollup/no data clear. COMMON
oracle512words/1024components PASS. Muck18364 maps COMMONd1ad,113581protected
executions/gaps0/device-lost0. Steam hardware Zink/Mali preserved. Game/probe
stopped and original sleep preference restored absent/default manual.
APK15/common24b4 preserved as rollback. Exact
hashes/evidence/scope STATIC_USE_2026-10-21.md. No production changes/commit/push.

## Latest — 2026-10-21 08:29 Moscow: D3 empty layouts, APK15

Empty layouts/zero-count reserved bindings now register as genuinely empty
state. Empty public slots retain numbering and need not be bound. Mixed0/2
resource sets + empty1 GPU512words PASS. Three all-empty public layouts and
descriptor-free rendering match native1024components (72red/184black pixels).
Test verifies real layout injection, not a renamed ordinary pipeline. All-empty
pipelines bypass guard compiler/metadata adapter; no dummy resource created.
Bound snapshots of absent placeholders still reject; registered empties differ
from null descriptors. Old bound-negative contract preserved (CPUreg caught an
over-broad acceptance before candidate promotion). Native adapters keep explicit
nonempty resource requirement to avoid back() on empty vectors.

Initial A/B pixels matched but native observer logged one guard-cache/set-limit
gap on descriptor-free reference. Strict verifier REJECTED that result. Fixed
descriptor-free handling in BOTH modes, avoiding pointless compiler at sets0;
final native/guarded stdout identical,gaps0/Corezero. Do not reuse initial logs
as PASS. EMPTY_LAYOUTS_2026-10-21.md scopes/hashes/evidence.

Final layer dd714682398f7b3224711daa27926d36cc495a36ce0dcaa2cc72c0a00a130167.
Dynamic512words,immutable192components,routing512words/8192components regressions
PASS; host9suites PASS including empty9/7 and all old bound6negatives. Fresh
Core5gates/13rows,images2D/live-array/3D/cube,descriptor generation/template and
BC123/45/67/texture regressions PASS with their own validation/SyncVal gates.
No guarded-loader CTS/SyncVal/FPS claim. Runtime compiler/atomic modes not rerun.

Installed asset-only signed experimental APK15, same cert;485other APK members/
261other runtime members unchanged. SHA256
4c6e30ea3542f6a9e0433580250a725c7bcfcf8ccfc99e7dea99f1e733dc0330,
E:/Mavk/artifacts/droiddeck-empty-apk15-20261021/DroidDeck-Mavk-empty-20261021.apk.
Common runtime24b402d336a21afda3f2bd212037396689e335789d3ec704173ffd07f5f2d455,
source asset hash/version match. APK14/common1f35 retained for in-place rollback.
Not a full Gradle rebuild. Session2026-10-21-04-steam loaded common24b4;
common-library empty oracle PASS512words/1024components,gaps0/Corezero.
Normal Muck11382 mapped COMMON24b4,400444protected executions at capture,
gaps0/device-lost0. Steam CDP retained ANGLE/Zink/Mali-G925. Common proof/log
in common15-results.tar.gz. Stopped test game/probe, restored original sleep
preference absent/default manual via existing service, preserved Steam/data.
Earlier isolated Muck7524 mapped final candidate,54168protected executions at
capture,gaps0/device-lost0; stopped and override removed from both stores.
No new visible FPS/gameplay claim, no commit/push, production U-AVF untouched.

Artifacts E:/Mavk/artifacts/empty-descriptor-gpu-20261021, PASS in verified/ and
verified-results.tar.gz; initial results.tar.gz deliberately retains rejected
native-gap observation. Consumerd6803d0673ad65ac6c902e0444cef5093550faa5f3efaf4f1cf04c29f3710b4b;
gap shader25623f0223c25e2cffaae51b7579d8e3d4bc2e5705dc8895d3bab32e530533c6.

NEXT: statically unused NONEMPTY sets/declarations still require binding and
initialization under current snapshot model. Do not call this solved by empty
slot support. Establish entrypoint/static-use reflection before relaxing those
checks; then D4 submit-time/version-aware lifetime/UAB, no flag-only admission.

## Latest — 2026-10-21 08:05 Moscow: immutable samplers, native copy correction, APK14

General immutable descriptor handling implemented: owning sampler arrays/leases,
standalone allocation initialization, ignored sampler in combined writes,
destination immutable sampler preserved on combined copies, owning templates,
layout compatibility by sampler references. Native materialization skips illegal
writes to immutable standalone samplers. Native YCbCr conversion remains rejected.
Ordinary empty immutable state is shared, not heap-allocated per layout/set.

Found independently reproduced native G925 Vulkan copy discrepancy: direct
ANDROID/Bionic/no Mavk/no hybris, phase1 component0 actual108 expected18,
validation+SyncVal0/0/Corezero. Same-sampler native control PASS192components.
Therefore not a Mavk/FEX/Wine/DXVK/WSI bug or inferred screenshot artifact.
Protected common shadow corrects this descriptor semantic path; GPU192components
PASS across writes/copy/template/combined+separate samplers/compatible layouts.
Do not claim equal native/guarded PASS: native failure/control/fix are distinct.
Guarded glibc oracle has no SyncVal/CTS instrumentation; native Android does.

Final layer8b72f0d28c7df99a7d0bee1f56feb7232b39faf147a879110c5ecfb31d344184.
Dynamic512words/four dispatches and static routing512words/8192components also
PASS,gaps0/Corezero. Host8suites rebuilt PASS including immutable18/8 and dynamic
21/13. Fresh Core5gates/13rows, descriptor generations/templates,null2D/live-array/
3D/cube,BC123/45/67/texture regressions PASS. Detailed exact scopes/probe versions
IMMUTABLE_SAMPLERS_2026-10-21.md; no full feature/CTS/performance claim.

Installed experimental common runtime1f3566b1426d26fc065c452fb1950201ba0cfdbc0b3515cf90f561f2048727f5,
signed asset-only APK14 (not full Gradle rebuild), APK SHA256
ad41cbd5172f498227f756cb63382a7b826f583fca8b9f5443eb15199ac4f86c,
E:/Mavk/artifacts/droiddeck-immutable-apk14-20261021/DroidDeck-Mavk-immutable-20261021.apk.
Same signing cert,485other APK members/261other runtime members unchanged.
APK13/commonb047 retained for in-place rollback, source asset hash/version match.
Session2026-10-21-03-steam loaded common1f35; installed common immutable oracle
PASS192components/3protected dispatches/5total submits/Corezero. Normal Steam
Muck406 maps common1f35 with no diagnostic override;55084executions at capture,
gaps0/device-lost0. Steam CDP remains ANGLE/Zink/Mali-G925. Not visible FPS proof.

Stopped test game and completed probe, restored sleep preference to originally
absent/default manual via existing app service; preserved Steam/data. Artifacts
E:/Mavk/artifacts/immutable-descriptor-gpu-20261021 include native/control JSON,
final-results.tar.gz,native-proof.tar.gz,game-regression.tar.gz,common14-results.tar.gz.
Shared consumer50b35050694b6ffa840bbf2f46a58b4c2fcb72e7f4db5606e392a96b41f3cbf3.
Source native Android oracle reuses same templated test; cannot label the native
known failure a compatibility PASS. No commit/push; production U-AVF untouched.

NEXT D3: empty layouts/unused public sets. Current createLayout/registerSet and
DescriptorState reject empty declarations; prepareExecution requires every public
set bound. Preserve empty slots/native layout numbering without manufacturing
logical null descriptors, and allow truly unused empty sets without binding them.
Mixed nonempty sets still need normal snapshot/guard metadata. All-empty public
layouts/no descriptor-using shader must not force a nonexistent metadata adapter.
Then D4 legal late updates need submit-time/version-aware generations, not flags.

## Latest — 2026-10-21 07:36 Moscow: D3 dynamic buffers + common APK13

Dynamic UBO/SSBO arrays now preserve native types/offsets through the protected
layer, pool/write/bind, common update/copy/template shadow and resource leases.
Offsets consumed immediately, canonical binding order, partial set binds;
alignment/effective range validated. No buffer copy/hidden wait/feature-bit lie.
Native and guarded actual-loader GPU PASS512checked words,seven dynamic buffers,
two sets,four dispatches,two submissions,including WHOLE_SIZE/zero offset.
First three dispatches reuse one native generation with different offsets.
Final layer skips validation scans completely for static-only descriptor paths.
Static command-routing regression PASS512words/8192components,gaps0/Corezero.

Six descriptor host regressions + new dynamic21checks/13negative cases PASS.
Fresh Android Core five gates/13rows PASS. Fresh bounded 2D/live-array/3D/cube,
descriptor generation/template and BC123/45/67/texture GPU regressions PASS with
SyncVal0/0; prior screen-off fresh-suite gap resolved. Runtime compiler/atomics/
separate-sampler modes not rerun (probe compiled without runtime compiler).
No CTS; no full dynamic/nullDescriptor/robustness2 compatibility claim.

Final layer a96aa08724694a4173ebbe4d2a05d2ad9e5ab51c35e513fb21e2ce7fec15a4a6.
Experimental common runtime b047abed1abef32032569287f8dbc23393b7540d67aa0116c356a4c91fb32331.
Installed signed asset-only APK13 (same certificate;485 other APK members and
261 other runtime members unchanged), not a full Gradle build. APK SHA256
c26925e36a2c7c73b5638bcd629d989abe0cc69338f58b08b290a598a69dd0dd,
E:/Mavk/artifacts/droiddeck-dynamic-apk13-signed-20261021/DroidDeck-Mavk-dynamic-20261021.apk.
Old APK12/common9fb3 preserved as rollback. App data/games preserved.
Session2026-10-21-02-steam loaded b047; installed common-library GPU dynamic
oracle PASS512words,four guarded commands/two submits,Corezero. Normal Steam
Muck25086 mapped b047 COMMON payload (game layer override absent),98038protected
executions at capture,gaps0/device-lost0. Steam CDP retained ANGLE/Zink/Mali-G925
after update. Smoke log and common-library oracle in common13-results.tar.gz.
Not a new visible FPS/gameplay claim. Stopped only test game; removed temporary
sleep preference back to original absent/default manual and refreshed service.
Standalone probe stopped after completed regressions; Steam/data preserved.

Earlier same-day common APK12 reached READY/firstFrame; Muck19893 mapped common
library. Isolated initial20613 and final22274 candidates rendered protected work,
zero observed gaps/device-lost; final73039executions at capture. Not visible FPS
or new gameplay proof. Diagnostic game overrides removed before APK13 install.
Detailed scope/evidence DYNAMIC_DESCRIPTORS_2026-10-21.md; artifacts under
E:/Mavk/artifacts/dynamic-descriptor-gpu-20261021. No commit/push.
Next D3 immutable sampler ownership/compatibility and empty layouts; D4 requires
submit-time/version-aware late updates, not record-time snapshot admission.

## Latest — 2026-10-20 21:46 Moscow: common experimental runtime installed

Promoted vetted layer f614df284058d4f7941539e130b372fe2923a01c5e9b4ea663d159e405d4dab7
into common DroidDeck runtime9fb32fc768a0948573263c373df60643efb8858db017d92974e1ebe4d1c79cfe.
Only library+manifest changed;261 other archive members are byte-identical.
Removed Muck-only scratch layer override in both configuration stores, with
before-common-runtime backups. Games/account data unchanged.

APK12 is an asset-only signed rollup of installed APK11, NOT a full Gradle build.
485 other APK members byte-identical; signing certificate matches baseline.
Installed in-place, common runtime staged successfully. Steam CDP still reports
ANGLE/Zink/Mali-G925 hardware renderer. Installed common-library actual-loader
GPU test PASS512words/8192components,12prepares/12submits,gaps0,Corezero.
Artifacts and exact APK/hash/rollback: COMMON_RUNTIME_2026-10-20.md.

No new visible Surface/gameplay/FPS PASS: Android remains locked, fresh session
STARTING_STEAM/no firstFrame; requested Muck did not start, old game logs not used
as fresh evidence. Temporary suspendPolicy.steam=never removed back to originally
absent/default manual, service refreshed. Fresh standalone Core Suite still
unverified after prior cancelled screen-off attempt; do not silently commit a
fully-regressed release. No feature bits changed, no CTS, no commit/push.
Next D3 binding semantics, then D4 submit-time late-update/lifetime model; see
DESCRIPTOR_COVERAGE.md. Production U-AVF untouched.

## Latest — 2026-10-20 21:14 Moscow: protected command routing

Indirect-count draw/indexed draw + DispatchBase core/KHR/AMD wrappers now prepare
the same protected generation as ordinary draw/dispatch. Native and guarded
standard-loader consumer each PASS512words/8192components,2dispatch/8drawcases,
guarded12prepares/12submits,gaps0,Corezero. Core/KHR GPU tested; AMD unavailable.
Candidatef614df284058d4f7941539e130b372fe2923a01c5e9b4ea663d159e405d4dab7
in files/mavk-execution. Actual consumer/hashes/limits DESCRIPTOR_COVERAGE.md.
No feature-bit,transport,production change; no CTS/SyncVal/FPS claim here.
Muck16983 mapped it, active render work/no observed gap/device-lost. Android
locked: only background startup regression, NOT new visible-frame PASS. Stopped
game. Temporary existing sleep preference removed back to original absent/default
manual; Steam suspended, Android security unchanged. No commit/push.

Fresh Core Suite attempt produced no reports, worker alive/top-sleeping; normal
wake did not complete. Cancelled probe only, journal saved in
E:/Mavk/artifacts/layer-execution-regression-20261020. NOT fresh Core PASS; do not
reuse older reports as new. Backend's separate real-loader GPU evidence is PASS.
Next common-runtime staging + D3 semantics, with suite recheck when usable.

## Latest — 2026-10-20 20:48 Moscow: sampled null cube

Common reflection/backing/init now supports sampled non-array cube with real
6faces. GPU1792/512,14fallback readbacks,SyncVal0/0/Corezero PASS. Native image
query includesCUBE_COMPATIBLE; allfaces transitioned/cleared. Null slots only,
not live cube/full feature. Cube-array/depth/MS/storagecube stay rejected.
2D,array,3D,newCore5gates,descriptors/templates,BC regressions PASS. Six host
suites rebuilt PASS. Artifacts/scopes/hashes DESCRIPTOR_COVERAGE.md.
Muck12280 maps files/mavk-null-cube layere87b886e7e1a2e9ff764281f3c89d14de02fc438198845acb07b573eedbe10f9;
correct menu/no observed gap/device-lost. Game stopped, Steam suspended. No
production changes, no commit/push. Next high-priority gap: indirect-count draw
and DispatchBase native calls bypass guarded descriptor preparation; no PASS
for those commands until implementation + real GPU test.

## Latest — 2026-10-20 20:36 Moscow: generic null color3D

Implemented real3D fallback/view and reflection/layer dimension routing, no2D
substitution. GPU null sampled3D explicit LOD1792/512 PASS; sampled fetch plus
live/null R32 storage3D load/store1792/512 PASS. Both SyncVal0/0/Corezero; storage
extent8x8x1, no live sampled volume/mip/query/atomics/fullfeature claim. Cube/depth
and impossible3D-array reflection remain excluded. Feature queries unchanged.
New harness2D and live-array regressions1792/384 each PASS; rebuilt six host
suites PASS. Core5gates + descriptor generations/templates + BC regressions
freshly rerun. Details/scopes/hashes in DESCRIPTOR_COVERAGE.md; E: artifacts.

Muck maps files/mavk-null-volume layer02be618634697bb34cb2a3db0bf30e9b24bc75cc6d6e221abff6ac6dae9ae185,
correct animated menu, no observed gap/device lost. Two inspected frames/logs
saved. Menu regression only, not gameplay/performance PASS. Game stopped and
Steam retained; no DroidDeck APK/production U-AVF change, no commit/push.

## Latest — 2026-10-20 20:26 Moscow: D2 array read coverage

Real GPU live/null2D-array fetch AND explicit LOD PASS:1792words/384guards each,
two distinct view layers0/1 over physical layers1/2; sentinel physical layer0
must not appear. Two phases swap null/live descriptors; R32 storage write-discard
and cache/lifetime checks retained. All-null array new-shader regression1792/512
PASS. SyncVal0/0 throughout. `nullDescriptor` remains false. No interpolation,
mip,query,atomic,full feature or cross-GPU claim. Harness emits exact operation,
actual reports/expected accesses and live-layer/base-layer fields now.

Fresh Core5gates/13native rows, GPU descriptor generations/templates and BC123,
BC45,BC67,BC texture lifecycle all PASS. Reports and mtime freshness verified in
E:/Mavk/artifacts/descriptor-layers-regression-20261020. New layer shader/results
in E:/Mavk/artifacts/null-array-gpu-20261020. Details DESCRIPTOR_COVERAGE.md.
Only org.mavk.probe APK updated, not DroidDeck/production. Steam preserved,
no target-FPS experiment or Muck relaunch for harness-only changes. No commit/push.

## Latest — 2026-10-20 20:20 Moscow

FPS diagnosis bounded/ended per user. Actual Muck menu16.0 Surface buffers/s;
guarded CPU descriptor preparation107–143ms per~1s. GPU/WSI/runtime costs not
separated; no definitive performance root cause. ThermalStatus4 forces game
stop/cooling, not more benchmarks. Full Steam session remains, production safe.
Accepted profile run maps `files/mavk-profile-record` (SHA36b3c5dd61b69bf7df225ed01dcf8b7c79519fbf962f6c105b79b3c334814192).
Game-only env selection now persists both Android and guest settings; backup
`files/game-environment.json.before-muck-profile`. Profiling disabled for next
normal run; protection stays enabled, diagnostic report omission remains scoped.

Descriptor package: WHOLE_SIZE texel range truncation fix; six host suites
PASS(34/25,18/6,20/20,9/7,13/18,39/32). Existing null-array GPU oracle extended
to explicit-LOD sampling,1792words/512guards PASS, SyncVal0/0. Array sampled slots
both null; live multilayer/filter interpolation still untested. New shader
SHA7257c33459e6d09042e09f924e31984a74b02b8cba809f23712211bbca013943.
Fresh existing Core suite five source gates/13 native rows PASS, reports under
E:/Mavk/artifacts/descriptor-package-20261020. Not CTS/fullVulkan/guarded-set proof.
See DESCRIPTOR_COVERAGE.md for audited classes and dependency-ordered batches.
Muck is an integration regression, no more target-FPS pursuit. Mavk-only private
commit permitted after good fixed version; nothing committed or pushed yet.

## Current — real DXVK-Sarek consumer integration

- LATEST: null-array fix reaches actual gameplay; user confirms works but slow.
  Array GPU1792words/512guards and baseline2D1792/384 PASS, SyncVal0/0, truthful
  nullDescriptor=false. DroidDeck CRLF pacing parser fixed, tool tests30/30 PASS
  under Linux. Actual on-screen buffer rate8.8fps/10s (not unique-content FPS).
  Large guard-report cursor380337309words suggests diagnostic atomic contention;
  next A/B keeps protection, omits reports via immutable compile policy. Layer
  built0; no-report GPU oracle building, no performance fix claim yet. Game
  stopped cleanly, same Steam session kept. See DESCRIPTOR_COVERAGE.md for
  systematic shared descriptor work after A/B, not application-specific fixes.

- LATEST MUCK_2026-10-20.md: ordinary Steam Windows Muck reaches D3D11/Mali,
  audio confirmed by user, black pixels including X11 drawable. Traced real
  Mavk pipeline rejection: guard report writes require vertex stores/atomics0.
  Candidate preserves checks but omits generated logging on unsupported stages;
  compiler validation/cache/read-only tests PASS21/9 in app UID. Built isolated
  layer SHA644e2fa1...; Muck-only layer path corrected to retain custom hybris.
  SPIR-V1.6 unused-interface cleanup completes compiler25/9 PASS. Actual new
  layer maps verified; actual game X11 drawable now nonzero and3D background
  visible. Vertex-instrumentation blocker FIXED for observed consumer, no full
  nullDescriptor/game/FPS claim. Next Mavk buffer-view pNext/usage2 tracking gap:
  general implementation +texel regression14/19 PASS. Consumer reveals native
  RGBA32_SFLOAT view rejected by scalar-only registry: format109/offset1054208/
  range96. Shared 32-bit vector sizing now implemented, regression17/22 PASS on
  tablet. Layer958c9273 consumer confirms RGBA32 views/elements6. Next layout
  refusal fixed as shared registry/native-adapter policy with retained aspects;
  descriptor20/20 PASS on tablet. Layer8e06329d... deployed Muck-only and launched
  via Steam; screenshot image-layouts2.png now full3D lobby and UI. Next Mavk gap
  null-color-image shader type, then command-buffer end failure. User finished
  testing; diagnostic proves sampled2D-array. Generic shape-aware fallback
  implemented, reflection6/4 PASS. Array GPU oracle initially failed only fixed
  report-word-stride assumption: actual2816=256*11, SPIRV writer confirmed11;
  current and host compiler outputs SHA93fe912f... identical. Oracle now parses
  bounded10/11-word reports, verifies null-init code and per-invocation coverage.
  Probe4 built0, GPU rerun in progress. No array/gameplay/unique-FPS PASS yet.
  DESCRIPTOR_COVERAGE.md records class-wide user priority and audited gaps.
  Steam Mali renderer rechecked unchanged.
  No game-specific shader flags. Classify next
  failure by layer; preserve universal correctness priority. Steam baseline
  untouched. SuperTux deferred. No commit/push.

- 2026-10-20 user installed Muck (1625450): now primary real-game consumer;
  SuperTux deferred. ADB online, Steam session01 READY, no restart. Installed
  Muck is Linux x86_64 (Unity), NOT proof of Wine/DXVK. Normal launch accepted,
  Steam fetching soldier dependency. Next actual Unity renderer/frame/errors.

- CURRENT NEXT_CONSUMER_SUPERTUX_2026-10-19.md supersedes old next-test notes.
  Official privately extracted VVL1.4.341 activated in actual DDNet; core+sync
  checks on corrupt menu report only unused-fragment-output warnings, no hazards
  in this bounded run. Not proof of CPU publication/shader correctness.
  Switch to real SuperTux SDL/D3D11 consumer, retaining all DDNet R&D evidence.
  Official x64 package/hash verified on E:, tar stream returned0; ADB then went
  offline BEFORE device file verification/first launch. Runner prepared, NOT PASS.
  Host PnP ADB/MTP OK; one transport reconnect +one server restart ineffective.
  No VM restart/production changes. Old wait5753 terminal; no live wait claimed.

- LATEST: actual Vulkan vertex formats/strides/offsets agree with sampled
  Gallium data; dynamic pipeline stride0 is legal because command supplies20.
  Equivalent baked-stride A/B confirms20 in pipeline, SAME corruption: NO-GO.
  GPU readback observer returns zeros, BUT its uploaded control also fails
  despite explicit completed fence: not valid evidence of bad source vertices.
  Independent same-app native Vulkan copy PASS40/40 across coherent/cached
  memory, offsets0/65536, BindMemory/BindMemory2. No generic Mali/ABI copy blocker
  proven. See DDNET_DRAW_BOUNDARY_2026-10-19.md for exact limits/evidence.
  Source reverted to archive hashes (four files); stable Mesa stage unchanged.
  Next capture actual Zink allocation/map/readback parameters vs native PASS,
  not more flags. No production memory workaround or APK changes this round.

- LATEST actual draw evidence: see DDNET_DRAW_BOUNDARY_2026-10-19.md. Bounded
  in-process observer obtained real mapped vertex bytes, plausible sampled
  positions/UVs; packed RGBA NaNs are not corruption. No full/GPU correctness
  claim. Remote own-process-map observation returned EIO, no security changes.
  Observer source restored to exact original C/H hashes after building; stable
  stage/APK11 unchanged. Isolated observer artifact/log retained on E:. Next:
  actual Vulkan vertex layout then GPU visibility, not more flag sweeps.

- 2026-10-19 DDNet boundary localized further: the game's OWN F10 screenshot
  already has corrupt glyphs/geometry before compositor/Android output. Nearby
  Android screenshot has the same class of defects (animated frames differ;
  not a bit-exact same-frame comparison). Compositor cannot be the sole cause.
  Evidence E:/Mavk/artifacts/zink-ui-25.1.9-20261019/ddnet-before-compositor.png
  and ddnet-after-compositor.png. This run is Windows ARM64, NOT x64/FEX.
  Game-only mesa_glthread=false trial still corrupt; no production change.
  Exact driver-internal glthread state was not independently instrumented, so
  do not claim all GL-thread lifetime problems ruled out from that trial.
  Same Windows AMD64 build on same Zink/GL3 path also corrupt. Its own log
  confirms win64 amd64 and Mali renderer; therefore not ARM64-only corruption.
  AMD64 sound init SUCCEEDS with WASAPI; ARM64 SDL float-converter error is
  separate. Audible output/latency NOT measured, do not claim full sound PASS.
  Source-supported ZINK_DEBUG=sync A/B, confirmed in actual game's /proc/env,
  still corrupt. No blanket sync/packed-vertex/GL-version workaround integrated.
  Logs saved ddnet-zink-diagnostics.tar alongside screenshots on E:. All test
  Wine processes closed via exact private prefix; Steam session10 still live,
  CEF hardware renderer rechecked PASS after tests. No VM/production changes.
  Next: capture one real textured/indexed DDNet draw's resources/vertex layout
  or a second small existing GL consumer to distinguish Wine-specific from
  shared Zink/hybris. No more blind GL versions/debug-flag combinations.

- CURRENT APK11: hardware Steam UI preference added under opt-in Mavk,
  default ON within Mavk only; OFF restores original software UI on next start.
  Explicit diagnostic env override preserved. No Adreno/default-package change.
  EN/RU labels no longer falsely claim Steam is always software rendered.
  Build PASS, runtime-input attestation PASS, Linux game-environment80 PASS.
  SHA256 bda1f9553d145cc256bf0301f8fbc6e7272f26a92ef42f1d14a2ad8edec53e4b;
  backup E:/Mavk/artifacts/droiddeck-ui-hardware-toggle-apk11-20261019.apk.
  Install Success. Normal start WITHOUT DROIDDECK_MAVK_STEAM_GL override opens
  session10; actual CEF WebGL again reports ANGLE -> Zink -> Mali-G925. PASS
  normal default inside opt-in Mavk. Current installed APK11, account preserved.
- Real DDNet ARM64 Windows OpenGL consumer with private Zink25 reaches its
  language-selection menu and logs hardware Mali-G925 renderer, GL3.2. However
  screenshot has SEVERE texture/text/geometry corruption; audio reports no
  conversion to float. NOT playable/PASS, NOT DXVK. This is distinct from the
  earlier native-Vulkan vendor-canary crash. No feature-bit spoofing used.
  Probe extension Backend/PrivateZink stays isolated, not a shipped game preset.
  First capture E:/Mavk/artifacts/zink-ui-25.1.9-20261019/ddnet-arm64-zink25.png.
  Follow-up no-layer launch waited for existing prefix server and was cancelled
  before rendering: INVALID A/B, do not draw a layer-causality conclusion.
  Test-prefix wineserver closed using verified files/bin-arm64/wineserver -k
  with exact WINEPREFIX4294967000. Steam process/account was not killed by that.
  Subsequent GL3.0 request is accepted but DOES NOT fix corruption. Valid
  no-Mavk A/B clears inherited VK_INSTANCE_LAYERS plus HYBRIS layer settings:
  same corrupt menu, /proc/game/maps confirms hybris/libgallium present and
  Mavk layer absent. Thus not caused solely by Mavk layer. Earlier 'native'
  label alone was INVALID: inherited instance layer still loaded Mavk.
  Source-led packed-vertex fallback A/B HYBRIS_VULKAN_COMPAT_PACKED_VERTEX=0
  changes fallback_mask0x1000 ->0 but SAME corruption. NO-GO, not integrated.
  All test-prefix Wine processes closed; session10 Steam remains running.
  Next narrow lead: actual Wine/Zink client-array/buffer lifetime and Mesa
  threaded GL (overridden in logs); distinguish with one game-only glthread
  off A/B, not new capabilities or shader feature spoofing. Sound separate.
  Consumer runner now uses installed /usr/local/bin/droiddeck-game-env rather
  than old private game-env9.py, which lacked sibling sync/component helpers.

- CURRENT 2026-10-19: APK10 installed/attested, actual authenticated Steam
  session09 hardware UI PASS. CEF's own WebGL renderer reports ANGLE -> Zink
  Vulkan1.3 -> Mali-G925 ARM_PROPRIETARY, not llvmpipe. User confirms the UI is
  very smooth; small one-time artifacts when opening some menus remain.
  Do not attribute those artifacts to shader compilation or sync without proof.
  APK SHA256 0e065438f0c274f0aaf7d5189c1919774e1b8223a0b3dd9a1d336d413a31f62f;
  runtime SHA256 02a188f5a9fcb5a77958a9d83eee1a56a09ba9e1a593893a974184020ada96cc.
  Proper WSL/Linux /tmp regression:80 tests PASS. Windows/NTFS invocation was
  invalid for FIFO/chmod tests. No production/rootfs library replacements.
  Actual Steam launch uses private Mesa25, lazy descriptors, CEF ANGLE/GL.
  This is an experimental UI milestone, NOT game compatibility or conformance.
  SurfaceFlinger pacing returned NO_FRAMES: actual unique visible FPS remains
  UNMEASURED. glxgears/application Present rates are not visible FPS.
  Current session is one-shot DROIDDECK_MAVK_STEAM_GL=1; default still software.
  Preserve this live session while user tests. Next: normal reusable UI-mode
  selection with software rollback, then return to a real game consumer.
  Remaining logs: glXGetSyncValuesOML failure and early recoverable surface-loss
  messages. Disabling unsupported GLX sync is only a candidate, not a fix yet.
  Artifacts: E:/Mavk/artifacts/zink-ui-25.1.9-20261019/steam-hardware2.png,
  steam-navigation1.png. Earlier pending notes below are superseded here.

- PRIORITY 2026-10-19: user suggests accelerating the existing Steam UI first.
  Pause DDNet; investigate real Zink requirements/Mavk compatibility without fake
  robustness2/nullDescriptor advertising. Preserve authenticated Steam/rootfs.
  DDNet original and actual transformed shaders:240/240 Vulkan1.1 validation
  PASS; no shader validity failure found. Single render-thread A/B still same
  vendor canary failure; ARM64 official exe tries OpenGL only, NOT Vulkan PASS.
  Bounded memcpy/memmove/memset observer only sees earlier stack-reuse fills;
  these do NOT establish the corrupting write. No canary suppression/integration.
  Artifacts ddnet-20.1.1-20261019/actual-spv, ddnet-single-render.log,
  ddnet-stack-writes.log. Shader debug stripping and single-thread branch closed.
- Private Zink UI experiment: official Mesa25.1.9 archive SHA256
  412df33a1bb3c785ed698555a3972118a37c458e7accf6ae53f4bb87b3db454a;
  source confirms real dummy-resource/lazy-descriptor fallback when native
  nullDescriptor=false. NOT feature-bit spoofing or full Mavk robustness2.
  Isolated ARM64 cross build on E:, rootfs headers/libs read-only SDK export.
  First build stopped on glibc2.43 C11 once_flag conflict. Switching the entire
  legacy shim to native C11 is NO-GO (mutex/condition interop types). Reverted.
  Narrow patch instead preserves pthread-based shim and defers ONLY call_once/
  once_flag to libc when ONCE_FLAG_INIT is already present; source patch saved
  experiments/droiddeck/zink-ui-glibc243.patch. Rebuild active, NO GLX/UI
  PASS yet. Software Steam session08 remains live; no library replacement.
  Restored app412220 original compat selection empty because DDNet is paused.
- 2026-10-19 15:31 private Mesa/Zink25.1.9 build/install PASS. Initial build
  process script was edited while running, causing a spurious trailing-shell
  error after install; clean second invocation exits0. Do NOT edit running
  shell scripts in future. Source patch reverse-check PASS with saved patch.
  libgallium SHA17097e10c9d1ea6534095ebbf64607742941bbab83fa895ec5f371e2cc7d6cc1.
  Actual GLX context PASS: direct/accelerated yes, OpenGL3.2/GLES2, renderer
  zink Vulkan1.3 Mali-G925 ARM_PROPRIETARY; native nullDescriptor still false.
  Known warning fillModeNonSolid absent: not OpenGL conformance/full feature PASS.
  Actual moving glxgears visible (gears.png),15s bounded run,58.845/59.463
  application frames/s; UNIQUE Surface FPS NOT MEASURED. No canary failure.
  Own private files/mavk-zink-ui-25.1.9, no rootfs libraries replaced.
- Layer logging-only fix bounds gap text32+suppression while preserving total.
  Built mavk-layer-ui-bounded-log-20261019; no capability/shader changes.
  APK10 payload02a188f5…262files/58.8MB adds private Mesa25 libs/notices and
  logging-bounded layer. Actual normal droiddeck-session Steam branch supports
  DROIDDECK_MAVK_STEAM_GL=1; default software unchanged until validation.
  Per-Proton game environment removes private UI Mesa path/restores prior
  descriptor mode. Windows-native test invocation was INVALID (Linux tests on
  Windows); WSL rerun/build10 active handle92050. APK10 NOT INSTALLED YET.
  Next: WSL test PASS -> APK10 attestation -> clean stop/update -> opt-in normal
  Steam hardware UI -> actual CEF/render/input check. Preserve account/rootfs.

- LATEST 2026-10-19 14:22: user confirms Steam login and completed DDNet download.
  Steam software-GL UI is laggy; do not extrapolate this to hardware game FPS.
  Session08 still live, APK8 installed; APK9 built/verified inputs, NOT installed
  yet. APK9 SHAe0880a24f6abf5fd2eafd1a2767e0d9d8a2a9fc85774d93044038e2337e8e7d7,
  runtime2483246c… includes helper10 (16 PE-map messages/process), extra notices,
  explicit experimental native-Vulkan profile. Assets now included in normal
  runtime source/output attestation, not an unchecked extra payload.
- DDNet actual Steam default selected Linux/FEX (no compat tool, sniper fallback),
  fails before graphics: FEXServer socket creation/start failure. Set ONLY app412220
  compat tool to existing `droiddeck-proton-arm64`, original selected string empty
  saved here for rollback. Normal launch requested; inspect download/UI before any
  restart. Never expose account/Steam auth data in reports/artifacts.
- Windows DDNet native Vulkan profile reaches real ARM/Mali-G925/Vulkan1.3,
  then Bionic stack-check failure. Same failure WITHOUT Mavk layer: not Mavk's
  descriptor tracker. Private diagnostics localize vendor libGLES_mali.so +0x9314f0,
  function0x930f20, canary compare at0x931428. Source/native register observer does
  NOT disable canary; failure still aborts. MRS is patched, x22 matches Bionic
  shadow, current guard0 vs saved nonzero. Saved stack/LR region has ASCII shader
  name bytes; TLS-reset vs actual overwrite not yet distinguished. Diagnostic
  callback via existing hybris hook API, loaded only in bounded game process.
- `spirv-opt --strip-debug` A/B on standalone test-copy38 shaders: original and
  stripped validate Vulkan1.1, SAME crash. Restored original shader files. NO-GO
  debug-name workaround, do not integrate it. No Steam game shader files touched.
- Layer29 native-private feature enable test: fragmentStores supported1 but
  vertexStores0, still cannot use diagnostic-writing guard compiler for DDNet VS.
  Saved experiment source/binary/logs in artifacts/mavk-glibc-layer29-20261019;
  reverted source to proven layer28 and kept layer28 in packaged backend.
- NEXT strong ABI A/B: official Windows ARM64 DDNet vs AMD64, same original
  assets/Proton/Mali to distinguish x64/Wine/FEX thunking from native driver path.
  ARM64 zip downloading verifiedhandle15460; SHA expected3221662c…0d18. No fresh
  broad graphics tests. Production U-AVF/Windows/Ubuntu untouched. No commit/push.

- LATEST 08:27: packaged experimental runtime build8 installed
  913bedfe267fc57252146bbecbcc07f51d89eacc1ac103c9293301950687914f.
  Ordinary Setup/Launcher Mavk toggle (default OFF) selected through UI. Runtime
  archive d7b2a56b… extracted/verified by APK into versioned private directory,
  mutable Xauthority/SysV separate. Normal Steam session08 reaches login again;
  no manual layer/WM copying required. Upstream/Adreno path unchanged.
- AMD64 PE consumer via actual Proton/native ARM64X Sarek PASS: S_OK/FL11_0,
  visible moving texture,1186 Present/20.020s (59.241 calls/s, unique visible FPS
  UNMEASURED). Screenshot proton-amd64-session7.png; architecture AMD64 proved
  by actual executable log. Late pacing attempt returned NO_FRAMES (consumer
  finished), not a valid performance result. Steam credentials not supplied.
- Actual official DDNet20.1.1-win64 SHA83854f827978b9ed8b1295d12338e4b6b08e7a00393f0a3c2cf54a1a6cb92dc4
  verified against official SHA list. Real game reaches Vulkan1.1, fails guarded
  pipeline because it enables no core features, but injected diagnostics need
  stores/atomics. No game PASS. Layer29 building: for legacy feature-create path
  only, query/enable supported private stores/atomics without changing public
  feature queries/caller memory. Features2 chain remains unchanged (not full
  generic feature-chain virtualization). Next run real DDNet, then Sarek
  regression if it advances. Exact full log files/mavk-window/ddnet-full.log.
  Must capture complete stderr; helper9 logs PE mappings verbosely/truncates agent
  response (redirect game output to private file, existing agent intact).

- LATEST 2026-10-19 08:16: build7 installed SHA
  19f8a3693a4a04c51ac77bad3347c78f6f42988a2c3132adb76efc2ac38a3568.
  Openbox BEFORE Steam fixes automatic input focus; normal session04 displays
  real login and `mavk_focus_ok` typed without diagnostic SetInputFocus. Cleared
  text and stopped cleanly. Agent Xauthority fix works on actual :99; parser
  roundtrip/truncated/wrong-display regression PASS. Live insertion of Openbox
  into previous Steam session gave black root (all RGB0), NOT adopted.
- Official signed Openbox/Imlib2/startup-notification/xcb-util now privately
  staged, dependencies resolve. Private package DB refreshed, Linux rootfs
  libraries/package DB/security policy unchanged. Session owns WM process tree.
- Actual normal run-mode -> droiddeck-game-env -> Proton waitforexitandrun ->
  Mavk Sarek ARM64X -> Mali produced visible D3D11 image in session06, screenshot
  proton-session6.png (console obscures most draw). Profile log confirms Mavk,
  native Mali and FL11_0; NOT game/FPS PASS. Dedicated numeric test prefix
  /root/.local/share/droiddeck/mavk-proton/4294967000. Non-numeric prefix plus
  Proton `run` does not exercise normal managed game routing (session05 only).
- Source configure now explicitly sets managed DXVK (required for actual Mavk
  game launches); this one-line change is AFTER installed build7, not built yet.
  Next: same GUI consumer AMD64 via actual Proton/FEX/ARM64EC, no console; then
  self-contained experimental backend packaging. Steam login needs user account,
  user away; continue independent work. No U-AVF changes/commit/push.

### Earlier checkpoints (superseded where noted above)

- LATEST 2026-10-19 07:51: real Steam Big Picture login screen VISIBLE in normal
  DroidDeck SessionActivity/session03; initial17part client +669522KiB self-update
  completed. Installed build6 SHA37bef6b484e6a078839f7f381bee574015e4653a3113ecf33096006f59967909,
  runtime source/output/APK verification PASS. Steam UI uses llvmpipe/software
  CEF only, games explicitly restore native Vulkan environment. Native Steam
  otherwise dies in VGUI glXChooseVisual because Zink requires nullDescriptor.
  This does NOT make full nullDescriptor supported. Steam login pending user
  async request; user away, continue independent integration (do NOT pause).
- INPUT boundary proved: initial X focus0x1200008 did not deliver test text.
  Diagnostic SetInputFocus to actual mapped Steam0x240002f immediately makes
  native Wayland FIFO +Android keyboard text visible. Cleared test text, no login
  attempted. Need automatic ordinary window focus, not permanent manual fix.
  Agent raw X11 protocol omitted auth. Source now reads bounded matching
  MIT-MAGIC-COOKIE-1 from XAUTHORITY; not built/installed/tested yet.
- Standard Openbox private-prefix investigation: official signed openbox3.6.1-14
  downloaded/integrity verified; private package keyring only, no rootfs upgrade.
  Needs only startup-notification/Imlib2 (existing other libraries). imlib2 cached
  repo1.12.6-2 now404. Do NOT install full pacman dependency plan (metadata does
  not describe actual preassembled rootfs; would replace working shared libs).
  Next bounded action: refresh private extra index/download exact2 deps, inspect
  ldd, then existing X session test. No WM launch or integration yet.
- Proton adapter preflight PASS using actual installed Proton/actual3 ARM64X DLLs.
  New adapters/droiddeck/{proton_d3d11.py,run-proton.py,check_proton_routing.py}.
  Native3DLL routing/stock32-prefix preservation/wrong64 reject/stockD3D9 PASS.
  Must preserve32 prefix files even for64 games (Proton copies both); not support
  for32bit Mavk D3D11. droiddeck-game-env opt-in flags/wrapper added in build5/6;
  actual Proton/game execution NOT TESTED. helper9 adds bounded approved PE roots,
  WSI instrumentation off; guest root mapping currently derived own.parent/linuxfs.
  Full APK payload packaging/UI backend toggle still pending, current private
  developer staging/marker NOT final end-user install. No commit/push.

- NEW normal-session integration (docs/DROIDDECK_SESSION_2026-10-19.md): optional
  .mavk/private-marker path in actual SessionService now starts Xwayland with
  ownership fix/private Xauthority and normal audio/input infrastructure. Build3
  installed41ba2c32…; pressing ordinary Start Steam passed old Gamescope boundary
  and starts official17part Steam ARM64 download. Actual live curl/install,
  not yet Steam UI/game PASS. Keep download; do not reinstall mid-download.
  Build4 SUCCESS includes log-setup correction, ready NOT INSTALLED. Startup
  guards0/native tracking; game Sarek/DLL selection NOT integrated yet.
- Layer28 regression40 guarded65536/4 Submit2/indirect/resubmit/init PASS,gaps0;
  normal nullDescriptor admission correctly rejects. Run41 explicit diagnostic
  real D3D11 clear/draw/texture/nearest/bilinear PASS, cleanup/Corezero. No full
  Vulkan support claim. No unrelated Feature Suite expansion.

- NEW Wine/PE visible D3D11 frame PASS (run21): real Win32 consumer, Sarek PE
  DLLs, layer28 -> Mali -> Xwayland -> existing Android compositor/Surface.
  Screenshot wine21.png visually contains intended moving textured/scissored
  draw; compositor full10s 593 on-screen frames (59.3/s), NOT independently
  measured unique visible FPS or game compatibility. Normal Steam NOT fixed yet.
- Black-frame root cause proven A/B: Xwayland's proot fake-root facade makes
  emulated SysV shmctl(IPC_STAT) report owner0; ordinary Wine client is10261.
  X server rejects SHM attachment: actual PutImage BadShmSeg128/major130.
  Explicit shared BL_SYSV_DIR alone did NOT fix. Isolated server-only helper
  corrects emulated IPC_STAT owner to actual app UID; run21 four checked PUTs
  error0 and actual XGetImage nonzero964308components. No kernel permission,
  security-policy or root change. Needs proper integration, not global shim.
- Layer28 device entry fix compiled/staged SHA
  7af705c5ea9bdd7ab327aeac96e88a6c8f9bc68cf9e1a68d1e4d9cbccd27a034.
  Actual Wine D3D11 S_OK/FL11_0/20s Present now confirmed; older device-3
  checkpoint below superseded. Guard/native regressions after28 still needed.
- NEXT integrate optional Mali/Mavk session into upstream SessionService/session
  rather than Gamescope-required launcher. Preserve upstream UI/Steam/audio/input,
  Adreno path; real Steam and game remain final acceptance. Current owner-test
  Xserver bounded300s, Wine21 bounded90s (actual draw20s); inspect handles/PIDs.
  Evidence E:/Mavk/artifacts/wine-arm64-inputs-20261018/{wine-window21-device.log,
  xwayland-owner21.log,wine21-compositor.log,wine21.png}. Run19 instrumentation
  itself crashed (RTLD_LOCAL symbol lookup); corrected helper8 in run20/21.

- User final acceptance clarified: normal DroidDeck/Steam install+game launch
  using experimental Mavk on Tab S11, real graphics/input/audio, no per-game CLI.
  Native/Wine test windows are intermediate only. realme8 portability follows;
  U-AVF Windows deferred. Detailed active order in docs/ACTIVE_PLAN.md.
- Resumed after download. linuxfs-r9/Proton installed. Normal Steam launch fails
  in Gamescope before Steam: required VK_EXT_robustness2 missing (actual logs).
  No fake extension/feature workaround. Bounded direct rootful Xwayland -> actual
  Android compositor connection works; not yet an integrated user session.
  Actual Win32/Wine test now loads Sarek DLLs, creates Vulkan instance, enumerates
  Mali via WSI/Mavk/hybris; next failure vkCreateDevice -3. See
  docs/DROIDDECK_WINE_BRINGUP_2026-10-18.md for exact evidence/scope/next test.
- Wine preparation: official DroidDeck Proton ARM64 build25502785 downloaded
  to E:/Mavk/artifacts/wine-arm64-inputs-20261018; SHA matches catalogue
  d6c480a90d981801be0906a3f3418eb9bd9461c30e411140951cb0daf9d2a189.
  Proton archive has winex11, no winewayland: use existing rootfs Xwayland/session,
  not the earlier native SDL Wayland shortcut as the Wine architecture.
- PE Sarek: new pinned LLVM-mingw20261006 archive hash verified; ARM64X test
  Win32 D3D11 exe built from the SAME consumer source (native branch preserved),
  raw machineaa64 +ARM64X metadata. Not run yet. Native renderer baseline untouched.
  Build E:/Mavk/artifacts/sarek-pe-arm64x-build1-20261018 COMPLETE (resume3 log).
  Layer27 localized device failure ABOVE Mavk. WSI resolves vkCreateDevice via
  GIPA(NULL), omitted by Mavk. Exact fix in layer28 building (layer28-build.log).
  Wine PE RX/file-backed errno13 localized; opt-in anonymous/read image fallback
  progresses to real Win32 window (not production-ready or a game PASS).
  Source includes three
  required standard headers added for libc++: dxbc-spirv/util/util_small_vector.h
  <new>, src/util/util_small_vector.h <new> and src/util/config/config.cpp
  <algorithm>. No semantic shader change.
  Initial build helper and cross file: experiments/droiddeck/build-sarek-pe.sh,
  sarek-pe-aarch64.ini. Incremental resume needs PATH LLVMbin+nativeTools/usr/bin,
  LD_LIBRARY_PATH nativeTools/usr/lib/x86_64-linux-gnu and PYTHONPATH
  nativeTools/usr/lib/python3/dist-packages (Meson regeneration uses it).
  All new large files on healthy E. No root/production changes/commit/push.

- Strategic target unchanged:real D3D11 frame ->opt-in DroidDeck fork ->current
  upstream DXVK compatibility. Shared Core/Compatibility;Windows branch retained.
- Real Sarek D3D11CreateDevice: diagnostic S_OK/FL11_0, offscreen clear AND
  actual HLSL VS/PS Draw readback16384 components each PASS (run30). Local admission with
  nullDescriptor still FALSE, NOT correct full Vulkan/game compatibility.
  Normal admission still rejects nullDescriptor. No full DroidDeck/Wine PASS.
- Common modules:static UBO/SSBO null backing;typed R32 texels;owned color2D images;
  real descriptor resources/templates;shared SPIR-V guards and exact layouts.
- Actual glibc layer executes Sarek shader guards/direct+indirect+resubmission.
  CbCr-null planar readback65536 components/4submissions PASS;retained same-queue
  Submit1/2 initialization. Default native/tracking baseline remains working.
- Earlier retry proof:Submit1/2 one pre-driver OOM rejection ->same CB/fence retry PASS,one
  successful init batch,gaps0/cleanup0/Corezero. Normal guarded65536/4 and native
  49152/3 final-binary regressions PASS. Layer17/Consumer11;run21–24 on healthy E.
- Null-vertex short experiment CLOSED: ordinary robustBufferAccess permits
  non-exact OOB vertex values; zero backing alone cannot prove null semantics.
  Native pipeline robustness ACCESS_2 requires absent robustBufferAccess2.
  NO-GO for this shortcut; no vertex patch or GPU support claim made.
- Real next blocker from run25: native non-nullable sampler update-after-bind
  heap rejected by layer. Fixed narrow native pass-through; run26 device PASS.
  Native sampler heaps are deliberately NEVER converted into guarded snapshots.
- Layer23/Consumer13: real D3D11 textured Draw PASS (run34),16384 components,
  16 distinct texels repeated across64x64, guarded secondary generations retained
  in primary. gaps0/fullcleanup/Corezero. Texture2D.Load, not filtered sampling.
- Concrete compiler gap: old SPIR-V tools rejects FloatControls2 capability6029.
  Run34 explicitly selects Sarek's optional fallback; no instruction stripping,
  native feature queries unchanged. This is NOT full FloatControls2 support.
- Run35 latest-layer normal guards65536/4 regression PASS; both diagnostic flags
  OFF, actual Sarek still correctly rejects nullDescriptor. All tests stopped.
- Layer24/Consumer14 run36: actual D3D11 nearest AND bilinear sampling PASS,
  16384 components each. Mixed native sampler heap +guarded image sets work;
  no native heap copy/snapshot/allocation by Mavk. gaps0/Corezero/fullcleanup.
- Run37 latest common/layer normal guarded65536/4 regression PASS, flags OFF
  and real nullDescriptor admission still correctly CLOSED. No active tests.
- Actual isolated DroidDeck app UID visible native D3D11 window PASS (layer26):
  real swapchain640x480/FIFO, moving sampled texture on Android Surface,
  1198 Present returns/20.009s; NOT measured visible FPS. Second run reproduced.
  Concrete present blocker: set1/binding1 texture1D gamma descriptor. Added
  dimension-keyed1D/2D fallback materialization, no fake feature advertising.
  Three swapchain RT view tracking gaps remain explicit; all owned cleanup/Corezero.
  Run38 actual draw/filter regressions PASS, run39 normal guarded65536/4 PASS
  with ordinary nullDescriptor rejection preserved. See
  docs/DROIDDECK_VISIBLE_WINDOW_2026-10-18.md for artifacts/build/install.
- NEXT: opt-in Wine/PE D3D11 consumer and matching PE Sarek DLLs in DroidDeck.
  Native ELF window proof is NOT Wine/game compatibility.
  Keep experiments opt-in; do not expand theoretical nulls.
  Native view-usage pNext was observed on clear; narrow usage tracking added.
  Diagnostic admission is NOT Vulkan correctness/full feature support.
- Evidence/requirements/hashes:docs/NULL_DESCRIPTOR_CONTRACT_2026-10-18.md.
  Real D3D11 device/draw/texture:docs/SAREK_D3D11_DEVICE_2026-10-18.md.
  Prior integrated color2D proof:docs/NULL_IMAGE_LAYER_2026-10-18.md.
  Common catalog proof:docs/NULL_{BUFFER,TEXEL,IMAGE}_BACKING_2026-10-18.md.
  Pipeline/compiler/API proof:docs/SAREK_PIPELINE_COMPILER_2026-10-18.md and
  docs/SAREK_GUARDED_EXECUTION_2026-10-18.md. Earlier real-consumer blocker:
  docs/SAREK_REAL_CONSUMER_2026-10-18.md.
- No commit/push/root/security changes. Production Windows/Ubuntu/P7/H264,
  user data and stock Adreno DXVK untouched. D never use;new large outputs on E.
  No active test/VM launch left by this stage. Goal incomplete/active.

## Earlier checkpoints (historical;current status above supersedes them)

2026-10-18 separate image/sampler gate and regressions CLOSED. Actual Mali:
2048 RGBA components/256 guard events,two sampler filters,two swapped null/live
phases,Core zero and SyncVal0/0. Sampler null writes rejected;no feature bits
changed. Same APK texel2304/1152 and combined/storage image1792/384 PASS.
Artifacts E:/Mavk/artifacts/separate-samplers-native2-20261018,
APK b0cd6f8970890f0ae9c73cbea3122692cff95996bf63159bb800863361420289.
Report 8fea318408d27751d52d5d84e25a6c039b73f3659bf3db5fcb3d5a6fe410cd1b.
Probe stopped. New priority:real DXVK-Sarek standard Vulkan/D3D11 consumer,
NOT additional unrelated Feature Suite features. Native ARM64/glibc Sarek build
and two real consumer runs subsequently completed (see above). Common guard modules are not exported
loader hooks yet. No commit/push authorized by newest instruction.

2026-10-18 R32_UINT uniform/storage texel buffer actual GPU PASS. Sarek uses both
types;common state/registry/updates/copies/templates/binder/native materializer
now accept R32 scalar views with element-count guards and real backing leases.
Actual original shader ->runtime compile ->VkWrite buffers/views ->Mali validates
2304values/1152guards(null768,uniformOOB128,storageOOB256),sentinel writes preserved,
SyncVal0/0/Corezero. CPU12checks/15negatives;state34/25,templates13/18,binder18/6
regressions PASS. Same APK actual image1792values/384guards regression PASS.
docs/TEXEL_BUFFER_CONSUMER_2026-10-18.md records hashes and limits. No loader-layer
delivery or actual DXVK PASS this turn;remaining null/shader/consumer work OPEN.

2026-10-18 multi-set shader/descriptor consumer building blocks actual PASS.
Sarek dxvk_pipelayout.h separates GpViews0/GpBuffers1; fixed private set1 was a
real conflict. GuardCompiler now reserves configurable private set after1..7
public sets;native adapter emits matching layouts/sets and checks device limit.
Common frontend snapshotBindings composes ordinary logical-set writes with weak
cache,immutable resource leases and empty public positions. Actual Mali original
shader ->device compiler ->VkWrite structs ->set0SSBO/set1UBO/empty2/private3 ->
GPU PASS1536words/1024guards,SyncVal0/0/Corezero. Single-set and descriptor-template
regressions PASS on same APK. Compiler CPU14/9,bound composition18/6,template13/18.
Hashes/scope:docs/MULTI_SET_CONSUMER_2026-10-18.md. No feature bits or DXVK/device
success claim;loader hooks,remaining descriptor/shader semantics still OPEN.

2026-10-18 descriptor update templates actual PASS. Sarek calls this API;
common frontend now owns/validates entry schema and bounded offset/stride source,
then uses existing transactional updates and resource leases. CPU13checks/18
negatives PASS,ordinary updates16/16 regression PASS. On Mali two template
updates+copy ->two recorded generations128words PASS,SyncVal0/0/Corezero.
compatibility/VULKAN_DESCRIPTOR_UPDATES.md has exact contracts/hashes. Still no
loader hooks/full shader/descriptor semantics or DXVK device claim. Production
Windows/Ubuntu untouched;next remains actual consumer integration and full null.

2026-10-18 shared runtime shader compiler actual Android PASS. Host CLI and
native adapters now use GuardCompiler;bounded exact-input LRU/immutable results,
serialized access. Original atomic3232B SPIR-V compiled on Mali host device to
9372B in7.19138ms,warm hit0.000385ms(single samples).1536words/1024guards actual
GPU PASS,4bad inputs reject,SyncVal0/0/Corezero. Host11assertions/5rejections PASS,
CLI output unchanged SHA a0ef7d95... Same APK two-recorded descriptor generation
regression128words/3rejects PASS. No feature advertising,D3D11 or full shader
semantics claim. docs/RUNTIME_SHADER_COMPILER_2026-10-18.md has exact hashes,
scope and next consumer integration work. Windows/Ubuntu/P7/H264 unchanged.

2026-10-18 BC5 FILTER_LINEAR candidate actual PASS. Standalone rebuilt existing
hybris compatibility DSO,NOT full libhybris/ICD replacement. New experimental
bc5-rg32f.patch changes decoded BC5 image to RG32F with matching GPU output,
destination bounds and scratch/copy sizing. Original DSO left untouched.
Rebuilt baseline reproduces original896component result+missing BC5 filtering.
Candidate all14 D3D BC format flags0xd001,896constant-component regression PASS;
actual bilinear BC5 U/S384components over six palette phases (including negative
SNORM) PASS;CPU destination-bound14checks PASS. Strict verify-bc5-filter.py PASS.
8bytes/texel instead4,performance NOT measured. No full BC/null/D3D11 advertising.
Candidate DSO179870fcf9e1cdfe0495d829df547b53bd950c63e2981ff162b53a4d02687654,
artifacts E:/Mavk/artifacts/hybris-bc5-layer1-20261018. Full provenance/limitations
in docs/HYBRIS_BC5_FILTER_2026-10-18.md. Native libs retain upstream licenses.
Next: shared null/image consumer +experimental per-format BC admission in Sarek;
global textureCompressionBC and mandatory nullDescriptor remain distinct blockers.

2026-10-18 actual hybris BC API reuse A/B completed (see
docs/HYBRIS_BC_REUSE_2026-10-18.md). Bare ICD reports0BC formats;enabling existing
VK_LAYER_HYBRIS_compat gives14D3D formats,real compressed-image upload+compute
sampling PASS896constant components on G925 under glibc/proot. Existing
HYBRIS_BC_TEXTURES=dxvk alone is insufficient without the actual layer.
NO full DXVK admission:BC5UNORM/SNORM lack FILTER_LINEAR (0xc001 vs0xd001);
RGB-onlyBC1 excluded. No performance/fullcodec/validation/DroidDeck-session claim.
No new library installation,VM/firmware/H264 changes. Native Mavk BC oracle and
null/robustness consumer work remain active;do not duplicate existing GPU BC
decoder blindly. Detailed hashes/logs+exact strict checker stored in reuse doc.

2026-10-18 Vulkan descriptor update frontend actual PASS:
compatibility/VULKAN_DESCRIPTOR_UPDATES.md documents bounded ordinary Vulkan
write/copy/layout structures -> common state -> native materializer. Trusted
native-handle registry retains buffer/image/sampler owners; transactional ordered
writes/copies,consecutive bindings/zero-count skip,whole-size and null validation.
Null combined-image retains its valid sampler;destroyed references become
undefined/reject snapshots,not stale handles. CPU16 assertions/16 negatives PASS;
existing DescriptorState34/25 regression PASS (LLVM runtime DLL directory on PATH
required for the host test executable; initial missing-DLL launch was NOT PASS).

Final APK vulkan-descriptor-api-native2-20261018 SHA256
817F41130C379ACD9A0DA32322D1051D6E967404FBF2F7EC6F3F4669498A5FF8.
Installed only org.mavk.probe. Fresh reports under same artifact directory:
- generations128words,two VkWrite structures+one VkCopy,real recorded lifetime,
  cache,3rejects:7ea67d88bfd9739255930582db1f2f51c9b71ecd99db2952136de25968d8e537;
  strict verifier --vulkan-api PASS.
- images1792words/384guards:
  58bc8fbecdeec87cd1e8947c4eeb15cfccdca510a745098e607872abbca24e09.
- atomics1536words/1024guards:
  cc761fd34892100e3cbde262e78e4a4e1eb4898ebb47a46fd53c0758df5efb64.
All SyncVal0errors/0warnings,Core live zero. No loader exports/full command-buffer
invalidation,automatic shader admission or full nullDescriptor/robustness2 claim.
No DXVK device/frame PASS. NEXT: consumer layer integration and real per-format
BC coverage;reuse existing hybris BC machinery where applicable,not duplicate it.

Reuse lead only,NOT tested BC evidence: DroidDeck fork0916a538 pins libhybris
bc8cb358 +protocolc6ae3c22;tools/hybris/patches/0001-dxvk-bc-profile.patch implements
per-format GPU BC decode. dxvk policy deliberately skips BC1RGB;0002 vendor patch
keeps full textureCompressionBC false (RGB/blit/filter guarantees incomplete).
Existing per-format DXVK patch may avoid requiring the global BC boolean ONLY
with real required-format checks. Do not widen G715 guards or claim G925 PASS.

Disk: original Windows ISO staging directory(6.755GiB) hash-verified relocated to
E:/U-AVF/inputs/win11-iso-work-20261018;old win11-iso-work path is a junction,
retired duplicate absent. ISO bytes unchanged,no VM disk/clone. C still47.71GiB
because independent installer consumption;I/O sample showed Epic helper~31MiB/s.
User will pause Epic;process untouched. Proposed old backup relocation command
was rejected before execution;backup remains on C,no claimed50GiB reserve yet.

Newest shared NativeDescriptorAdapter actual PASS. Portable native Vulkan API
materializes real immutable pool/set/metadata/log generations from CPU snapshots;
explicit shader ABI,typed resource/fallback resolver,strong leases and cached
unchanged generations. No updates to older sets,no hidden destructor queue idle.
Currently set0public+set1guard,no loader hooks/full descriptor API,see
compatibility/NATIVE_DESCRIPTOR_ADAPTER.md. Diagnostic buffers remain enabled;
changed-generation allocation/recycling and full consumer entry points OPEN.

Actual Mali lifetime gate: two ordinary compute command buffers recorded with
different generations BEFORE either executes,then shadow state/caller resource
references dropped. Both original outputs PASS128words,distinct sets,old leases
survive then expire after GPU completion. PoolCreates2/metadataUploads2/updates2,
3 invalid materializations reject before allocations,cache hit no reallocation,
SyncVal0/0,Corezero. Native adapter now used by existing buffer/atomic/image gates
instead of their own VkCreate/Allocate/Update implementations.
Latest installed APK native-descriptor-adapter3-20261018 SHA256
992BB6DA80A3042C4F549EC6E2AD302C194293A907E3E114A021ED978EB68A73.
core-descriptor-generations.json SHA256
44987046bf7c2af076b314056002fcef31558c51c512dda8d7fd4b2467a4b944.
Strict verify-descriptor-generations.py PASS. Same APK native null regressions:
atomic1536/1024guards report47c7444c5e5f19da6fb8e752c75c85f34aea58f2b43471ef67cc462d5f487e7e;
image1792/384 report58bc8fbecdeec87cd1e8947c4eeb15cfccdca510a745098e607872abbca24e09;
buffer1536/896 report90811e0f41a96376cdb1cfc1a6b749c81dc174e8c76edad6b470e1d980368068.
All SyncVal0/0,Corezero. Initial adapter2 lifetime test failed only at host
invalidate-before-map (c86cb423... report retained),fixed mapping before readback;
no platform changes. Adapter1 compile-only never deployed. No live build/test.
NEXT: real consumer Vulkan update/copy/image entry points and shader/layout
admission;complete BC/null/robustness semantics before Sarek D3D11 device/frame.
No feature bits advertised,no DXVK PASS,no Windows/Ubuntu/P7/H264/storage edits.

Space housekeeping: original LLVM archive/download pieces(16 files,1.083GiB)
hash-verified on E:/Mavk/artifacts/toolchain-download-cache-20261018,source copies
removed;actual extracted toolchain paths untouched. Historical September28 APKs
also hash-archived(8 files,2.007GiB) to E:/U-AVF/artifacts/archived-test-apks-20261018;
C51.266GiB free after relocation. Only ignored historical installers,not current APK,data or
P7 images. This is recoverable file relocation,not release/source changes.

Newest component common DescriptorState (compatibility/DESCRIPTOR_STATE.md):
portable C++17,explicit unwritten/null/bound states,checked buffer ranges/whole
size/alignment,kinds,overlap-safe copies,rejection atomicity,resource leases,
generation+immutable per-ABI cached snapshots. CPU34 assertions/25negatives PASS.
Ownership identity includes control block,not just raw address. Snapshots retain
resources across descriptor-state mutation/teardown;caller retires afterGPU.
Unwritten slots rejected,NOT silently treated partially-bound. This CPU model
does NOT own native descriptor sets/pools or allow updating in-flight sets.

Actual native gates now use that state for BOTH native VkWriteDescriptorSets
values and guard metadata,instead of hand-packed offsets. Real Core Buffer and
SampledImage owners plus complete storage-image+view leases,valid null fallback
resources retained. Synchronous fence ownership,not async update-after-bind.
shared-descriptors-native1-20261018 APK SHA256
B43E1D628C337225FEF34C0FFC8DF359B565C777D062BB7BF1C84A2C2D8E6134.
Fresh strict --shared-state gates on this APK all actual PASS:
- atomicIAdd32+scalar range/UBO:1536words/1024guards,SyncVal0/0,Corezero;
  report30515e1eed6f201a7ecdc6c670deea5af78bd975e2d5069b143b42926ba95884.
- combined/storage image:1792words/384guards,SyncVal0/0,Corezero;
  report58bc8fbecdeec87cd1e8947c4eeb15cfccdca510a745098e607872abbca24e09.
- stock buffer-only ABI:1536words/896guards,SyncVal0/0,Corezero;
  report79a03d0883ab83c9125174ccc265109daa929499b8c0f16d310d26639d846ac4.
Snapshot cache pointer reuse checked natively in each phase. Tiny GPU command
times NOT production overhead/FPS evidence. No full null/robustness2/BC feature
advertising,no DXVK/D3D11 PASS. Existing shader bytes unchanged,4a0c898 compiler
and old standalone BC/native gates preserved;no Windows/Ubuntu/H264/VM changes.
No live build/test worker. C54.9GiB,E874.1GiB,tablet54280676KiB free,all>50GiB.
NEXT: consumer Vulkan descriptor/image entry points and shader/layout admission,
typed fallback cache,remaining arrays/image/atomic/robustness semantics before
actual Sarek device/frame. No VM boot needed for every shared function.

Newest integer buffer atomic extension actual Mali PASS:32-bit atomicAdd returns
zero/no memory change for logical null; normal atomicAdd returns previous value
and applies increment. Same gate retains scalar null/range/UBO checks;1536 words,
1024guards,2submits,total GPU0.0347692ms,tiny correctness workload only.
SyncVal0errors/0warnings,callback1,Corezero. Strict --atomics verifier PASS.
Installed isolated APK null-atomics-native1-20261018 SHA256
E1DC7B0BCA8A2C8160F718E19D5307F983F2E96FF0138EDB5DF42A0AF48BD638.
core-null-atomics.json SHA256
51b7d15c9aad103d4d270f7efde6b11aced10e200292538fd8e6289d43195671.
Same APK fresh stock buffer regression PASS1536/896/SyncVal0/0,
report845265a2bc25156a8ef945a23d8e5fefe65a64b7065649f529312cc23837c836;
image regression PASS1792/384/SyncVal0/0,report98d28129... as below.

Shared configurable host shader compiler built and executed successfully from
upstream SPIRV-Tools eb0a366+headers85a1ed2 with reviewed buffer-atomics.patch.
Compiler executable SHA256
5134087A8C13CCACF722930FCFB67BF7982EECD2913790107C07A90403E39FA7.
Generated/independently validated shader SHA256
A0EF7D95782E1C7F551CF0EDF8FCA918BB5CDD8CF8F00486094ADC0C2720F381;
rebuild after flag-atomic preflight rejection emits identical shader bytes.
Patch adds core integer buffer atomic analysis+explicit type_traits include;
no Adreno source,Windows/Ubuntu/owner APK/platform/display/storage changes.
Stock legacy CLI helper still rejects ALL atomics. Combined compiler rejects
image/flag/float/pointer-arithmetic forms. All admitted opcodes are NOT certified:
only IAdd32 tested; out-of-range atomic,other atomic semantics/stages,full image
ops/dynamic arrays,robustness2 and complete BC Vulkan API remain OPEN.
No native/ICD feature advertising or DXVK/D3D11 PASS. Runtime Android shader
compiler/cache and common consumer descriptor/image adapter remain next.
No pending build/test worker. All sessions26072/35217 completed,do not resume.

2026-10-18 current extension: null sampled/storage images actual Mali PASS1792
words/384guards,4Core submits (2initial uploads+2dispatch),SyncVal0/0,Corezero.
Tests two static slots switched by metadata; RGBA8 texelFetch includes zero alpha
for null; R32_UINT imageLoad zero/imageStore discard verified by separate transfer
readback. Native app only,no VM/owner/H264/Ubuntu changes. APK null-images-native1
SHA25681F58FCCB3CB791E9EF750377863C605F5102E69316C4F96F7E49D1D7DD31D29;
report98d28129aecbfae968c4d2e3fe0053f784514e10d2a49c20cf6f10f916e5f561.
Strict verify-null-images.py PASS. Legacy instrument-null.py now has explicit
--mode buffers/images and rejects core/extension atomics;4offline parser tests
PASS,not shader/GPU conformance. nullDescriptor/robustness2 stay unadvertised.

Completed compiler preparation: configurable combined SPIRV pass+minimal integer-buffer atomic
extension. External source E:/Mavk/research/spirv-tools-v2022.4 pinnedeb0a366,
headers85a1ed2; compatibility/shaders/{CMakeLists,tool.cpp,buffer-atomics.patch}
track exact reviewable changes. Missing transitive type_traits caused first host
build failure; explicit include fixes standalone header check. Build restarted
incrementally parallel6 after stopping only verified CMake build process tree;
session26072 completed shader-tool-build1-20261018 with logbuild-fixed.
Bounded atomicAdd PASS is recorded above;do not extend it to other atomics.
Old buffer/image gate still uses stockCLI+oldmetadata;
atomics branch uses combined pass's four-level metadata. No pending native test
worker: latest atomics+buffer+image gates completed. This slice is uploaded only
to private Mavk after source checks;no public release or U-AVF push.

2026-10-18 scalar null/range extension actual Android PASS:1536 verified words,
896 guards,2submits,total GPU0.0305385ms (tiny correctness only),SyncVal0/0,
Corezero. Both metadata phases prove SSBO null reads0/writes discarded,UBO
null read0,range reads0/out-of-range stores leave allocated sentinels intact.
APK null-range-native3-20261018 SHA256
58E53BCF5AF64A04D07ED1E1AD05E808FFF61CB704655CAA59B72092584E7CC7.
core-null-buffers.json SHA256
8fd6be2fd9df016dd932e84b31297ac3642a782622e090a670c8aa062da4c961.
Strict verify-null-buffers.py PASS. First range run failed because CLI buffer
mode uses three-level metadata lookup,not four-level idx/init lookup; exact
GenDebugReadInit source+generated SPIR-V identified issue. Corrected metadata
only; failed report84ecbc0fc5395f7804b85a143cbc93aa31c3006e2c92f91e71900f244c3ed231
retained in null-range-native1-20261017. native2 diagnostic build NOT deployed.
Current common limitations: atomics/images/dynamic arrays/aggregate-pointer
access,full robustness2 and full BC Vulkan API remain OPEN. No feature bits
enabled; no DXVK/D3D11 PASS. Next common shader+descriptor/format adapter,then
actual Sarek device/frame. Baselines untouched,no Windows VM/owner change.

Newest user plan saved in ACTIVE_PLAN.md: real DroidDeck/DXVK D3D11 first,
then permanent Windows IddCx,then product Windows workspace integration.
Direct permission now allows commits/uploads ONLY to private dt0-imsyu/Mavk;
remote isPrivate=true verified,main initial547eadd includes Apache2 LICENSE,
preserved. No U-AVF commit/push/publication. Mavk local Git now tracks that main.

New null-buffer descriptor-init feasibility actual Android PASS768 verified
words/384 guarded accesses,SyncVal0/0,Corezero,2GPU submits,total GPU0.0181538ms
(tiny correctness workload,NOT performance target). Metadata switches null
slots without rebuilding shader,valid SSBO writes pass,null writes discarded,
UBO null reads zero. APK null-buffers-native1 SHA256
0362CA4CD986ACE18EA071D34E50B439C15F760A5527F0FC3FBE933AAA94B1EF.
Report SHA256 e0ceacc16af1c75a8e707b224722439bbf3158a9056b1ff5d8f9285b60aae859.
Same APK BC texture lifecycle regression PASS6144pixels/SyncVal0/0/Corezero;
report d90962435bfc8217017b08cc11527595b84aa57fff6eea9f0e9e29939cf41c20.
Uses existing NDK SPIRV-Tools guards,not dummy-storage semantic substitution.
No nullDescriptor/robustness2 advertising or DXVK PASS. Scalar range extension
being built/tested next; see compatibility/NULL_RESOURCES.md for exact gaps.
Old pass omits atomics/image-init in this mode; do NOT claim full null support.
The range build mentioned above is now completed by the2026-10-18 gate at top.
Same latest APK BC lifecycle regression strict PASS6144pixels/SyncVal0/0,
report d90962435bfc8217017b08cc11527595b84aa57fff6eea9f0e9e29939cf41c20.
No pending worker/build/test. Private snapshot includes source/checkpoints only,
not APKs,keys,VM disks or E:/ artifacts. Existing remote LICENSE unchanged.

Newest milestone common BC texture lifecycle actual PASS. BcTexture owns exact
compressed shadow+decoded cache for16 format identities; mip init masks, padded
block-region updates, same-format/overlap copies, dirty revisions and stale-ack
protection. CPU300 assertions/19 negatives PASS. Only touched blocks decoded.
Core adds explicit synchronous partial update (one staging+submit/fence per
upload, retained image/view/sampler+pipeline), native sRGB sampled format plus
default UNORM/float, runtime format checks. Native source bc_texture_comparison.
G925 PASS6144 pixels,UNORM+sRGB,12 GPU submits/2pipelines,18 decoded blocks,
256 partial-upload bytes,6 invalid native updates,SyncVal0/0,Corezero.
APK bc-texture-native1-20261017 SHA256
FC1D65B17ED84291373B81CD80173601C6278393CBA593663E459886C8090834.
core-bc-texture.json SHA256
a6f533489d4a26450c1566869bba2d0584e38d6c64ab0a46862399d2065a52ee.
No guest format/feature exposure, no Vulkan ICD interception, no full BC/API or
DXVK PASS. Complete initialized mip snapshots only;2D one layer,Core1024extent
bound. See compatibility/BC_TEXTURE_LIFECYCLE.md for scope/performance boundaries.
Next real adapters need correct execution-time upload integration, remaining
image/view/array semantics and null-resource implementation. Baselines untouched.
Same-build BC67 native regression strict PASS12288pixels/SyncVal0/0/Corezero;
retained RGBA8 regression fresh PASS32768pixels/32submits/zero per-frame GPU
allocations/SyncVal0/0/Corezero. No pending build/test process. Tablet free
54340200KiB (>50GiB), C56.9GiB/E874.8GiB. No cleanup required.

Newest milestone BC6H/BC7: bounded common wrappers use unmodified pinned bcdec
80859ed3b7afb1c527a2a99d70c61457bea72d0c, MIT alternative; notices preserved
third_party/bcdec, license included in isolated harness APK. No Adreno code.
Independent Microsoft D3D11 WARP host oracle PASS589824 components:BC7 all8modes
2048blocks,BC6H all14modes unsigned3584/signed3584. Separate3502 goldens/bounds,
18 negatives including reserved modes and+/-65504 extrema PASS. First detex
oracle disagreed; third BC7 reference and WARP support bcdec, not waived errors.
Counterexamples retained in compatibility/test/BC67_ORACLES.md +experiments.
Native G925 actual PASS12288 sampled pixels,3variants x4mips,12submits,
125negative/197HDR>1 input components,SyncVal0/0,Corezero. HDR native check uses
tone-mapped RGBA8 output, not full-precision GPU oracle. No performance claim.
bc67-native1-20261017 APK SHA256
3E60872F19E6C5AF87413A83FEDE9914FE09F6C4EB6787EC0C699FF550E1ECA4.
core-bc67.json SHA256
e30aecf1e3e6b663c29176c2666e3f0ddc70cdd97ca2beac002183640b36aa34.
Strict verify-bc.py --hdr PASS; previous BC45 same-build regression PASS.
RGBA8 retained regression from that build PASS32768 pixels,SyncVal0/0,Corezero.
Next: common compressed-image upload/mip/copy/view lifecycle+sRGB and real null
resource semantics. Codec family now present, NOT full textureCompressionBC,
Vulkan compressed API,DXVK/D3D11 or Windows acceleration PASS. Guest flags closed.
No Windows VM/owner/H264/Ubuntu changes, no commit/push.

Newest milestone: BC4/BC5 UNORM+SNORM actual CPU+native sampling PASS.
Shared decodeBcChannels preserves signed values/interpolation in float RGBA;
CPU exhaustive131072 endpoint pairs,8421776 comparisons,11 negatives,324edge
images. Native Core sampled upload now explicitly admits RGBA32F alongside
default RGBA8; format/filter query and matching mip byte layout, no wire change.
Actual G925 PASS16384 pixels,4format variants x4mips,16submits,SyncVal0/0,
Corezero. GPU verification remaps signed samples into RGBA8 output, NOT a
full-precision sampling oracle. FloatRGBA is a correctness representation,
NOT the final memory-optimized production format. BC6H/7 added by newer slice above.
bc45-native1-20261017 APK SHA256
AB37D453E3A615EA4AECC68D22BF1BB9F32AACF9C056E8A5ECA0F51D4D3570F1.
core-bc-channels.json SHA256
8e6178ea22651c23ff6aac7d23d6b81e613f22d1ad22800fc717d706a5dca09f.
Strict verifier PASS; old BC123 native sampling regression PASS on same APK.
Float mip CPU3 negatives +legacy mip9 negatives PASS. No VM/owner APK change.
Native retained RGBA8 graphics regression fresh PASS32768pixels,32submits,
zero per-frame allocations,SyncVal0/0,Corezero on same APK. No pending build/test.

User authorizes work on actual missing DXVK features; remote Mavk repository
exists at https://github.com/dt0-imsyu/Mavk (read-only HEAD checked). No commit,
push, publication or remote mutation. No Windows VM/production changes.
Full Turnip750 Mesa26.3-devel upload52003 +fresh Mali GPU-service profile saved
under compatibility/profiles; audit artifacts capability-audit-20261017.
Exact reviewed DroidDeck DXVK25ca63f:53 mandatory rows,46 pass,7 missing.
Windows guarded UI candidate remains unfinished as checkpointed below.

New Sarek lead checked at b6653094a2b8f3b824ffcf8f50aaa3b3da41263b:
48 mandatory rows,46 pass; remaining BC +nullDescriptor. Clip/cull removal is
reduced semantics, NOT emulation; don't import as generic Vulkan support.
Source/profile diff in E:/Mavk/artifacts/sarek-audit-20261017/comparison1.
First actual shared module implemented: BC1 RGB/RGBA,BC2,BC3 bounded CPU decode,
edge blocks,padded rows,alpha palettes,encoding metadata. CPU PASS862 assertions,
13 negatives,324 random small images. NOT full BC1-7/ICD/DXVK PASS.
Native existing-Core decode/upload/sampling actual PASS on Mali-G925:
bc123-native1-20261017,16384 verified pixels,4 format variants x4 mips,
16 GPU submits,SyncVal0 errors/0warnings,Corezero. Isolated harness APK
4307341CF42F763B2538C74FD458EFF8D4125FFDF0A6120B3170F0E78D515994
installed only org.mavk.probe. These tiny-texture timings are NOT performance
claims. No native Vulkan capabilities were changed or advertised.
Strict verify-bc.py PASS; core-bc.json SHA256
052dea0a01b1757dad38c751fc30dffc057a564361494d268356046608b9f400.
Existing retained-graphics regression fresh PASS on same APK:32768 pixels,
32 submits,zero retained per-frame allocations,SyncVal0/0,Corezero;
core-reuse.json SHA256 a1da9fa221f64e7477ccb1157fa55205ed549577a48a9f59c36adeb9cd584195.
Next: compressed-image lifecycle and explicit null-resource semantics. Sarek remains
experimental consumer lead, not a D3D11 PASS. No owner APK/VM/transport changes.


## Active priority — Turnip / Mali / DXVK capability audit

User-directed pivot: compare a pinned modern Turnip reference, measured G925
inventory and actual Mavk coverage; identify minimum honest DXVK/D3D11 admission
gates before more implementation. No guest boot required. Preserve Windows branch.

Windows UI ICD candidate checkpoint: guarded MAVK_EXPERIMENTAL_UI_LAYERS records
per-draw snapshots and emits opcode24 ordered groups. CPU recording test PASS
(3 snapshots/5 negatives); CPU packet test PASS (2 groups/3 snapshots/6 negatives).
These are NOT loader/GPU/guest proofs. Later sampler-binding0/font-only descriptor
factory edits are UNTESTED; pipeline factory blend/packed-color/depth-disabled
admission remains unfinished. No ARM64 candidate deployment. Preserve source,
keep feature guard off in baseline; resume compiler/regression gates first.
Last known-good Windows textured application and native Bridge24 proofs below
remain unchanged. No VM, owner APK, Windows/Ubuntu/H264 or storage changes.


## Current — bounded layers command shared Bridge actual native packet PASS

2026-10-17: previous turn actual native layered UI PASS. New opcode24
RenderLayersSnapshot implemented:inner v1 packet,1..8 exact records,explicit
blend/sampler binding/texture ID/UBO/extended scene,per-draw base vertex+scissor.
Old13/14/17/23 formats and shader-binding defaults preserved. Packed RGBA8
admitted only in new extended scene. Full packet CPU/schema/descriptor/shader
admission before any packet-owned GPU pipeline; old snapshot bounds preserved.
Pure parser test PASS578 rejects incl all truncations; legacy parser regression
running. Actual original ImGui workload extended to encode Bridge packets,
upload actual font through19/20/21,render24/read15/destroy16. Runtime gate is
in-process encoded request/response, NOT FD/vsock or Windows PASS; existing
generic FD framing is not being rewritten. Tests19 negative packet/admission
cases, asserts no new GPU allocation/pipeline/submit on rejection. Actual
PASS:30 commands,19 negatives,4608 correct pixels,258 changed widget pixels,
3GPU submissions(font+2frames),5pipelines(compute+4graphics),SyncVal0/0,
Corezero. Strict verify-native-ui.py --bridge PASS.
Evidence bridge-ui-layers1-20261017/core-ui-bridge.json SHA
a594262c969537991b5362ae5a8cd4f616019a85e35d857e32b645e92fce67ce.
Harness APKB07C7453... installed only org.mavk.probe. CPU regression PASS:
new layers578,legacy scene630,shader12,geometry275 rejection cases.
Legacy descriptor texture regression actual PASS:4096pixels,40commands,
16negatives,5GPU,3pipelines,SyncVal0/0,EOF/Corezero. Default non-UI native
translation-unit compile PASS. No pending build/test process.
Native24 currently builds/retire chains per request; no new pipeline-cache or
FPS claim. Next real consumer=Windows ICD command snapshots/multiple pipelines,
but no guest advertising until implemented and tested. Current ICD Command
still keeps one pipeline and rejects bind after drawing; implement explicit
per-draw state snapshots/ordered groups rather than overwrite those fields.
Keep a guarded candidate and preserve default ICD behavior/closed gates.
No pending VM/carrier. Tablet free52.5GiB; no unrelated cleanup needed.
No VM/production owner/Windows/Ubuntu/H264 changes, no commit/push.

## Current — shared GPU target / preserved UI layers actual PASS

2026-10-17: previous turn was actual progress (native upstream UI packet PASS).
Next gap implemented in shared renderer: reference-owned color target, root
CLEAR then following pass LOAD, separate depth per pass; same command batch,
one submit and one final GPU→CPU readback. renderLayers accepts per-layer
push/UBO/scissor inputs, validates all before recording, bound16 layers. Child
retains parent pipeline/target; no raw dangling target ownership. Existing
single-layer API defaults preserved, multi-layer input misuse rejected.
Relevant gate reuses actual ImGui test, adds distinct depth-enabled background
pipeline writing a blue rectangle different from clear, verifies preservation
under/around alpha UI, checks image sharing and parent-handle retirement.
Actual PASS:2frames,2passes/frame,1shared color target,one submit/final readback
per frame; font upload adds1GPU submit,total3.4pipelines,4608 verified pixels,
258 changed widget-region pixels,4 negative API inputs,SyncVal0/0,Corezero.
Images-count assertion proves font+one color+one depth (not per-layer color).
Parent public handle retired before render; child lifetime retained correctly.
Evidence native-ui-layers1-20261017/core-ui-layers.json SHA
ffb1a024560423ecf02c081715b847f178ca9e633ac864602355697d45dde59e.
Harness APKCF793D8B... installed only org.mavk.probe. Relevant single-layer
reuse regression PASS:32768pixels,32submits,zero per-frame GPU allocations,
SyncVal0/0,Corezero. Its new timing evidence is not a production benchmark.
Bridge regression PASS:4096pixels,40commands,16negatives,5GPU,3pipelines,
SyncVal0/0,EOF/Corezero. Default non-UI native translation unit compile PASS.
Suite strict cached aggregation PASS13 bounded NATIVE rows (not a fresh full
run); SHA dc3db2f33920c2bdf65a34fce452718bc4b7cc1cfd0675f49aa092deca242a3f.
No pending build/test/VM or carrier patch. Next practical adapter step is
bounded multi-pipeline scene/packet support in Bridge+ICD, retaining actual
native state semantics and not faking guest support. Current new layer path is
native-only. Avoid more unrelated synthetic feature/benchmark polishing.
No Windows/Ubuntu/owner APK/transport changes, no commit/push.

## Current — actual upstream UI packets native PASS (not full app/guest PASS)

2026-10-17: pinned upstream ImGui1.65 sources copied with Git-blob verification
to E:/Mavk/research/ui-4ee2501; original GLSL UI shaders fetched/pinned from
commit4ee2501c8280cf71d1f9ce231cd8ebd47b58adde (no shader rewrites).
Optional -UiSourceDirectory build compiles original3cpp and carries licenses.
Shared renderer now supports native sampler binding0, exact upstream alpha
equation, packed RGBA8 vertex attributes via native geometry, per-draw scissor
and base vertex, full-target dynamic viewport. Native scene bounds checked;
Bridge wire decoder still defaults new native fields, no ICD advertising change.
Native gate creates real font atlas/widgets +two analytic draw lists, adapts
original packets, uploads font once and renders2 frames. Includes exact probe
rectangle pixels and changed widget pixel counts, not full glyph oracle/3D
overlay/complete upstream app or guest PASS. Actual native PASS:font512x64,
2frames,8 indexed draws including4 widget and6 nonzero-base-vertex draws,
4608 analytically verified pixels,258 changed widget-region pixels,3GPU
submissions,2pipelines,SyncVal0errors/0warnings,Corezero. Strict UI verifier PASS.
Evidence E:/Mavk/artifacts/native-ui1-20261017/core-ui.json SHA
01bf53a5a3ae40e8c5baafc24805ef8caa8eee9c1d94c734016e74444137db51.
Harness APK64A3FF40... installed only org.mavk.probe. Relevant raster regression
18432pixels/30negatives and Bridge4096pixels/16negatives/40commands also PASS
same new build,SyncVal0/0 and clean teardown. No VM/production change.
No pending test/build process; no carrier patch. Existing13-row suite remains
cached bounded evidence; native UI proof is separate, NOT new guest advertising.
Next concrete gap: multiple pipeline/pass attachment preservation, then native
UI packet adapter requirements in Bridge/ICD; no fake shader substitution or
full-app/Windows claim. See UI_NEXT_SLICE.md. Goal active; no commit/push.

## Current — shared native alpha blending / dynamic clipping PASS

2026-10-17: next real-app compatibility slice, not another performance benchmark.
Shared renderer adds opt-in straight-alpha source-over and per-frame dynamic
scissor (null restores full target). Default blending remains disabled; legacy
callers still render full target. Native format blend support queried explicitly.
No Bridge wire/ICD advertising changes. Existing sampled shaders reused for
18 frames:alpha0/128/255,replace/source-over,full/partial/full-reset clipping;
CPU pixel oracle and30 rejected clipping bounds. Actual native PASS:18432
pixels,18 GPU submissions,6 pipelines,no per-frame Core GPU allocations,
SyncVal0errors/0warnings,Corezero. Evidence native-raster1-20261017/core-raster.json
SHA6db8b5eeb4e7c0d375889897a71801c9a279a9ce023ed5b64143c27a23b6b1d2.
Same new build Bridge descriptor regression actual PASS:4096 pixels,40commands,
16negative,5GPU,3pipelines,SyncVal0/0,EOF/Corezero; bridge-probe.json SHA9e4e0856...
(deterministic identical report; mtime08:14:50 after APK update08:14:15 confirms
new run, not stale collection). No VM/production owner update. Suite aggregation
of cached source reports verified:13 bounded NATIVE rows,CACHED_REPORTS,strict
verifier PASS; no false fresh-suite claim or rerunning closed gates.
Evidence feature-suite-raster1-20261017/feature-suite.json SHA
62b359323aeac7c2cca1ed05b62499beca7aeaf1b136d7a23fd22d1f8c912355.
Harness APK0E088838... installed only org.mavk.probe. No pending test process.
Next concrete consumer: pinned upstream UI overlay, see UI_NEXT_SLICE.md.
Its alpha equation is NOT identical to source-over; do not silently substitute
it or claim overlay compatibility. Core native first, periodic ICD/Bridge gate
after adapter changes, WDDM separate. Goal active and incomplete.

## Current — retained renderer in shared Bridge actual native PASS

2026-10-17 next aligned step: shared Bridge uniform/sampled snapshot commands
now use one bounded OffscreenSession keyed by exact immutable scene bytes,
opcode,texture ID and UBO size; UBO values change without rebuild. Any texture
destroy and session teardown retire renderer before textures. Wire protocol,
ICD, production owner APK and VM unchanged. No guest performance claim.
Native local descriptor test extended from existing texture gate:4 draws,
changing UBO pixels, immutable-field invalidation, explicit+disconnect cleanup.
Actual PASS:40 commands,16 invalid requests,5GPU submissions,4096 verified
pixels,5592404 uploaded bytes/11 mips,compute1+graphics2 pipeline creations.
SyncVal0errors/0warnings,clean EOF and Corezero; strict verify-bridge-reuse.py
PASS. Evidence E:/Mavk/artifacts/bridge-retained1-20261017/bridge-probe.json
SHA9e4e085602328cd05b75533a08278edc2acb821d85cb1e07f9c79346509d4e1e.
Harness APK28696C07... installed only org.mavk.probe. This is NOT a guest
performance or production owner update. Next adapter regression on its next
justified Windows gate; native-first policy remains. No pending VM/test process.

## Current — native shared renderer reuse PASS; suite integration verified

2026-10-17: relevant native A/B completed, not abandoned. `OffscreenSession`
retains pipeline/target/descriptors/buffers/fence/commands; synchronous readback
and legacy one-shot path preserved.32 verified frames,32768 pixels,4 distinct
LOD outputs,2 rejected inputs; pipeline creates16→1, Core GPU allocations64→4,
zero per-frame GPU allocations. SyncVal0errors/0warnings, teardown Corezero.
32x32 validation-enabled sequential A/B median5.64942→0.339231ms;
retained initialization2.41915ms. NOT desktop/guest/visible FPS or a statistical
performance claim. Bridge integration is now proven locally in milestone above;
production owner APK and Windows runtime remain unchanged.
Evidence E:/Mavk/artifacts/native-graphics-reuse1-20261017/core-reuse.json,
SHA08e13b0844273153ff66dfc9c9937862bc45aac3cd65dafdaea0d33f1bb460b0.
Feature Suite integration completed:11 bounded NATIVE rows,strict verifier
PASS in honest CACHED_REPORTS mode; prior gates not rerun. Source hashes and
device identity verified across all4 reports. Suite evidence
E:/Mavk/artifacts/feature-suite-reuse1-20261017/feature-suite.json,
SHA2bd843d90e4e3783a79724677d8fdc6973cca51303c7b1a6ab25c4b520448093.
Harness APKBE7D1375... installed only org.mavk.probe.
Next: apply reusable session to an actual adapter need rather than further
synthetic performance polishing; Bridge/ICD changes require their relevant
gate. Native timeline/retained graphics are NOT automatically guest features.
Windows remains active adapter, no VM needed for this completed native slice.
No owner APK/platform/Windows/Ubuntu/display changes. No commit/push.

## Latest milestone — unmodified external Windows Vulkan app PASS

2026-10-17 CURRENT Core timeline native PASS, no Windows boot this milestone.
Core TimelineSemaphore owns/query/wait/hostsignal; explicit owner-enabled
Vulkan1.2 capability, unavailable context rejected before driver calls. New
timeline feature path is opt-in; existing Root/DeviceContext defaults unchanged.
GPU fill→timeline dependency→copy→hostread in2 rounds,4GPU submits,8192 verified
words,2 final host waits (no intermediate CPU fence),timeout1,negative3,
finalcounter5,SyncVal0errors/0warnings,Corezero. Report core-timeline-build1/
core-timeline.json SHA cebec7a32d9418896d1c53f814ff6bb726648015d74ffc3456e320d74d06bced.
Feature Suite fresh common-Core regression PASS,now10 bounded NATIVE rows;
existing compute169869312/image81231872/sampled12288 passed same new build.
Suite SHA f785128adc6bf97644e1273110c3413d5714725da1f17d3fe90ddf97534a8ed3.
APK955F0F77... installed only org.mavk.probe. No Bridge/ICD capability flags
changed; native timeline proof is NOT async guest Bridge/game/performance PASS.
Next: extend reusable Core toward actual adapter needs; native relevant gate
first. Do not rerun Windows texture/compute or other closed gates without a
specific adapter/common regression reason. Goal remains active and incomplete.

2026-10-17 CURRENT Feature Suite actual Android PASS, default next work=Core.
FeatureSuiteActivity thin runner reuses2 existing native JNI tests, no renderer
duplication or VM. Fresh integration gate:169869312 compute words,81231872
image-transfer pixels,12288 sampled pixels;9 bounded NATIVE feature rows PASS,
SyncVal0/0 and Corezero. EMULATED Windows readback/blit explicitly separate gate;
UNSUPPORTED means unimplemented Mavk wrapper/adapter, NOT missing Mali hardware.
No CTS/game/generalVulkan claim. Strict verify-feature-suite.py --fresh PASS.
APK feature-suite-build3 SHA9A0889A7... installed only org.mavk.probe; evidence
feature-suite.json SHA dade19f5a0ec0fd35ee938b9db8be1b5f0625d270edd889de93ae21ffdd1b6e7.
Saved-result button tested:modeCACHED_REPORTS and source hashes unchanged,
no new Core run; cached report1c7de25a.... Screenshot suite1.png inspected.
Windows texture milestone below completed, not aborted: actual app window and
both24frame exits0. No VM active or carrier patch remains; observer48058 stopped
after normal guest shutdown. Storage C54.1GiB/E875.8GiB; no commit/push.
Next new capabilities first through Core+relevant native test; avoid rerunning
all closed gates unnecessarily. Windows ICD active adapter, WDDM separate.

2026-10-17 CURRENT Windows textured upstream app actual PASS CID2347
(mavk-windows-texture2-20261017). Original pinned texture.cpp/shaders/KTX,
no app edits. Standard loader+ICD→4057→Mali. IMMEDIATE/FIFO24frames each,
each79commands/25GPU,rx1540888/tx7376012,SyncVal0/0,EOF/Corezero. Both waited
process exits0 (UTF16 PowerShell exit files, decoded by BOM); both benchmark
CSVs24frames. Actual correct metalplate in Android texture-fifo1.png inspected.
Strict verify-texture-report.py PASS. NOT game,WDDM,DWM,60visibleFPS or CTS.
Evidence windows-texture-bundle2-20261017 host-texture{1,-fifo1}.json SHA
6c2457fea06965ce2e2141d37436453287c8928a1aee86fa9825a91c3bbad570,
guest-texture-cold/{exit2,exit3,stderr,stderr2,benchmark.csv,fifo.csv}.
CID2346 was intentionally stopped on user architecture pivot before app launch;
one justified resume used already delivered files, no repeated fetch or changes.
Normal shutdown actualVM[] confirmed, texture-carrier-rollback1 exact3381bd95
PASS; no pending carrier. Observer48058 may still be finite/read-only, VM stopped.

USER PRIORITY update: compact Android Feature Suite first, reuse native Core
tests, NATIVE/EMULATED/UNSUPPORTED classification separate from PASS/NOT_RUN.
Windows ICD remains active; test ICD/Bridge changes and periodic common updates,
not every native feature. WDDM separate. No more automatic Windows boots.
FeatureSuiteActivity source implemented, existing JNI Core comparison+sampled
reused; cached vs fresh, hashes and scoped capabilities. feature-suite-build3
in progress (61391), NOT installed/runtime PASS yet. Next build terminal check,
install only org.mavk.probe, one fresh suite integration run, verify report,
document actual remaining Core capability slice. No production changes/commit.
Earlier LIVE/rollback-pending paragraphs below superseded.

2026-10-17 LIVE test in progress: owner texture-build1 installed; new chunked
carrier applied exact57331302... at36028416. ROLLBACK PENDING untilVM[].
Approved launch evidence mavk-windows-texture1-20261017, observer44634 live,
current owner token only in that evidence/private file. Boot progressing through
driver notifications, no Vulkan consumer launched yet. Do not restart blindly.
Use new chunked-fetch-preflight1-20261017 for apply/rollback, not old fetchplan.
After launch/test shutdown actualVM[], restore original3381bd95... and coldlogs.

2026-10-17 NEXT external texture build ready, NOT yet guest PASS. Immutable
upstream texture app+KTX compiled ARM64 E4994340... from82 verified source
blobs; asset submodule a27c0e584434d59b7c7a714e9180eefca6f0ec4b, original
512 RGBA10mips1398236B KTX SHA38495B08.... Bundle1853775B EC5E66AC... at
windows-texture-bundle2-20261017. One1MiB legacy archive bound rejected it;
development op9 now additionally supports fixed archive size+bounded offset
chunks, maxarchive4MiB, unchanged1MiB packet. No arbitrary files/paths or
graphics protocol changes. Chunked fetcher ARM647680B SHA3670BBF1..., cold
preflight chunked-fetch-preflight1-20261017 candidate57331302... NOT applied.
Actual same-UID archive test PASS:10commands,6 negatives,1853775 exact bytes,
EC5E66AC... matched,EOF/Corezero;0GPU and VVL disabled (not graphics proof).
archive-local.json SHA cb1eb35bec031b7a3b4f4f876ef12ee8989df785f2bef4837273e4235902ed7e.
Native bridge-texture-delivery2 APK DCAB139D... installed only org.mavk.probe.
Owner texture-build1 terminal0 compiled, not installed yet. CurrentVM[];
tablet61%,52.6GiB free,C55.4/E876GiB. Next coldowner install+newcarrier apply,
approved profile, hash-guard rename current TEMP ZIP99267DAE... to unused
sampled0 suffix,chunkfetch,hashEC5E66AC... expand MavkTexture1,waited upstream
24frame320x240 run+hostprobe. Then normalshutdown,rollback,coldlogs and actual
image review. No raw/display/firmware/CPU/Ubuntu edits. Goal remains active.

2026-10-16 CURRENT CID2345 standard sampled texture ICD actual Windows PASS.
Standard vulkan-1.dll→candidate ICD→app-owned4057→Mali. Own consumer verified
3072 pixels in3 indexed textured renders (LOD0/1/3), full1024 RGBA11mip chain,
5592404B upload, retained native cache reuse, second content revision invalidation,
late staging/UBO writes,148B UBO descriptor. Guest EXIT0; host29commands/5GPU,
rx11193996/tx13572,cleanEOF/Corezero,SyncVal0errors/0warnings. Strict
verify-sampled-report.py PASS. NOT external texture compatibility/performance,
games,WDDM,DWM or conformance proof. Evidence windows-sampled-bundle1-20261016:
host-sampled1.json SHA b7f6141f7624f0ec21f1311dd0e4a15939b00ffc82a48bdc346339070e040847,
guest-sampled-cold/stdout.txt SHA420e77187c868ceaadef37efda52131166e5f48c0b0202d42fcb815ddbe17e90.
OwnerAPK FA11E0D6...,native4AFFBFD6...,ICD9A048F5A... retained E.
Normal shutdown actualVM[] confirmed; sampled-carrier-rollback1 exactoriginal
3381bd95... PASS. Observer71551 retained only for finite observation, VM stopped.
No Ubuntu/P7/firmware/CPU/display change, no commit/push. Next bounded gate:
unchanged pinned upstream texture example+actual KTX bytes; inspect minimal
dependency/API needs before boot. Earlier NOT-delivered candidate notes superseded.

2026-10-16 CURRENT sampled ICD build5 host gates PASS, not yet guest PASS.
Typed set0 UBO+combined sampler layout/pool/write/bind implemented behind
MAVK_EXPERIMENTAL_SAMPLED; lazy19..21 upload/cache,23 render,22 invalidation.
CPU resource11mips/5592404B/12rejects and descriptor148B/9rejects/latewrites PASS.
Standard API ARM64 consumer built:3 textured draws,mip0/1/3,2 content revisions,
3072 expected pixels. No Mavk calls in consumer. DLL9A048F5A... build5.
Default macro-off compute7/UBO12/push14/readback4MiB CPU gates PASS, ARM64/x64
builds terminal0 (default-regression2). Prepared sampled bundle558843B,
windows-sampled-bundle1-20261016, NOT delivered yet. Native delivery build
in progress bridge-sampled-delivery1-20261016. Next owner build/cold install,
guarded original carrier fetch, finite Windows test, shutdown/rollback/coldlogs.
VM[] observed, battery40%, tablet52.6GiB/C55GiB/E876GiB free. No carrier pending.
Earlier 'descriptors not implemented' partial-candidate notes below superseded.

2026-10-16 CURRENT next Windows sampled candidate: compile-time
MAVK_EXPERIMENTAL_SAMPLED only. Standard vkCreateImage/sampler/full-chain view,
full-mip vkCmdCopyBufferToImage + ordered barriers implemented; input read at
QueueSubmit. CPU test5592404B/11mips/latewrites/12invalid requests PASS.
ARM64/x64 candidate build1 terminal0, ARM64DLL
3CC53449E34FD9A42CBEB1F8D552B1C0095676FE6B354A22CF6C678141D77805,
E:/Mavk/artifacts/windows-icd-sampled-build1-20261016. NOT delivered to Windows.
Sampled format/features/descriptors not advertised/implemented in this partial
candidate yet. Existing default4MiB limit preserved; only opt-in candidate8MiB
for full mip staging, total32MiB unchanged. No device/system driver install.
Remaining exact API work: set0 binding0 UBO+binding1 combined sampler layout,
pool/update/bind; keep typed compute/old UBO cases. At draw, lazy create/chunk/
commit19..21,cache remote texture+content revision+sampler state,reuse until
mutation/destruction; prependtextureID to op23 UBO scene. Complete typed cleanup
and honest sampled capabilities only with tested implementation. Then finite
own standard API texture test, followed by unchanged external texture app.
Do not VM boot this partial candidate. Default build regression exec16397
terminal0:ARM64/x64 DLL build,compute7/UBO12/push14 rejects and4MiB readback
CPU PASS. Default ARM64DLL B15C4DE5... is a build regression,NOT new guest PASS.
Native sampled verifier Mavk/harness/verify-sampled.py gates actual saved reports.

2026-10-16 PERSISTENT TEXTURE BRIDGE PASS, not yet Windows texture API.
Create19/Write20/Commit21/Destroy22/RenderSampledSnapshot23 implemented.
1024 RGBA full11mip chain5592404B over6 bounded chunks;one real native upload
retained on GPU and reused by TWO UBO/indexed textured renders.2048 pixels
verified,16 rejected invalid requests. Native3GPU,34commands,cleanEOF/Corezero,
SyncVal0errors/0warnings. Explicit texture destroy and partial-upload disconnect
cleanup covered. Old wire13/14/17 and fragment-descriptor rejection preserved;
new sampled fragment permits ONLY set0/binding1. CPU shader guard12rejects PASS.
Evidence native-sampled-build1-20261016/bridge-sampled.json SHA
9318b59f67a12be946124185584cc49f031e7e7ddc47312c2d161bd3c9adafe9.
Isolated harness bridge-sampled-harness1 APK50AB655D... installed only org.mavk.probe.
OwnerWindows APK/profile/data unchanged; no active VM or carrier patch.
Next implement candidate standard Vulkan image upload/sampler/descriptors in
compile-time opt-in ICD, CPU recording tests,then one finite standard API guest
test before claiming Windows texture support. Native gate does NOT establish it.

2026-10-16 NEXT SLICE native texture gate PASS (not yet Windows textures).
Core SampledImage owns RGBA8/mip upload, sampler/view, staging lifetime and
explicit transfer→shader barriers. Hardware tested4mips,2contents,nearest,
trilinear mip blend and bilinear spatial sampling:12288 verified pixels/12GPU,
SyncVal0errors/0warnings,Corezero after EACH job. Evidence
E:/Mavk/artifacts/native-sampled-build1-20261016/core-sampled.json SHA
420a235431d23800fcea2196b6a7c77a489cee1180322bac3480eb047bfc3970.
CPU mip layout9rejects/full1024 chain5592404B PASS. Existing scene bridge
regression3330pixels/1GPU/8rejects/SyncVal0/0 PASS (scene-regression.json).
Isolated harnessAPK3952A6F4... only org.mavk.probe installed; owner APK unchanged.
Native helper drawOffscreen gains optional texture, old wire guards unchanged.
Next: bounded persistent texture Bridge (chunked upload under1MiB packets,
upload once and retain GPU resource), then ICD transfer/image/sampler descriptors
for pinned unchanged external texture app. No VM is active or needed for native
Bridge gate; do not rerun proven window test blindly. Goal still active.

CURRENT PASS CID2344: unchanged pinned SaschaWillems upstream triangle completed
24 hardware Mali frames in IMMEDIATE and24 in FIFO. Standard loader→ICD→4057→
Mali; each host session74commands/24GPU, cleanEOF/Corezero,SyncVal0/0.
Windows FIFO process EXIT0; both original benchmark CSVs24frames. First cmd GUI
wrapper exit.txt absent, NOT assumed0; follow-up used existing ordinary Run with
PowerShell Start-Process -Wait -PassThru and separate exit2.txt/fifo.csv.
Strict verify-triangle-report.py PASS. Android fifo1.png shows correct RGB
triangle in real upstream window through unchanged H264. Not sustained/unique
visible-FPS,zero-copy,general WSI,game,WDDM/DWM or conformance proof.
Evidence E:/Mavk/artifacts/windows-view-triangle-bundle1-20261016:
host-{triangle,fifo}.json SHA447929407f38f47257ae91181d0cabf682f917ab9316c8e540a8e4d15f35548a,
guest-triangle-cold/{benchmark.csv,fifo.csv,exit2.txt,stderr.txt},fifo1.png.
Normal shutdown/actualVM[]; view-carrier-rollback1 exact3381bd95... PASS.
Observer92166 terminal0 retains VM only until manual shutdown (now stopped).
Diagnostic ownerAPK B0D36B51...,ICD A433DF72...,native C7C231DC... retained E.
No active VM or pending carrier rollback remains. No commit/push.
Next meaningful compatibility slice: ordinary app with sampled textures;
first inspect actual upstream requirements, implement only bounded needed API
and native sampler/upload semantics. Do not repeat triangle/compute gates blindly.
Broader Mavk/Windows GPU goal remains incomplete. Historical launch notes below
are evidence, NOT instructions to resume old LIVE handles/carrier writes.

CURRENT follow-up: CID2343 upstream triangle now passes compute queue setup,
but stops at vkCreateImageView, VulkanSwapChain.cpp:353 (explicit RGBA mapping).
Guest stderr193B/no EXIT/CSV; host2commands/0GPU,cleanEOF/SyncVal0/0. Evidence
windows-compute-triangle-bundle1-20261016/guest-triangle2-cold and host-triangle2.
Normal shutdown/actualVM[] confirmed; observer25832 intentionally interrupted.
No carrier used in this boot. Minimal source fix accepts per-component explicit
R/G/B/A as identity, still rejects channel rearrangement/constants. All16 identity
combinations and3 invalid mappings pass CPU WSI test; compute/UBO regressions
CPU PASS. CandidateDLL A433DF72...; new bundle1011256B SHA
46e7121a35dd8de4eff0f8e9992ce00973e3d93692bcd26788f7883d690cd0f4,
windows-view-triangle-bundle1-20261016. Native build4623 terminal0;
owner build87144 terminal0 APK B0D36B51... installed diagnostic only.
LIVE CID2344, observer92166, mavk-windows-triangle-view1-20261016 token ONLY.
Exact view-carrier-apply1 PASS; MUST shutdown/actualVM[]/view-carrier-rollback1.
Next rename old ZIP (hash21E326...),fetch new ZIP,expandMavkTriangle3,upstream run.
Prepared short commands in new bundle directory; all<=252 characters.
Unchanged upstream application/shaders, Windows platform and production paths.

2026-10-16 STANDARD COMPUTE ICD PASS, CID2342. Standard Windows Vulkan Loader
→ application-local ICD → app-owned4057 → Mali executed TWO actual application
SPIR-V dispatches. Guest verified8192 words, late mapped writes and prefix/suffix
guards, EXIT0. Host4commands/2GPU, cleanEOF/Corezero, SyncVal0errors/0warnings.
Evidence windows-compute-triangle-bundle1-20261016/host-compute.json SHA
2cfc5b021272506a6f657232a67b7ac5590d85d5cb1de5ac99b312615e7ff697 and
guest-compute-cold; strict verify-compute-report.py PASS. Normal shutdown/VM[];
compute-carrier-rollback3 exact original3381bd95... PASS. No carrier remains.
Fetch first admission failed connect; timely retry delivered1011004 bytes.
All3 host admissions consumed, so upstream window test needs next cold boot;
already delivered ZIP remains, no carrier or another delivery needed.
Next: same approved profile, expand hash-guarded ZIP to MavkTriangle2, run
UNCHANGED pinned upstream triangle with genuine compute queue now available.
Bounded one-SSBO/1D synchronous compute only, NOT Vulkan conformance/general
compute/performance or WDDM/DWM acceleration. Below candidate notes historical.

2026-10-16 NEXT CANDIDATE: genuine standard compute ICD (not yet guest PASS).
LIVE retry3 mavk-windows-compute-icd3-20261016; token only its directory.
Attempt2 never created VM:52GiB reserve guard,actualVM[]. Carrierrollback2PASS.
Archived ONLY eight old QEMU v1-v4 img/raw files from /sdcard/Download to
E:/U-AVF/artifacts/tablet-old-qemu-images-20261016, all SHA256 verified before
removal; logs and latestv5 remain on tablet. Recovered768MiB,available52.64GiB.
No user media/workspaces/current Windows disk removed. Recoverable from E.
Compute-carrier-apply3 PASS; MUST normal shutdown/VM[]/rollback3 after test.
One set0/binding0 compute SSBO<=512KiB, literal1D local<=256/groups<=256,
push<=64B, synchronous native dispatch and readback. Opcode18 sends actual
app SPIR-V/input/push bytes. Old graphics/UBO wire unchanged. Own API consumer
uses different shader from fixed Bridge probe, two seeds +late mapped inputs,
8192 words +prefix/suffix guards. No application-specific shader substitution.
CPU compute7rejects/UBO12/push14/readback4MiB passed; ARM64/x64 DLL compiled
0ae66d62... . Native/owner build exec26172 terminal0.
Attempt1 exec22314 handle missing on resume, actualVM[]; CID2341 hit native
Windows bugcheck1DB/IPI watchdog BEFORE any fetch or ICD test. Not compute FAIL
or PASS. Original carrier restored by compute-rollback1 exact3381bd95... .
Owner report confirms display had connected; stale first error wasn't a hang.
Retry2 same frozen profile justified to execute previously unrun API workload;
no CPU/timer/GIC/firmware mutation. No random platform investigation.
Previous attempt: mavk-windows-compute-icd1-20261016, unchanged approved profile,
owner7d230c8f... and carriercompute-apply1 exact32bae66f... . Current token ONLY
that evidence directory. Bundle1010924B SHA21e32640b8db56a863c0933716c94982614b4badbe00f465d4c82c147d581911.
Own existing ZIP SHA85a682ee... must be renamed before fixed fetch. Prepared
short Run commands in E:/Mavk/artifacts/windows-compute-triangle-bundle1-20261016.
After fetch: expandMavkCompute1 +compute/hostprobe; expandMavkTriangle2 +triangle/
hostprobe only if compute succeeds. Then normal shutdown/VM[]/cold logs --compute
and --triangle --triangle-attempt2, exact compute-carrier-rollback1 REQUIRED.
Next finite boot: fetch bundle → own standard compute → upstream triangle,
max3 host sessions. Must pass actual compute before claiming queue capability.
No new production installation. Carrier rollback remains required if applied.

Triangle checkpoint CLOSED, not PASS: CID2340, unchanged8CPU profile.
Ownertriangle-build1 installed, exact guarded carriertriangle-apply1 PASS.
Normal shutdown/actualVM[] and triangle-rollback1 exact3381bd95... PASS.
Observer61293 intentionally interrupted AFTER shutdown/rollback (terminal1).
No active VM or test remains. OwnerAPK4f9d092b... isolated diagnostic only.
Bundle E:/Mavk/artifacts/windows-triangle-bundle1-20261016,880834B,
SHA85a682ee80e58f4bdc1d4afcb70391ac4507d3f817e5aeab521cb0ede9b43b01.
Unmodified pinned upstream triangle +actual framework ARM64 compiled;
46 downloaded files about2MB verified against immutable git blob tree.
build-triangle.ps1 records source manifest. No application/shader/helper edits.
Own namespaceMavkTriangle1; short Run expansion242chars/test235chars prepared.
CLI320x240 -b -bw0 -br10 -bfs24 -bf benchmark.csv (bounded24 frames,not FPS claim).
Fetch verified880914 sentbytes; API host2commands/0GPU,cleanEOF,SyncVal0/0,
Corezero. Guest cold logs confirm uncaught std::runtime_error:
"Could not find a matching queue family index". No exit marker/benchmark CSV:
do NOT count as success or infer EXIT0. Source VulkanDevice.cpp:208-210 requests
COMPUTE in its default logical-device setup; current ICD advertises GRAPHICS|
TRANSFER only. This is a concrete ICD API coverage blocker, NOT GZVM/Mali failure.
Do not add a fake COMPUTE queue flag just to get past the framework. Next slice
must actually implement standard compute pipeline/dispatch/resource semantics,
or choose an independently useful graphics-only app without changing its source.
No need to repeat this launch before that concrete capability exists.
Evidence host-triangle.json SHA9bf40b105831eeb52ccaf4e83b0f1331d290bff55503a96157d2e02356814b89,
guest-triangle-cold/{stdout,stderr,extraction}; fetch and screenshots preserved.
Required upstream two command buffers/reset now added; CPU12rejects PASS,
optional WSI build ARM64 F5B6B087... . Reset/multi-buffer entry CPU-only PASS;
upstream stopped before executing these entries. Do not claim guest coverage.
Old UBO/headless PASS preserved. This test's report must be judged fresh.

2026-10-16 UBO milestone PASS: CID2339, unchanged8CPU/platform/display.
Standard Windows Vulkan Loader → app-local ICD → app-owned4057 → Mali:
192B vertex UBO/set0/binding0, descriptor offset32, mapped writes AFTER command
recording, nonidentity P*V*M, indexed staging. Guest verified3906 pixels,EXIT0.
Host5commands/1GPU,2512rx/16852tx,SyncVal0errors/0warnings,cleanEOF/Corezero.
Evidence E:/Mavk/artifacts/windows-uniform-bundle1-20261016:
host-uniform-retry.json SHA098273666b7085d228a733210cf41e39776490bf883559792d2cba0937cec33c,
guest-uniform-retry-cold; strict verify-uniform-report.py PASS.
First CID2338 rejected beforeGPU: old shaderWords guard disallowed descriptor
decorations. Fixed ONLY opcode17 vertex set0/binding0 allowance; fragment and
old graphics descriptor rejection retained. Actual filter CPU unit8rejects PASS.
Native deliverybuild2 and ownerbuild2 compiled; installed owner7f871a01... .
Header extraction after build is equivalent guard logic, not yet rebuilt APK.
CID2338 normal shutdown/VM[] + exact carrier rollback3381bd95... PASS.
CID2339 reused own already delivered bundle (no carrier/disk mutation), normal
shutdown +actualVM[] confirmed, cold read-only logs. Observer47628 intentionally
interrupted after shutdown (terminal1, not boot failure). No live GPU test remains.
This is own standard API UBO consumer, NOT third-party WSI/texture/perf/WDDM
or Vulkan conformance PASS. Next concrete target: unchanged external triangle
window using UBO; inspect/build only required upstream framework dependencies,
then fix only observed API gaps. Do not rerun proven headless/UBO tests blindly.

2026-10-16 current work: next upstream triangle requires vertex UBO192B/set0
binding0. Added bounded descriptor pool/set/update/bind path and Bridge17
uniform snapshot: actual native Vulkan UBO/descriptor, not shader rewriting.
Uniform bytes copied at submit (late mapped writes supported),range<=4096B,
single vertex binding,no textures/dynamic offsets. Old13/14 wire unchanged.
CPU uniform state10rejects +existing push14/readback4194304B PASS; scene parser
630 rejects PASS. Candidate windows-icd-uniform-build3 DLL305fdf89... compiled.
NOT native/Windows GPU PASS yet. Owneruniform-build1 SHAb3ca9526... installed,
ZIP544584B SHA1d9de8df...; prior WSI report exact56e0ccb9... preserved.
LIVE: approved unchanged profile mavk-windows-uniform1-20261016; carrier
uniform-apply1 exact32bae66f... . MUST shutdown/VM[]/uniform-rollback1 afterward.

Windows ARM64 CID2336: pinned SaschaWillems renderheadless4ee2501 runs unchanged
through standard Vulkan Loader → app-local Mavk ICD → app-owned AVF4057 → Mali.
1024×1024, three indexed RGB triangles, 64B MVP pushes, depth/culling, staging
uploads and separate linear-image readback. EXIT0; strict independent analytic
verifier PASS3624 samples (triangles279/823/292,background2230). PPM3145745B
SHA089e42548bbaa606feead8fb1509d99e6c9eee2b70a91cf7d906466ccbae69c1.
Host8commands/1GPU,2444rx/4194868tx,SyncVal0errors/0warnings,cleanEOF/Corezero.
Host SHA405c5aa9748b0a8d5bc34d9c396b3259e5984f8f7a5f6cc60b855767bac75a53.
Evidence E:/Mavk/artifacts/windows-headless-bundle1-20261016:
host-headless.json,guest-headless-cold/{stdout,stderr,exit,headless.ppm,extraction}.
Normal guest shutdown,actualVM[],carrier rollback2 exact3381bd95... PASS.
Observer28051 intentionally interrupted AFTER shutdown/rollback (terminal1,
not boot failure). No live VM/test left. No commit/push or production changes.
This proves ONE external offscreen application, NOT conformance, WSI, games,
DWM acceleration, sustained FPS or a high-performance universal Vulkan driver.
Next: extend concrete real-app compatibility and measure persistent-resource
costs; do not repeat completed offscreen/first-window milestones or redesign
manual control.

WSI candidate (opt-in compile macro only): Win32 surface/swapchain, two owned
images, synchronous binary acquire/submit/present, guest CPU blit of genuine
Mali readback, FIFO DwmFlush pacing, resize/lost-window handling. Not zero-copy
or general asynchronous semaphore/WSI conformance. Host hidden-window state
unit PASS and default offscreen CPU regressions PASS. ARM64 WSI DLL2dbca5e8...
in windows-icd-wsi-build3-20261016; original proven offscreen artifacts intact.
Standard API-only own window consumer320×240/12 changing draws compiled;
no third-party WSI/runtime PASS yet. Ownerwsi-build1 SHA3716a63d... installed.
Old host PASS report preserved before-wsi; fresh report absent before launch.
Current live test: mavk-windows-wsi1-20261016, guarded wsi-carrier-apply1PASS.
MUST guest shutdown/actualVM[] and wsi-carrier-rollback1 exact readback after test.

WSI runtime update: CID2337 FUNCTIONAL PASS through standard loader Win32
surface/swapchain/acquire/binary semaphores/submit/present. Own API-only consumer
rendered12 moving images320×240 on Mali;12 distinct guest-window sampled hashes,
blue center checked each frame,EXIT0. Fresh Android current-window1.png visibly
shows Vulkan window via unchanged H264. NOT Android visible-FPS measurement.
Host14commands (Hello+Identity+12draws;close is EOF,NOT a15th opcode),12GPU,
23056rx/3687140tx,SyncVal0errors/0warnings,cleanEOF/Corezero. Strict window
verifier corrected its initial counting expectation from15 to14 using actual
protocol (no bye opcode); all pixel/submit/validation gates unchanged.
HostSHA56e0ccb93a5c33643f7ce8a33eb199ae6269728b985bd1b9c5fd88d6b641fccd.
Evidence E:/Mavk/artifacts/windows-wsi-bundle1-20261016:
host-window.json,guest-window-cold/{stdout,stderr,exit,extraction},current-window1.png.
Normal shutdown,actualVM[],wsi-carrier-rollback1 exact3381bd95... PASS.
Observer13625 interrupted only AFTER shutdown/rollback; no active VM/test.
WSI remains compile-time opt-in and CPU readback/blit, not third-party WSI,
zero-copy,games,DWM,conformance or production performance. Keep native/Mali
renderer and all golden boot/display/storage baselines unchanged.

## Решение

Рабочее имя Mavk; отдельная соседняя папка, будущий отдельный репозиторий.
Главный приоритет: native Android Vulkan proof → небольшое общее ядро →
Windows GPU → DroidDeck → Ubuntu Vulkan. Не продолжать старые FPS-прогоны
по инерции. Не commit/push/publish, не запускать новую VM для записи плана.

## Что есть и чего пока нет

- Изолированный native harness реализован/собран/установлен: org.mavk.probe,
  без разрешений/VM/system changes. Три отдельных запуска процесса: аппаратный
  compute PASS, по8,388,608 проверенных uint32 значений (итого25,165,824).
  Device Mali-G925-Immortalis MC12, device API1.3.278, loader1.4.0,152 extensions.
  Доказательства/тайминги/ограничения: NATIVE_BASELINE_2026-10-15.md.
- Direct graphics/offscreen: 3330 пикселей проверены; движущийся треугольник
  1920×1200. Независимые SurfaceFlinger actual-present timestamps: 1678 за
  фиксированные28.0036с =59.9208/с; max gap33.326мс. Это простой native workload,
  не предел GPU, не доказательство уникальности каждого изображения/guest GPU.
- Минимальный Core выделен в core/include + core/src (без JNI/Android),
  отдельная статическая библиотека: buffers/images/shaders/pipelines/fences/
  command batches/submission. Device/queue пока caller-owned; семантика
  Vulkan-команд остаётся явной, никакого полного ICD/compatibility обещания.
- Compute/image A/B на одном device/queue, ABBA на каждом размере:
  3 отдельных процесса, 509,607,936 uint32 и243,695,616 image pixels без ошибок.
  Buffer sizes4096/65536/262144 words; images64²/640×360/1920×1200.
  Все Core live counters после каждой фазы нулевые. Это не независимый leak
  detector и не validation-layer PASS.
- Core graphics/Surface A/B:1920×1200,30с,3330 offscreen pixels PASS в обоих
  путях; Core59.9559 vs direct59.9667 actual-present timestamps/s за фиксированные
  ~28с. Один APK, существующий renderer, без замены WSI. Core live counters
  после реального teardown нулевые. Graphics GPU timestamps пока не измерены.
  Отчёт: CORE_AB_2026-10-15.md; raw в E:/Mavk/artifacts/core-ab-build2-20261015.
- Overhead record+submit: медиана run-level delta Core-direct = -5.308,
  +5.307, +1.847мкс для трёх размеров. Малые величины/разброс/DVFS не позволяют
  заявлять причинный speedup или нулевой overhead. Сырые samples сохранены.
- Ограниченный guest Vulkan доказан (latest milestone выше); WDDM/DWM и
  DXVK compatibility пока НЕ доказаны.
- Bounded Core gate завершён: Khronos validation1.4.363.0 включён app-local,
  validate_sync активен (намеренный записанный WAW обнаружен, никогда не submit).
  Compute/image ABBA169,869,312 words +81,231,872 pixels;0 errors/0 warnings.
  Две потери активного Surface1920×1200→1280×800→1920×1200, без перекрытия
  native owners; последний цикл30с.0 errors/0 warnings, Core live zero после
  каждого teardown. Это bounded gate, не исчерпывающая проверка Vulkan.
  E:/Mavk/artifacts/core-validation-build1-20261015/validation-lifecycle-summary.json.
- Bridge v1 local proof PASS:32-byte LE framing, connection-local resource IDs,
  bounded allocations, trusted compute dispatch/readback.49 commands/10 negative
  cases/16 actual GPU submissions,557056 words correct,0 validation errors/warnings,
  live resources zero after disconnect (including resource intentionally left alive).
  Same-UID AF_UNIX stream/separate threads/fragmented writes control.
  E:/Mavk/artifacts/bridge-build1-20261015/bridge-run1.json.
- Короткий DroidDeck PoC выполнен: exact r9 glibc/libs/proot + pinned fork hybris,
  hardware G925/3330 pixels PASS direct loader и proot. Только minimal subset под
  Termux debug UID/runas_app, не полный DroidDeck app/session/WSI/game PASS.
  DXVK pinned profile preflight NO-GO: missing native multiViewport/nonSolid,
  exact G715 driver guard не совпадает. D3D11 не запускался, guard не расширен.
  DROIDDECK_POC_2026-10-15.md и E:/Mavk/artifacts/droiddeck-poc-20261015.
- Bridge descriptor adapter проверен на реальном PFD:49 commands/16 GPU jobs,
  557056 words,0 validation errors/warnings,clean EOF,live zero. Optional isolated
  U-AVF diagnostic APK с port4057/явным opt-in и same-UID/token admission собран
  и установлен в isolated package (не production). Старые4055/4054 policy unchanged;
  84 assertions PASS.
- Windows ARM64 → app-owned AVF4057 → Mavk Core → hardware G925 compute PASS.
  CID2328/run6:23 commands,8 GPU submissions,131072 uint32 проверены гостем,
  process EXIT=0; host1000 rx/525056 tx bytes,clean EOF,live zero,
  sync-validation enabled,0 errors/warnings. Отчёты обеих сторон сверены строгим
  verifier; не считать это graphics/Vulkan ICD/WDDM/DWM PASS.
  E:/Mavk/artifacts/windows-bridge-run6-20261015/{host-report.json,guest-cold/}.
  Clean shutdown VM[]; single8192B diagnostic carrier exact rollback PASS.
- Graphics Bridge PASS, CID2329: Windows command→real Mali graphics pipeline→
  guest3330 verified RGBA8 pixels,process EXIT=0. Host3 commands/1 GPU submission,
  112 rx/16496 tx bytes,clean EOF/live zero,SyncVal0 errors/warnings.
  E:/Mavk/artifacts/windows-graphics-run1-20261015/{host-report.json,guest-cold/}.
  Strict verify-report.py --graphics PASS. Это trusted offscreen slice, не ICD.
- По накопленной диагностике U-AVF: Ubuntu аппаратный OpenGL через VirGL/Mali
  работает; Windows BasicDisplay/WARP/IddCx не являются аппаратным 3D.
- Vendor Venus backend не имел Vulkan support; gfxstream branch упиралась
  в GZVM register_memory EFAULT. Это исторические ограничения конкретных путей,
  а не доказательство работоспособности нового guest/host bridge.
- Общий backend не должен обещать исправление всех недостатков vendor driver,
  всех Vulkan features, WDDM или производительность Snapdragon.

## Сохранённый Windows checkpoint

2026-10-14, CID2320: реальный Explorer/primary Idd1920×1200, source/encode
56.233 FPS за30с; Android render callbacks56.131 FPS, pending<=1,
receive→render6.942ms. Это не доказательство физических60 FPS или аппаратного3D.

CID2321: повторная топология/restore и native children0/0; контрольное движение
попало после возврата на исходный монитор. Не считать провалом производительности
или успешным60FPS тестом. 2026-10-15 проверены VM[] и отчёты отката обоих display
helpers + ALL112 original codec blocks, unknown0.

Недоставленный host-only workload helper9728B не влезает в старый8192B slot;
не ставить его и не переразмечать ESP ради замороженного FPS-теста.
Артефакты сохранены в E:/U-AVF/artifacts/idd-long-workload-* и
idd-continuous-workload-*; вывод Ubuntu/Windows сейчас не изменять.

## Текущее состояние / следующий этап

2026-10-15 21:30: compute + trusted graphics Bridge milestones PASS.
CID2328 и2329 завершены; VM[] подтверждено. Graphics carrier exact rollback:
E:/Mavk/artifacts/windows-graphics-carrier-rollback1-20261015/report.json.
Observer exec66641 остановлен только после clean guest shutdown/VM[];
outer exit1 — intentional observer cleanup, не GPU или Windows boot failure.
Ручной input/UAC/AVF метод сохраняется; не автоматизировать login/launch infrastructure.

Предыдущий compute shutdown:
VM[] подтверждено. Observer exec41211 завершился0 после clean shutdown;
его final KD timeout произошёл после остановки гостя, не Windows boot failure.
Временный carrier восстановлен byte-for-byte только на холодном диске:
E:/Mavk/artifacts/windows-bridge-carrier-rollback6-20261015/report.json.
Оригинальный8192B cluster SHA3381bd9574ddecd6a6674d99b5b5be81e506393746c66460082e2bde35f4320d.
Никаких изменений firmware/CPU/SMP/Ubuntu/production H264.

В isolated package com.example.uavfwinpetest остаётся graphics owner build1:
E:/U-AVF/artifacts/mavk-owner-graphics-build1-20261015/U-AVF-Windows-diagnostic.apk,
SHA5b103c1c1705ded684ba325dd1518561d0cebf7e019d91a9030790e632438d43.
Baseline APK сохранён в E:/U-AVF/artifacts/mavk-owner-installed-before-20261015/.
Bridge требует явного opt-in; нет автоматически открытого GPU endpoint.
Следующая загрузка не нужна для повторения уже доказанного compute slice.

Закрытые причины подготовительных неудач (не повторять как platform A/B):
- Run1: standard user не читает ESP; input в elevated console не доставлялся.
  Использован finite script до обычного UAC, не обход прав/защиты.
- Run2: accept(NULL,NULL) падал в guest WSP; исправлены valid peer outputs.
- Run3: обязательный setsockopt на AVF FD падал до Core. Заменён whole-session
  transport watchdog60s; локальный PFD control повторно PASS.
- Run4: guest deadline истёк до подключения; ограниченная координация admission.
- Run5: команда ушла в Search из-за потерянного focus/modifier state, клиент
  не запускался. Обычный modifier-up + наблюдаемый console focus исправили.
- Run6: обычный UAC подтверждён; фактический Windows client/GPU PASS.
Каждый carrier был откатан; подробные неудачные отчёты сохранены на E,
не превращать их в новую серию повторных boots.

Не повторять пройденные compute/trusted-graphics тесты. Новый фокус: стандартный
Windows Vulkan Loader→ограниченный Mavk ICD→обычное offscreen API приложение.
Swapchain/окно не нужны для первого PASS; никакого Vulkan1.3/conformance/WDDM claim.

Реализованы и собраны icd/windows/{icd.cpp,transport.hpp,build.ps1,mavk_icd.json}
и независимый Vulkan API consumer icd/test/offscreen.cpp (без Mavk API/protocol).
App shader СИНИЙ, trusted host triangle был ОРАНЖЕВЫЙ; это необходимый контроль
реальной передачи guest shader вместо подмены. Есть bounded logical buffers/images,
memory/map, shaders/pipeline, renderpass/draw/copy, synchronous queue/fence.
Scope/known incomplete semantics: icd/README.md. НЕ production/conformant driver.

Последний полный guest DLL/app build7:
E:/Mavk/artifacts/windows-icd-build7-20261015/{aarch64,x86_64}/
ARM64 DLL SHA6a9d3d1f14df3582d1bc86f4f574ebb6beb20a3bcc181e8d5eb61c1b41e02362.
Стандартный HOST X64 Windows loader находит JSON, загружает DLL, вызывает ICD
CreateInstance; concrete pNext47 loader-private rejection исправлен (device48 тоже).
Fresh host-loader-report.json PASS только manifest/entry boundary; отсутствующий
на ПК Viosock даёт ожидаемый init failure. Это НЕ ARM64 enumeration/render PASS.

Host DeviceIdentity7/DrawOffscreen8 source compiled in bridge-icd-build1-20261015,
native bundle SHAf2922363c860d8238399242a680f43e74d49f28d341ca6f6b708c2524a0ef018.
Там реальные app SPIR-V/clear/push parameters, не fixed trusted shaders;
structural checks не semantic/security validator. На устройстве ещё НЕ проверено,
этот bundle/ICD ещё НЕ установлены. Старый tested graphics APK сохранён.

Точный следующий шаг: доставить ARM64 DLL+JSON+independent app и стандартный
ARM64 Vulkan Loader (сначала проверить наличие) в отдельный guest TEMP directory,
без registry/system install. Не перезаписывать display/codec helpers ради доставки.
Existing single-cluster carrier7168B не вмещает DLL622592B/app430080B; сеть текущего
profile networkSupported=false. Не считать это тупиком GPU. Минимальная допустимая
доставка — ограниченный known-bundle по уже app-owned4057, с явным hash/size и
manual запуском через существующий PowerShell/UAC, не новый input/auth channel.
Не запускать новую VM, пока payload/loader/test/rollback не подготовлены целиком.
После guest loader discovery сразу API offscreen blue-pixel test+host SyncVal report.
Goal активна; enumeration и реальный ICD app PASS пока не доказаны.

2026-10-15 continuation: прошлый turn — progress, не wait/no-progress:
compute/graphics runtime PASS, ICD source/build и host loader discovery entry.
Git session10329 завершилась0: Loader commit00893b9a…/Headers217e93c6… скачаны.
ARM64 standard Khronos Loader1.3.275 собран в
E:/Mavk/artifacts/vulkan-loader-arm64-build1-20261015/loader/loader/vulkan-1.dll
(600064B,ARM64 PE,только OS/UCRT imports). C fallback unknown-extension
trampolines: test не использует unknown extensions, не считать release-certified.
ICD source после transport serialization rebuilt8, ARM64 SHA
a1454b6057be6a1d286d80e2745d5489eecde98d6fe4cc395db01b02aaa59d29.
HOST x64 loader discovery/entry smoke повторно PASS; не guest PASS.

Private bundle1 готов:527965B SHA
c478ce445c160c912006071854e55ef7fb15fb955700b2df3102e030f68d279c,
E:/Mavk/artifacts/windows-icd-bundle1-20261015/mavk-icd-development.zip.
Включены upstream licenses. Fixed readonly artifact opcode9 compiled only in
explicit development build, no path selector/arbitrary filesystem service.
Это доставка test files по тому же4057, не новый input/auth channel.
Guest finite fetch7168B SHA8574b919… пишет CREATE_NEW TEMP/MavkIcdDevelopment1.zip;
не выполняет bundle. Manual API command checks exact zip hash, expands only into
fresh TEMP/MavkIcdApi1, sets VK_DRIVER_FILES for ordinary test process,60s deadline.

Ready-to-test owner APK0b68c2caa47d60e11eccc5fcc7010a987ae1bc096f434b642f9272630b6bfc50:
E:/U-AVF/artifacts/mavk-owner-icd-bundle-build1-20261015/U-AVF-Windows-diagnostic.apk.
Native bundle201c7985… in bridge-icd-bundle-build1-20261015. NOT installed yet
at this checkpoint. Cold fetch preflight ready, plan
E:/Mavk/artifacts/windows-icd-fetch-preflight1-20261015,cluster3182/offset36028416.
Candidate SHA32bae66f…; original3381bd95… . If applied, MANDATORY exact cold rollback
with this plan after VM[]. No firmware/display/CPU/Ubuntu changes.
Next one boot: fetch once(manual UAC/Probe7)→collect host transfer report→ordinary
API launch(guest-api-command.txt)→same existing mavk_probe admission→collect actual
host report and own API stdout/stderr/exit after clean shutdown, then rollback.
No repeated trusted GPU tests or input/control infrastructure refactoring.

ICD test1 historical: CID2330/UID10042/PID15822, observer exec97740 terminal0.
APK bundle0b68… installed; fetch carrier applied EXACT READBACK PASS, journal
E:/Mavk/artifacts/windows-icd-fetch-apply1-20261015/report.json.
Mandatory rollback plan windows-icd-fetch-preflight1-20261015 after actual VM[].
Current token only under EvidenceName mavk-windows-icd-api-20261015; never print.
Fetch Probe7 completed: fixed bundle527965B delivered,clean EOF/2 commands/0 GPU.
API ordinary-user call reached native DeviceIdentity (Hello+Identity2 commands,
356 tx,0 GPU), then standard ARM64 loader rejected missing mandatory
vkGetPhysicalDeviceSparseImageFormatProperties. Concrete guest stderr preserved;
not hardware blocker, not GPU render PASS. Correct zero sparse-format enumeration
implemented + lookup export in build10; all12 required instance entrypoints checked
against pinned loader source. Need test this fix, not platform mutations.

Before first API execution, app shaders corrected to SPIR-V1.0 for declared API1.0:
exact two-byte update of OWN disposable app matched complete build9 recompilation
SHA18ea17ea… . No Windows/system/signed binary modified. New bundle uses correctly
compiled app directly (no patch). Standard Start-Process.ExitCode was blank;
new prepared command uses direct .NET Process/captured streams,60s deadline.

After API ended/idle, screen became black while input channel wrote; did not
debug/change production display or type blind console commands. Stopped isolated
VM with its observed app Stop button; VM[] confirmed by cold extraction guards.
Own guest API/fetch logs in E:/Mavk/artifacts/windows-icd-api-run1-20261015/
{guest-api-cold,guest-fetch-cold}. Exact carrier rollback1 PASS.
Old TEMP/MavkIcdDevelopment1.zip still exists and must be renamed with exact hash
check before next fetch (CREATE_NEW). No current VM/observer alive.

Next test2 fully prepared bundle528005B in windows-icd-bundle2-20261015,
ARM64 DLL build10 SHA8cb52934… with sparse entrypoint fix, app SPIR-V1.0.
Native APK bundle rebuild running exec58424; verify same handle, no duplicate build.
Then build isolated owner/install cold/apply same fetch preflight1 with new journal,
one known-profile boot. First normal console rename old OWN zip after exact
c478ce44… hash check (preserve .before-sparse-fix). Fetch finite Probe9, then
guest-api-command.txt from bundle2 creates fresh TEMP/MavkIcdApi2. Ordinary user,
same input/UAC mechanism. Collect --api --api-attempt2 after stop and rollback.
Do not repeat compute/trusted graphics or claim full Vulkan conformance.

Test2 launch now in progress, observer exec86919 (not58424, which completed0).
Owner bundle2 APK SHA9ff151d71ab07087549630f315ec0720f3b8487f03e14c26cd6e1240d5e03b0e
installed cold; native bundleb54b5a76… . Fetch carrier apply2 exact readback PASS,
same preflight1 plan/mandatory cold rollback with fresh rollback2 journal.
EvidenceName mavk-windows-icd-api-sparsefix-20261015. Do not duplicate launch.
Before fetch: preserve old own TEMP zip1 by exact c478ce44… check + rename.
Then finite Fetch Probe9 command from windows-icd-fetch-build1, same client hash.
API2 command from windows-icd-bundle2 is1340-ish chars, parser checked; no
SPIR-V binary patch required, shaders compiled for Vulkan1.0. Bundle2 SHA
3d8a53ad28b9ded8a58a593675065b5a458c76b23eb4ecd40727378db9150f91.

DroidDeck bounded PoC закрыт: functional glibc/proot G925 PASS, но не WSI/zero-copy/
performance PASS. Не переносить G715 guards и не переписывать libhybris без
измеренного bottleneck. Pinned DXVK profile — preflight NO-GO, не общий запрет
DXVK. Mavk Native не устраняет glibc/Bionic ABI или WSI costs автоматически.
Не расширять короткий PoC до установки игр; вернулись к U-AVF Bridge.

Сырые Core measurements:
E:/Mavk/artifacts/core-ab-build1-20261015/core-ab-summary.json;
E:/Mavk/artifacts/native-graphics-build5-20261015/correlated-graphics-summary.json.
Отсутствующие robustness2/descriptor_buffer — unsupported, не включать фиктивно.
Стратегическая цель активна и не завершена; никаких commit/push/publication.

2026-10-15 LATEST AUTHORITATIVE CHECKPOINT — ICD API OFFSCREEN PASS.
Resumed CID2331 without a new VM boot. Previous UAC expired/cancelled while idle;
same compact encoded finite fetch was entered again through existing manual UIN1.
Observed ordinary UAC approved; bundle2 transferred:2 commands/0 GPU/528085 tx.
Then ordinary-user API2 command (1186 chars) ran successfully through standard
ARM64 Vulkan Loader and application-local manifest; no special Mavk app API.
Windows stdout: STANDARD_VULKAN_BLUE_PIXELS=3330,
STANDARD_VULKAN_OFFSCREEN_PASS,device Mavk experimental / Mali-G925-Immortalis MC12.
External EXIT=0. Host:3 commands,1980 rx/16772 tx,1 real GPU submission,
SyncVal enabled,0 validation errors/warnings,clean EOF,Core live resources zero.
Strict verify-api-report.py PASS after cold read-only extraction of own logs.
Evidence E:/Mavk/artifacts/windows-icd-api-run2-20261015/
{host-api-report.json,guest-api-cold/,api-guest-verdict.png}.
Windows shutdown.exe /s /t0 completed; actual VM[] confirmed. Observer86919
already terminal0; no live observer remains. Carrier rollback2 exact readback PASS:
E:/Mavk/artifacts/windows-icd-fetch-rollback2-20261015/report.json.
Original cluster3381bd95… restored; firmware/8CPU/H264/Ubuntu/P7 unchanged.
No commit/push. This is a limited, non-conformant offscreen ICD PASS, not a
third-party game, swapchain, WDDM, DWM or Vulkan1.3/conformance PASS.
Next: select one concrete independent application's API requirements and extend
only the smallest required slice. Do not repeat compute/trusted graphics/API PASS
or rebuild VM-control infrastructure. Overall strategic goal remains incomplete.

2026-10-15 NEXT SLICE — VERTEX DATA (progress, not another VM baseline).
External target inspected: SaschaWillems/Vulkan examples/renderheadless/
renderheadless.cpp (upstream master, not vendored/pinned yet). It needs real
vertex/index buffers, depth, pipeline cache and matrix push constants; current
ICD cannot run it. Do not modify that app into the old VertexIndex test or claim
third-party compatibility. First implemented prerequisite: vertex buffers.
DrawVertices10 adds bounded explicit stride/attributes/count/bytes; host creates
and flushes a real vertex buffer and binds it to native Mali pipeline. Existing
DrawOffscreen8 remains unchanged wire contract. ICD now supports one binding,
4 float attributes,stride<=64,up to4096 vertices; binds from mapped guest data
at submit (including buffer and attribute offsets). Index buffers remain absent.
CPU wire test rejected129 malformed/truncated/overflow/duplicate-location cases.
ARM64/x64 ICD/API app builds PASS in windows-icd-vertex-build3-20261015,
ARM64 DLL SHA12384031ed058809dd155d5cd1d506a6b31055766857d46b4e54cbdd4c60d273.
Native DEVICE TEST PASS: bridge-vertex-build2-20261015/vertex-gpu-report.json:
Mali-G925,3330 verified blue pixels,1 GPU submission,2 commands,SyncVal enabled,
zero errors/warnings,clean EOF,Core live zero. APK org.mavk.probe only updated;
diagnostic U-AVF APK and production Windows/Ubuntu/H264 untouched,VM remains[].
Separate API-only vertex-buffer consumer compiled (padded stride16,attribute
offset4,binding offset16) but NOT run in Windows yet. Same standard Vulkan API,
no Mavk app calls. Verifier --vertices rejects prior VertexIndex PASS markers.
Private vertex bundle prepared (not delivered):530820B,
E:/Mavk/artifacts/windows-icd-vertex-bundle1-20261015/mavk-icd-development.zip.
Next exact step: embed this bundle into isolated diagnostic APK (same existing
development delivery),prepare API attempt3 and own-zip exact-hash preservation,
then ONE known-profile boot/fetch/API vertex test/host+guest verify/cold rollback.
No reason to rerun trusted compute/old graphics or alter manual input machinery.
After Windows vertex PASS, implement index-buffer prerequisite for external
headless sample; full external sample and performance remain unproven.

LIVE vertex API test: observer exec74284, EvidenceName
mavk-windows-icd-vertices-20261015, CID2332. Same known-profile boot; no duplicate.
Owner APK19345d294082a49367429ccde1fc29a4276422146c63e5e9781840e33c5889cc;
native development bundle5a38e89c…; private ZIP530820B SHA
be5b1e5d3d6b03f4298d47b0b21384d0fb93b4980db39bbca7e3f241622b8280.
Carrier applied exact preimage/readback under windows-icd-vertex-carrier-apply1;
MANDATORY cold rollback preflight1→windows-icd-vertex-carrier-rollback1 after VM[].
Finite fetch command10-encoded prepared; API attempt3 prepared in vertex-bundle1.
Must first preserve exact old own zipSHA3d8a53ad… before CREATE_NEW fetch.
Current owner token under new EvidenceName only; never print. Do not repeat boot.

2026-10-16 interruption audit: previous VM2332 really terminal, actual VM[],
owner PID absent,observer74284 missing; events end OBSERVER_STOP ADB runner closed.
No fetch Probe10 logs exist in guest (cold read-only extraction). UAC was never
approved before interruption. Old zip2 preservation WAS observed before stop.
host-fetch-report.json in vertex-bundle1 is STALE (same hash9875c021… as prior
API2,3commands/1GPU), NOT new fetch or vertex PASS; admission failed with no owner.
Mandatory carrier rollback1-20261016 exact readback PASS. No newGPU conclusion.
Safe next: same prepared installedAPK/bundle,one repeat of interrupted test only;
new evidence/token required. Existing own zip is already .before-vertices;
do not repeat its rename blindly. Fetch10 then API3 remain unexecuted.

LIVE resumed test CID2333/UID10042/PID12663,observer exec20286, EvidenceName
mavk-windows-icd-vertices-20261016. Same installed owner APK19345d29… . Carrier
apply2-20261016 exact PASS; mandatory rollback2-20261016 after actual VM[].
Old own host report9875c021… preserved by same-UID rename to
files/mavk-bridge-report.before-vertices-20261016.json before new admission;
no stale report can now be collected as a fresh result. Input token new directory.

External sample pinned (source+MIT license only,not built/not modified) under
E:/Mavk/research/renderheadless-4ee2501; upstream commit
4ee2501c8280cf71d1f9ce231cd8ebd47b58adde,sourceSHA
3353eaf467e223bbc34eb6a9d41d16a0d02cb7dac17d8d6909d6219f0e810d64.
Concrete remaining requirements beyond vertex upload: staging CopyBuffer into
device-local vertex/index memory;uint32 index draw;64B vertex-only matrix push;
depth/culling;3 draws in one render pass;separate render/transfer submissions;
1024x1024 image (>1MiB reply,requires bounded chunk readback);linear-image copy/
MapMemory/GetImageSubresourceLayout;empty descriptor layout and pipeline cache.
Do not silently shrink/disable these in upstream to call it application PASS.

2026-10-16 LATEST AUTHORITATIVE — WINDOWS ICD VERTEX BUFFER PASS.
CID2333 same frozen Windows8CPU/H264 profile completed new API test. Standard
ARM64 Loader→ICD→DrawVertices10→Mali:3commands/1GPU,1788rx/16772tx,SyncVal enabled,
0 errors/warnings,clean EOF,Core live zero. Own guest API3 stdout verifies3330
blue pixels with padded stride16,attribute offset4,vertex binding offset16.
EXIT=0. Strict verify-api-report.py --vertices PASS after cold extraction.
Evidence E:/Mavk/artifacts/windows-icd-vertex-bundle1-20261015/
{host-api-20261016.json,guest-api3-cold-20261016/}. No external app/performance PASS.
Windows shutdown.exe /s /t0 completed,actualVM[]; observer20286 already terminal0
at its normal bound (retained VM then stopped by normal guest shutdown).
Carrier rollback2-20261016 EXACT READBACK PASS,original3381bd95… restored.
No firmware/SMP/storage layout/Ubuntu/production display/input/security changes.

Delivery lessons — do NOT repeat today's UI detour:
- Windows Terminal launch stalled,classic conhost PowerShell did open. Existing
  Unicode input_text did not enter text in classic console,ordinary keys did.
- Run dialog truncated long commands (1638B encoded fetch never createdProbe10;
  raw fetch11 returned MissingEndCurlyBrace). This is NOT GPU/ICD failure.
- Short conhost native launch of existing exact carrier worked,normal Run UAC:
  conhost.exe "\\?\Volume{7430910d-646b-494d-8ba4-7c4c7e462fa9}\IDDWORK\idd-motion-source.exe"
  Volume was observed via Win32_Volume (FAT32,not C:); re-observe on future boots,
  do not assume GUID permanent. No mapping/formatting/install/task service added.
- Wait for visible LISTEN_READY4057 before host admission; first host attempt
  failed before native listener,second fetchPASS2commands/530900tx,third API PASS.
- Own zip2 had already been preserved before overnight interruption. New ZIP
  exactSHAbe5b1e5d… checked; 252char Run Expand-Archive command succeeded.
- Short173char Run command launched conhost cmd /d /v:on /c,cd ownTEMP API3,
  child-local VK_DRIVER_FILES=.\mavk_icd.json,redirected stdout/stderr and recorded
  delayed !errorlevel! into exit.txt. No global environment/registry setting.
  The long .NET60s-deadline launcher was NOT used in this run; manual observation
  plus host transport watchdog,normal completion,not production cancellation proof.
- Read-only own fetch status confirmedProbe10 absent; don't count parser errors
  as GPU failures. host-fetch-20261016.json is failed admission; later fetch2 and
  host-api are separate preserved actual reports. Old stale report preserved.

Next meaningful step: index buffers and staging CopyBuffer/device-local vertex
upload for pinned independent headless sample; do not rerun this vertex PASS.
Its depth/matrix/multiple-draw/1024²/linear-readback requirements remain open.
Also current ICD format query still lacks VERTEX_BUFFER bufferFeatures for newly
accepted float formats: reconcile with actual host capability before external app.
Strategic goal ACTIVE/incomplete; no commit/push/publication.

2026-10-16 INDEXED/STAGING CHECKPOINT (after context recovery).
Native indexed Bridge opcode11 hardware PASS:3330 checked pixels,1GPU,
SyncVal enabled,0errors/0warnings,clean EOF,Core live zero. Nonsequential
indices3,1,2 and unused junk vertex0 prevent ordinary nonindexed draw passing.
Evidence E:/Mavk/artifacts/bridge-index-build1-20261016/indexed-gpu-report.json
SHA69b265b21f9ec3fc512d7038d84c948da7da1510b4e3e5cf3fd9b6675d289f55.
CPU wire-parser 275 rejection checks PASS. ARM64/x64 ICD and indexed API test
compile PASS in windows-icd-index-build2-20261016. New test uses actual queried
device-local memory type,host-coherent staging CopyBuffer,nonzero index binding
offset and firstIndex. CopyBuffer currently synchronous guest shadow-memory
emulation; NOT native GPU transfer/zero-copy/performance PASS.
Windows indexed API runtime NOT YET RUN. Preparing fixed private development
bundle + diagnostic owner APK; preserve original carrier/readback rollback.
Current VM[]; original carrier restored. No baseline or production changes.

LIVE INDEX TEST CID2334/UID10042/PID21345,observer exec98841. Diagnostic APK
E:/U-AVF/artifacts/mavk-owner-index-build1-20261016/U-AVF-Windows-diagnostic.apk
SHA4bffe6668ae2131fc49aa815b1218907180ee82c300a61ca5ebf0369d6d28619.
Private ZIP534010B SHAa98a26fb294662384d35ca60228e058fbd72198fd0ab57170b36c6bbc4f912c2.
Carrier apply1-20261016 exact readback PASS; MUST rollback after actualVM[].
EvidenceName mavk-windows-icd-index-20261016; new token in that evidence dir.
At08:05 Windows driver init observed,no display connection yet; API not started.
Old ownTEMP ZIPbe5b1e5d… remains:preserve exact hash before next CREATE_NEW fetch.
New API directory4; short checked Run commands in new bundle dir. No rerunAPI3.
Verifier --indexed correctly rejects earlier vertex PASS; --vertices still PASS.

2026-10-16 LATEST AUTHORITATIVE — WINDOWS INDEXED/STAGING ICD PASS.
CID2334 same frozen Windows8CPU/H264 profile:standard Loader→ICD→DrawIndexed11
on Mali-G925. Own API4 stdout:STANDARD_VULKAN_INDEXED_STAGING_PIXELS=3330,
STANDARD_VULKAN_INDEXED_STAGING_PASS,EXIT=0. Strict --indexed verifier PASS.
Host3commands/1GPU,1832rx/16772tx,SyncVal enabled,0errors/0warnings,clean EOF,
Core live zero. Host-reportSHA94987af3ff695247d424bd3ddf943cd0d37b3cdb0b8b5bbda6a92b335a7560df.
Evidence E:/Mavk/artifacts/windows-icd-index-bundle1-20261016/{host-api.json,
guest-api4-cold/}. Fetch separate2commands/0GPU,534090tx; not render evidence.
Existing old ZIPbe5b1e5d… preserved as .before-index; newZIPa98a26fb… verified
before expansion,ownTEMP/MavkIcdApi4. Same finite short manual Run/UAC channel.
Windows normal shutdown completed,actualVM[]. Carrier rollback1-20261016 exact
readback PASS(original3381bd95… restored). No production baseline changes.
Staging remains synchronous guest shadow copy,lazy host upload at draw; NOT GPU
transfer/zero-copy/performance/conformance/external-app/WDDM/DWM PASS.
Next: pinned renderheadless requires64B matrix push,depth+culling,multiple draws,
1024²/chunked image readback,linear-image copy/layout,empty descriptor layout/
pipeline cache. Do not repeat passed index test; implement next required slice.
Observer exec98841 intentionally interrupted only AFTER normal guest shutdown,
cold log extraction and carrier rollback; terminal1 due Ctrl+C,not boot failure.
No observer/VM remains. PC reserve C56GiB,E878GiB; tablet last check52GiB.

2026-10-16 MATRIX IMPLEMENTATION — NEXT REQUIRED EXTERNAL-APP SLICE.
Bridge12 carries explicit push bytes/stages (4..64B,vertex/fragment),indexed
geometry and shaders. Original8/10/11 unchanged. Native hardware nonidentity
matrix(scale0.5,translate0.25/-0.125) PASS3906 analytic blue/background pixels,
five bad range/stage requests rejected,1GPU/7commands,SyncVal0errors/0warnings,
clean EOF/Core live zero. Evidence bridge-matrix-build1-20261016/matrix-gpu-report.json
SHAbfcbcbc28d484b8dde37d75080bffa02e64c9b0af43832c175f14db93e67d8b0.
Windows ICD supports64B vertex-only push range,aligned partial writes,exact
byte preservation; snapshots compatible pipeline range. Explicitly rejects
extended pushes without indexed geometry in current slice. Empty descriptor
layout creation/destruction and empty pipeline cache lifecycle added. Cache
does not cache/serialize anything; initial data/bindings rejected,not ignored.
ARM64/x64 builds PASS windows-icd-matrix-build2-20261016,DLLSHA
bfc55e4e9874f892b46153214158666d31d75a1ecc7472c6dfb128a4d5e1fb93.
Host internal CPU push-state test PASS13 rejects,partial64B/NaN-bit exactcopy,
unchanged state on failed writes and empty-object lifecycle. NOT guest/GPU test.
Matrix API consumer built with two32B push updates and3906pixel check. Windows
matrix runtime NOT TESTED; installed diagnostic owner APK still INDEX build1.
No VM boot/mutation this stage; native probe APK only org.mavk.probe updated.
Before next Windows run combine remaining meaningful requirements as practical:
depth/backface culling,multiple draws in one pass,1024² bounded chunk readback,
separate render/transfer submissions and linear image copy/layout. Pinned source
GLSL default is Vulkan1.0;1.1/SPIRV1.4 extensions only Slang,not default blocker.
Do not claim unmodified upstream renderheadless works before actual execution.
Strategic goal active/incomplete,no commit/push.

2026-10-16 SCENE DEPTH/CULL/MULTIDRAW IMPLEMENTATION.
Bridge13 bounded one-pipeline scene:up to16 indexed draws with independent push
snapshots,clockwise/counterclockwise front face,cull flags,D32_SFLOAT depth
test+write LESS_OR_EQUAL. Actual native depth-format support queried before use.
One renderpass/submission,not CPU compositing of separate rendered triangles.
Native PASS3330 pixels:near blue survives later far red,near reversed green
is culled. Last draw has scale0.5; earlier pushes retain identity.8 malformed
scene/range/state requests rejected,1GPU/10commands,SyncVal0errors/0warnings,
clean EOF/Core live zero. Evidence bridge-scene-build1-20261016/scene-gpu-report.json
SHAd68fa1c91082c120ffba16a223bf2c819c9de61ed318dbbc7245b4fa14ba6362.
CPU scene framing/parser PASS418rejects incl all truncations/range overflow;
output unchanged on fail. WINDOWS SCENE RUNTIME NOT TESTED.
ICD D32 image/view/renderpass/framebuffer/pipeline recording implemented;
per-draw matrices snapshotted,all requested indices bounded/concatenated at
submit,Bridge13 emitted for depth/cull/multidraw. Old8/10/11/12 path retained.
ARM64/x64 scene-build1 compile PASS,DLLSHA
be5d65ddf1af38ee57737460be54c3aa5ef3af9e07b3956eb07032c7d6141358.
Host CPU recording unit PASS14rejects verifies independent push snapshots and
draw16 bound. Unit executable push-scene-state-test.exe; not GPU/guest evidence.
No VM launched this stage; installed diagnostic owner still INDEX build1.
Next meaningful work before guest run:1024²/chunked readback and independent
render/transfer submission+linear-image copy/layout for pinned upstream sample.
Do not shrink upstream1024² or remove depth/draws to call it third-party PASS.
Shared renderer regression:scene-build1 native opcode12 matrix rerun PASS3906,
1GPU/7commands,SyncVal0errors/0warnings,Corelivezero. Report transitioned from
scene d68fa1c9… to matrix bfcbcbc2… after own probe force-stop/new Activity;
deterministic matrix hash matches earlier run,not a stale scene result.
Evidence bridge-scene-build1-20261016/matrix-regression-report.json.

2026-10-16 1024² CHUNKED READBACK + SEPARATE SUBMISSIONS.
Native RenderSnapshot14/Read15/Destroy16 PASS:1024² depth/cull/three-draw scene,
4MiB pixel readback downloaded in4x1MiB chunks,860488 analytic pixels checked,
11 malformed/bounds/retired-ID rejects,1GPU/18commands,SyncVal0errors/0warnings,
clean EOF/Core live zero. Retained object is CPU pixels,not a persistent GPU
resource or zero-copy. Bound4objects/16MiB per connection;old13 remains512².
Evidence bridge-snapshot-build1-20261016/snapshot-1024-report.json
SHA293042fc5a4ee9a187ce9a6c7cb9f6b0ce9d818df2ed3fa6661a36e1854676cf.
ICD now supports1024² indexed scene renders via chunked snapshot readback,
stores completed colour image bytes/layout in existing guest shadow allocation,
then separate command-buffer/submission CopyImage to host-visible linear image,
deferred layout transitions and GetImageSubresourceLayout,rowPitch=width*4.
RenderPass final TRANSFER_SRC accepted. CopyImage executes CPU memcpy of actual
completed GPU pixels,NOT a host GPU copy. Virtual memory budget32MiB/max4MiB
per allocation. No performance/conformance claim; no physical shared mapping.
ARM64/x64 snapshot-build1 compile PASS,DLLSHA
6bd7c37099e6c8b21f2c95f8f338268253b86d841de34d228b6f41a4b6e36d89.
CPU linear-readback unit PASS full4194304bytes,rowPitch4096,map/lifecycle;
artificial input bytes,not guest GPU evidence. Windows new runtime NOT TESTED.

PINNED THIRD-PARTY APP PREPARED,NOT EXECUTED:
Unmodified SaschaWillems renderheadless4ee2501 +same pinned helpers/shaders
and actual repo GLM submodule1ad55c5016339b83b7eec98c31007e0aee57d2bf.
GLM archiveSHAcec07d19a13df44030d0a3fac920303aa3bb72add52a3b80bfeb3187e71d7015;
only headers and two upstream licenses extracted (copying.txt absent; initial
tar command partial-success retained headers,then exact archive license entries
extracted; no source changed). No full repository/assets clone.
build-renderheadless.ps1 verifies pinned source/helper/shader hashes,builds C++20
release ARM64 with standard loader import library and local ./shaders/ build
configuration; application/shaders/1024² workload unchanged. Build2 PASS,
exeSHA13f5919677d60e6fc2fe79dcbaa0bf1c3ff26c60c725b1ee3f9d0b5a5e0ef4d2.
Private bundle1 ZIP763739B SHAe90890d36f55a34882ce949a5bbf1cd0d440ccd2d8c58dc661660afccad2f6b0.
Includes loader/ICD/upstream exe (mapped to existing vulkan-offscreen.exe),
SPV nested shaders and third-party licenses. Bundle1 predates optional
UPSTREAM-BUILD.json addition; source/build-report preserved on PC.
Native delivery bridge-headless-delivery-build1 compiled with exact ZIP.
Next: build/install isolated owner APK,COLD guarded existing carrier fetch,
ownTEMP/MavkHeadless1. Preserve old ZIPa98a26fb… before CREATE_NEW fetch.
Run existing short cmd launcher with child-local VK_DRIVER_FILES and <NUL
to supply ordinary stdin EOF to upstream final std::cin.get (no source edit,
no new guest control infrastructure). Record stdout/stderr/EXIT. After normal
shutdown,cold collector --headless --headless-attempt1 includes only own logs
and bounded <=4MiB headless.ppm. verify-headless-report.py requires host8cmd/
1GPU,clean validation/EOF,upstream stdout/EXIT0 and analytic RGB/depth samples
covering all3triangles/background. Verifier syntax checked,NO runtime PASS yet.
No VM boot/Windows mutation this stage;installed owner still INDEX build1.
Goal active/incomplete,no commit/push;keep all established baselines.
Owner headless-build1 compiled READY NOT INSTALLED:
E:/U-AVF/artifacts/mavk-owner-headless-build1-20261016/U-AVF-Windows-diagnostic.apk
SHA25aa3297ed61c065cd843c934ee536818d4210ee6b879549999a58515d6699a5.
All build handles terminal;VM[]. Scene parser updated420 rejects includes
1024 only on retained path/1025 rejection. Verifier analytic coverage3624
points(classes279/823/292/2230) precomputed ONLY,not pixel/runtime evidence.
Short Run expansion/launch commands preserved in windows-headless-bundle1;
guard length<=252 before input. Current PC reserve C55.3GiB,E878.5GiB.

LIVE UPSTREAM TEST:owner headless-build1 exact25aa3297… installed,cold carrier
headless-apply1-20261016 exact readback PASS32bae66f… . Observer exec87033,
EvidenceName mavk-windows-upstream-headless-20261016,new private token there.
MUST normal shutdown/actualVM[] and carrier rollback before cold-read conclusion.
No new GPU result yet; one unchanged8CPU/frozen platform/H264 boot started.

Recovery checkpoint: prior observer87033 ended at its bound before guest fetch
or upstream app execution. Actual VM[]; no external-app result, not a GPU FAIL.
Carrier rollback1 exact original3381bd95... PASS. Reapplied same guarded slot
headless-apply2 exact32bae66f... PASS; unchanged approved profile started as
mavk-windows-upstream-headless2-20261016. No new platform/driver experiment.
Installed owner remains exact headless-build1. Must rollback after VM[].
# 2026-10-24: experimental app-owned AHB selection integrated

See AHB_APP_SELECTION_2026-10-24.md. Installed signed candidate97d421b3,
runtimec2f107a4, WSI99a12d70. Real app prepare/configure instrumented PASS:
AHB path, SHM reversal, invalid-dimension fallback, preferences restored,
no Steam started. Native D3D11 from bundled component S_OK/11_0,480calls/8s,
CoreZero/gaps0; visibleFPS unmeasured here. Kotlin/Compose compile and14package
tests PASS plus81existing launch-environment tests. Toggle viewed/tapped on/off
on tablet, readbacks verified, leftoff. Normal session/game checks still pending.
Production untouched, no commit/push, strategic goal still active.

