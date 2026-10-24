# Experimental display launch selection — 2026-10-24

Prepared in an isolated DroidDeck launch-script candidate, NOT installed as the
active launcher or selected in any user's game/Steam profile.

## Behaviour

Existing system_graphics_launch() now accepts DROIDDECK_MAVK_PRESENTATION:

- unset/profile: prior behaviour, including legacy private-WSI experiments.
- ahb-bgra: explicitly selected private backend, pinned library, compositor
  opt-in marker, and mavk-ahb.json receipt matching the binary/FIFO/min3/BGRA
  contract. Activates the existing WSI proxy + GPU conversion flags.
- x11-shm: explicit reversal, clears all four experimental AHB flags, removes
  the selected private directory from the Vulkan layer search path and restores
  WSI_X11_FORCE_SHM. Works even if the candidate hash/receipt is unavailable.

Invalid selection or unverifiable AHB request fails before launching a game.
There is **no automatic retry after GPU failure**, no claim that an arbitrary
game can recover mid-session, and no implicit AHB enablement. The receipt binds
tested userspace implementation properties to a hash; it does NOT guarantee
hardware capabilities or replace native allocation-time format queries. It is
not a security boundary or a declaration of Vulkan feature support.

Source patch: experiments/droiddeck/droiddeck-ahb-selection.patch, checked against
the unchanged E: DroidDeck tree (read-only). Receipt mavk-ahb.json pins99a12d70.
Candidate full source and extended existing test file are on C:
recovery/2026-10-24/ahb-integration/tools/. Patches outside ignored recovery
preserve both code and assertions. No duplicate Vulkan renderer/transport.

## Verification

81 existing game-environment/compatibility tests PASS under WSL, with the
AHB-selection case extended for missing/mismatched receipt, two-buffer rejection,
explicit SHM reversal with stale flags/bad candidate pin, and invalid selector.
Actual helper dependencies droiddeck-esync/droiddeck-wincomponents copied into
the isolated test tree before final run, not silently relying on missing-module
fallbacks. Steam-compatibility messages in those tests refer to temporary
fixtures, not the user's Steam client or accounts.

Actual Android Python3.14 executes ahb-selection-preflight.py directly through
the existing glibc loader. It ONLY imports the candidate and calls launch
preparation with placeholder command strings; never main(), exec_proton(),
os.exec or subprocess. Device results:

```
DEVICE_AHB_SELECTION_PASS actualFiles=1 execs=0
DEVICE_SHM_SELECTION_PASS unavailableCandidateIgnored=1 execs=0
DEVICE_BAD_PIN_REJECT_PASS
DEVICE_SELECTION_PREFLIGHT_PASS originalScriptUnchanged=1 steamOrGameStarted=0
```

Candidate SHA256:
5cd0a6dad8e9a71595a0cc6a7aa777365304919a02c8b84e37067fd7ef0255cd.
Private device filename files/mavk-wsi-xcb/droiddeck-game-env.selection-candidate;
old GLM candidate and active linuxfs/usr/local/bin/droiddeck-game-env unchanged.
Only private receipt/preflight were staged. No game configuration JSON edits,
no APK update this step, no Steam/game launch, no U-AVF/VM/data change or push.

## Remaining integration

Not a normal-user UI/backend-selection PASS yet. Still need experimental
runtime packaging/selection through the fork, controlled compositor opt-in,
and a real Wine/game regression when it cannot interrupt Rust/Steam. Keep
default Adreno/Turnip, stock DXVK and existing SHM profiles unchanged. Native
Zink1200p60 is established separately; it does not prove Wine/Muck performance.
