# Muck GPU attribution and recurring diagonal artifacts

## Authoritative current state

Steam/game use is authorized again. Outputs remain on C:, not the unreliable E:.
Installed experimental APK is `recovery/2026-10-25/DroidDeck-Mavk-notices-demo.apk`,
SHA256 `443c300b99ca671204c5c2e8c58ff53b7eefa7193584a0e4574e704095a7ac88`.
Runtime `3bd128fb11cb69944af02650030507211d30317dd19c86266b6e4c73d0f87510`
adds license notices/provenance to the previous runtime; executable bytes did not
change. Corresponding-source completeness remains an open publication gate.
Normal-session SHM fallback was visibly checked with the existing native D3D11
consumer: 598 copied frames/10s, zero-copy0, pool drops0. This is not game FPS.

## Performance evidence

- Animated menu: 680 SurfaceFlinger buffers/20.1s, 33.8/s. Game CPU108.85% of
  one core; busiest dxvk-cs thread43.36%. Not saturation of all eight cores.
- Heavy gameplay: game CPU123.06%, dxvk-cs46.88%, queue29.43%; substantial waits.
- Same-view gameplay A/B: actual2560x1600 AHB imports vs actual1920x1200 after
  normal mode toggle both measured92 displayed buffers/10.1s, 9.1/s. Lowering
  resolution alone did not improve that scene. Saved dimensions are not evidence
  of actual swapchain dimensions.
- Existing bounded GPU query probe produced64 valid samples: two-command-buffer
  batches mean24.867ms (21.858–29.073); three-command-buffer batches55.729ms
  (52.903–61.970). These timestamp spans include synchronization and must not be
  added together or called shader-only execution time/full-frame GPU time.
- Mavk recording attribution: roughly16–18k guard calls/s,303–330ms/s guard
  wall time; native submits7–10ms/s. Semaphore waits overlap other work and are
  not additive. Compositor scenes ~0.6ms, no CPU copy/pool drops in AHB mode.
- User sees sky/floor~45FPS versus forward~8FPS. This is workload sensitivity,
  not proof of a fixed30FPS limiter. Saved settings144FPS/VSync0 already audited.

## CSE experiment and artifacts: do not misclassify

1. Bundled f108 binary does not contain `MAVK_GUARD_CSE`. The first environment
   flag attempt left metadataReadOnly0 and was a NO-OP, not a CSE performance A/B.
2. That GPU-probe run logged Mali `GROUP_ERROR_FATAL` and visible black tiles.
   GPU timestamp probing was disabled. Earlier artifacts predated this probe,
   so the probe is not established as the sole cause.
3. Selected the existing private CSE candidate, SHA256
   `4b16fa3f680f4c5090e48503a1b4d11038e388c9167c019287976784ab22a109`,
   using the reversible Muck-only profile. Actual metadataReadOnly1 confirmed
   selection; GPU probe remained off. Candidate had prior own-GPU regressions,
   not a proven game performance win or CTS conformance.
4. User again reported the same upper-right→lower-left black artifacts. Current
   captured menu frame was clean and candidate log had no `GROUP_ERROR_FATAL`.
   This disproves treating absence of that error as proof of visual correctness.
5. Restored both game configuration copies through the existing checked backup;
   quit/relaunched Muck. Normal metadataReadOnly0 confirmed; new log errors0.
   Three screenshots captured, first/third inspected clean. This is a rollback,
   NOT a confirmed permanent artifact fix or exhaustive stability test.
6. Later normal menu measured382buffers/20.1s,19/s. Android thermalservice reported
   Thermal Status3, SKIN45C and AP~51C. The earlier33.8/s and candidate26FPS HUD
   cannot establish a fair optimization regression/speedup across thermal states.
7. Quit Muck for cooldown, leave Steam/session open. Diagnostic profile is restored
   and inactive. No APK replacement, feature-bit change or guard disabling.
8. After brief cooldown status fell3→1. Relaunched normal renderer: first20s
   included startup and are not a gameplay/menu sample. Once menu was visibly
   loaded, next10.1s measured340buffers,33.6/s (earlier normal33.8/s). New captured
   menu frame clean, Mali errors0; thermal status rose2 again during startup.
   Quit Muck afterward, leaving Steam open. This confirms return to the measured
   normal menu rate, not a permanent artifact cure or60FPS gameplay.

Artifacts are ignored/local in `artifacts-local/demo-20261025/`: control GPU log,
batch summary, real1600p/1200p screenshots/pacing, `cse-artifacts.log`,
`cse-artifacts-current.png`, `restored-frame1/2/3.png`, thermal snapshot.
Some images contain a Steam nickname; do not publish them without review.

## Follow-up: second game and source APK checkpoint

- The sole Steam library has no HL2 installation. DDNet native launch currently
  fails in Valve FEX before graphics. Existing DroidDeck FEX selection reproduced
  that failure; a two-line child-environment fix advances past the socket failure.
  Sniper x86-64 `true` then reproduces the new `sbrk() failure while processing
  tunables` / exit127, proving a non-Vulkan runtime boundary. See the fork's
  `docs/MAVK_NATIVE_FEX_2026-10-25.md`. Not a current DDNet/gameplay PASS.
- Minimized Steam main/friends windows through existing client API: actual15s
  helper CPU1.4%+0.47%+0.07%, Muck119.15% of one core. Menu34.9buffers/s, actual
  AHB350–351GPU/zero-copy/on-screen per10s, copy0/pooldrops0. Lower background
  work is proven; a large game FPS gain is not. No auto-minimize app change.
- Normal Show Grass setting was unchecked through the game's UI before creating
  seed12345. New lake-view gameplay, visually no grass:132buffers/10.1s=13.0/s.
  Restored the checkbox in-game; same world still had no grass and137/10.1s=13.5/s.
  Do NOT present this as a valid grass-on/off geometry A/B: restoration did not
  visibly rebuild this world's grass. It only shows this reduced scene still
  falls far below60. Menu background grass was unaffected by the setting.
- New source APK `recovery/2026-10-25/DroidDeck-Mavk-fex-boundary-demo.apk`,
  SHA256 `6c8680f51db2db0551675b84f58ae09e4535ffb5326c0fd54535e549aa915b88`,
  built/verified/installed. Only portable-FEX child environment changed relative
  to443c notices APK; Vulkan/WSI/native display payload unchanged. Normal session
  `2026-10-25-10-steam` starts, installed FEX helper SHA2561d374a8660a268410189a2e09b2828a4e3ed462de62119a83a6ddf89593ce45c.
  Muck shows animated menu/AHB, copy0, errors0. Hot Thermal Status3 sample20.4/s
  is a smoke test, not an APK performance comparison. Prior APK443c retained.
-20 packaging/notice tests and37 FEX regressions PASS. Original upstream COPYING
  whitespace is retained; own-source whitespace check passes with notices excluded.

## Next bounded work (unchanged priority)

Do not repeat GDI/DXGI/Windows, synthetic descriptor expansion or resize polishing.
Do not promote CSE or re-enable GPU probing on this game yet. After thermal
cooldown, compare the same scene/extent and verify actual AHB source. If artifacts
recur on baseline, one checked AHB-vs-SHM presentation A/B with the same renderer
is informative; do not assume a diagonal defect proves tearing or a shader bug.
Capture the first corrupted frame and relevant release/acquire state rather than
random Vulkan flags. Keep FPS work bounded and move to an available second game
if Muck stops yielding actionable evidence. HL2 availability/game test still open.
