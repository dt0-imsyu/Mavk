# Common experimental runtime rollup — 2026-10-20

This is DroidDeck-Mavk only, not production U-AVF or a conformant Vulkan release.

## Inputs and rollback

- Baseline APK11 `E:/Mavk/artifacts/droiddeck-ui-hardware-toggle-apk11-20261019.apk`,
  SHA256 `bda1f9553d145cc256bf0301f8fbc6e7272f26a92ef42f1d14a2ad8edec53e4b`.
- Baseline runtime `02a188f5a9fcb5a77958a9d83eee1a56a09ba9e1a593893a974184020ada96cc`;
  saved as `E:/Mavk/artifacts/mavk-runtime-rollup-20261020/baseline-runtime.zip`
  plus baseline-version.txt. No baseline directory overwritten on Android.
- Layer `f614df284058d4f7941539e130b372fe2923a01c5e9b4ea663d159e405d4dab7`.
- New runtime `9fb32fc768a0948573263c373df60643efb8858db017d92974e1ebe4d1c79cfe`.
- APK12 `E:/Mavk/artifacts/droiddeck-common-runtime-apk12-20261020/DroidDeck-Mavk-runtime-20261020.apk`,
  SHA256 `87a18de8c078b5da2636a1550a6947acd7286d588c7d081c7bdc06b213d33c58`.
- Signing certificate SHA256
  `a40da80a59d170caa950cf15c18c454d47a39b26989d8b640ecd745ba71bf5dc`.

Rollback APK with normal in-place install of baseline APK11; do not uninstall,
clear app data, replace disks or runtime game data. Restore source runtime.zip
and version.txt from the named backups if reverting the source fork. Game JSON
before-common-runtime backups retain only prior configuration, not game files;
do not restore a stale scratch override unless intentionally using that test.

## Verified scope

`adapters/droiddeck/update_session_runtime.py` checks trusted input hashes,
manifest membership/hash/mode, bounded archive and ELF64 little-endian ARM64
ET_DYN; only library and manifest change. All261 other members unchanged.
Eight updater unit tests PASS, including malformed/duplicate inputs and
preservation of existing outputs. Two independent rollups produced identical
archive hash. No Proton, Mesa, hybris, Sarek or license bytes replaced.

`package_runtime_apk.py` replaces only runtime assets in exact APK11;485 other
members unchanged. Old V1 signature entries removed, SDK zipalign16KiB and
apksigner applied, certificate equality verified. VersionCode remains13,
same package `com.droiddeck.launcher.mavk`. Full Gradle build was cancelled
after cold dependency fetching; do not describe this as a clean full rebuild.

Clean experimental Steam session2026-10-20-02-steam uses common9fb32fc7 payload.
CDP WebGL renderer: ANGLE/Mesa/Zink Vulkan1.3/Mali-G925-Immortalis MC12.
Real Vulkan consumer executed from retained test binary with VK_LAYER_PATH
set ONLY to new common payload; MAVK_EXPECT_LAYER_PATH asserted actual mapping.
GPU PASS512words/8192color components,2DispatchBase/8indirect-count draw cases;
12guarded commands/12submits,gaps0,resources0,Corezero. Core/KHR exercised,
AMD unavailable. Logs `mavk-runtime-rollup-20261020/common.stdout/common.stderr`.
No CTS/SyncVal instrumentation or FPS claim for this consumer.

Muck-only DROIDDECK_MAVK_LAYER_PATH removed from both Android/guest JSON using
existing helper --common, preserving unrelated settings and making backups.
Fresh Muck launch request accepted but no game process appeared while fresh
Steam session remained STARTING_STEAM without firstFrame under keyguard. Old
21:09 game log explicitly NOT reused as fresh integration PASS. No speculative
display/lifecycle change made; repeat visible game regression when usable.
Temporary existing app sleep preference restored by removing key (originally
absent), not forcing a new permanent value. Android security unchanged.

Full nullDescriptor/robustness2 still unadvertised. Existing bounded descriptor
GPU results remain valid; fresh standalone Core suite remains an open regression
after the separately documented cancelled screen-off attempt. No commit/push.

## Next dependency boundary

D3 dynamic descriptor kinds currently cannot enter descriptor_api::kind;
saveSets rejects any dynamic offsets. Proper implementation must preserve
offset ordering across sets/bindings/arrays, device alignment, effective ranges,
null behavior and snapshot identity without baking offsets twice. Immutable
samplers need owning native leases and compatibility comparison, not retained
caller pointers. D4 late updates cannot be enabled by accepting layout flags:
record-time snapshots do not preserve legal submit-time updates. These are
general API contracts, not game-specific workarounds.
