# Mavk developer review guide

Mavk is a userspace Vulkan compatibility/runtime project over Android's stock
Vulkan driver. It is **not** a replacement Mali kernel driver, PanVK, a complete
Vulkan implementation, or a WDDM driver. Primary measured device: stock Galaxy
Tab S11, Mali-G925, Android16. No root/system modification is required.

## Start here

| Component | Purpose / boundary |
| --- | --- |
| `core/include/mavk`, `core/src` | Native Vulkan resources, uploads and synchronization; independent of Steam, Wine and Android UI |
| `compatibility` | Descriptor state/lifetime, BC compatibility and bounded SPIR-V instrumentation; coverage is explicitly limited |
| `adapters/vulkan-layer` | Standard Vulkan layer connecting native calls to the common compatibility code |
| `adapters/android` | Reusable Android presentation interfaces |
| `third_party/wsi-layer` | Pinned MIT WSI fork; XCB/Wayland proxy and AHB interoperability |
| `adapters/droiddeck` | Proton payload selection/integration; derived wrapper retains GPLv3 |
| `bridge`, `icd` | Separate experimental U-AVF/Windows transport and limited ICD; not DWM acceleration |

The DroidDeck fork owns launcher/UI/session lifecycle and its Android
compositor. It retains Steam, ARM64 Proton/Wine/FEX and the normal Adreno path.
Mavk supplies an optional experimental graphics path rather than a replacement
gaming environment.

## What the measurements establish

* Hardware compute and offscreen graphics readback on G925; Core/Bridge pixel
  checks and limited standard Windows Loader/ICD tests, documented in STATE.md.
* Actual Steam UI and Muck rendering through the experimental stack.
* AHB presentation without CPU framebuffer readback: native RGBA direct;
  BGRA requires a GPU conversion blit. This is not zero GPU work.
* D3D11 ARM64/AMD64 resize integration and a real SHM presentation fallback.
* Bounded descriptor, lifetime and shader tests on GPU, including retained
  command generations. These are not an exhaustive Vulkan CTS run.

Known Muck results are roughly34–35 displayed game buffers/s in the menu,
around12–13 in a heavy gameplay scene, and9.1 in another heavier view at both
1600p and1200p. Scene/thermal state differ; these are not universal game scores.
The last same-scene shader A/B was12.5 /12.7 /12.3, not a useful performance win.
See [closed A/B](MUCK_SHADER_AB_CLOSED_2026-10-25.md).

User-confirmed HL2 Full HD gameplay is usually20–30FPS near maximum settings,
30–50 on low, with rare character/audio repeats. These are user observations.
Its PE32/D3D9 work and exact current boundary are recorded in
[PE32_D3D9_2026-10-25.md](PE32_D3D9_2026-10-25.md). Do not call a Valve intro a
successful gameplay test.

## Honest compatibility boundaries

Native Vulkan features are passed through where implemented. Checked emulation
is described per operation, not advertised as blanket Vulkan support.
`nullDescriptor`, robustness2 and full BC support are not established merely by
Sarek running. The diagnostic Sarek admission remains experimental. Upstream
DXVK compatibility, Vulkan conformance and performance parity with Turnip are
not claimed. See DESCRIPTOR_COVERAGE.md, NULL_DESCRIPTOR_CONTRACT_2026-10-18.md
and TURNIP_DXVK_CAPABILITIES_2026-10-17.md.

Intermittent diagonal black artifacts have been reported; no permanent root
cause/fix is proved. The GPU-query and CSE experiments that caused trouble are
disabled. Adreno/other-Mali portability needs actual device tests. No equivalent
runtime result is claimed for Mali-G76.

## Independent builds

The default CMake target builds only reusable **Mavk::Core**:

```sh
cmake -S . -B build/core -DCMAKE_BUILD_TYPE=Release
cmake --build build/core
cmake --install build/core --prefix build/install
```

Provide the Vulkan SDK/headers and loader for the target. Android NDK users add
its standard `android.toolchain.cmake`, `ANDROID_ABI=arm64-v8a` and
`ANDROID_PLATFORM=android-29`. No DroidDeck checkout is required. This is a Core
build, not a complete compatibility-layer/ICD build.

The optional SPIR-V compiler has its own CMake project under
`compatibility/shaders` and requires the pinned patched SPIRV-Tools source.
The Android harness and integration build helpers are documented in
`harness/README.md` and `experiments/droiddeck`. Some historical research helpers
still reference local external compiler caches; they are not portable release
builders. Review the preserved source pins/patches rather than copying unknown
prebuilt libraries.

## Work worth upstream review

AHB allocation/import, acquire/release fence ownership, compositor layer
eligibility, fullscreen viewport handling, resize and scaled input can be
reviewed independently of Mavk shader compatibility. They may benefit Adreno,
but this remains a portability proposal until tested on Adreno hardware.
Keep SHM rollback and reject invalid geometry instead of forcing AHB.

Own code is Apache2 except individually marked derivatives. WSI is MIT,
DroidDeck-derived wrapper GPLv3, and all external components retain their own
licenses. Read THIRD_PARTY_NOTICES.md before distributing binaries; complete
corresponding-source preparation is a separate gate. No account data, Steam
prefixes, private keys or device logs belong in either published repository.
