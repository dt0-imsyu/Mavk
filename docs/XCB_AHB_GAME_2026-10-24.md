# Wine/XCB to AHB: first real Muck frames, 2026-10-24

The preserved GLM APK29/Sarek visible result was inspected. Its unfinished
wine_ahb_surface.cpp was not in the build and had no XCB entrypoint wiring.
Implemented a private, explicitly selected XCB-to-Wayland presentation proxy in
the existing WSI; it reuses the shared AHB image/fence implementation, not a new
renderer. One proxy surface at a time; owned connection/window outlives every
WSI proxy. Normal X11 surface creation remains unchanged without the opt-in.

## Actual hardware results

Artifacts: E:/Mavk/artifacts/xcb-ahb-20261024/.

- Native DXVK-Sarek via SDL3 **X11/XCB**, not SDL Wayland: actual 2560x1600,
  1238 Present calls / 11.004 s; warm sample 113.464 calls/s. This is not a
  unique-visible-FPS claim. apk30-a/b.png show the expected moving scissored
  patterned image. Native compositor: 978 zero-copy frames in a mixed 10 s window,
  no layer copies or pool drops. CoreZero, gaps=0.
- **Actual Muck.exe through Wine/Proton/Sarek** opens the proxy and three AHBs
  at 2560x1600. Real moving menu is visible (muck-ahb-a.png, muck-aftertap.png).
  Full 10 s windows: 189 and 186 AHB frames, no copy/layer copy/pool drops.
  Independent Surface timestamp sample: 59 records / 3.125 s approximately,
  **18.56 buffer presentations/s**. Not a matched performance A/B, not gameplay
  FPS, latency or proof of unique content every frame. No gain claim yet.
- Swapchain/surface recreation occurs twice during Muck startup, returns to
  visible rendering. Original game input was intercepted by the proxy; Settings
  did not open on the first tap. This is a separate product gate, not a game PASS.
- Without v3, XCB still creates D3D11 FL11_0, then fails before presentation with
  VK_ERROR_FEATURE_NOT_PRESENT. no-v3-run.log, clean CoreZero.

The earlier Steam startup exit139 happened while APK29 **still advertised v2**:
the session marker generalization in source had not been included in APK29.
Do not attribute that one-off crash to v3/AHB. The next ordinary launch worked.
Saved steam-v3-exit139.tar has the actual v2 log despite its historical filename.

## Integration and isolation

APK30 enables app-owned session marker as well as debug-session marker. It
advertises v3 only when opted in. In marker mode, v1/v2 consumers are NOT told to
switch graphics paths; only v3 receives the enabled mode. This preserves Steam's
existing X11/SHM selection. Also tightened viewport identity to exact full-buffer
source; nonidentity crops no longer pass by rounding/oversize comparisons.

The game-launch script has a hash-pinned per-game private WSI selector. Only
Muck's two explicit environment keys select it. Installed common runtime, original
system WSI, Steam UI and other games are not replaced. xcb-ahb-game-profile.py
saves/restores the exact launch script plus just those two game keys, rejecting
concurrent edits. First enable/restore verified on device.

Tests: Linux SystemGraphicsLaunchTest 16/16 PASS, including absent marker,
incorrect hash, ordinary unchanged launch and private precedence. Whole suite on
Windows failed because of Linux-only paths/fcntl/Bash and is NOT a regression PASS.
Native AHB explicit synchronization mock PASS on the tablet, including new
v2/v3 mode-isolation assertions. Mock is not a Surface correctness test.

APK30 SHA256: 4313533b8f38dc5ba53cff8faad5ac167bcc7ba41d3308c690636f8d9cd56a03.
Library: 1c81d19f7783218eabbf1684c3b8de8d33c6e9d6e304a6803ad7303efb54d72e.
WSI rendering-only: 756dd6497bcbe3ea6d4587a5ab9938e9c29dbaeea26a6f150842fbe6184685c4.
Both APK30 and APK31 change only libdroiddeckwayland.so, with 486 other members
byte-identical. APK29/28 rollback retained. No production U-AVF/VM changes.

## Current input work / incomplete gates

APK31 implements the standard explicit-empty wl_surface input-region case on
commit and skips those windows in pointer/touch hit testing. Keyboard fallback
skips clients without a keyboard binding. WSI sets an empty region on its proxy
so the original Xwayland/Wine window can retain actual input. This is not full
arbitrary input-region algebra, multiwindow integration or a Wine Wayland driver.

APK31 native hash: 32e4c614da9f19de9e5cdc65bb276e21019606d1392a4c7d28da1c16169473aa.
WSI with empty input region: f1b9370ec973f4bb7ba545aaaeb1ed8dcb7ebe8c8280a308131090d02e2c494a.
APK31 SHA256: 01b916a4c8ecac9fc2d29824b59b2268037b2a6e89590b06d8799c96e1b2b85d.

Native D3D11 XCB consumer with input observation actually received
`WINDOW_INPUT type=mouse-button` through the AHB proxy. Native compositor logs
the touch recipient as **Xwayland**, not the presentation proxy. Rendering
continued (1091 AHB frames in a mixed ten-second window), gaps=0/CoreZero.
Artifacts: E:/Mavk/artifacts/xcb-ahb-input-20261024/input-run.log,
input-frame.png and input-session.tar. A sent A key produced no consumer key
event, so keyboard routing/focus is **not PASS**. No claim of full input support.

Muck could not be retested on APK31: Steam's actual dialog says Rust is running
on another computer and continuing would disconnect that Steam session. Did NOT
approve it; cancelled, restored the exact original game-launch script and both
Muck environment keys, stopped the test session and removed only our session
v3 marker. APK31 remains installed as experimental; normal v2 is the next default
launch. Stay-on while powered remains enabled. No Steam account/logout changes.

Next: isolate keyboard focus with the independent XCB consumer, then retest
Muck input when no competing Steam game is active. After input: matched
SHM/AHB game A/B and presentation lifecycle before default
enablement. FIFO/opaque/RGBA8/one queue/one proxy subset remains experimental.
No new Vulkan feature bits, CTS or upstream DXVK support claims. No commit/push.
