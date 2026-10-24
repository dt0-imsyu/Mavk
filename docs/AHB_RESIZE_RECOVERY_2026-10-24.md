# AHB resize recovery — 2026-10-24

## Scope and recovered state

Continue GLM's experimental DroidDeck work only. No U-AVF/VM/storage/firmware
changes or commit/push. The known-good APK and original WSI rollback remain.
Builds/source recovery stay on C:, not unreliable E:; existing toolchains on E:
are read-only inputs. C: free49.05GB at this checkpoint.

GLM's installed WSI was d878e181485fd0c50568060c64401f37dc155c812698190a166cbf7af97b3c35.
Its Wayland capabilities fixed min/max/currentExtent to the Android marker
2560x1600. This does not implement an application-selectable1920x1200 swapchain.
Source in recovery/2026-10-24/work-wsi, incremental build in build-local/build.

## Actual observations, not inherited assumptions

- Session24 Muck PID9314 was black and looping on VK_ERROR_FORMAT_NOT_SUPPORTED.
  X11 tree showed Muck's parent0x4800003 and child0x4600047 at2560x1600,
  not1x1. Therefore the earlier1x1 observation alone does not explain this run.
- Added exact source/display usage diagnostics. A BGRA44 swapchain requests
  usage0x8 (STORAGE); the common AHB importer correctly rejects storage usage.
  RGBA37 game swapchains request0x12 and import successfully.
- After one controlled restart, session25 Muck15360 visibly rendered; native
  compositor303/304 and311/311 GPU/on-screen frames with zero-copy over10s
  intervals. Approximately30–31buffers/s,2560x1600. Not pixel-unique FPS,
  cold peak performance or a measured optimization benefit.
- A later unclamped candidate87fc5ae8 imports RGBA2560x1600 and ALSO a BGRA1x1
  surface. The latter was wrongly given a fullscreen Wayland proxy. The screen
  became black despite continued producer/compositor activity. This suggests
  an auxiliary-surface/window ownership issue, not allocator req/got mismatch.

## Changes under test

1. BGRA conversion now imports the RGBA display AHB with TRANSFER_DST only.
   Consumer storage/color/sampling usage remains on the distinct BGRA source
   image. No storage feature is fabricated on an AHB and no CPU readback added.
   On Mali, source0x8/display0x2 imports succeeded, including1x1.
2. Removed GLM's marker-size capability clamp. Wayland extent stays variable;
   compositor output size is not the consumer swapchain size.
3. Candidate0e6cb2db966c46c132eaa2336d758d432d448d0f2d85ad685076e670516b770d
   queries the actual XCB window at surface creation and only creates the
   experimental fullscreen proxy for windows at least320x240; small windows
   retain the native X11 surface. Actual game test pending at this checkpoint.
   This is a bounded experimental eligibility gate, not complete multiwindow WSI.

## Isolation / rollback

Candidate and receipt live in app-private files/mavk-wsi-resize. The reversible
ahb-resize-profile.py changes only two Muck environment keys in both existing
config files, preserving every other preference. `restore` checks concurrent
edits. No mapped library is overwritten in place; candidate updates use rename
after terminating only the observed game process.

The initial one-line usage diagnostic e25e9ba8 was staged in GLM's already
experimental bundled presentation directory with matching receipt. d878e181
library rollback and original99a12d70 rollback are retained there. Main APK is
unchanged. A later session must restore/repackage verified bytes before shipping.

Two minimal control-launch mistakes were isolated: stale nativeLibraryDir was
resolved through pm path; /data/local/tmp isn't bound into the container, so the
small profile script is staged in app-private storage. Direct Steam CLI from a
separate minimal environment did not attach reliably and was abandoned; the
ordinary app session was cold-started instead. Do not repeat that CLI experiment.

## Next acceptance gate

Inspect the same new launch, verify candidate hash in actual maps, actual XCB
window dimensions and AHB requests. Change resolution through ordinary game
settings; require1920x1200 AHB buffers and matching zero-copy/on-screen counters.
Do not classify a SHM/GPU-composited fallback rate as direct-layer AHB performance.
No60FPS claim. Preserve the baseline if the gate fails.

## Subsequent hardware result

Candidate0e6cb2db was verified in actual Muck process20059 maps/environment.
Ordinary Alt+Enter switched the actual Wine/X11 client and AHB allocation to
1920x1200, then2560x1600, then1920x1200; image remained visible and native
copy0/zero-copy counters stayed matched. No SHM performance substitution.
The resolution text alone was not evidence: its Apply retained the fullscreen
desktop-sized client; Alt+Enter caused the observed real ResizeBuffers.

SurfaceFlinger layer3990 buffer-delta measurements:

| Resolution / scene | Buffers / seconds | Rate |
|---|---|---|
|1920x1200 settings|629 /20.2707392|31.03/s|
|1920x1200 main menu|601 /20.2717242|29.65/s|
|2560x1600 main menu|606 /20.2398783|29.94/s|

The fair menu pair shows no material resolution benefit. It does not prove a
specific CPU/GPU bottleneck. WSI/compositor release was approximately15ms and
scene CPU~0.8–0.9ms, but release latency is not necessarily a blocking CPU wait.
Do not infer root cause just by adding these numbers. No60FPS or speedup claim.

Absolute pointer mapping is still incomplete after the render proxy scales a
1920x1200 X11 client to2560x1600. Original-client coordinates successfully clicked
Back (961,205), while display coordinates differ. Relative keyboard Alt+Enter
works. This is an explicit next product limitation, not complete input PASS.

Final candidate d0ea715acae6898287d953592097f81d9a7fba9d55a1c48e34f5ae90752fbc25
keeps the eligibility query/log strictly inside the opt-in branch; default X11/
Adreno no extra roundtrip. Portable eligibility patch reverse-check PASS.
Actual Mali RGBA image/view/acquire/fence regression PASS with CoreZero/gaps0.
The first draw2 binary ignored the BGRA selector (format37), so it was NOT a
BGRA test. Reused the existing consumer-bgra binary55648123, whose format44
selector was verified: source usage0x13/display0x2, three1920x1200 imports,
image views/acquire fence PASS, CoreZero/gaps0. Neither allocation probe drew
pixels or measured presentation.

Session26 subsequently ended with an explicit app clean-exit request; no OOM or
GPU-loss cause established. Do not call that a WSI crash. Android remained Awake/
StayOn. Restarted the normal app session to verify the final candidate.

Bundled WSI+receipt have been restored to original99a12d70; only the separate
private Muck candidate remains selected. No main APK rebuild or baseline swap.
TimeStats disabled after the fixed samples. New work must preserve this
separation and the reversible profile backup.

## Final candidate game recheck

Normal session27, actual Muck26064: mapped private candidate d0ea715a; exact
SHA256 checked. XCB client0x4600030 initially1920x1200/proxy1. Standard DXGI
created1920x1200 ->2560x1600 on startup; Alt+Enter returned1920x1200. All four
RGBA images imported successfully each time; screenshot final1200.png viewed,
moving real menu visible. No FORMAT_NOT_SUPPORTED retry loop in the current
Muck D3D11 log. Native10s sample:349GPU =349zero-copy =349on-screen,copy0,
pooldrops0,scene0.79ms mean,release15.19ms mean. This is34.9buffers/s, not a
pixel-unique content proof or comparable speedup against earlier thermal state.

The main acceptance gate (real1200p AHB game frames after resize) now passes.
The general goal (fast universal backend /60FPS real games / normal scaled input)
remains incomplete. No clean-launch claim for every fullscreen mode; Unity still
selects desktop-sized fullscreen on startup before the ordinary mode toggle.
Tiny-window fallback is an experimental policy, not a demonstrated fix for every
prior black-screen cause. Preserve this qualification.

Next high-value work: correct scaled-proxy absolute input, then one bounded
frame-pipeline profile of the actual game. The same-menu resolution pair gives
no reason to chase texture resolution, more allocator variants or codec tuning.
Do not rerun separate Steam CLI or expand Vulkan feature tests for this blocker.
