# Current handoff — 2026-10-26

DroidDeck/Mavk developer prototype implementation is locally committed. No more
Muck FPS research or speculative shader optimization. No push/publication.
Production U-AVF/Windows/Ubuntu are unchanged.

## Current evidence

* User HL2 Full HD gameplay: usually 20–30 FPS near maximum settings (10–30
  range), mostly 30–50 on low. Rare character/audio repeats remain unlocalized.
* Clean DroidDeck checkout ea9c18b rebuilt normally, APK installed. Runtime
  8213c6709ef42a73cbd32c1e48aadad49788800a8552fe83f802fa6472ba08cb.
* Removed only owned temporary HL2 overrides after user's recording ended.
  Standard Steam launch requests started packaged HL2 and Muck, without private
  layer/PE directories. HL2 first train scene and keyboard menu input observed;
  Muck moving menu observed. AHB game intervals have CPU/layer copies 0.
* Restarted with AHB disabled: Muck loading image reached screen via SHM/copy,
  zero-copy 0. Full SHM gameplay was not repeated. Restored AHB and cold-started
  HL2 again. Session 2026-10-26-05-steam; leave it available to the user.
* Focus-wait observer does not recognize private Xwayland windows; its timeout
  is not evidence of game launch failure. First chapter introduction was black
  for a period; never call those 58+ buffer arrivals gameplay FPS.
* Scalar-layout/specialization tests: compiler checks 32/rejections 14, four GPU
  report/sharing cases and 2048 retained generations / 131072 words PASS.
  Some live D3D9 image aliases still reject. No conformance/full DXVK claim.
* Independent Mavk Core builds in a clean checkout without DroidDeck.
* Focused packaging/routing and Linux policy tests PASS. Broad Windows Android
  host suite: 866 tests, 105 failures, 2 skipped; not a green regression gate.

## Artifacts / remaining gates

Local files: artifacts-local/release-20261026. Includes installed APK, checksums,
source snapshots, exact PE32 Sarek source, checked dependency seed and reports.
Fork: C:/Users/denis/MainProjects/DroidDeck-Mavk (mavk-demo).
Mavk: C:/Users/denis/MainProjects/win11ontab/Mavk (main).

Reviewer guides and message draft are committed. No personal fork remote or
video URL exists yet. Preserve explicit distinction: source review is ready,
but public binary distribution still needs complete corresponding-source
coverage for bundled GPL/LGPL components. Notices alone do not close that gate.
Do not claim the entire publication goal complete while this remains open.

Next useful work: close that bounded distribution/source inventory (no new FPS
experiments), then resume the separate U-AVF GPU bridge roadmap. Historical
Windows/ICD milestones remain in STATE.md and Git history.
