# Mavk integration snapshot

Upstream base: https://github.com/ginkage/vulkan-wsi-layer
revision `40cfec09d0e2d42c5e1f2642cf4cba11e7008112` (MIT).
Before build-path normalization:144 tracked files were byte-identical to that
revision,16 changed, no upstream file missing; two Wine proxy files added.
Original copyright notices and LICENSE are retained. This code is not Mavk's
original Vulkan Core and is not relicensed by the root Apache2 LICENSE.

Integration changes: Android AHB allocation/import, explicit SYNC_FD handoff,
optional BGRA GPU conversion, fullscreen XCB/Wayland proxy and pointer adapter,
plus the pre-existing SHM/driver compatibility changes. See the source diff
against the pinned upstream revision; do not attribute upstream work to Mavk.

The reusable buffer import/blit interfaces are in `adapters/android`; the
X11/Wine pointer adapter is in `adapters/droiddeck`, not Core. Native RGBA can
use a direct AHB display layer; BGRA conversion adds one GPU blit. Neither path
requires CPU pixel readback. Actual zero-copy eligibility is runtime-dependent.

Protocol XML preserves its own embedded license. banner-ahb-v1 comes from the
DroidDeck fork; the other three standard protocol files are the exact existing
build inputs. They are provided here so builds do not depend on a private E: path.

This remains a restricted prototype, not a conformant general Vulkan WSI driver.
Single fullscreen proxy, FIFO and explicit sync only. Missing XTest keeps the
previous input passthrough. No claim of tested Adreno or general multiwindow input.
