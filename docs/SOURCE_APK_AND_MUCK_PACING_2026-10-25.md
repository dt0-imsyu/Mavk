# Source APK and read-only Muck pacing audit

## New authoritative state

Separate C: DroidDeck fork now built a normal Gradle debug APK, not a DEX/native
rollup: Kotlin/Java/native/resources/DEX packaging plus verifyRuntimeInputs passed.
70 missing external runtime dependencies,100138173bytes, were imported from a
read-only local cache only after matching BOTH manifestfde81948 and APK360c4765.
Maintained source assets/current Mavk ZIP were excluded. Full-source rebuilding
of every third-party dependency and Rust download engines is NOT claimed.

The bounded importer has5 passing tests: audit/no writes, exclusive copy/repeat,
both input pins, preserve different files, tracked-source exclusion/path guards.
Windows CRLF checkout caused recipe hash mismatches; LF restored exactly the
existing release pin hashes, with no recipe/feature mutation or pin relaxation.
Added tools/ LF attributes so fresh Windows checkouts retain valid input bytes.

Installed source-built APK:
`recovery/2026-10-25/DroidDeck-Mavk-source-demo.apk`, SHA256
`675e9cb6f83a0a76d7ae199158066f14a294dc9bade4711a5d43cb87331f97b3`.
Packagecom.droiddeck.launcher.mavk,version13,0.4.0-mavk-experimental.
Actual APK runtime assets checked by runtime_inputs.py apk after assembly.
Previous rollup360c4765 retained for rollback. No uninstall/data reset.

Actual AMD64 PE/Wine D3D11 regression in existing isolated .mavk-wine-poc prefix:

- S_OK/11_0;1920x1200→1280x800→1920x1200 ResizeBuffers PASS;
- new AHB imports,2130Present calls/18.004s, no visible/gameFPS claim;
- exit0,CoreLiveZero,gaps0,normal-session and AHB/SHM policy checks PASS;
- compositor interval1187on-screen/1186zero-copy buffers,copy1,pooldrops0,
  includes startup/resize; not a unique gameplay FPS measurement;
- current private fixture output saved under artifacts-local/demo-20261025.
  Preserved old fixture output/DLL links in .before-source-apk, no game prefix edited.

## Concrete new evidence about Muck's30-ish FPS

No Steam/game process was launched. Read only the owned installed managed
assembly and the Muck-specific saved registry value. Neither was modified.

Assembly-CSharp.dll SHA2565f9d0dc1e0e72f5493013c92d05ed9fb8628019f54f8bc02162dad0d2f20810c.
External dnfile0.18.0/dncil1.0.2/pefile2024.8.26 tools are local-only inputs;
the game binary is NOT redistributed or committed. All method bodies parsed
without errors. Relevant IL establishes:

- SaveManager saves PlayerSave as XML in PlayerPrefs key save.
- CurrentSettings.Start→InitSettings→UpdateSave reads PlayerSave.vSync/fpsLimit.
- UpdateVSync forwards the supplied value to QualitySettings.set_vSyncCount.
- UpdateMaxFps forwards the supplied value to Application.set_targetFrameRate.
- PlayerSave default fpsLimit is144, not30.

Actual saved XML: fpsLimit144,vSync0,fullscreentrue; shadows0/0/0,AA1,
textureQuality0,bloom2. Saved-value SHA256b2bdf968dc48f3e041cdd27f6caca58ae4219c471d00c2b134de86bbc75ecc70.
Registry display dimensions separately show1920x1200 and use-native0.

This rules out an explicit saved30FPS setting in this version/prefix. It does
NOT prove these branches run with those values in a particular live gameplay
scene, nor exclude driver/WSI pacing, FEX/Wine serialization or GPU execution.
No generic Muck30FPS root cause or performance fix is claimed.

Reproduction: managed-frame-pacing.py reads local IL without execution;
muck-pacing-settings.py reads only the known save and prints whitelisted fields.
No new framework, scene patch, game-specific runtime workaround or cap bypass.

## Next necessary live test

Steam remains reserved. When permitted, use the actual same gameplay scene with
known saved144/VSync0 and verified AHB route, collect CPU/thread timing and
existing GPU/submit/present profiler together. Do not repeat settings guessing,
pretend a native triangle is a game, or tune encoder/VM paths. HL2 absent from the
checked main-library manifest list; do not download a large game without need.

Public release still needs game/input regressions, complete dependency notices
and required corresponding source. The future runtime generator now includes
the derived Proton wrapper's GPLv3 text; the already-pinned old runtime is not
silently relabeled as containing that new notice or complete compliance material.
No push/publication, U-AVF/VM changes, root or security-policy changes.
