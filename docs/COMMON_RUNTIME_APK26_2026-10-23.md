# Experimental common runtime rollup — 2026-10-23

## Installed result

Only `com.droiddeck.launcher.mavk` was updated. U-AVF, Windows, Ubuntu, P7,
firmware and H.264 were not changed. No commit or push.

The common runtime includes the checked native subgroup-size stage passthrough,
zero-valued extended layout chain admission/deferred unused extended layouts,
and enabled `uniformBufferStandardLayout` validation fix. Used unsupported
extended layouts still fail closed. No new Vulkan feature bits were advertised.

Artifact directory: `E:/Mavk/artifacts/droiddeck-compat-apk26-20261023/`.

| Artifact | SHA256 |
|---|---|
| DroidDeck-Mavk-compat-20261023.apk | 6bf34c5730bf5386843b65d4ae4acb90556a278f38a9bff556b7c3216af88ca4 |
| Installed APK25 rollback copy | 044f33a9a7ddac9cd1b9491778ffb44b99ac90f6ce1c4ad1ad57a76f13a2d7d6 |
| New runtime | 7549290e1a5b9685b6bbc29c1c58088a6cb24aa65d12c85e8903cc83196e56fa |
| New layer | f1081e2c783a4ea8256981f669cba97233383fb24470b0a3ea6a661a160d288e |

Asset-only rollup, not a full Gradle build. Existing runtime updater's eight
tests pass. 261 runtime members and 485 APK members are unchanged. APK signing
certificate matches the installed experimental application; v2/v3 signature and
16 KiB alignment checked. The prior runtime remains on the tablet. Repository
`app/src/main/assets/mavk/runtime.zip` and `version.txt` match the new payload.

## Actual hardware verification

Tab S11 SM-X736B, stock Android, Mali-G925:

- Installed normal Steam session starts.
- 26 common GPU cases pass with the exact installed runtime hash, guards on,
  diagnostic reads/events off, no layer gaps and CoreLiveZero. Includes dynamic
  offsets, buffer/texel/image guards and atomic/component regressions covered by
  the existing aggregate verifier. This is not CTS or complete robustness2.
- Normal Steam launch of Muck reaches a visually correct menu with the new
  common runtime mapped, standard UBO enabled and no diagnostic layer override.
- No observed layer gap/device loss in this bounded integration run. No gameplay
  FPS improvement, full input check or all-game regression claim. Android tap of
  Settings did not open it; the screenshot named `muck-settings.png` is still the
  menu and is NOT proof that Settings/input was tested.
- Muck quit normally. Diagnostic per-game profiles were restored before desktop
  benchmarking. Existing system WSI SHA256 remains
  d07b961181b9547fd48ad9f05cc6d3b808b824676eb5a186fe3feb8caa013f19.

## Read-only metadata experiment

The compiler can optionally mark only its newly generated private descriptor
metadata variable `NonWritable`. Metadata snapshots are immutable after creation;
application buffers and diagnostic stores are not marked. No `Restrict` claim.
`MAVK_METADATA_READ_ONLY=1` selects it; default remains OFF.

Compiler checks: 21 checks/9 intended rejects. The enabled candidate passes all
26 actual Mali GPU regressions. This does not establish a performance benefit.

Muck menu control: 18.3 Surface buffer updates/s; read-only candidate: 10/s, both
with thermal/order confounding (thermal status 3). These are NOT unique image FPS
and cannot establish a causal regression or improvement. A subsequent offscreen
terrain sequence gave 113 / 95 / 95 completion FPS (control/read-only/control),
again no demonstrated gain. Therefore no default switch.

Optional compiler cache counters show 96 calls, 78 compiles and 18 hits. Guard
compilation was about 0.81–1.00 seconds of a 76–77 second Muck startup, so increasing
the cache was not pursued as the principal startup/performance fix. Default cache
remains four entries; optional bounded tuning is diagnostic only.

Artifacts: `E:/Mavk/artifacts/readonly-metadata-{frontend,layer}-20261023/`.

## Test harness correction

The existing `--regressions-only` runner used to verify a stale nested-buffer
output without executing that case. It now executes it before verification.
Fresh isolated runner and installed common-runtime runs both pass all 26 cases.
This was a harness defect, not a newly found product shader defect.

## Separate non-shipped WSI branch

`OPTIMAL_READBACK_2026-10-23.md` records functional optimal-image/readback A/B.
No reproducible speed gain; it is NOT bundled in APK26. Original cached-linear
WSI, default metadata policy, game profiles and all production baselines remain.
