# Third-party notices

The repository's existing root LICENSE is preserved (Apache License2.0).
Third-party code retains its own license; the root license does not replace it.

## bcdec

`third_party/bcdec/bcdec.h`: unmodified upstream iOrange/bcdec at
80859ed3b7afb1c527a2a99d70c61457bea72d0c, copyright Sergii Kudlai2022.
Mavk selects the MIT alternative. Complete upstream license and header notices
are retained in that directory; harness builds include the license in APK assets.
See third_party/bcdec/README.mavk.md for hashes and wrapper scope.

## Optional external build/test inputs (not vendored here)

- Android NDK shaderc/SPIRV-Tools: externally installed compiler/validator and
  instrumentation tools. The feasibility helper invokes them; no tool binary
  or modified upstream source is bundled in this repository.
- Khronos Vulkan headers/validation layer: external SDK/build inputs. Use their
  respective upstream licenses when distributing those artifacts.
- Optional Sascha Willems Vulkan examples / Dear ImGui: externally supplied,
  pinned test inputs. The harness checks hashes and copies their upstream
  licenses into the APK when these optional tests are enabled.
- detex/bc7enc_rdo: external independent research oracles, not linked into the
  Mavk runtime. Investigation code requires those separately obtained sources.

Turnip and Mali capability reports are reference measurements, not copied GPU
driver implementations. DXVK-Sarek is a reviewed experimental consumer, not
bundled or relicensed as Mavk. No Adreno implementation is imported.
