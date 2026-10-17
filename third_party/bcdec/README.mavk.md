# bcdec (third-party; not Mavk-owned code)

Upstream: https://github.com/iOrange/bcdec
Pin:80859ed3b7afb1c527a2a99d70c61457bea72d0c (v0.985 header).
Author/copyright: Sergii Kudlai,2022. **MIT alternative selected**; original
complete dual-license text and in-header notice preserved, unmodified.
bcdec.h SHA256:134520764d96f70a27db814173616c89ad85db95fbd3328417c7a8c77e7ca189.
LICENSE SHA256:2414c782587f6e96661cb0d19d752432ddecfcc1a221b4cb12bc2253a54dc5ac.

Used ONLY through bounded Mavk BC6H/BC7 wrappers. This is a portable CPU codec,
not Adreno/Turnip implementation, not a GPU driver, not claimed performance-
optimized code. No upstream source edits. License is packaged with harness APK.
Upstream documents that it is size-oriented, not specifically speed-optimized.
Use as correctness/fallback implementation; measure before production decisions.
