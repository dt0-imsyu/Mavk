# Developer demo preparation — 2026-10-25

Current objective: reproducible DroidDeck/Mavk demo, local commits only, no push.
Steam is reserved by the user: no game/Steam launches until that restriction is
lifted. Do not replace gameplay testing with a synthetic FPS claim.

## Repositories and source boundaries

- Mavk: C:/Users/denis/MainProjects/win11ontab/Mavk.
- Separate DroidDeck fork: C:/Users/denis/MainProjects/DroidDeck-Mavk,
  branch mavk-demo, base0916a538aff4c08f799a4bd9fa17dd3c98b01e99.
  `upstream` points at Droid-Deck/DroidDeck; no publishing remote configured.
- Imported existing fork edits and the verified normal-session sources from C:
  recovery. Runtime assets brought to matching9e2a00b1 version. Binary runtime
  stays ignored until corresponding-source distribution is checked.
- WSI source now under third_party/wsi-layer,144 original files match pinned
  ginkage40cfec09 base;16 changed plus two proxy sources. MIT notices preserved.
  Explicit protocol XML included; private E: paths removed in that snapshot.
- Android AHB buffer/blit interfaces stay adapters/android. X11/Wine pointer
  logic stays adapters/droiddeck; no X11 dependency added to Core or Bridge.

## New pointer candidate

2730120fdeec44da17fc7b441b0fe01d2b0dc9aef6f36376d98d0a978a747e03.
Own X connection, XTest loaded dynamically, configure/reparent tracking; it does
not consume Wine/SDL's event queue. Wayland proxy receives pointer coordinates
and maps configured output space to actual X11 client bounds. Keyboard stays
on the existing route. If XTest isn't available, old passthrough remains.
Pointer/seat objects are destroyed before their Wayland display; held buttons
are released during cleanup. Default Adreno/X11 does not instantiate this adapter.

Actual stock Mali, ordinary MODE_RUN/native D3D11 session, Steam not started:

- Client rect320,208,1920,1200 on2560x1600 output; pointerReady1.
- Inject display1280,200 -> actual X server1280,358, exactly expected affine
  mapping. XQueryPointer mask0 afterward; SDL received mouse-button and key.
- Native D3D11/AHB stream completes, image import successful, CoreZero/gaps0.
- APP_AHB_SELECTION_PASS and NORMAL_SESSION_D3D11_PASS.
- Initial shader-header build warnings fixed. A first direct fixture found no
  running Xwayland; that exact blocked test process was stopped, not restarted
  because of an observation timeout. Normal MODE_RUN then supplied its own Xwayland.
- No game FPS or general-input conformance claim from these checks. Wheel,
  captured relative input, other resolutions and game integration remain gates.

The original private fixture run.sh/consumer.py and prior result receipts are
preserved with `.before-pointer` suffixes. Restore after candidate integration.
No production U-AVF, VM, firmware, host policy, account or user game files changed.

## Remaining demo gates

1. Package the verified bridge into a fresh manifest-checked runtime and APK,
   compile the separate C: fork, check a clean launch and SHM rollback.
2. Finish pointer regressions at another point/size and real game input.
3. Profile actual Muck CPU/FEX/Wine/DXVK/GPU/pacing; its exact30-ish bottleneck
   is still unresolved. Historical1200/1600 same-menu rates were29.65/29.94.
4. When Steam is free: check installed HL2 availability and one heavier game,
   use bounded gameplay runs, record frametimes/route/CPU/GPU, select demo.
5. Review source/license distribution for bundled GPL/LGPL payload dependencies,
   then local commits. Do not publish or call all release criteria complete yet.

## Packaged checkpoint (supersedes initial pending packaging above)

Portable vendored WSI build passed all67 steps with warnings-as-errors:
`b1bad07fbbf82b3a013b7d8b441f7dd4e54b81914a249aa3d325628668647e79`.
Same functional source as the initial pointer candidate; source/build paths changed.
Runtime33c0cfe705bfb9090099ca7ea1af45f703acc44d32497ca4227e9ce79fe6aa4d
changes exactly the WSI library, its receipt and manifest;267 members unchanged.

Second input point revealed a real compositor routing defect: the X11 desktop
received pointer coordinates outside the smaller1920x1200 source buffer even
though the AHB layer visually covered2560x1600. Choose the visible fullscreen AHB
candidate before the underlying desktop, while respecting an empty input region.
Keyboard/default native X11 selection is unchanged. Native source recompiled and
relinked; compositor06dfcf7778909bc2550de8d0d1e8f23b1674a518982995a24e0d6c7a8436d87b.

Actual before/after at display2000,1200:

- Before: XQueryPointer2000,1200 (wrong, passed to underlying desktop).
- After:1820,1108 =320+2000*1920/2560,208+1200*1200/1600; mask0.
- D3D11 consumer received button/key; completion CoreLiveZero/gaps0.
- App instrumentation passed real runtime selection, AHB↔SHM reversal and invalid
  geometry fallback. This is policy fallback verification, not a fresh full SHM game.
- Final native compositor interval:1197 GPU=1197zero-copy=1197on-screen/10s,
  copy0,pooldrops0. It is a tiny moving native consumer, NOT real game FPS.

Final local installed experimental APK:
`recovery/2026-10-25/DroidDeck-Mavk-demo2.apk`, SHA256
`360c47655ba1acff64eae9ef68d69fb0c4f9c49376fcae21f9ef939103f36fdb`.
Previous c56d APK and first9044 candidate remain available for rollback.
This is an integrity-checked asset/native rollup of the known-good APK, NOT a
clean full source APK build. Full Android Kotlin compilation of the separate C:
fork passed with Rust skipped; final changed native C compiled separately.
Full Rust/runtime dependency preparation remains necessary for a clean release build.

Packaging regressions17PASS; Linux launch-environment81PASS and controller30PASS.
Initial Windows-native execution of Linux-only tests failed (fcntl/shebang/text
translation); corrected invocation uses WSL, not product workarounds. Windows
checkout LF rules retained/extended. Updated wrapper assertion matches the quoted
optional launcher with stock fallback rather than its old hard-coded path.

Muck-only d0ea override restored using its guarded backup; next ordinary game launch
selects the packaged runtime. Private run.sh/consumer.py fixture restored to its
pre-pointer contents; test logs retained locally. No Steam launched. Main library
manifest inventory contains no app220; other libraries were not exhaustively searched.
HL2/heavier gameplay, current Muck FPS attribution and captured relative input remain
unverified. Do not replace these gates with the native consumer's Present counts.

All outputs on C:. External E: caches were read only. No U-AVF/VM/firmware changes.

Additional reproducibility check: the entire droiddeckwayland native target built
from the separate C: fork in79 steps, not just a cached single-object relink.
FrameLoopTest4/4 lifecycle cases passed. The installed demo2 remains the already
device-verified pinned rollup; no clean full-release APK claim is made.
