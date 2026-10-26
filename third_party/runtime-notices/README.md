# Optional private runtime dependency notices

These are upstream notice texts, not Mavk-owned code or a relicensing of the
libraries. No Openbox/Imlib2/startup-notification/xcb-util binaries are vendored.

Actual retained Arch Linux ARM package archives were compared to the current
Mavk runtime: all230 packaged files matched, no unmatched file or changed byte.
See package-attribution.json for exact versions, archive hashes and per-file
hashes. The original package installer used signature checks; this audit did
not independently repeat those checks and does not claim to have done so.

- Openbox3.6.1-14: GPL2-or-later. COPYING/AUTHORS from the official3.6.1 source
  archive https://openbox.org/dist/openbox/openbox-3.6.1.tar.gz.
- startup-notification0.12-9: LGPL2-or-later plus the notices in its COPYING,
  from https://www.freedesktop.org/software/startup-notification/releases/startup-notification-0.12.tar.gz.
- Imlib2 1.12.7-3 and xcb-util0.4.1-2: COPYING texts already present in the exact
  package/runtime, retained without modification.

The derived DroidDeck Proton wrapper's GPLv3 text is separately retained in
adapters/droiddeck/LICENSE.DroidDeck. Root Mavk Apache2 does not replace these
licenses. The new runtime notice rollup adds only text/metadata; no binary change.

This is NOT complete corresponding source or a distribution-compliance approval.
Public distribution still requires the exact downstream package build recipes,
patches and required corresponding sources, as well as source/notices for the
remaining runtime components. Plain license text or a URL alone is not proof
that all those requirements are fulfilled.

The 2026-10-26 source supplement now retains upstream archives, the recipe-used
startup-notification Git tag, all eight checksum-matched Openbox patches,
Mesa source/local patch, and retained hybris/SPIRV compiler source. Public pins
are in `source-pins.json`; exact boundaries and the still-missing ARM recipe
hashes are in `docs/RUNTIME_SOURCE_COVERAGE_2026-10-26.md`. No binary changed.
