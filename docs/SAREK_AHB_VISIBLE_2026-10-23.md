# D3D11/DXVK visibly through the AHB zero-copy layer — 2026-10-23

## Result

A real D3D11 consumer (DXVK-Sarek 3.0.2, native ARM64 glibc, SDL3 wayland window)
now renders **visibly on the device display** through the standard Vulkan
AHB-backed swapchain with **zero-copy presentation on the DroidDeck
SurfaceControl layer**: 219 of 220 frames in the 10 s statistics window went to
`droiddeck_wayland_game` without a copy, `gaps=0`, clean teardown, no CPU
readback of rendered frames. Screenshots taken during the run differ by
62832/256000 sampled pixels (24.5%) with X movement range 36..1648 on a 2560
wide panel: the scissored textured triangle actually moves on screen.

This closes the gap left by the previous session (sarek-ahb-window-20261023:
60 Present/s but a black screen and `visible_fps=UNMEASURED`).

## Root cause of the black screen

SDL3 always installs a `wp_viewport` on its wayland surface at window
creation:

```
-> wp_viewporter.get_viewport(wp_viewport, wl_surface)
-> wp_viewport.set_source(-1, -1, -1, -1)      # unset = whole buffer
-> wp_viewport.set_destination(1920, 1200)     # identity here, but sets dst_set
```

The compositor's `ahb_layer_only_candidate` (waylandcomp/src/compositor.c)
rejected any window with `src_set || dst_set`, even when the viewport is a
no-op (destination equals the buffer). The AHB swapchain was created and
presents flowed (`copy 540, zero-copy 0`), but every frame fell back to the
GPU composition path, which cannot import these gralloc buffers, so the window
rendered as a black rectangle at 60 fps. The non-SDL WSI consumer (`draw2`,
which calls `xdg_toplevel_set_fullscreen` and never creates a viewport) worked
in the same session — that asymmetry located the fault.

## Fix

`ahb_layer_only_candidate` in
`E:/Mavk/work/DroidDeck-mavk/app/src/main/cpp/waylandcomp/src/compositor.c`
now admits a window whose viewport is identity: source unset (negative) or
covering the whole buffer, and destination unset or equal to the buffer size.
Non-identity viewports (real rescaling) are still excluded from the display
layer.

Build: ninja target `droiddeckwayland` in the existing Gradle cxx tree
(`app/.cxx/Debug/5a1m492x/arm64-v8a`, NDK 28.2.13676358), then
`experiments/droiddeck/package-native.py` replaced
`lib/arm64-v8a/libdroiddeckwayland.so` inside the pinned installed baseline
APK (`DroidDeck-Mavk-AHB-full1200.apk`, sha256 7430fd64…), zipalign `-P 16 4`,
apksigner testkey. Installed in place as APK29
`E:/Mavk/artifacts/droiddeck-viewport-apk29-20261023/DroidDeck-Mavk-AHB-viewportfix.apk`,
sha256 `d63e36e26d0cfb7d38ee226285389ff1fcec59929413f21e508078f5d887ab44`,
486 other APK members byte-identical. New library sha256
`2fe2d1a7fef6c0ef5c607cbc1c891078556ff60c58621e915c758cb66d313e77`.
Rollback: full1200 APK28 in `E:/Mavk/artifacts/droiddeck-ahb-apk28-20261023/`.

## Evidence

Artifacts: `E:/Mavk/artifacts/sarek-ahb-window2-20261023/`

| Item | Result |
|---|---|
| sarek DXVK run, APK29, WSI 3b705bef | 662 presents / 11.003 s = 60.2/s, gaps=0, `viewportfix-run.log` |
| Compositor stats during run | `220 on screen (copy 1, zero-copy 219)`, `layer shown`, `presenting … without a copy` |
| Visible movement vp-1 vs vp-3 | 62832/256000 sampled pixels changed, X 36..1648 |
| draw2 regression after fix | `WSI_AHB_CONSUMER_PASS`, `zero-copy: presenting … without a copy` |
| Black-screen reproduction before fix (same session) | `copy 540, zero-copy 0`, black screenshots, vis-*.png |

Compositor log lines are in logcat (pid of com.droiddeck.launcher.mavk,
tag DroidDeckWayland); copies in `logcat-app.txt` and the run logs above.
The WSI layer and installed common runtime hashes were not changed
(WSI still 3b705bef, runtime 7549290e, system 8a17ef88).

## Scope and limits

- One fullscreen window on the display layer, FIFO, 1920x1200 RGBA8, 3 images;
  same private v3 subset as before (`mavk-explicit-ahb.enable` marker carries
  the 1920x1200 size).
- The moving triangle is a diagnostic workload; it is not a game FPS, latency
  or input result. `visible_fps` remains unmeasured as a frame-uniqueness
  metric; the screenshot delta above is the visible-motion proof.
- Steam/X11 SHM path was NOT re-run after the APK29 install (change only
  touches the AHB layer-only candidate selection; SHM windows do not enter
  that path). Normal default-v2 Steam/Muck regression still pending — run it
  before any further runtime change.
- v3 marker `files/mavk-window/run/mavk-explicit-ahb.enable` remains present
  from the previous session; remove it to return the main app to v2-only.
- No commit/push; DroidDeck-mavk worktree carries both the previous v3 work
  and this fix as uncommitted changes.

## Next steps

1. Steam/Muck visible regression on APK29 (normal X11 path) to confirm the
   production path is untouched.
2. Repeat the sarek window with an animated/rotating D3D11 workload and larger
   duration; measure Surface updates/s like earlier checkpoints.
3. Point the experimental per-game config at this WSI for a real Wine/FEX
   D3D11 title (Proton lacks a wayland wine driver; SDL3/DXVK-native route is
   the proven substitute so far).
4. Then: performance A/B (old X11 readback WSI vs AHB zero-copy) under
   matched thermal state.
