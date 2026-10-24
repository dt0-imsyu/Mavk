# Private GPU profiler safety — 2026-10-23

SUPERSEDED FOR TIMING: subsequent real-game run exposed uninitialized first query
polls. Candidate3be3334e GPU timings are INVALID, including plausible tiny test
timings below. GPU output regressions remain actual PASS, not query-API correctness.
MUCK_GPU_TIME_2026-10-23.md records minimal host-reset fix7d7805bf and control.
Do not deploy/use3be3334e as a timing profiler.

Scope: prepared/uninstalled diagnostic layer only. No performance improvement
or installed Mavk product defect is claimed. New Vulkan feature work remains paused.

## Proven issue and change

The opt-in queue-submit profiler prepared timestamp resources, then constructed
the decorated command list outside its diagnostic error handler. std::bad_alloc
from that list reached the general queue-submit catch and returned an error
instead of forwarding the application's original submission. This was an unsafe
diagnostic failure mode; it was NOT present in installed APK25 and cannot explain
the earlier GravityMark GPU faults.

Extracted eligibility and decoration into gpu_profile_submit.hpp. Decoration is
noexcept, reserves the whole list before use, and returns false on allocation
failure. Callers disable the private profiler and retain the original native
submit pointer/count/fence when decoration fails. Submit2 command metadata and
signal-semaphore records are copied, not reconstructed. Eligibility excludes
wait-semaphore batches, prelude/protected/device-group/unknown-pNext batches,
empty commands and count overflow from inserting two markers. No extra submit,
queue idle, fence wait, barrier, public feature bit or pixel-path change added.

## Verification

Real helper host test: compatibility/test/gpu-profile-submit.cpp. Compiled as
optimized x64 Windows executable using existing LLVM-MinGW and Vulkan headers.
`GPU_PROFILE_SUBMIT_HOST_PASS checks=33 gpuExecution=NOT_TESTED`.

Includes both submit APIs, untouched original commands/signal records, prefix/
suffix order, eligibility rejection and real injected std::bad_alloc from each
decorated vector allocation. The test checks fallback selection, NOT execution
of a real Vulkan driver/queue. Does not verify GPU timestamp accuracy, query
availability or timestamp resource lifetime on Mali.

Full ARM64/glibc layer build with -Wl,-z,defs PASS. Candidate:
E:/Mavk/artifacts/gpu-profile-safe-20261023/libVkLayer_mavk_compat.so,
SHA256 `3be3334e2a48486209f2f12233289aa72a187c1373f557c2114c7cb845b56002`.
Host executable E:/Mavk/artifacts/gpu-profile-host-20261023/gpu-profile-submit.exe,
SHA256 `fcd2243ffc57b6dd5685b399f92f1d01a217d9855b458831f6b737fcb81569db`.
## Real GPU regression update

USB recovered after one normal host ADB-server restart. Staged candidate3be3334e
ONLY in files/mavk-gpu-check/payload, with symlinks to existing hybris layer.
Reused existing aggregate consumer/control.sh --regressions-only via per-process
MAVK_BUFFER_COMMON=1 and DROIDDECK_MAVK_RUNTIME pointing at this private folder.
MAVK_GPU_PROBE=1, warmup0 for this short regression only; guards remain enabled.

All12 modes PASS in11.139s: nested,dynamic,resubmit,inflight,empty,unused,
selected,immutable,grouped,routing,texel,images. Every mode reports coreLiveZero;
no Mavk gap, GPU probe error or GROUP_ERROR_FATAL in their stderr.11 timestamp
records from actual Mali, validBits64,period76.923080ns, all submit1. This proves
the candidate executes timestamp-marked submissions and preserves these tests'
checked GPU results. It is NOT broad conformance, submit2 GPU coverage, Muck FPS,
GPU utilization or timestamp precision calibration. Sampled batches can be test
initialization, not the meaningful workload; do not extrapolate their ~0.03ms.

Original aggregate library SHA89c8c463... unchanged; no mapped file overwritten,
no installed runtime or game config replaced. Prior results saved before reuse;
new archive E:/Mavk/artifacts/gpu-profile-safe-20261023/gpu-results.tar.gz.
Installed APK25 remains unchanged. No performance improvement claimed.

Remaining profiler limits: bounded64 samples, no-semaphore-wait batches only,
30s default warmup, subsequent-submit query polling; final batch may lack a report.
Batch timestamp span is NOT GPU busy percentage or isolated shader execution.

## External state / next

The following is historical disconnection state; USB subsequently recovered.

USB was absent, briefly returned, then disconnected during a new native Android
GravityMark run. Last confirmed start: com.tellusim.GravityMark NativeActivity,
PID23532. Native result is UNKNOWN, not failed/stopped. Inspect this SAME process,
log and result screen after reconnection before starting another benchmark.
The25s SurfaceFlinger capture failed because ADB disappeared; empty output is not
an FPS result. Desktop08 was already IDLE when USB briefly returned. No VM/session
restart, production runtime change or commit/push.

GravityMark native-layer control is now retrieved at
E:/Mavk/artifacts/gravitymark-1.89-20261022/native-layer-control.log; see the
2026-10-23 update in GRAVITYMARK_2026-10-22.md. Do not repeat faulty ASTC variants.
