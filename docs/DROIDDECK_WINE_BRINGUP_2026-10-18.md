# DroidDeck Wine bring-up (not a game PASS)

## User-visible failure

Normal app install completed linuxfs-r9 and Proton ARM64 25502785. Three normal
Steam sessions exit before Steam. Exact archived03-steam/session.log ending:
Mali-G925 selected -> required VK_EXT_robustness2 absent -> Failed to create backend.
No Android crash. Pulse/DirectAudio starts. Gamescope3.16.29 requires the extension
and nullDescriptor unconditionally; actual source8f212644 inspected (E:/Mavk/research/
gamescope-3.16.29). Do not advertise the missing feature or merely remove this check:
its actual descriptor writes include null2D/YUV/LUT3D/storage targets.

Minimal alternative proof: existing Xwayland -shm rootful connects directly to
actual DroidDeck compositor. Fixed display99/TCP disabled; bounded diagnostic
only. Its lock hardlink failed under normal UID; existing proot fake-root facade
allowed -nolock. Kernel UID still ordinary app10261; no Android root or policy
change. Wine client runs separately as app UID, NOT fake-root. No Steam/game or
production session integration claimed for this alternative yet.

## Actual PE/Wine progress

Sarek ARM64X DLL build completed from existing diagnostic source and pinned
LLVM-mingw20261006. Three missing standard includes fixed for libc++:
dxbc-spirv/util/util_small_vector.h <new>, src/util/util_small_vector.h <new>,
src/util/config/config.cpp <algorithm>. No shader/feature semantic changes.
Build E:/Mavk/artifacts/sarek-pe-arm64x-build1-20261018; log resume3 terminal0.
d3d11 SHA256865b6f9c4b0fc0bc7cb374f9f962a3aae55bdd96edb1ad13bde66df71a0e3583;
dxgi SHA25605715b2a486286148ac6f31e97008e9477dffde65fae69d6e9e4a8b04a43c360.
Raw PE machineaa64, ARM64X metadata. No complete Wine/D3D11 PASS yet.

The shared window consumer has native SDL and Win32 compilation branches;
same actual D3D11 draw/shaders. Plain ARM64 exe is now used; initial ARM64X exe
was not a successful main-module proof. Runtime Wine11.0 --version PASS.

### Wine file-backed protection failure and scoped diagnostic adaptation

Unmodified Wine failed BEFORE Vulkan: mprotect(RX) errno13 on private RW
file-backed ntdll.dll text/ARM64X thunk pages, followed by stack overflow.
Exact maps/protection trace in wine-window5-trace.log. Same failure with both
old rootfs proot and current APK-owned proot (not explained by that parity).
No evidence of Android mount noexec; no SELinux/vendor trace inference claimed.

Wine's own virtual.c supports anonymous allocation/read fallback for private
image mapping (upstream reference downloaded, not asserted exact packaged commit).
Opt-in diagnostic helper wine-protection-trace.c implements this shape for
private RW MZ files ONLY in this test's bundled Wine/system32/syswow64/PE dirs.
No shared mapping, RWX allocation or policy modifications. Later original RX
mprotect still runs normally. No permanent integration/conformance claim:
one-time copy/memory overhead and lifecycle must be assessed before shipping.

Latest staged helper is libmavk-wine-protection-trace5.so, read-fallback argument.
It admits both virtual proot names and host paths and all bundled PE architectures,
plus Windows DLLs copied into the isolated prefix. Older helper3 missed i386 and
prefix DLL copies, so run7 is not a completed bootstrap proof.
Isolated prefix /root/.mavk-wine-poc (956MiB); do not touch user Steam prefixes.

Run8: real Win32 window created, Sarek PE DLLs loaded; Vulkan instance failed
before Mavk. Cause: experiment put runtime/lib's old/direct Vulkan frontend
ahead of real rootfs Vulkan loader. Fixed library order to payload:/usr/lib:runtime/lib,
as prior native successful pipeline and upstream build comment require.
Run11: real Vulkan instance through WSI -> Mavk -> hybris and Mali enumeration
PASS; native nullDescriptor still FALSE. Now vkCreateDevice fails -3 in layer
chain, before D3D11 device. Need localize layer stage, not change features blindly.

## Next / active tools

### 2026-10-19 actual visible Wine milestone

Layer28 completed and fixes actual device creation: S_OK/FL11_0. Earlier run13
1182 Present/20.049s was BLACK and not visible proof. Direct XGetImage during
subsequent actual draw confirmed mapped Wine window but entirely black X root.
Helper8 checked first4 actual xcb_shm_put_image requests: BadShmSeg128/major130
(run20). WSI previously discarded checked replies, hiding failed presentation.

Both controllers now use explicit same private BL_SYSV_DIR. This alone fails.
Upstream tools/linuxfs/preload/sysv.c fills IPC_STAT owner from getuid(); proot
fake-root server reports0 while ordinary Wine X client is10261. Server-only
xwayland-sysv-owner.c owner-test corrects this userspace-emulated metadata to
actual app UID (captured before proot); no kernel permissions/policy altered.
Run21 logs old0/new10261; PUT error0 for4checked requests, XGetImage root
nonzero964308components. Android wine21.png shows intended D3D11 texture.
Compositor 593 on-screen frames/10s (59.3/s), NOT independently unique visible
FPS. First native Wine PE visible milestone, still NOT Steam/game PASS.
Latest test helper8; helper7 crashed due RTLD_NEXT missing RTLD_LOCAL xcb symbol;
helper8 uses explicit library handles. Do not repeat helper7.

Next: preserve this result, proportional layer28 native/guard regressions, then
integrate optional Mavk user session preserving existing Steam/Proton/input/audio.
Normal Gamescope robustness2 blocker remains. Avoid globally advertising it or
turning diagnostic null admission into claimed complete feature support.

Layer27 stage diagnostics confirmed failure precedes Mavk createDevice. Exact
WSI source40cfec09 layer.cpp:497 helper defaults instance=NULL; create_device
line802 requests next vkCreateDevice through that NULL-instance helper.
Mavk's GIPA(NULL) omitted it, so WSI aborts before calling Mali. Layer28 exposes
only this static entry; its invocation still requires tracked physical/instance.
Build layer28 in progress (layer28-build.log). Stage with existing
window-stage-layer.sh, rerun actual Wine consumer. Stop bootstrap-only work as
soon as actual device/draw passes, then integrate opt-in normal DroidDeck session.

Controllers: xwayland-control.sh (bounded300s; start ONLY if no actual app-owned
Xwayland process), wine-window-control.sh (run/trace/read-fallback/stop,90s).
Stop uses selected prefix's ordinary wineserver -k. Do not infer running server
from socket file alone. Scripts currently fixed test package/display, not product UI.
Current compositor listening .wayland-rt; debug Activity attaches its Surface but
does not migrate singleton server to mavk-window/run. /proc/net/unix proves path.

Evidence all under E:/Mavk/artifacts/wine-arm64-inputs-20261018/. No Steam login,
game, FPS, input or audio end-to-end success claimed. Diagnostic Sarek admission
and FloatControls2 fallback still opt-in; full robustness2 remains unimplemented.
Production U-AVF/Windows/Ubuntu/P7/H264 unchanged. No commit/push/root/security changes.
