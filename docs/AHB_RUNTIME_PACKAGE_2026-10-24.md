# Optional AHB runtime package — 2026-10-24

## Artifact

Built from the exact runtime7549290e extracted from verified APK33 on C:.
New runtime SHA256:
c2f107a4d4eb8aeff2240712977ad93c9e256d599c403f77e779477fb209383b.
Archive19263339bytes, output:
recovery/2026-10-24/presentation-package/output/runtime.zip with version.txt and
presentation-provenance.json. This is an artifact, not an installed/selected
runtime. No APK, Steam profile, account or VM change this step.

Added only:

- presentation/ahb/libVkLayer_window_system_integration.so (verified99a12d70)
- presentation/ahb/VkLayer_window_system_integration.json (relative library path)
- presentation/ahb/mavk-ahb.json (matching experimental FIFO/min3/BGRA receipt)
- presentation/ahb/LICENSE.wsi-layer (actual upstream Arm MIT license)
- integration/droiddeck-game-env (tested5cd0a6da launch-policy candidate)

**262 original members preserved byte-for-byte and with their permissions**;
only existing manifest.json changes. Core/Mesa/hybris/DXVK/Proton payloads and
licenses unchanged. Nothing copied from Steam/user data. The optional component
does not enable itself or advertise new Vulkan features.

adapters/droiddeck/package_presentation_runtime.py reuses the existing runtime
manifest verifier. Checks fresh output, pinned baseline/library/policy hashes,
ARM64 ET_DYN ELF, receipt/library agreement, relative WSI manifest path, archive
limits and no replacement of existing components. Reads back final archive and
checks all original members. Packaging tests + original rollup regressions:
11 PASS. Actual archive passes the same read_runtime() used by APK asset rollup.

Not staged on Android: latest df showed36271896KiB available (~34.6GiB), below
the user's50GB preference. Avoided another19MB device copy. No deletion of
protected VM/game data. Host source/build outputs remain C:; E: read-only.

## Normal launch hook, source candidate only

Existing generated Valve and third-party Proton wrappers can select an explicit
DROIDDECK_GAME_ENV_LAUNCHER. Default remains /usr/local/bin/droiddeck-game-env.
The selected path is quoted, preserving spaces; exit status and game arguments
remain unchanged. This permits a future experimental session preference to use
the bundled integration policy without overwriting the global launch script.

Canonical external-source patch:
experiments/droiddeck/droiddeck-launch-policy-hook.patch, including regressions.
Source candidate in recovery/2026-10-24/ahb-integration/. Full existing81 tests
PASS again, including both generated wrapper families, default behaviour,
explicit alternative in a path containing spaces, per-game environment and
argument/exit propagation. Tests use temporary fake Steam/depot fixtures only;
actual Steam client was not started or configured. Patch apply --check PASS
against the unchanged E: source. No regeneration of the user's live launchers.

## Remaining before normal-user integration

Still need app preference/UI plus session wiring: optional component path/hash,
launch-policy override, and compositor v3 opt-in BEFORE compositor initialization.
Do not create the marker after startup and assume v3 is already advertised.
Wire explicit SHM fallback and preserve normal Adreno/Turnip/DXVK defaults.
Only then build/install an experimental APK and verify the ordinary launch
scenario. Real Steam/game test must not interrupt Rust. No claim that packaging
or fake launch fixtures establish game performance/compatibility.

Previously measured native Zink1920x1200 ~59.5 displayed buffers/s and native
D3D11 regression remain separate evidence. WDDM/Windows and production U-AVF
are untouched. No commit/push, root, security policy or firmware changes.
