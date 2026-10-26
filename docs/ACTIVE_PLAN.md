# Current handoff — 2026-10-26

Finish the DroidDeck/Mavk developer prototype. No more FPS research, no Muck
optimization, no push/publication. Production U-AVF/Windows/Ubuntu are unchanged.

## Proven / currently happening

* User confirmed HL2 Full HD gameplay: usually20–30FPS near maximum settings
  (range10–30),30–50 on low. Rare character/audio repeats remain unlocalized.
  In-game resize can crash the game/Steam; choose the mode before session start.
* Cold launch1920x1080 independently verified from D3D9/swapchain parameters.
  User is recording: do not install an APK, stop the session or start another game.
* Scalar-layout feature policy and checked specialization-only descriptor aliases
  have compiler checks32/rejections14; four GPU report/sharing cases and2048
  retained generations/131072 words passed. Some live D3D9 aliases still reject.
* New packaged PE32 Sarek selection/WoW64 placement need no private game paths.
  Mavk sessions prefer Windows/Proton for dual-platform x86 games, preserving
  explicit tools and native ARM64. AHB/BGRA/SHM policy is now in fork source too.
* Runtime8213c6709ef42a73cbd32c1e48aadad49788800a8552fe83f802fa6472ba08cb:
  normal Gradle APK build PASS, not installed. APK6c8680f5/runtime3bd128 is rollback.

## Remaining closure work

1. Clean-checkout build/input verification, reviewer docs/source origins/notices.
2. Local commits and clean tracked trees in both repos; final APK/checksums.
3. After user finishes recording: install the prepared APK, remove only owned
   temporary launch overrides with rollback, test ordinary Steam Play for HL2
   and Muck, AHB/input/restart. Do not return to FPS optimization.
4. Keep unresolved corresponding-source coverage of prebuilt binary components
   explicit; source review is not blanket binary-distribution clearance.
5. Then resume the separate U-AVF GPU acceleration roadmap.

See DEVELOPER_REVIEW.md, PE32_D3D9_2026-10-25.md and
MUCK_SHADER_AB_CLOSED_2026-10-25.md. Historical Windows/ICD milestones remain
in STATE.md and Git history.
