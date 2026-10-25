# Third-party notices

## Vendored WSI integration snapshot

`third_party/wsi-layer` is based on ginkage/vulkan-wsi-layer
40cfec09d0e2d42c5e1f2642cf4cba11e7008112 (MIT), retaining upstream notices and
the included protocol XML licenses. Mavk modifications do not change that license.
The new `adapters/droiddeck/xcb_pointer.hpp` is individually MIT-marked so the
WSI integration can reuse it independently. Root Apache2 still applies to Mavk's
own code unless a file states otherwise. See the vendored README for boundaries.

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
- Configurable shader tool: externally built SPIRV-Tools v2022.4 at eb0a366,
  SPIRV-Headers85a1ed2 (MIT-style); SPIRV-Tools is Apache2. The reviewed Mavk patch in compatibility/shaders
  adds integer buffer atomic recognition and an explicit standard header include.
  Additional tracked texel-operands/texel-atomics patches preserve supported
  image operand masks and add bounded integer storage-texel atomic guards.
  The tracked texel-components patch distinguishes known-format OOB component
  substitution from null-descriptor zero results; full feature support is not implied.
  Upstream complete sources/licenses remain external; no wholesale driver copy.
  Optional runtime-compiler harness builds statically link that external code;
  complete SPIRV-Tools and SPIRV-Headers licenses are included in APK assets.
- Khronos Vulkan headers/validation layer: external SDK/build inputs. Use their
  respective upstream licenses when distributing those artifacts.
- Optional Sascha Willems Vulkan examples / Dear ImGui: externally supplied,
  pinned test inputs. The harness checks hashes and copies their upstream
  licenses into the APK when these optional tests are enabled.
- detex/bc7enc_rdo: external independent research oracles, not linked into the
  Mavk runtime. Investigation code requires those separately obtained sources.

Turnip and Mali capability reports are reference measurements, not copied GPU
driver implementations. DXVK-Sarek is a reviewed experimental consumer, not
relicensed as Mavk. No Adreno implementation is imported.

## Private DroidDeck integration package (2026-10-19)

The2026-10-25 package attribution identifies230 exact unchanged runtime files
from Openbox3.6.1-14, startup-notification0.12-9, Imlib2 1.12.7-3 and
xcb-util0.4.1-2 (Arch Linux ARM). Additional upstream COPYING/AUTHORS texts and
source/archive hashes are retained in `third_party/runtime-notices`. This
completes that narrow byte-attribution check, not corresponding-source compliance.

`adapters/droiddeck/run-proton.py` derives from DroidDeck's
`tools/hybris/run-proton.py` (base0916a538aff4c08f799a4bd9fa17dd3c98b01e99).
It remains under the upstream GNU GPL version3, not the root Apache license;
the complete license is retained as `adapters/droiddeck/LICENSE.DroidDeck`.

The local experimental APK additionally bundles the pinned ARM64X DXVK-Sarek
D3D10core/D3D11/DXGI DLLs, the tested Mavk/hybris compatibility layers, and a
private Openbox window manager with Imlib2/startup-notification/xcb-util.
They retain their individual licenses; this does not grant ownership of those
components to Mavk. The package generator preserves installed upstream notices
and includes Sarek, SPIRV-Tools, SPIRV-Headers and bcdec license texts.

The optional hardware Steam UI experiment adds Mesa/Zink 25.1.9 (official source
archive SHA256 412df33a1bb3c785ed698555a3972118a37c458e7accf6ae53f4bb87b3db454a).
Mesa's upstream license summary and C11 shim sources/notices are included in the
private payload. The saved glibc2.43 build patch changes only once_flag/call_once
integration; it does not add Vulkan features or establish OpenGL conformance.

This is NOT a distribution-compliance completion statement. Before any public
APK distribution, preserve exact source revisions/patches and provide required
corresponding source and all notices for GPL/LGPL components, including Openbox
and libhybris. No publication or upload is authorized by this checkpoint.
