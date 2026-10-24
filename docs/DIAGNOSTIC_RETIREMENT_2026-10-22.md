# Disabled diagnostic retirement: bounded CPU overhead fix

APK23 game environment sets MAVK_GUARD_REPORTS=0, and shader compilation
correctly omits diagnostic stores while retaining access guards. However,
Device::retireCommand unconditionally invalidated/mapped the log buffer and
printed one message per last-owner generation. APK23 menu smoke recorded245188
such messages despite reports being disabled. This is proven redundant work,
not proof it is the principal game-performance bottleneck.

## Change

Only read/print diagnostics when the device's existing guardReports policy is
enabled. Command clearing, shared generation ownership, GPU access guards,
descriptors and synchronization are unchanged. No per-call getenv or new wait.
Added diagnosticReads to device teardown summary for actual-path verification.
Enabled reporting keeps the last-owner rule and actual nonzero OOB diagnostics.

Layer da2ac31caad4395e8fed1143e6af580adae949b931124587356115e886d3edde,
same9d34b154 frontend/1f1f274a optimizer as APK23. ARM build terminal SUCCESS.

## GPU evidence

Real loader, Mali-G925, existing nested-buffer workload:

| Version | Reports | Read/log count | GPU result |
|---|---|---|---|
| APK23 layer9c00e7d4 |0|1|PASS, guards still active |
| APK23 layer9c00e7d4 |1|1;22 diagnostic words|PASS |
| Candidate da2ac31c |0|0|PASS, OOB read zero/write suppressed |
| Candidate da2ac31c |1|1;22 diagnostic words|PASS |

All26 existing isolated GPU scenarios PASS65.722sec with strict verifier
--texel-view-components --no-diagnostic-reads; actual checks enabled in every
case, zero diagnostic reads/logs, gaps0 and CoreliveZero1. Suite elapsed time
is not a benchmark and cannot be compared as a performance delta.

Normal Muck13481 maps isolated candidate, visible menu and Settings/input smoke
executed. No per-generation guard log emitted. Inspect saved full game logs
before claiming absence of other errors. No actual FPS/CTS/full-game claim.
Artifacts:E:/Mavk/artifacts/diagnostic-retirement-gpu-20261022/.

Prepared asset-only APK24 be073d18bf599daed55285342469fe0623da136d1b3c1753f4fe69391ce5e3b3,
runtime0d2d96b4e8bf185462a7d70fbfcb3017089a85f5faaa16b1b614471452c368e6.
261 runtime members and485 APK members unchanged. Same development signer;
v2/v3 signature and16KB alignment verified. Install/common integration status
must be recorded separately after actual verification. APK23 remains rollback.
No commit/push, production U-AVF or platform changes.

## Installed COMMON gate

APK24 install-r SUCCESS;Steam2026-10-22-04-steam reached READY with firstFrame.
All26 actualCOMMON cases PASS61.695sec using0d2d96b4 payload; strict runtime-path
and --no-diagnostic-reads checks PASS. Old APK23 archived COMMON results fail
this new gate at the expected unwanted log, so it catches the original issue.
COMMON archive SHAac0643bbd21d11b65c8fc96e540c3a982782a0e0b2d2772d0ecad270171b9df6.
Source fork runtime/version synchronized. No compiler/optimizer changes.

Normal installed Muck16415 maps0d2d96b4 payload with no candidate override;
visible menu/Settings/input PASS, quit normally. Saved common log has0GUARD_LOG
and no observed Mavk-gap/compile/device-loss markers. Steam session04 remains
READY. Common Muck archive SHA8742fd993cd20102a1e878bd7502ffb65c6450ae4fa90c1fc101a99ba9fcf5df.

Isolated archives SHA:
baseline f9dfaf4ac3aec140e99f71c9a4ac0363f4b157d1d9bc38ec8d505839bb7e9404;
candidate d8870984d1e29f71c0d693c21cf5551d3a3cd56d5d37f9a2155e00ab0e1f2568;
regressions58860b1c2180c9f88daa2498883022c63ba5451074318840848ee921a66c8243;
muck77918634bfeb0a3eb8b2cb41d0a97c80528a0568dc9ffb3796a7da90686260e7.
Saved candidate game log checked:0 GUARD_LOG and no observed Mavk GAP/FAIL or
VK_ERROR_DEVICE_LOST markers. 18.2MB remaining log contains other tracing,
which remains a separate measurement candidate, not a proven FPS bottleneck.
