# Active development order — 2026-10-17

CURRENT installed source APK6c8680f5/runtime3bd128, normal Steam+Muck AHB smoke
PASS; only portable FEX child-environment fix added. DDNet still fails before
Vulkan, now isolated with x86-64 true loader/sbrk()/127; HL2 not installed.
37 FEX/20 packaging regressions PASS. Detailed latest evidence in STATE.md and
MUCK_GPU_PROFILE_AND_ARTIFACTS_2026-10-25.md. FPS/diagonal artifacts still open;
do not spend another turn redoing these startup/resolution/menu tests.

LATEST MUCK_GPU_PROFILE_AND_ARTIFACTS_2026-10-25.md: Steam is available, unlike
historical reservations below. Live gameplay1600p/1200p both9.1 buffers/s; GPU
batch spans and CPU attribution saved. Recurring diagonal black artifacts remain
unresolved. GPU probe OFF; real CSE experiment rolled back through saved profile.
Normal renderer restored, no new Mali errors, sampled screenshots clean; do not
declare fixed. Thermal Status3 at end, Muck quit for cooldown, Steam left open.
Prioritize bounded FPS/artifact localization and second real game, not more
resolution/input/package polishing. Installed notices APK443c300b/runtime3bd128.

LATEST SOURCE_APK_AND_MUCK_PACING_2026-10-25.md: source-built Gradle APK675e9cb6
installed; full declared runtime APK verification + AMD64 Wine AHB/ResizeBuffers
PASS CoreZero/gaps0. External prebuilt dependencies pinned, optional Rust skipped
as in baseline (not full-source dependency/release claim). Static real Muck IL +
saved XML confirms fpsLimit144,vSync0, no saved30FPS cap. Runtime FPS attribution
still requires Steam-free live gameplay; user reservation remains in force.

LATEST DEMO_PREPARATION_2026-10-25.md: separate C: DroidDeck-Mavk/mavk-demo fork,
portable vendored WSI source/provenance, packaged runtime33c0 and installed APK
360c4765. Scaled pointer adapter + compositor visible-AHB input routing confirmed
by actual X coordinates1820,1108 for display2000,1200. Normal native D3D11 + AHB,
CoreZero/gaps0, AHB/SHM policy reversal PASS. Kotlin source compile/native recompile
PASS;17packaging+81environment+30controller tests. Steam currently RESERVED:
no Steam/game launches. Muck~30 root cause, gameplay/HL2/heavier game gates remain.
User now authorizes LOCAL COMMITS ONLY; no push/publication. Older no-commit and
Steam-free instructions below are historical, superseded. Production untouched.

LATEST AHB_RESIZE_RECOVERY_2026-10-24.md: actual session27 Muck26064/private
d0ea715a verified1200->1600->1200 AHB, visible menu349GPU=349zero-copy=349on-screen
per10s,copy0. BGRA display usage inheritance fixed; marker clamp removed; tiny
XCB windows keep X11 instead of fullscreen proxy. Same-menu SF samples1200p29.65
vs1600p29.94: NO measured resolution gain/60FPS claim. Bundled99a12 restored;
only reversible Muck candidate active. Next scaled absolute input + bounded real
frame-pipeline profile, not feature-suite expansion/Steam CLI. No commit/push.

LATEST AHB_WINE_CONSUMER_2026-10-24.md: actual ARM64+AMD64 Windows PE/Wine
consumers visible on bundled AHB; AMD64 standard1920->1280->1920 ResizeBuffers
passes,CoreZero/gaps0. Not gameFPS/upstreamDXVK support. User lifted Rust/Steam
restriction: actual AHB app switch enabled, normal Muck LAUNCH_GAME requested;
session2026-10-24-21-steam live, Steam13513/helper13589 observed. Inspect same
launch, not a new synthetic suite or restart for slowness. No commit/push.

LATEST AHB_NORMAL_SESSION_2026-10-24.md: actual ordinary-session X11/AHB path
tested without Steam. Fixed missing packaged policy helpers + confirmed black
screen from1920x1200/source vs2560x1600/output; minimal same-aspect fullscreen
display-layer eligibility fix now visible. Installedc56d55ff/runtime9e2a00b1.
SF2747buffers/drops0, basic native mouse/key receipts, CoreZero. Not game FPS
or complete input mapping. AHB preference restoredoff. Next normal Wine/game/
coordinate gates when competing Steam session safe.15package testsPASS. No
commit/push; preserve production. C:free48.86GB, avoid large new artifacts.

LATEST AHB_APP_SELECTION_2026-10-24.md: signed experimental candidate97d421b3
installed; actual Android prepare/configure instrumentation PASS, bundled WSI
D3D11 regression PASS, 14 packaging +81environment tests PASS. Toggle viewed
and actual preference on/off verified, leftoff; localization pending. Full
normal-session/game checks pending. Keep Rust/Steam untouched.
Do not infer visibleFPS from regression Present calls. Continue integration,
not capability test expansion; preserve rollback/no commit/push.

LATEST AHB_RUNTIME_PACKAGE_2026-10-24.md: optional runtimec2f107a4 packaged;
source launch-policy override supports both wrapper families, tests PASS.
Next implement app selection + compositor opt-in before initialization + pinned
bundled paths. Artifact not installed/selected yet. Maintain default SHM/Adreno
and user Rust/Steam non-interruption. Device free~34.6GiB; avoid large copies or
protected VM/game data deletion. No commit/push.

LATEST AHB_LAUNCH_SELECTION_2026-10-24.md: candidate selection and explicit SHM
reversal tested81cases + actual Android prepare-only PASS; no active launcher/
profile replacement. Next experimental runtime packaging and fork option, not
more synthetic FPS runs. Preserve user Rust/Steam session; no game launch yet.

LATEST ZINK_1200_VISIBLE_2026-10-24.md: visible1200p Zink30s~59.5FPS achieved;
APK33 debug output control only, WSI99a12d70 retained, D3D11 regression PASS.
No further synthetic FPS polishing. Next reversible experimental DroidDeck
backend selection/fallback/capability guards, then actual game integration when
Steam/Rust can safely be used. Do not claim this establishes game performance
or full upstream DXVK support. No commit/push; preserve APK32 C: rollback.

LATEST AHB_THREE_BUFFER_VISIBLE_2026-10-24.md: keep experimental AHB min3 images
(99a12d70), measured1280x800 visible30->~60 over30s. Correct output + clean
regressions. Next fix debug Activity output/reattach for actual1920x1200 test;
no further color/descriptor synthetic expansion now. Do not claim1200p60 or game
gain from current result. No Steam launch while user's Rust session active.

LATEST ZINK_BGRA_FUNCTIONAL_2026-10-24.md: real Zink BGRA-AHB functional PASS at
debug1280x800/FIFO,CoreZero/gaps0.599 incoming GPU buffers,0 visible presents
while Android locked; not visible60FPS. Pattern1200p pixel gate PASS. No more
synthetic color cases needed. Next correct debug output/reattach,visible1200p
Zink measurement with identical SHM/AHB swap policy; no Steam while Rust active.

LATEST: BGRA shared-command GPU pixel check PASS13,824,000 exact pixels;
intentional raw-copy negative catches channel swap. Uniform-only isolated blit
timestamps0.152ms mean are not game throughput. WSI d53b291b latest candidate,
both allocation regressions PASS. Next remains complete WSI color/display gate,
then independent real Zink workload. Do not start Steam/Rust. See BGRA document.

LATEST BGRA_AHB_CONVERSION_2026-10-24.md: GPU conversion implemented, initial
180-frame native draw completes in2.952s, resource teardown clean. Pixel/visible
FPS not established. Direct RGBA and BGRA allocation controls PASS. Resolve
debug configure dimensions and validate colors before actual Zink measurement.
User playing Rust elsewhere: NO Steam/game launch. Do not replace production
runtime; private4727674c candidate only, f1b rollback. Android currently locked.

LATEST KEYBOARD_ZINK_AHB_2026-10-24.md: APK32 validates early keyboard targets;
real native D3D11 consumer receives tap,A,Shift+B with AHB visible, CoreZero/gaps0.
No Muck input/gameplay claim (Rust on another PC). Zink AHB fails because actual
swapchain format44/BGRA8, not modes/pNext. Stock Mali rejects BGRA8 AHB queries
even COLOR_ATTACHMENT-only (-11); RGBA8 passes. Do not fake support/reinterpret
channels. Next RGBA negotiation or measured GPU conversion, not CPU readback.
Harness now rejects glmark's fake1965 FPS after swapchain failure. E: has real
disk153 retries; NO new E writes. Small source recovery + diagnostic build on C:
recovery/2026-10-24 (gitignored), exact locations/hashes and pending host handles
documented. Original WSI restored, marker removed, test session stopped.

LATEST XCB_AHB_GAME_2026-10-24.md: real Wine/Proton Muck menu now visibly
presents AHBs via private XCB proxy, 2560x1600, ~18.56 Surface buffer presents/s.
Not matched FPS gain/gameplay PASS. APK30 v3/session gate isolates legacy clients;
16 Linux launch tests and native sync mock PASS. APK31 + WSI f1b9370e confirms
native consumer mouse-button through input-transparent proxy; keyboard unproven.
Muck retest blocked by Steam warning that Rust on another PC would disconnect;
not approved. Per-game keys/script restored, marker removed, test session stopped.
Next keyboard focus via independent XCB consumer, then actual game input/A-B.
Keep original system WSI,
Steam UI, production VMs untouched. Do not repeat triangle benchmarks.

LATEST SAREK_AHB_VISIBLE_2026-10-23.md: FIRST VISIBLE D3D11/DXVK through the
AHB zero-copy display layer. Black screen root-caused: SDL3 identity
wp_viewport dst_set excluded the window from ahb_layer_only_candidate; fix
admits identity viewports; APK29 d63e36e2 native rollup installed (rollback
APK28 kept). DXVK-Sarek 219/220 zero-copy frames, gaps=0, visible moving
triangle proven by screenshot delta. draw2 regression PASS. NEXT: Steam/Muck
visible X11 regression on APK29, then longer animated D3D11 workload, then
real-game per-game AHB WSI config, then readback-vs-zerocopy A/B. Do not
claim game FPS from the diagnostic triangle.

LATEST AHB_WSI_PRESENT_2026-10-23.md: standard Vulkan AHB Present actually works,
1920x1200 triangle1800/~30s and independent unchanged ARM64 vkcube visible1280x800,
short Surface samples~60. GPU fence/ownership graph; final candidate3b705bef,
capabilities conservative, private v3-required subset only. Source/output/fallback
scope documented; no game gain claim. Stop triangle/clear benchmarks. Next inspect
existing Wine Wayland + FEX/native Vulkan adapter, isolated D3D11 consumer through
this same WSI. Normal X11 fallback and all production VM/runtime data preserved.

LATEST AHB_WSI_ALLOCATION_2026-10-23.md: real standard Vulkan Surface + swapchain
3x1920x1200 AHB + views + first acquire fence PASS in private existing-WSI candidate.
Public API memory ownership teardown CoreZero. No Present/pixels/FPS PASS yet;
allocation-only mode rejects Present. Next implement v3 AHB buffer, external GPU
release/acquire dependencies and ownership/layout transitions, then ordinary Vulkan
renderer. Do not expand standalone/rare tests. No installed runtime/game profile change.

LATEST shared-adapter continuation: AHB_SHARED_ADAPTER_2026-10-23.md. Common
AHB image importer now linked and GPU-tested in both glibc/native consumers,
207.36M pixels exact + negative oracle. No new APK or game-speed claim. Next
standard WSI image/backing factory and real display release -> acquire dependency;
existing WSI signalled-sentinel acquire cannot be copied blindly for AHB reuse.
Do not repeat headless pool benchmarks; use them only for changed resource code.

LATEST 2026-10-23: AHB_DIRECT_SURFACE_2026-10-23.md. Actual moving shader output
1920x1200 from glibc/Mavk into persistent AHBs reaches real Android SurfaceControl
with explicit bidirectional GPU fences. 360 frames / ~6 s; independent ~1 s
Surface sample ~60 buffer presentations/s, not game FPS or measured latency.
Experimental native APK28 installed, normal v2 Steam/Muck rendering/input checked,
private marker removed, session IDLE. Next integrate a standard Vulkan Wayland
swapchain consumer; do not confuse it with existing Wine/X11 SHM. Preserve common
runtime, production U-AVF/VMs, SHM fallback and APK27 rollback. No commit/push.

LATEST2026-10-23 AHB_WAYLAND_EXPLICIT_2026-10-23.md: gated v3 AHB-native buffer
factory+explicit fence API now implemented in native DroidDeck fork. Actual
candidate loaded on device: product factory,real Wayland wire/AHB import and
defaultv2/optinv3 PASS. Surface state test uses mock,NOT displayPASS. No APK update.
Next private real glibc Wayland producer using existing pool/fences,actualSurface
frame/release/resize/reconnect,then realWSI(X11 boundary explicitly unresolved).
Do not expand headless clear/featurematrix. Keep fullscreen opt-in,SHM fallback.
Earlier strictdma-bufregistryclaim corrected: android_wlegl uses sameregistry/noFD.

LATEST2026-10-23 AHB_SYNCFD_POOL_2026-10-23.md: prerequisite now GO: three
persistent1920x1200AHB slots and90changingframes,exact207.36Mpixels,actual GPU
SYNC_FD import/export BOTHdirections(includingpendingimports) through glibcMavk
and separate nativeMali. Negativeoracle/legacysync/commonnestedregression PASS.
NOT actualSurface/gameFPS/finishedWSI. Stop headlessclearbenchmark expansion.
Next bounded opt-in AHB-native compositor buffer+explicitfences,actualSurface
release/resize/reconnect,then realWSI consumer; preserveSHMfallback. Current
bannerAHBv2 demandsdma-buf andcannot justwrapX11SHM;accountforXwaylandboundary.
InstalledAPK27/commonf108/WSId07b andVMs untouched; no commit/push.

LATEST2026-10-23 AHB_CROSS_ABI_GPU_2026-10-23.md: 1920x1200 AHB GPU-written through
installedcommonMavk/glibc,handle passed to independent Bionic process,nativeMali
GPU-read allpixels PASS;sameID,negative oracle FAIL as intended. Native resource
interopGO withoutroot/vendorfix; NOT moving Surface or FPS PASS. Next implement
opt-in shared AHB-backed Wayland buffer/WSI adapter+pool+actual fence handoff, then
real Surface/Wine/D3D11. Existing ahb_attach requiresdma-buf/currentX11SHMdoesnot
qualify;no blindflags/fd0/modifier guesses. No new feature-matrix work, no CSE
tuning; installedSHMfallback andWindows/Ubuntu/P7/H264 preserved. No commit/push.

LATEST2026-10-23: HEADLESS_GPU_AND_CSE_2026-10-23.md. Common glibc/Vulkan layer
can now be checked directly under app UID without Steam/desktop/Surface/unlock:
24compilerchecks+26GPUcases PASS,existing fixtures/oracles,guardsON. Real glmark2
terrain also runs through upstream surfaceless native state. CSE experiment does
not show reliable speed gain;keepOFF and stop optimizing it blindly. Installed
APK27/commonf108 and WSId07b preserved;no live desktop session left. Next choose
actual end-to-end render/present bottleneck with comparable power/thermal state,
not more rare feature cases or micro-CSE. Strategic goal remains incomplete.

CURRENT2026-10-23 latest: common experimental APK26 installed,26fresh GPU cases
and normal Steam/Muck menu PASS. COMMON_RUNTIME_APK26_2026-10-23.md. Real common
offscreen3D benchmark now available: GLMARK2_COMMON_3D_2026-10-23.md;do not compare
visible60Hz cap to native uncappedFPS or call partial glmark score official.
Readonly metadata and WSIoptimal A/B showed no verified gain,neither default
enabled. Stop cache/WSI micro-variants. Compiled lifecycle candidate fixes queued
vsync callback duplication;6JVM cases/100cycles PASS,APK27 installed,actual normal
Steam/Surface background-resume regression next. Preserve rollbacks and restore
temporary profiles. Historical CURRENT entries below are earlier checkpoints.

CURRENT2026-10-23: UNIFORM_LAYOUT_2026-10-23.md. Enabled standard UBO validation
fixed for real Zink/glmark2; native GPU old/new valid/OOB +12regressions PASS.
Three common real3D scenes visible1080p at~60,Surface58.8updates/s; presentation
limited,not pure GPU ratio. No APK/product binary changes. Main next experiment:
Muck-only opt-in WSI optimal GPU render targets -> linear cached readback image,
copy recorded in existing present semaphore/fence submit,no extra CPU wait.
Build C73201c3 staged private; control cached+linear flag0,then flag1,then reverse
control. Preserve guards,game scene/resolution and record thermal. Restore profile
before stopping; no performance gain claimed until actual A/B results.

## PRIORITY OVERRIDE — performance first, user 2026-10-22

CURRENT2026-10-23: UNUSED_EXTENDED_LAYOUTS_2026-10-23.md. Consumer-driven generic
zero-flags/deferred-unused-UAB layout admission fixed and GPU verified; real used
extended sets remain fail-closed. Native capabilities unchanged; earlier push
diagnosis/filter experiment was incorrect and removed. GravityMark GL now reaches
context creation but GLXBadFBConfig; actual hardwareZink GL3.2/GLES2 vsgl45/gles32
benchmark path. Vulkan still blocked by BC admission. No comparable score.
Next choose a supported common real3D benchmark, not full GL/Vulkan feature
expansion for GravityMark. InstalledAPK25/production unchanged; private f1951495.

LATEST2026-10-23: Native GravityMark COMPLETE7004score/41.9avgFPS167.1s at
1920x1080Temporal100kDefaultLOD. No valid container/native ratio. Original Linux
ARM64 build exposed native subgroup-size16 stage requests rejected by Mavk.
Fixed bounded passthrough with guards enabled; final49369e39 private candidate
passes real Mali old/new nested-buffer A/B,12GPUregressions and2cull-link controls.
GravityMark first candidate passes previous PrefixScan boundary and reaches
RenderManager,gaps0; BC7 resource admission still blocks a valid benchmark.
See SUBGROUP_STAGE_PASSTHROUGH_2026-10-23.md. Desktop03 available; APK25 and
production baseline untouched. No new feature bits or FPS gain claimed.
Earlier3be3334e GPU timings below are INVALID (uninitialized-query race); only
checked output regressions remain valid. Profiler host-reset fix is separate.

2026-10-23: bounded native-only WSI+hybris GravityMark control reproduces GPU
fault without Mavk; resource substitution invalid, not general ABI proof. Stop
ASTC variants. Native Android run23532 started but USB disappeared: inspect SAME
run/result after reconnect, do not restart blindly. Desktop08 nowIDLE.
Private GPU profiler OOM fallback fixed,33hostchecks+ARM64build PASS3be3334e,
not installed or GPU tested. GPU_PROFILE_SAFETY_2026-10-23.md. Next resume native
result collection and existing GPU regression before any profiler staging.

NEW USER CLARIFICATION: stop the tiny-triangle benchmark work. Need a real 3D
benchmark outside Steam against the SAME benchmark on native Android, not another
synthetic consumer or invented native-power ratio. Selected GravityMark1.89,
official Windows MSI + Android APK downloads under E:/Mavk/artifacts/
gravitymark-1.89-20261022. Match version, raster scene, object count, resolution,
AA/temporal settings, vsync and thermal state. Windows D3D11 via existing isolated
Wine prefix/Sarek is the first container target; Android Vulkan is native control.
Different APIs include DXVK cost; not pure Mavk overhead. No benchmark score yet.

GRAVITYMARK_2026-10-22.md now records actual launches: native Android scene PASS,
no completed score; D3D11 needs11_1 vs Sarek11_0; Windows Vulkan BC7 not exposed;
isolated mobile-ASTC substitution renders but texture errors/GROUP_ERROR_FATAL,
including one fullscreen control. STOP faulty runs, no valid FPS/native ratio.
All isolated Wine processes stopped; user desktop08 READY1280x720 remains.
Do not pretend transient counters or cancelled tests are benchmark results.

Latest short benchmark: QUICK_D3D11_BENCH_2026-10-22.md. Corrected borderless
actual2560x1600: ARM64 45.47 vs AMD64 46.42 Present/s, AMD64 actual1280x800
59.86. NOT unique visible FPS, thermal3. Preliminary architecture disparity was
invalid window-size mismatch, corrected/retested. Do not infer FEX as primary
limit or promise Muck FPS from tiny textured-triangle workload. Baseline unchanged.

LATEST: WSI_READBACK_PERFORMANCE_2026-10-22.md. Real WSI split:~86–91ms
render-ready fence wait,~20ms copy;cached-memory candidate reduces copy12ms
but no stable overall FPS gain. Reversed tocontrol and restoredall overrides.
Do not keep optimizing copies. Next actual-game GPU execution timestamps vs
queue/presentation serialization,then one measured main-bottleneck fix.

First attribution checkpoint: MUCK_PERFORMANCE_2026-10-22.md. Actual forced X11
SHM output,~80ms/fence slow phase vs~2.1ms Android composition. CPU recording
not sole limitation. Isolate WSI render-ready/map/copy; no blind DRM/DRI3 toggle.
Profiler candidate tested/restored; no FPS fix yet. Thermal3 must be controlled.

TES->GS and its regressions are finished; retain limitations in
CULL_TESSELLATION_2026-10-22.md. Freeze new Vulkan feature implementation for now.
Existing-GS point lowering was discussed but NOT implemented. One agent, one
sequential performance investigation: Muck baseline -> whole-chain attribution
-> fix largest proven bottleneck -> same-scene A/B. Current launch is FEX plus
ARM64 Wine/DXVK-Sarek, not assumed Box64. Preserve APK25 and all protection.
Muck100+FPS is ambitious target, demanding game30+FPS later; neither promised.
PanVK comparison requires an actually runnable equal-GPU workload, not invented
numbers or a different GPU/device silently substituted. No root/security changes.
Use real display intervals plus CPU/GPU attribution, not Present calls alone.

## CURRENT — tessellation producer integrated, 2026-10-22

TES->GS common lowering and4->5stage API hook GPU PASS98304bytes4modes. Real
subdivided fan/partial cull, TES UBO+guards, native dependency reject verified.
PrimitiveId+26GPU+candidate Muck regressions PASS. APK25 unchanged. Details in
CULL_TESSELLATION_2026-10-22.md. Do not repeat closed TES fan/location/ID cases.
Existing GS cannot be handled by inserting another GS; next consumer-relevant
lowering/admission work must respect actual stage/output contracts. Public
feature staysfalse. Steam06 running; battery100%. C51GiB: big artifacts E:.

## CURRENT — PrimitiveId forwarding implemented, 2026-10-22

Inserted cull GS now preserves original IDs consumed by FS. Real three-primitive
hole/reset cases4modes393216bytes PASS,previous base4 Block and26GPU cases PASS.
Candidatea4dc8959;APK25 unchanged. CULL_PRIMITIVE_ID_2026-10-22.md has evidence.
Do not repeat closed ID/location matrices. Continue remaining consumer-relevant
stage contracts/admission; no public feature spoofing. Battery14% observed;
avoid unnecessary visible-game loops while USB discharges.

## CURRENT — whole-Block Location implemented, 2026-10-22

Common cull compiler supports variable-level Location with implicit member
footprints,not only hand-located members. Actual base4+array+flat payload PASS,
member-block regressions and26olderGPU cases PASS. Candidate5774b8bd; APK25
unchanged. CULL_BLOCK_LOCATION_2026-10-22.md records scope/hashes. Do not retest
closed location shapes; next consumer-relevant stage contract/admission gap.
Public feature stays false; no broad upstream DXVK/CTS claim.

## CURRENT — user Block forwarding implemented, 2026-10-22

Member-located user interfaces now survive common VS->GS->FS cull transformation:
actual loader/UBO/guards proof131072bytes and26GPU regressions PASS. APK25 stays.
See CULL_USER_BLOCKS_2026-10-22.md. Do not repeat this closed matrix; next handle
real stage/interface admission gaps. Public shaderCullDistance remainsfalse.

## CURRENT — automatic cull pipeline linking, 2026-10-22

Private original-VS/FS hook now PASS131072GPU bytes in4modes includingUBO+
guarded execution,native dependency fail-closed PASS.26priorGPU+Muck candidate
regressions PASS. APK25 unchanged. See CULL_PIPELINE_LINK_2026-10-22.md.
Do not reimplement interception or repeat closed triangle tests. Public cull
feature stays false;remaining stage/interface/PointSize/topology contracts
and eventual device admission are real work,not a query-bit fix. Continue
consumer-relevant semantics,not more descriptor/logging matrices.

## CURRENT — CullDistance mechanism, 2026-10-22

Selected ordinary-DXVK P0 shaderCullDistance. Shared compiler+native GS works
on G925 for VS->GS->FS triangles,8cases native/tracking+guarded65536bytes PASS;
host6valid/5negative and default-renderer GPU regressions PASS. No feature bit,
no automatic loader-hook/Bridge/ICD/ordinaryDXVK admission yet. APK25 retained.
CULL_DISTANCE_2026-10-22.md has required remaining scope. Continue actual
pipeline-link integration/limit contracts and consumer-required semantics;
do not call this full cull support or replace it with fragment discard.
Do not return to closed logging/descriptor tests without relevant changes.

## CURRENT after APK25, 2026-10-22

Optional trace implemented and controlled A/B verified:~3.29us record command
off vs~6.59us on,not FPS. All26COMMON GPU cases+normal Muck menu/Settings/input
PASS;runtime4de26844 installed,source assets synchronized,APK24 rollback kept.
Error/rejection gate with trace0 PASS. TRACE_POLICY_2026-10-22.md records evidence.
Logging/diagnostic overhead task FINISHED;do not expand its matrix or do another
APK solely for cosmetic logging. Return to cross-subsystem consumer P0/P1;
select shader/render-state semantic blocker from actual DXVK requirements.
Preserve finite descriptor checkpoint;no broad feature admission/CTS claim.

## CURRENT after APK24, 2026-10-22

Disabled-report diagnostic read/log overhead fixed in actual layer; enabled
reporting preserved.26COMMON GPU cases+normal Muck menu/Settings/input PASS,
runtime0d2d96b4 installed and source assets synchronized, APK23 rollback kept.
DIAGNOSTIC_RETIREMENT_2026-10-22.md has exact scope and artifacts. Do not repeat
this gate without a relevant code change. Next consumer-relevant P0/P1 remains
across image/shader/render-state compatibility, not rare descriptor expansion.
Other per-draw/template/secondary tracing is visible in18.2MB candidate smoke
log; measure its overhead before calling it the main game bottleneck or
silently suppressing diagnostics required by current test consumers.
No full-feature/CTS/upstreamDXVK/visible-FPS claims. Older sections historical.

## CURRENT after APK23 integration, 2026-10-22

Runtime-format metadata implemented, not pending:26COMMON GPU cases and normal
Muck menu/Settings/input PASS on installedAPK23/runtime62fcf634. Source assets
and matching compiler/optimizer pins synchronized; APK22 rollback retained.
TEXEL_FORMAT_METADATA_2026-10-22.md records scope/provenance. Do not repeat
closed DeepSeek cases or extend rare descriptor/atomic forms for coverage alone.
Next choose one consumer-relevant P0/P1 in image/shader/render-state compatibility
or a measured overhead, preserving finite descriptor checkpoint and truthful
feature bits. CTS/upstreamDXVK/full-game/performance remain separate gates.
Earlier next-step sections below are historical and superseded.

Concrete overhead candidate observed during APK23 smoke:245188
MAVK_LAYER_GUARD_LOG records despite MAVK_GUARD_REPORTS=0 in actual game env.
Device::retireCommand in adapters/vulkan-layer/layer.cpp unconditionally reads
diagnostics and prints each retired last-owner generation. Before claiming a
performance bottleneck, check report-disabled semantics and measure this path;
do not silently discard requested diagnostics or claim measured FPS benefit.

## Next after isolated component-substitution fix

Layer8470144c FAIL-before/PASS-after on missing-alpha OOB;25GPU+6forms+visible
Muck PASS. APK22 stays installed;new candidate not rolled into default pins.
Unknown-format samplerBuffer requires actual VkBufferView format information;
do not infer RGBA/R32 from SPIR-V sampled scalar type or set a feature bit.
Choose the smallest real format-metadata/specialization mechanism,then actual
GPU execution,not another atomics matrix. Existing known-format patch and
artifacts documented in TEXEL_COMPONENT_SUBSTITUTION_2026-10-22.md.
Independent DeepSeek tests reviewed;no need to repeat closed cases or adopt
hardcoded test infrastructure wholesale. No production U-AVF changes.

## Current after verified APK22 rollout — 2026-10-22

APK22/runtime1e859af6 installed,24COMMON GPU PASS,visible normal Muck menu/
Settings/input PASS and Steam intact. Source assets/default archive pins synced.
No more native/atomic/rollup repeats without a code change. Checkpoint the bounded
shader/buffer mechanism;next choose one consumer-relevant P0/P1 across remaining
image/shader/render-state compatibility,not another rare atomic/descriptor form.
No full feature-bit admission until mandatory semantics implemented/proven.
STATE.md and STORAGE_ATOMICS_NEXT_2026-10-21.md carry exact evidence and rollback.

## Latest action — actual guarded atomics implemented

Isolated ca87ea54 GPU gate PASS: five phases/320words; prior23 GPU cases PASS.
Host structure and compiler regressions PASS. Finish six aggregate-form and
visible Muck integration gates before any runtime/default pin update. Do not
expand atomic dimensions/rare variants just to grow coverage; keep this bounded
mechanism truthful and move next by actual P0 semantics after integration.
STORAGE_ATOMICS_NEXT_2026-10-21.md records source patch/archive provenance.

## CURRENT next step after APK21 common/visible PASS

Native R32 storage-texel atomic prerequisite PASS:64words add/exchange/CAS/read,
Mavk absent in loader trace. Proceed to actual guarded reference/clone/bounds
implementation,not another native test. STORAGE_ATOMICS_NEXT_2026-10-21.md.

InstalledAPK21c5f68760/runtime8de92183/layerc2a12a21:23common GPU cases PASS,
visible Steam+Muck menu/Settings/input PASS,source assets/build synchronized.
Stop buffer-package deepening;finite checkpoint BUFFER_PACKAGE_CHECKPOINT_2026-10-21.md.
Next real missing mechanism is guarded storage-texel integer atomics,not another
rare descriptor variant. Native R32 format atomic support first;then proper
OpImageTexelPointer reference/clone + init/bounds semantics. Keep unsupported
dimensions/widths/forms explicit. No ordinaryDXVK/full-feature/CTS claim yet.
Keep APK21 and production baselines while doing isolated candidate work.

## Current action after verified APK20 rollout

APK20/bfc10a4c installed+22common GPU PASS;visible Steam/Muck/input PASS at menu
level,no game-completion/FPS claim. Source assets/default builder synchronized.
Storage texel bounds test exposed real mixed-signedness generated UMulExtended
type error;fixed on host,frontendf0a0c057 built. Finish isolated GPU gate with
MAVK_STORAGE_TEXEL=1,then previous22 regressions and finite checkpoint.
Keep this candidate separate from APK20 until verified. No repeated rare
descriptor expansion,no fullrobustness2 advertising,production untouched.

## Current next action — 2026-10-21 (older sections are historical)

Vulkan1.2/SPIR-V1.4 CopyMemory fix PASS on Mali:c41b2f2d,20cases/6forms1152words.
Native vertex gate2formats/3firstVertices1536components PASS with/withoutMavk;
not whole feature proof. Now fix reproduced OpCopyObject pointer alias bypass:
host fixture FAIL before/PASS after. ARM804d9271/872c57ec now21GPU cases and
6forms1152words PASS;real Muck D3D11 FL11_0/81534guarded commands,no observed
Mavk errors. Headless only,game stopped/override removed. Preserve this package;
next actual integration gate is visible Steam/Muck,not another rare descriptor
form. Continue remaining P0 semantics without fake feature advertisement.
Preserve finite aggregate package. Plain-copy candidateb46cfb43 already
has real Muck headless execution202826commands,no observed Mavk gaps;this is
not visible rendering/FPS. Game stopped,override removed,installedAPK19 intact.
Do not roll APK20 without application regression or declare robustness2 from
this partial slice. Next meaningful contract is remaining buffer/vertex/texel
robustness,not more speculative descriptor forms. No production or VM changes.

## Current work — aggregate/CopyMemory P0 contract

CopyMemory hardwarePASS now:b46cfb43/6e330b19,19cases+5forms/960words. Finite
aggregate checkpoint recorded. Do not roll into APK20 before application
regression; sourceassets/installedAPK19 stay73ec2df4. Keep other missing
robustness2 contracts explicit instead of declaring featurecomplete.

AggregateBuffers shared pass implemented,struct/array/col-major/commonmemory
GPU proof768words plus17priorregressionsPASS. Candidate only,no APK20.
Plain CopyMemory lowered into same typed load/store path; hostPASS,GPU pending.
AGGREGATE_BUFFERS_2026-10-21.md. Keep featurefalse and explicit scoped memory,
spec-sized array/row-major/nonuniform coverage limitations. APK19 common17GPU
PASS,but visible Steam/Muck requires actual Android session display (keyguard).

## Latest implemented runtime — APK19

Shared buffer guards now normalize nested chains, saturate generated32-bit
offset math and reject lost bits during64-bit narrowing.17casesPASS isolated
and normal installed runtime73ec2df4,layer9bab9c9c;APK0f77d072/rollbackAPK18.
Finish Steam13 client repair/UI/Muck check, then next concrete P0 contract is
aggregate buffer load/store handling. Keep featurefalse until full semantics
proved. Do not substitute more P2 descriptors or FPS tuning for this work.

## Current next implementation: upstream P0 buffer robustness

Generated-offset32-bit overflow is now fixed and Mali-verified (multiply+add),
alongside12 prior GPU scenarios. Saturation changes only generated bounds math.
Next concrete gaps:64-bit index narrowing and aggregate buffer accesses,then
complete vertex/texel contract. No feature advertisement from partial coverage.
Common build uses overflow-frontend6bb1b750;installedAPK18 remainsunchanged.

Finite shader audit is done. robustBufferAccess2 selected ahead of image atomic
P1. First real gap fixed: nested AccessChains now canonicalized before guards;
hardware read-zero/write-suppression PASS +11 prior GPU regressions. See
BUFFER_ROBUSTNESS_2026-10-21.md. Do not deepen green descriptor/image matrices.
Continue this P0 by concrete remaining semantics; feature bit staysfalse until
sufficient implementation/evidence. No scalarization-is-mandatory assumption.
APK18 remains installed; candidates isolated, Windows/Ubuntu untouched.

## Active priority change — stop descriptor deepening

User explicitly prioritizes practical cross-subsystem P0/P1 coverage. Broader
UAB implementation deferred; actual Sarek native sampler heap is already kept
native, Steam tracking/native is working. Descriptor package checkpoint and
remaining gaps: DESCRIPTOR_CHECKPOINT_APK18.md. CTS NOT RUN, no all-games claim.
COMPATIBILITY_PRIORITIES_2026-10-21.md supersedes older next-descriptor suggestions.
Next images/formats/BC subresource+transfer correctness, then shader/pipeline
semantics. Systematic performance later: Muck first, then HL2. No new P2 null
shapes/YCbCr/descriptor-model expansion without a concrete consumer requirement.

Images P1 first package complete on UNCHANGED APK18: actual BC1RGBA3mips/2layers,
16mutable UNORM/sRGB views, padded upload, host change AFTER recording and image
copy; native/protected256components match,protected16draws/18submits,gaps0/
Corezero. IMAGE_SUBRESOURCES_2026-10-21.md scopes artifacts and untested cells.
No runtime fix/reinstall needed; do not deepen this green matrix indefinitely.
Next shared shader blocker: image atomics/OpImageTexelPointer are explicitly
rejected,where buffer integer atomics work. Native capability != safe null/OOB
instrumentation. Design/implement at the operation,not an invalid guarded pointer;
do not remove rejection or enable features prematurely. No new game/FPS work yet.

## Latest — 2026-10-21: RGBA8 metadata package

Real Steam format37 gap reproduced with standard sampled-buffer API. Common
metadata now recognizes native RGBA8 four-byte texels without native capability
or null-feature advertising. CPU53/39+9suites, GPUUNORM/UINT512components,
all prior guarded-layer cases and Muck isolated80209executions/gaps0 PASS.
APK18/runtime161fe2e8 installed, rollback APK17. COMMON texel oracle512components
PASS/Corezero. Big Picture session10 now READY/Steam11074 alive/sessiongaps0,
hardware ANGLE/Zink/Mali. No exact causal/exhaustive stability claim from one
successful boot. COMMON Muck12020 check PASS11569protected/gaps0/device-lost0;
game stopped,override absent,Steam running. Next choose descriptor/format semantic
package from actual consumer or audited matrix, not a game-specific exception.

## Latest — 2026-10-21: selected bindings isolated GPU PASS

Final layer219b5bf0: strict selected/negative/dynamic/empty/unused/routing gates
PASS. Grouped SPIR-V exposed real reflection + guard-compiler bugs; corrected
generically and verified192immutable components on Mali. Compiler25/9, reflection
formats6/negatives10/groups11, fresh Core/image/descriptor/BC regressions PASS.
Isolated Muck141349protected/gaps0/device-lost0; override removed. APK17/common
acda20c7 installed asset-only, common oracle512words PASS and Muck25059protected/
gaps0/device-lost0. APK16/d1ad rollback preserved. Steam
Big Picture failed independently on that baseline; normal desktop Steam UI
starts and is the working integration route; game stopped/preferences restored.
Next D4A completed: existing dynamic consumer reusable mode, two fence-completed
replays/reset, and two genuinely pending SIMULTANEOUS_USE uses behind a host
timeline gate. Native/protected512words match,gaps0/Corezero. No runtime/Core
change required; DESCRIPTOR_LIFETIME_2026-10-21.md records bounds. General late
updates remain missing. Next choose a coherent native/private-metadata late
update design before widening flags, or close real format37 native-view metadata
gap seen in baseline Steam. Do not confuse that gap with proven crash cause.
No broad UAB/nullDescriptor advertisement.

## Historical preparation — selected bindings SOURCE ONLY

Next package implemented offline: selection-aware CPU snapshots/cache/lifetime,
full native allocation schema/pool budget, complete dynamic-slot mapping and
per-stage reflection filtering. Fresh host12/16 + nine rebuilt suites PASS.
Isolated ARM64 layer, final consumer and Android probe built; GPU NOT RUN.
ADB reports no devices. Installed APK16/commond1ad stays unchanged; no promotion,
feature advertising or commit. SELECTED_BINDINGS_2026-10-21.md has exact matrix,
hashes, prepared payload and minimal positive/negative/regression order.

NEXT ACTION on device return: isolated --selected native/guarded, then protected
--unwritten-used rejection, strict verifier and prior GPU/consumer regressions.
Do not skip this GPU gate to begin D4. D4 record-time/UAB semantic gap audited,
not implemented. Goal remains active, no production changes.

## Latest — 2026-10-21 08:53 Moscow

Selected-entry static-use/whole unused NONEMPTY sets package implemented,
isolated GPU verified; STATIC_USE_2026-10-21.md. Host analyzer14/7, ARM64 earlier
fixture8/7, native/protected512words+1024components, missing-used negative with
zero submissions. Prior descriptor/routing and fresh bounded Core/image/BC
regressions PASS. Muck15053 maps candidate,220054protected executions/gaps0;
not a visible-FPS claim. Override removed, game stopped. Common runtime d1ad4522
installed in APK16 (SHA5097806c), oracle512words/1024components PASS. Muck18364
maps COMMONd1ad,113581protected executions/gaps0/device-lost0; Steam Zink/Mali.
Game/probe stopped, original sleep preference restored absent/default manual.
APK15 is rollback. No production changes, no commit/push.

NEXT: UNUSED BINDINGS within USED sets, with full-layout native pool budget,
precise metadata and dynamic-offset accounting. Then D4 submit-time/versioned
snapshot lifetime before general UAB. Do not weaken initialization globally.

## Latest — 2026-10-21 08:29 Moscow

D3 empty layouts/unused EMPTY slots implemented and promoted to common APK15.
Native/guarded mixed set0/2 + empty1 match512words; all-empty descriptor-free
rendering matches1024components,72red/184black pixels. Registered empty state
is not an absent placeholder/null descriptor; old negative gates retained.
All-empty avoids metadata adapter and guard compilation. Also fixed native
descriptor-free observation gap instead of weakening strict verifier.
Final layerdd714682; ordinary dynamic/immutable/routing and host9suites PASS.
Fresh Core/image/generation/template/BC bounded regressions PASS. No CTS/full
indexing/nullDescriptor/robustness2/performance claim.

Common24b402d3 installed and empty oracle PASS. Normal Muck11382 maps COMMON24b4,
protected rendering/gaps0/device-lost0; Steam retained Zink/Mali. Test game and
probe stopped, original sleep preference restored absent/default manual.
APK14 is rollback, no data clear. Exact hashes/scope in
STATE.md / EMPTY_LAYOUTS_2026-10-21.md. No production changes, no commit/push.

NEXT: entrypoint/static-use-aware NONEMPTY unused sets/declarations, not simply
accepting unbound/unwritten resources globally. Then D4 submit-time/lifetime
semantics before general UAB/indexing flags. Preserve native sampler heap path.

## Latest — 2026-10-21 08:05 Moscow

Immutable sampler package implemented and promoted to common experimental APK14:
owning arrays/leases, standalone init, ignored combined-write sampler, correct
combined-copy destination semantics, templates and layout compatibility.
Protected GPU192components PASS. Direct ANDROID Vulkan reproduces native copy
108vs18 with SyncVal0/0; matching-sampler native control PASS. Common Mavk shadow
corrects it without game rules/pixel copies; do not claim equal native PASS.
Dynamic/static command routing, host8suites and Core/image/BC bounded regressions
PASS, scopes IMMUTABLE_SAMPLERS_2026-10-21.md. YCbCr remains explicitly unsupported.
No full nullDescriptor/robustness2/CTS/performance claim.

Common runtime1f3566b1 verified installed; immutable oracle192components/Corezero.
Normal Muck406 maps COMMON library with no override, protected rendering/gaps0/
device-lost0; Steam still Zink/Mali. Test game stopped; original sleep preference
restored absent/default manual. Production/data unchanged, no commit/push.
Exact hashes/rollback/state in STATE.md.

NEXT D3 empty descriptor layouts / unused public slots, preserving set numbering
and metadata without treating an empty set as a null descriptor. Include mixed
nonempty/empty and no-descriptor shader cases with real native/guarded GPU checks.
Then D4 submit-time/lifetime model before general indexing/UAB admission.

## Latest — 2026-10-21 07:36 Moscow

D3 dynamic UBO/SSBO native protected slice implemented and GPU verified:
seven descriptors/two sets, arrays/partial binds/immediate offset-array reuse,
nonzero base+offset and WHOLE_SIZE/zero offset. Native/guarded512words matched,
four dispatches/two submits,static generation reused across changed offsets.
Final layer a96aa087; static descriptor hot path has no new per-element scan.
Host six suites + new21/13 PASS. Fresh Core five gates/13rows,2D/array/3D/cube,
generation/templates,BC regressions PASS; no CTS/full feature claim.
Installed experimental APK13/common b047abed; exact package/hashes/rollback and
remaining coverage STATE.md / DYNAMIC_DESCRIPTORS_2026-10-21.md. Common-library
dynamic oracle PASS; normal Muck25086 maps common b047 without per-game override,
98038protected executions at capture,gaps0/device-lost0. Steam still Zink/Mali.
Test game stopped, original sleep preference restored absent/default manual.
Production U-AVF untouched. No commit/push. Runtime compiler/atomics/separate
sampler modes retain older evidence, not newly rerun.

NEXT: D3 immutable sampler ownership and layout compatibility/empty sets, then
D4 legal descriptor late-update semantics. Do not advertise unsupported indexing
flags, full nullDescriptor or robustness2; no Muck FPS-target detour.

## Latest — 2026-10-20 21:46 Moscow

Common experimental runtime rollup DONE: not just Muck override. Installed APK12,
same baseline certificate/unchanged non-assets, common-library real GPU test
PASS512words/8192components,gaps0/Corezero. Steam hardware renderer retained.
Runtime9fb32fc7; details/rollback COMMON_RUNTIME_2026-10-20.md. No fresh visible
gameplay claim while locked; Muck launch request did not spawn game. Original
sleep preference restored absent/default manual. No commit/push/feature claim.
Eight runtime-updater host tests PASS. Full Gradle build was deliberately
cancelled; APK was asset-only repack, not a newly compiled Java/native app.
Fresh standalone Core regression remains OPEN after prior cancelled attempt.

Next package D3: normalize dynamic offsets against device alignment/range and
preserve immutable sampler ownership/layout compatibility before admission.
Do not simply remove layout/bind rejection. D4 legal late updates need
submit-time/version-aware generations, not current record-time snapshots.
Keep native/guarded/unsupported distinction and bounded GPU oracle per slice;
do not return to Muck FPS targets or claim full robustness2/nullDescriptor.

## Latest — 2026-10-20 21:14 Moscow

Execution routing fixed: indirect-count draw/indexed draw and DispatchBase use
shared protected descriptor preparation, exact core/KHR/AMD spelling/arguments.
Real standard-loader consumer native vs guarded PASS512words/8192components,
2base-dispatch/8count-draw cases,12guarded prepares/submits,gaps0/Corezero.
Core/KHR actually executed; AMD unavailable. No CTS/SyncVal/FPS claim for this
consumer. Muck background startup/render-log regression PASS, candidate mapped;
Android locked, so no new visible Surface/gameplay claim. Original DroidDeck
sleep preference restored after temporary background test setting; game stopped,
Steam suspended/preserved, no production/commit/push change.
Fresh Android Core Suite attempt stalled/no reports, cancelled only probe after
inspection; own journal retained, NOT fresh PASS or proven new-layer regression.
See DESCRIPTOR_COVERAGE.md for hashes/evidence/limitations. Next: repeat that
suite under usable lifecycle, stage vetted layer in COMMON experimental runtime
(not just Muck override), then D3 descriptor semantics/execution coverage.

## Latest — 2026-10-20 20:48 Moscow

Sampled null cube implemented with actual6face image/view/initialization.
GPU1792words/512guards,14fallback readback words, SyncVal0/0/Corezero PASS.
Common reflection cube/noncube distinctions PASS; cube-array/depth/MS/storage
cube remain rejected. New native binary2D,array,3D + Core5gates + descriptor
generations/templates + BC GPU regressions all PASS; rebuilt6host suites PASS.
Real Muck new layer mapped, correct menu/no observed gap/device lost. Game
stopped; Steam suspended/preserved. No feature-bit,production,commit/push change.
Evidence/scopes DESCRIPTOR_COVERAGE.md. NEXT: protected execution routing gap:
DrawIndirectCount/DrawIndexedIndirectCount and DispatchBase core/aliases currently
bypass prepareExecution. Implement/test shared routing before more shape polish.

## Latest D2 implementation — 2026-10-20 20:36 Moscow

Common null color3D path implemented: reflection->typed real3D image/view->layer
resolve, same init/prelude/cache. GPU sampled3D explicit LOD PASS1792/512; sampled
fetch + live/null R32 storage3D PASS1792/512, SyncVal0/0. Shape exclusions remain
(cube/depth/MS/nonexistent3D-array); no extension/feature bit advertised.
New native binary regressions2D/array1792/384 each PASS; six host tests rebuilt
PASS. Fresh Core5gates, descriptor generations/templates and BC suites rerun.
Real Muck new layer mapped, animated menu/no gap/device-lost; menu-only check,
not performance or gameplay claim. Stopped game, kept Steam. Production intact.
Artifacts/hashes/scopes in DESCRIPTOR_COVERAGE.md. No commit/push.
Next: fill genuinely missing D2 operation/resource classes; avoid duplicating
already passed fixtures. D3/D4 remain dependent architectural work, not flags.
Obvious later perf cleanup: reports-off still reads diagnostic buffers and
prints zero logs on retirement; preserve checks while separately testing removal
of those unused diagnostic operations, not a new Muck FPS sweep.

## Latest D2 evidence — 2026-10-20 20:26 Moscow

Live/null array reads PASS for fetch and explicit LOD on Mali: two distinct
layers through baseLayer1 view, two descriptor generations,1792words/384guards
each, SyncVal0/0. All-null array regression1792/512 PASS. Parameterized existing
harness only; fixed truthful operation/count/layer reporting, not feature bits.
Fresh Core five-gate/13-row regression, descriptor writes/templates GPU and
BC123/45/67/texture lifecycle PASS. GPU report mtimes verified fresh. Device
thermal0; production/Steam unchanged. No commit/push. Next D2: mip/filter
boundaries and remaining image-shape/operation gaps; then D3/D4 as below.
See DESCRIPTOR_COVERAGE.md for artifacts, hashes and deliberately missing scope.

## Superseding checkpoint — 2026-10-20 20:20 Moscow

User priority: systematic compatibility batches, descriptors first; Muck only
integration regression, no target-FPS chase. After a good fixed version the user
permits a Mavk-only commit to its private repository. No other commit/push
authorization; no commit/push performed at this checkpoint.

Short FPS attribution ended: real menu16.0 Surface buffers/s over5.1s,
record-profile guard preparation107–143ms per approximately1s, ~3200calls/s.
This is CPU recording wall time under the existing mutex, NOT GPU time or
unique FPS. ThermalStatus4 invalidates a clean performance A/B; GPU shader,
driver/WSI and runtime costs remain unattributed. Do not claim a root-cause fix
or resume FPS sweeps. Game stopped; Steam session preserved/suspended to cool.
App settings republish had overwritten guest-only test selection on resume;
helper now updates only Muck overrides in both stores, with app backup.
Actual profile-record mapped library confirmed before accepted measurements.

Descriptor audit/next batches: DESCRIPTOR_COVERAGE.md. WHOLE_SIZE texel truncation
fixed generically; six host suites PASS34/25,18/6,20/20,9/7,13/18,39/32.
New explicit-LOD null2D-array sample GPU PASS1792words/512guards, SyncVal0/0,
same parameterized oracle (no fullfeature claim). Existing array-fetch,
baseline2D and no-report results retained. Fresh five-gate Core suite PASS,
13 bounded native rows, not guard/CTS/fullVulkan proof. Artifacts on E:.
Next D2: live array/layer and sampling boundaries before expanding shapes;
then D3 dynamic offsets/immutable/empty layouts, D4 late updates/lifetime.
Full nullDescriptor/robustness2 still unadvertised. Production untouched.
Historical entries below are not current run-state.

## Current priority — 2026-10-19

## Latest verified — 2026-10-20 16:48 Moscow

Muck scene/gameplay now renders via shared null-array fix; user confirms works,
but low FPS. Array GPU1792/512 and baseline2D1792/384 PASS, SyncVal0/0. Real
layer1e9ef0cb maps confirmed; no old gap/CS-thread failure. Keyboard W accepted,
successive Android captures show real changing game world/HUD. Not full game
compatibility/performance PASS. DESCRIPTOR_COVERAGE.md is the class-wide plan.
Fixed CRLF parser in DroidDeck pacing tool; Linux tool tests30/30 PASS (Windows
fake-ADB suite incompatible). Actual10s Surface capture89frames/~8.8fps,110ms
cadence; not unique-content FPS. Report stream contains380337309words in some
retired GPU generations: investigate atomic diagnostic overhead before random
runtime flags. Same access guards with reports omitted is the next isolated A/B.
Layer guard-policy built0; no-report shader validated, GPU oracle building.
Baseline game stopped cleanly after log archive, Steam session retained.
No commit/push, no production U-AVF changes. Historical entries below.

2026-10-20 USER PRIORITY: test Steam-installed Muck (1625450); defer SuperTux.
LATEST: MUCK_2026-10-20.md supersedes older consumer-next notes. Windows depot
via normal Steam/managed Proton reaches Mali/D3D11/audio, BLACK picture. Actual
Mavk vertex guard pipelines rejected for absent vertexStoresAndAtomics. Candidate
Read-only guard fix validated incl.SPIR-V1.6, compiler25/9 PASS. Actual Muck now
renders3D menu background instead of black; UI missing, not playable/FPS PASS.
Maintenance5 buffer-view flags2/subset regression PASS14/19. Consumer rerun
advances to exact native view format109(R32G32B32A32_SFLOAT), offset1054208,
range96. Driver creates it; scalar-only Mavk metadata rejects it, invalidating
subsequent descriptors. General 32-bit scalar/vector texel sizing implemented;
regression17/22 PASS; actual consumer logs native format109 views/elements6.
Next Mavk gap: image descriptor layout rejected. Aspect-aware read-only layouts
candidate + existing descriptor regression20/20 PASS on tablet; layer building.
Source DXVK sampled depth/stencil uses read-only depth layout. Final shared-policy
layer8e06329d consumer screenshot image-layouts2.png shows real3D lobby +full UI.
Next observed Mavk gap: guarded execution null color image shader type not
admitted, followed by secondary command-buffer end failure. Not full game/FPS
PASS. User finished testing and authorized continuation. Preserved scene logs;
shader diagnostic proves sampled2D-array(dim1/depth0/array1/MS0/sampled1).
Generic color-image shape map + type-aware null view cache implemented; no
Muck/Unity conditions. Reflection6/4 PASS incl.array shape/no scalar alias.
Layer null-arrays built0; existing GPU oracle parameterized for sampled2DArray,
probe APK built0, install/test in progress. Do not deploy consumer before GPU
result. DESCRIPTOR_COVERAGE.md is the next shared audit/matrix, per user priority.
No arbitrary2D substitution or fake feature bits.
No game-specific shader rule.
Steam unchanged; full nullDescriptor/robustness2 still NOT advertised.
ADB restored, live Steam session 2026-10-20-01-steam READY; no session restart.
Muck Windows depot installed through normal Steam after Linux/FEX failure.
User priority: universal correct game path, not Muck-at-any-cost. Classify each
blocker as Mavk / Sarek / ABI-WSI / external runtime before changing its layer.
DDNet evidence retained, no more broad graphics flag sweeps.

Current consumer step: NEXT_CONSUMER_SUPERTUX_2026-10-19.md. DDNet diagnosis saved;
core/sync validation ran on real corrupt menu, no reported sync hazards. Prepare
real SDL/D3D11 SuperTux through existing Proton/Mavk, not more DDNet flag sweeps.
Official x64 assets verified on E:, streamed transfer0 but device offline before
verification/launch. No game PASS. Resume by checking ADB and file hashes, reuse
live Steam, then capture first D3D11 frame/blocker. Full strategic goal unchanged.

Pause DDNet per user. Hardware Steam UI PASS through a private Zink25.1.9 build,
whose normal lazy-descriptor path does not require nullDescriptor. No fabricated
Vulkan feature bits. Modern bundled Zink fails explicitly on that feature;
full Mavk nullDescriptor implementation is still incomplete. First glxinfo/
context gate and actual authenticated accelerated CEF UI passed. User confirms
very smooth navigation, with small one-time menu artifacts. Unique visible FPS
unmeasured (pacing tool NO_FRAMES); do not claim 60 visible FPS. Preserve session.
APK11 installed: normal Mavk start selects hardware Steam UI, real CEF confirms
Mali; settings toggle restores software UI next session. EN/RU labels updated.
Real DDNet ARM64 Wine/OpenGL reaches menu, but severe corruption and audio
failure remain: NOT game PASS. GL1.1 ->3.0, removal of Mavk layer (maps checked),
and disabling hybris packed-vertex fallback do NOT fix it. Preserve Steam.
Next: one game-only Mesa glthread/client-array lifetime A/B; investigate real
consumer failures, not unrelated Feature Suite. Native Vulkan/DDNet vendor
canary blocker remains separate. Do not claim DXVK from this OpenGL test.
Keep software UI as rollback, Adreno/runtime/rootfs untouched. New artifacts E:.
Windows/Ubuntu deferred. Actual game remains the final target, not GLX success.

## User completion target clarified 2026-10-18

Full opt-in DroidDeck fork on stock Tab S11: ordinary Android installation ->
existing Steam/FEX/Wine/Proton -> user Steam login/game install -> actual Windows
game using hardware Mavk/Mali graphics, input and sound without per-game terminal
commands. Native/Wine D3D11 windows are intermediate gates, not final success.
Preserve original UI/runtime/component architecture and stock Adreno/Turnip DXVK.
Mavk/Sarek experimental selection only; upstream DXVK remains strategic target.
Measure unique visible FPS/frametimes, artifacts and stability, not Present count.
After Tab S11 works, check realme8/Mali-G76 portability on available hardware.
U-AVF Windows work deferred; no production changes/commit/push.

User's newest attachment and direct authorization supersede older ordering.
Newest instruction (2026-10-18): NO commit/push without separate permission,
including private Mavk. Preserve remote license and production U-AVF.
Finish separate sampler regression, then real DXVK-Sarek consumer integration;
do not expand unrelated Feature Suite gates. Fix observed consumer API blockers.
Sarek is an experimental Mali option,not the permanent/only DXVK architecture.
First real D3D11 frame ->working opt-in DroidDeck fork ->exact upstream DXVK
gap audit and shared Compatibility semantics. Keep stock Adreno DXVK unchanged.
Device creation is not game compatibility;DXVK is not itself a WDDM GPU driver.

Current priority: null-vertex zero-buffer shortcut NO-GO (robust1 insufficient).
Stop theoretical nullDescriptor expansion. Run actual Sarek with isolated
diagnostic admission, feature bit still FALSE; fix only observed API blockers.
Diagnostic admission is not Vulkan correctness or production support.
Real D3D11 device/FL11_0 +solid HLSL Draw PASS (run30), textured Texture2D.Load
Draw/readback PASS (run34), latest-layer guarded regression PASS (run35).
Actual filtered native sampler-heap pipeline PASS (run36), nearest/bilinear
16384 components each; latest guarded regression PASS (run37).
Actual visible native DroidDeck/swapchain in its app UID PASS (layer26),
reproduced moving texture, owned cleanup/Corezero. Not Wine/game/visible-FPS PASS.
Next opt-in Wine/PE consumer with matching PE Sarek DLLs.
FloatControls2 parser gap remains in old shared SPIR-V tools. Run34's local
optional Sarek fallback is diagnostic only, not general feature implementation.

## 1. DroidDeck / real D3D11 through DXVK-Sarek (current)

- [x] Pinned Turnip/Mali/DXVK capability comparison; Sarek leaves BC+nullDescriptor.
- [x] BC1–BC7 CPU correctness and bounded native GPU upload/sampling gates.
- [x] Common BC mip/update/copy shadow+dirty lifecycle; native partial upload/sRGB.
- [x] Null SSBO load/store +UBO load guarded feasibility, no feature advertising.
- [x] Scalar SSBO range read-zero/write-discard feasibility on native Mali;
      same metadata phases,1536 values/896 guards,SyncVal0/0. Full feature OPEN.
- [x] Null combined-sampler texelFetch and R32_UINT storage image load/store
      feasibility1792 words/384 guards,SyncVal0/0; all image semantics OPEN.
- [x] Configurable combined shader pass and minimal integer-buffer atomic
      extension; actual32-bit atomicAdd null/valid result+side-effect PASS.
- [x] Shared descriptor state with explicit unwritten/null/bound distinction,
      resource leases,checked updates/copies,immutable cached snapshots/both ABIs;
      integrated into native descriptors and buffer/atomic/image GPU gates.
- [x] Shared native Vulkan materializer with immutable set/metadata generations,
      retained resources/fallbacks,unchanged-generation cache; actual two-recorded
      generation execution/lifetime gate on Mali,SyncVal0/0.
- [x] Bounded Vulkan-structure update/copy frontend and trusted resource registry;
      ordered transactional writes/copies,consecutive bindings,resource retirement.
      Not loader exports or full Vulkan update-after-bind semantics.
- [x] Descriptor update-template expansion required by Sarek:owned schema,
      bounded interleaved source,transactional reuse;CPU13/18 and actual two
      recorded GPU generations128words PASS,SyncVal0/0. No loader interception.
- [x] Same portable guard compiler in host CLI and Android runtime with bounded
      immutable-result LRU;actual original SPIR-V ->on-device compile ->Mali
      atomic/null/range gate PASS1536words/1024guards,SyncVal0/0. Vulkan1.1
      experimental ABI only,not exhaustive shader admission/full consumer hooks.
- [x] Multiple public sets +configurable private guard set;logical Vulkan-set
      composition/cache/leases ->native layouts,actual Mali SSBO0/UBO1/empty2/
      guard3 PASS1536words/1024guards,SyncVal0/0. No loader binding interception.
- [ ] Full Vulkan image/format/view/subresource adapter, execution-time integration.
- [ ] Correct null images/texel buffers/arrays/atomics and required robustness2.
- [x] Bounded R32 texel descriptor/view frontend through templates/binder/native
      materialization;actual R32_UINT fetch/load/store/null/OOB GPU gate2304words/
      1152guards,SyncVal0/0. Other formats/operations/full feature remain OPEN.
- [ ] Actual Vulkan device +D3D11CreateDevice +swapchain +correct first frame.
- [x] Separate image/sampler native gate:2048 components/256 guards,nearest and
      linear filtering,swapped samplers/null images;SyncVal0/0. Texel2304/1152 and
      image1792/384 regressions PASS. No full nullDescriptor advertising.
- [x] Pinned Sarek ARM64/glibc real consumer built/run. ICD,instance and basic
      Mali device PASS. Original D3D11CreateDevice rejects aggregate BC;opt-in
      exact14-format admission PASS,then actual nullDescriptor rejection.
      D3D11 device/frame NOT passed. Evidence/docs:SAREK_REAL_CONSUMER_2026-10-18.
- [x] Real loadable glibc Vulkan-layer tracking stage:common Core +buffer/set
      writes/copies/pool lifecycle,actual Sarek startup1write/1copy/gaps0/cleanup0.
      Explicit downstream Core dispatch fixes raw physical handle regression.
      Existing Android GPU guard regression2048/256,SyncVal0/0 PASS.
- [x] Real standard resource APIs7kinds/templates ->common frontend PASS,gaps0,
      full tracked cleanup. Template created before any actual set allocation.
- [x] Shared glibc guard compiler +explicit Vulkan13 target for actual Sarek
      SPIR-V1.6;default11/CPU14-9/Android GPU2048-256 SyncVal0/0 regressions PASS.
- [x] Standard shader/layout/graphics-pipeline hooks compile actual Sarek shader
      pair via common compiler,COMPILE-ONLY tracking. No guarded draw/frame claim.
- [x] Actual augmented layouts/instrumented modules +primary-command binding/
      materialized generations in SAME layer,with exact native layout ownership.
      Real Sarek shaders65536components/4submissions,direct+resubmission+indirect+
      bounds,572diagnosticwords,gaps0/cleanup0. Default49152 and Android2048/256
      SyncVal0/0 regressions PASS. SAREK_GUARDED_EXECUTION_2026-10-18.md.
- [ ] Null-write/template/copy ->typed native fallback resources ->guarded
      execution;broader image/format/operation and command admission. Public
      nullDescriptor still false. Do not repeat unchanged D3D11 gate until this
      actual blocker changes;Sarek D3D11 device/frame NOT passed.
- [x] Device-owned shared UBO/SSBO null backing,real leases/cache,guarded
      logical updates without native null forwarding. Existing multiset GPU
      1536words/1024guards,backing unchanged,Corezero/SyncVal0/0 PASS;Sarek shader
      65536/4 regression PASS. NULL_BUFFER_BACKING_2026-10-18.md.
- [ ] Typed image/texel null resources,initialization/synchronization and
      broader shader semantics;public feature admission remains CLOSED.
- [x] R32 typed null texel catalog +actual shader type reflection connected to
      guarded API layer. Existing R32_UINT GPU2304/1152,SyncVal0/0 PASS;numeric
      reflection6/4 PASS;Sarek shader65536/4 regression PASS. Signed/float GPU
      not proven. NULL_TEXEL_BACKING_2026-10-18.md. Next IMAGE null resources.
- [x] Common owned color-2D image fallback +explicit initialization leases;
      existing GPU1792/384 +backing zero readback,Corezero/SyncVal0/0 PASS.
      NULL_IMAGE_BACKING_2026-10-18.md. Fixed wrong-artifact build preflight.
- [x] Connect admitted color-2D reflection/common catalog to SAME loader layer
      with retained same-queue one-shot initialization prelude in Submit/Submit2.
      No hidden queue waits;preserve consumer waits/signals/fences/submit chains.
      Broader image semantics and public feature admission still OPEN.
- [x] Actual layer CbCr-null fault-injection Sarek shader readback65536/4 PASS;
      Submit1/2 prelude and final native/guarded regressions PASS. See
      NULL_IMAGE_LAYER_2026-10-18.md. NOT ordinary app feature/D3D11 admission.
- [ ] Remaining full nullDescriptor resource/operation/API contract,including
      broader image shapes/forms. Verify submit retry/failure and nontrivial
      waits/signals/pNext. Keep feature false until required semantics exist.
- [x] Primary-source contract audit distinguishes static descriptors from vertex
      fetch and conditional maintenance6 index semantics. Input attachments and
      null samplers are NOT newly allowed. NULL_DESCRIPTOR_CONTRACT_2026-10-18.md.
- [x] Actual Submit1/2 pre-driver OOM rejection/retry with same CB/fence PASS;
      reservations/commands retire,green pixels65536/4,Corezero. Normal guarded/
      native regressions PASS. This is NOT actual Mali failure/DEVICE_LOST proof.
- [ ] Null vertex fetch bindings/dynamic buffer descriptors and broader image/
      shader operations remain real admission gaps. Nontrivial wait/signal/pNext
      runtime proof OPEN. No more unchanged green-frame/feature-filter reruns.
- [x] Reuse proof: existing hybris compat layer executes compressed-image API
      upload+sampling for14 D3D BC identities on G925,896constant components.
      BC5 missing FILTER_LINEAR localized to native RG16 normalized backing.
- [x] Isolated compatible BC5 RG32F backing/output/bounds patch:actual bilinear
      signed/unsigned384components PASS;all14 format flags now satisfy fork's
      per-format admission. Native bytes/texel8vs4,performance/fullBC still OPEN.
- [ ] DroidDeck ABI/ICD integration and real D3D11 application, then small game.
- [ ] Measure CPU/GPU/frame time/overhead; compare PanVK only if available/same device.

Do not claim clip/cull removal in Sarek is emulation. Do not advertise unsupported
features. Native Core PASS != complete ICD/D3D11/device/game PASS. Prefer shared
implementations over a demonstration-only consumer. No new Windows boot for each
Core function. Stop deep DroidDeck investigation only on an evidenced serious
blocker after meaningful alternatives; retain logs/code and return to Windows.

## 2. Windows IddCx after D3D11 success or justified DroidDeck suspension

Reproducible signed-driver installation, startup/reconnect/recovery, integration
with existing H.264→MediaCodec,1920×1200,resolution changes,input latency,safe
fallback. Respect Code Integrity; experimental driver !=production readiness.
IddCx is a display driver, not a hardware WDDM GPU driver.

## 3. U-AVF Windows workspace product integration

UI/lifecycle/start/stop/reboot; supported persistence; keyboard/mouse/touch,
network/audio/clipboard/display; resource/disk/error handling. Preserve Ubuntu.
Then resume advanced Windows GPU/ICD/Bridge/WDDM research independently.

## Invariants

Stock Android,no root/unlock/flash/SELinux changes. Production Windows8CPU,
Ubuntu/P7,H.264,storage stay unchanged. No repeat of closed gfxstream/Venus
branches without new evidence. Tests/artifacts on healthy E; preserve C free
space and tablet ~50GiB. Avoid long cosmetic reports or synthetic polishing.
Current detailed evidence/blocker: STATE.md and compatibility docs.
