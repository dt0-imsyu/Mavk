# Mavk

**Experimental Vulkan compatibility backend over stock Android GPU drivers.**
First tested on the Galaxy Tab S11 / Mali-G925, without root or firmware changes.

[Русский](README.ru.md) · [DroidDeck fork](https://github.com/dt0-imsyu/DroidDeck) · [Architecture and evidence](docs/DEVELOPER_REVIEW.md) · [Third-party notices](THIRD_PARTY_NOTICES.md)

## What it does

Mavk develops reusable Vulkan resource/synchronization code, compatibility logic
and presentation adapters for DroidDeck and U-AVF. Android's vendor driver still
executes the GPU work. **This is not a replacement Mali kernel driver, a complete
Vulkan ICD, a WDDM driver, or a drop-in Turnip driver ZIP.**

The working gaming prototype is:

`Steam → ARM64 Proton/Wine/FEX → experimental DXVK-Sarek → Mavk → libhybris → Android Vulkan/Mali`

Optional XCB/Wayland AHardwareBuffer presentation avoids CPU framebuffer readback.
RGBA uses the direct route; BGRA needs a GPU conversion blit. X11/SHM remains a
fallback. The Android launcher and compositor belong to the separate DroidDeck fork.

## Try it — install the DroidDeck APK

Use the fork's [test releases](https://github.com/dt0-imsyu/DroidDeck/releases) and
[install-and-play guide](https://github.com/dt0-imsyu/DroidDeck/blob/mavk-demo/docs/TRY_TEST_APK.md).
The APK bundles Mavk and the tested integration. Install it, complete the normal
runtime download, select Mavk/AHB in Launcher settings, then use ordinary Steam
Play. No source build or per-game terminal commands are needed for that route.

Mavk itself is a developer library/layer project: there is no universal Android
"install this driver" action. Do not copy its libraries into Android system/vendor
partitions. Standalone consumers need an adapter with the correct loader/ABI,
resource ownership and presentation contract.

## Actual results

| Test on stock Tab S11 / Mali-G925 | Result |
| --- | --- |
| Native Core compute/graphics and GPU readback | PASS; detailed evidence in the review guide/state |
| Limited Windows Vulkan Loader/ICD and Bridge | Experimental offscreen PASS; not DWM acceleration |
| Wine D3D11, AHB and 1920→1280→1920 ResizeBuffers | PASS in the focused consumer |
| HL2, 1920×1080, low settings with medium objects/textures | Owner reports 30+ FPS, stable 60 in some areas |
| HL2, nearly maximum settings | Owner reports usually 20–30 FPS; rare character/audio hitches |
| Muck heavy gameplay | Around 12–13 displayed buffers/s in one scene; 9.1 in another |
| Muck moving menu | Around 34–35 displayed buffers/s; AHB without CPU copies |
| SHM fallback | Visible moving D3D11 stream; game loading image also rechecked |

Gameplay reports, Present calls and compositor/SurfaceFlinger buffer counts are
separate evidence. HL2 is not locked to 60 FPS across all scenes. The final Muck
shader A/B gave 12.5 / 12.7 / 12.3, with no material gain; FPS research was stopped.
No exhaustive Vulkan CTS, full upstream DXVK, broad game compatibility or
Adreno/other-Mali hardware validation is claimed. In-game HL2 resolution changes
can crash; select resolution before starting the session. Intermittent artifacts,
first-chapter black transitions and some live D3D9 descriptor aliases remain open.

## Embed Mavk Core in another application

Requirements: CMake 3.22+, a C++17 compiler and Vulkan headers/loader for the target.
The caller creates and owns the Vulkan device/queue; Core does not create a guest
GPU or remote transport. Build/install the independent static library:

```sh
cmake -S . -B build/core -DCMAKE_BUILD_TYPE=Release
cmake --build build/core
cmake --install build/core --prefix build/install
```

For Android add the NDK toolchain and target options at configuration:

```sh
cmake -S . -B build/android -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-29 -DCMAKE_BUILD_TYPE=Release
cmake --build build/android
```

In a downstream CMake project, point `Mavk_DIR` at the installed
`lib/cmake/Mavk` directory, then:

```cmake
find_package(Mavk CONFIG REQUIRED)
target_link_libraries(your_application PRIVATE Mavk::Core)
```

Include `<mavk/core.hpp>`. Keep the borrowed `VkDevice` and queue alive, externally
synchronize queue use, and complete GPU work before retiring resources. With an
Android cross-toolchain, package search may require
`CMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH`; Vulkan still comes from the target NDK.
This target builds **Core only**, not the compatibility layer, ICD or DroidDeck.

## Integrate the compatibility/presentation adapters

| Directory | Responsibility |
| --- | --- |
| `core/` | Native Vulkan resource and submission helpers |
| `compatibility/` | Descriptor state/lifetime, BC handling, bounded shader guards |
| `adapters/vulkan-layer/` | Explicit glibc Vulkan layer and native dispatch |
| `adapters/android/`, `third_party/wsi-layer/` | Android/AHB interfaces and MIT WSI fork |
| `adapters/droiddeck/` | Runtime packaging and the GPLv3-derived Proton wrapper |
| `bridge/`, `icd/` | Separate experimental U-AVF/Windows integration |
| `harness/`, `experiments/` | Hardware consumers and retained development tools |

The glibc layer requires compatible Vulkan/SPIRV-Tools build inputs and an Android
ABI bridge in DroidDeck; it is not a Bionic drop-in. The optional SPIR-V compiler
has a separate CMake project under `compatibility/shaders`. Build helpers and
source pins are documented in the [review guide](docs/DEVELOPER_REVIEW.md).
Some historical research scripts still use local toolchain paths; they are not
portable one-command installers. Use the checked APK integration to try games.

Reuse AHB allocation/import, acquire/release fences, fullscreen geometry and
compositor support as a unit. The WSI library alone does not provide the
DroidDeck-side protocol or lifecycle. Preserve SHM and the original Adreno path.
Do not advertise features the chosen driver/adapter does not implement.

## Debugging and verification

* For DroidDeck, use **Share logs** in Setup/session drawer. Include device/GPU,
  APK version, game, resolution, settings, AHB/SHM selection and a short moving
  scene. Remove account data before sharing logs.
* For a controlled developer process, `MAVK_LAYER_PROFILE=1` enables bounded CPU
  timing summaries; `MAVK_LAYER_TRACE=1` adds verbose call tracking. Both default
  off. `MAVK_GUARD_REPORTS=0` disables shader diagnostic stores/readback, not the
  access guards. Keep logging off for performance comparisons.
* Investigate `MAVK_LAYER_GAP` at the failing API/resource operation. Do not solve
  it by inventing a supported feature bit or disabling guards for all games.
* Use actual GPU execution/readback for compatibility changes and recheck a real
  consumer after Bridge/API changes. Native, verified emulation and unsupported
  paths must remain distinguishable. Synthetic PASS is not game compatibility.
* GPU-query/CSE experiments associated with regressions remain disabled. Present
  counts and refresh Hz alone do not prove unique visible game FPS.

[Current handoff](docs/ACTIVE_PLAN.md) · [Evidence/history](docs/STATE.md) ·
[Source coverage](docs/RUNTIME_SOURCE_COVERAGE_2026-10-26.md) · [Roadmap](docs/ROADMAP.md)

## License and attribution

Mavk's own code is open source under **Apache License 2.0**. Preserve the copyright,
license and [NOTICE](NOTICE) when redistributing, and identify your modifications
as required by that license. Original Mavk work: **dt0-imsyu** and contributors.

Third-party code keeps its original terms: the WSI fork is MIT; the DroidDeck-derived
Proton wrapper is GPLv3; DXVK-Sarek, Mesa, libhybris, Openbox and compiler dependencies
are not relicensed as Mavk. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
This experimental source/reviewer package does not claim complete corresponding-source
coverage for every bundled external binary. The recorded source gaps remain explicit.
