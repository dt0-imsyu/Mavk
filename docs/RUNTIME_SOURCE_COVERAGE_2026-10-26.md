# Runtime source checkpoint — 2026-10-26

Scope: the exact optional Mavk runtime
`8213c6709ef42a73cbd32c1e48aadad49788800a8552fe83f802fa6472ba08cb`
inside the developer APK. This checkpoint does not change/install any binary,
run a game, or certify the inherited DroidDeck APK dependency set.

All 280 non-manifest archive members are indexed with individual hashes in
the local source bundle's `runtime-source-inventory.json`. The 230 Openbox-family
members retain the prior exact binary-package attribution. Counts:

| Component | Members | New source evidence |
| --- | ---: | --- |
| Openbox | 144 | Upstream 3.6.1 archive matches the recipe SHA256; eight downstream patches match the recipe and apply in order |
| startup-notification | 9 | Release archive retained, plus the actual Git tag used by the recipe, commit `fce365db713897a9cc6b52cac6536f4cb9c3abf1` |
| Imlib2 / xcb-util | 66 / 11 | Prior package-byte and upstream-notice attribution retained |
| Mesa/Zink | 10 | Pinned 25.1.9 source archive and tracked local C11/glibc patch retained |
| hybris compatibility | 2 | Retained source tree archived; its saved build library matches the packaged library SHA256 |
| DXVK-Sarek | 5 | Prior exact PE32 build source and tracked ARM64X/PE32 build recipes retained; no new ARM64X rebuild claim |
| WSI/AHB | 4 | MIT source and integration in the Mavk repository; no new standalone WSI binary reproduction |
| Mavk/DroidDeck integration | 9 | Tracked code/build helpers and generated policy files |
| Notices | 20 | Text only; license presence is not source-coverage proof |

Retained SPIRV-Tools source, external SPIRV-Headers and per-file inventory were
also archived. This is source preservation, not a new compiler/GPU test or a
claim that every retained source byte was independently rebuilt.

## Downstream recipe boundary

The official Arch package recipes were pinned to immutable commits:

* Openbox: `0ef5985630beaa4ed9272d7208f0c3c4067a5163`.
* startup-notification: `1b6e1e2f75d5ad8638d28ebc5cd890979a52e018`.

Both have the exact package versions used by the runtime. Openbox's archive
and all eight patch checksums match; all patches apply to the source. However,
these are the upstream Arch **x86_64** recipes. They are not silently relabelled
as the exact Arch Linux ARM build recipes.

The two ARM binary archives were fetched and matched the already-retained
SHA256 values before reading `.BUILDINFO`. Their exact recorded PKGBUILD hashes:

* Openbox: `f67699630296f270c5f23858ed69548bcd0e557a303feccbe12a0c9f826c6f63`.
* startup-notification: `adac1e472f7aaf8eb12598336fdbb31d2ea45f6ed3758fefcbccaf653a81c843`.

Changing only the recipes' architecture does **not** reproduce those hashes.
The ARM source-file endpoints returned HTTP 500 during this checkpoint. This
leaves an exact downstream-recipe gap, rather than an unspecified missing
license. The archive fetch used the official public HTTP mirror with exact
previously-known content hashes after the HTTPS endpoint failed hostname
verification; TLS checks were not disabled. Package/tag signatures were not
reverified, and no downloaded build script was executed.

## Artifacts and next action

Local `artifacts-local/review-package-20261026` contains the source supplement,
checksums and existing review APK/source snapshots. It excludes Steam accounts,
prefixes, device screenshots and private launch settings. Full raw preparation
evidence stays under `artifacts-local/corresponding-source-20261026`.

`third_party/runtime-notices/source-pins.json` records the small public pins.
The remaining binary-distribution work is concrete: recover those two exact ARM
recipes/build adjustments, then finish corresponding-source mapping of inherited
APK prebuilt dependencies. Keep the source review and APK demonstration usable
while those tasks remain explicit. No public upload or compliance approval is
implied by this checkpoint.
