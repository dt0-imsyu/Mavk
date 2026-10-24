# Next real consumer: SuperTux SDL / D3D11

## Why switch

Keep DDNet's actual-draw evidence, but do not let its Wine/Zink corruption become
the entire Mavk project. Native copy controls pass; bounded core + synchronization
validation on the corrupt DDNet menu finds no reported hazards/API errors, only
unused fragment-output warnings. That does not prove host-write publication or
shader semantics. DDNet readback observer remains unvalidated (control fails).

Ubuntu ARM64 validation package1.4.341.0-1 was extracted privately, not installed:
SHA256045596d3ced7fb462f0afc6a8cc576606c50719ee1708f66f1a0e92e851bf9ad
matches packages.ubuntu.com. Layer activation verified in loader log AND actual
DDNet process maps; sync enable reported explicitly by the layer. Evidence in
E:/Mavk/artifacts/zink-validation-20261019, including corrupt-menu screenshot.
Validation is enabled only by probe -Validation + explicit settings path.

## Prepared consumer, NOT a runtime PASS

Official SuperTux0.7.0 Windows x64 portable:
https://github.com/SuperTux/supertux/releases/tag/v0.7.0
Archive SHA256d637f79e8a77fc4d42982cfea40f74f812d5050b9d6d53309b287dae734e1970
verified against release metadata. Host E:/Mavk/artifacts/supertux-0.7.0-20261019.
Uncompressed396564897 bytes. EXE is PE32+/AMD64; bundled SDL2.dll contains the
direct3d11 renderer. Upstream SDL backend uses SDL_CreateRenderer:
https://github.com/SuperTux/supertux/blob/v0.7.0/src/video/sdl/sdl_video_system.cpp

Use normal existing DroidDeck session, installed droiddeck-game-env and managed
Proton/Mavk/Sarek, with SDL_RENDER_DRIVER=direct3d11 and --renderer sdl for this
bounded diagnostic. No private Zink, no Mavk-disable/native-Vulkan override.
Runner: experiments/droiddeck/run-supertux-probe.ps1. Reuse disposable Wine prefix
4294967000, not a cloned VM. This is an intermediate real-game test, not final
ordinary-user integration and not a full nullDescriptor conformance claim.

Archive streamed into app files/mavk-games/SuperTux-portable via tar/adb exec-in;
transfer returned0. Subsequent device went ADB offline BEFORE file verification
and BEFORE the first game launch. Do not claim installation/game/D3D11 PASS.
Expected EXE SHA6249f25a6ac1cc3d27955ca09852b8368618ce58ad9c13e0c5084051c2c02b43;
SDL2.dll SHAdeb8fc73a233b61ba3fa9122a94430e7dace85cd0b3572b9c338e63e7e9d7ea2.

## Current blocker and exact resume

ADB R52Y9072A2R offline; Windows PnP sees Samsung ADB/MTP interfaces Status OK.
One reconnect-offline and one host ADB-server restart did not restore it. VM/
Steam not deliberately restarted for this. Previous wait handle5753 terminated
on server restart, NOT still live. No repeated server/VM restart loop.

1. Read adb devices; once authorized online, verify the two files' SHA256.
2. Check actual DroidDeck session; reuse if live, normal start only if stopped.
3. Run prepared SuperTux probe, capture first concrete D3D11/Mavk error or frame.
4. Fix only the blocking API; verify frame, input, sound, actual visible pacing.
5. Integrate working backend through ordinary DroidDeck game launch, not a demo.

Deleted only verified duplicate SDK/runtime exports (~128MiB, host copies on E:)
and temporary pushed observer/validation libraries (~112MiB). Working SDK/runtime
directories, account, Steam games, Ubuntu/Windows/P7/H.264 untouched. No commit/push.
