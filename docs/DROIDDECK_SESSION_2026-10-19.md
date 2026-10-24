# Normal DroidDeck session integration

## Proven before integration

Wine run21 renders the actual PE D3D11 moving texture on the Android Surface.
1181 Present returns/20.025s; compositor593 on-screen frames/full10s. Unique
visible FPS is unmeasured. Four checked X11 PUT requests return error0 after
server-only SysV metadata correction (old emulated owner0 -> actual10261).
No root/security-policy changes. Ordinary Wine remains ordinary app UID.

Layer28 regression run40: real guarded readback65536 components,4submissions,
indirect/resubmit/init PASS, gaps0; normal Sarek admission rejects incomplete
nullDescriptor as intended (exit20). Run41 explicit diagnostic admission: real
D3D11 clear/draw/texture/nearest/bilinear readbacks PASS, all owned cleanup/Core0.
No full nullDescriptor or game compatibility claim.

## Integration

Fork E:/Mavk/work/DroidDeck-mavk retains upstream SessionActivity/SessionService,
audio, input, session logs, Steam/Proton installers and Adreno settings.
New MavkExperimental.kt is gated by package suffix.mavk, stock Mali/default
driver, mode Steam/run, and private developer marker mavk-window/enable-session.
It starts an app-owned SessionPart Xwayland helper via existing LinuxRuntime
binds/proot. Root facade is only for Xwayland -nolock; kernel UID stays app UID.
Server-only helper corrects its emulated SysV ownership, shared private IPC dir.
Private random MIT-MAGIC-COOKIE authority, TCP disabled. This is NOT Android root.
Suspension participates in existing helper-root lifecycle; stop uses Process API.

Mavk session currently tracking/pass-through (guards0), no Sarek diagnostic flags
globally enabled for Steam. Stock DXVK game payload is NOT yet replaced by Sarek.
Game-layer/Sarek package integration remains required; do not call this done.
Current assets/libs are staged in private test storage, not yet self-contained
end-user install. Final target remains ordinary selectable backend +Steam/game.

Build3 installed41ba2c3234efd2c6d05c54ac479558eca2918bd28788200763eadf4af8e716d5.
Runtime source/output/APK verification PASS. Ordinary MainActivity Start Steam
button starts real session2026-10-19-01-steam, app-owned Xwayland1280x720 and
PulseAudio/DirectAudio; passed old Gamescope missing-robustness2 boundary.
Real droiddeck-steam-install downloads official17part ARM64 client; not yet
Steam login/UI PASS. Keep active download, do not restart just to collect logs.

Build3 BL_INSIDE=1 skipped outer script log redirection. Fixed source/build4:
DROIDDECK_MAVK_DIRECT flag enters inner mode AFTER normal log setup. Build4
SUCCESS, not installed while initial Steam download is live. Final APK runtime
inspection PASS; SHA2387e72e5453e38653a7d79b587291cfb0116ef29ddd34d06b4db8abd7400fa7.
Install pending. No need repeat build.

## Next

### Latest actual state 07:51, supersedes initial download checkpoint

Client finished17parts then a669522KiB native self-update. First session dies
in Zink requiring nullDescriptor, not transport. Build5 CEF-software alone
still fails main Steam VGUI glXChooseVisual. Build6 scopes llvmpipe/softwareGL
to main Steam+its CEF via command-local env; actual game wrapper removes this
UI-only marker/overrides, keeps Mali Vulkan. Build6 installed37bef6b484e6a078839f7f381bee574015e4653a3113ecf33096006f59967909.
Normal session03 actual Steam Big Picture login visible screenshotmavk-steam3.png.
Fonts now present (normal font/locale preparation retained before direct route).
Audio client connects Pulse/DirectAudio (not acoustic latency proof). Login
awaits user asynchronous request, user away; independent work continues.

Normal input initially no text. Diagnostic XSetInputFocus to actual mapped
Steam window0x240002f fixes native Wayland +Android injected text, screenshot
mavk-steam-focus.png; test text then cleared. Need automatic X11 focus policy.
Agent own raw X11 handshake omitted Xauthority auth, causing focus snapshots to
fail. Correct bounded cookie parsing/source handshake added, not yet built/tested.
Lifecycle regression: after session02 exit only Android app PID remains; helper
stop now uses existing SessionService tree teardown callback, no Xwayland leak.

Proton sidecar at Mavk/adapters/droiddeck reuses stock selector PE validators,
patches in-memory Proton copy expressions (not installed Proton source), overrides
only D3D10core/11/DXGI ARM64X. Preserves stock32 prefix files/other DLLs; wrong64
architecture fails. Actual installed script/3 realDLLs preflight/routing PASS,
artifact wine-arm64-inputs-20261018/proton-routing-check.log. NOT game runtimePASS.
Original upstream tools/hybris/run-proton.py restored unchanged; separate sidecar
keeps system archive provenance. Game-env receives sidecar/debug diagnostic flags
only for explicit Mavk game profile; fullfeaturebit stillfalse.

Openbox is a standard candidate, NOT deployed: own private prefix package
download/integrity verified using stock Arch Linux ARM public keyring/private
GPG dir; no system keyring or Linux rootfs libraries upgraded. Missing2runtime
libraries (startup-notification/Imlib2); stale imlib2 DB file404. Do not install
50dependency transaction into assembledrootfs. Refresh PRIVATE package index,
download exact dependencies into same private prefix, inspect ldd before launch.
Private archive symlink metadata errors resolved by excluding4knownaliases and
recreating valid private links after extraction (not chmod of symlink target).

Payload self-contained APK packaging +ordinary UI opt-in still TODO. Sidecars
currently staged private test files. No app-game/Steam-login/visible-FPS success
claim, no U-AVF changes/commit/push. Current UI stays running, no need reboot.

Observe current own-UID curl/droiddeck-steam-install until terminal; inspect
actual native Steam/UI. Do not equate Xwayland first black frame with Steam ready.
After download, install build4 for normal logs if needed, restart only with a
specific diagnosis. Then fix concrete Steam graphics/API blocker, integrate
experimental Sarek via existing Proton per-game wrapper, get actual game frame.
Never replace ordinary Adreno/DXVK or touch U-AVF Windows/Ubuntu/P7/H264.
No commit/push. Evidence E:/Mavk/artifacts/droiddeck-ci-inputs-20261018/;
layer/Wine evidence in wine-arm64-inputs-20261018 and sarek-consumer-run40/41-20261019.
