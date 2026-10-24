# DroidDeck adapter boundary

This directory contains the opt-in runtime selection, integrity-checked packaging
and X11/Wayland input adapter. It is not part of the portable Vulkan Core.

`xcb_pointer.hpp` maps Wayland output coordinates to the actual X11 client
rectangle using its own X connection and XTest. It never consumes Wine/SDL's
event queue. Relative mouse capture and high-resolution wheel semantics are
not yet validated. The native X11/SHM route remains the fallback.

The complete modified MIT WSI source is in `third_party/wsi-layer`; reusable
Android buffer import/blit interfaces are in `adapters/android`. Android session
lifecycle, preferences and compositor integration live in the separate
DroidDeck fork, not Core. See `docs/DEMO_PREPARATION_2026-10-25.md` for evidence.

Build WSI with `experiments/droiddeck/build-wsi-ahb.sh` under Linux/WSL. Source,
build output and temporary output default to the current checkout; external
tool/SDK inputs can be supplied with MAVK_CROSS_TOOLS, MAVK_CMAKE_ROOT,
MAVK_GLIBC_SDK, MAVK_WAYLAND_SCANNER, MAVK_VULKAN_HEADERS and MAVK_NINJA.
The dated defaults refer to read-only local caches, not portable dependencies.

Run packaging regressions with:

    python -m unittest discover -s adapters/droiddeck -p 'test_*.py'

`package_presentation_runtime.py --replace-component` requires the exact existing
optional component and pinned inputs. It regenerates the extraction manifest,
receipt checks and archive version, verifies every unrelated member and its
permissions. It does not install an APK or claim Vulkan/game conformance.
