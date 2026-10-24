# AHB keyboard fix and exact Zink boundary — 2026-10-24

## Verified keyboard fix

The compositor's key_event selected droiddeck_ime_target(), which returned the
topmost presentation proxy despite that client having no wl_keyboard binding.
It returned early instead of trying Xwayland. Checking only the last fallback
loop (APK31) was insufficient: IME and g_key_target had already selected it.

APK32 validates keyboard eligibility at all three earlier selection points.
No app name, title or game exception. The same independent SDL3/XCB D3D11
consumer, same WSI f1b9370e, now actually reports:

```
WINDOW_DEVICE_HRESULT=0x00000000 level=0xb000
WINDOW_INPUT type=mouse-button
WINDOW_INPUT type=key
WINDOW_INPUT type=key
WINDOW_INPUT type=key
BENCH_RESULT measured=957 seconds=7.994 present_calls_per_second=119.709
MAVK_LAYER_DEVICE_END ... gaps=0 ... remainingSets=0 remainingBuffers=0 ...
MAVK_LAYER_CORE_END liveZero=1
```

Sent tap, A, Shift+B through the existing agent. This proves keyboard events
reach the original XCB app while its AHB proxy is visible; it is NOT a character
mapping/modifier correctness suite or Muck gameplay/input PASS. Present calls
are not independently measured unique-visible FPS. No Steam game was launched,
so the user's competing Rust session was not interrupted.

Actual installed APK SHA256 (queried on Android, not inferred from filenames):
3e4408121c8305cf69b452fe66f742c3b3704f339d2bcafd1578f120577b81a8.
Native library bfc199e4bae1f5c151114c499aabf9addb4abdd839ace0c73ca46af9371cfb1d.
486 other APK members unchanged. v2/v3 signatures verified; installed with
--no-incremental. The optional v4 sidecar writer stalled on E:, was cancelled
after main-APK verification. Do not use that incomplete sidecar as evidence.

## Real 3D consumer exposed a format limit

Used the existing upstream glmark2 jellyfish scene, 10 s, 2560x1600, guarded
Mavk+Zink. SHM control: 54 benchmark FPS / 18.645 ms. Not a native-power ratio,
unique-visible FPS or thermal-normalized game comparison.

The AHB run reported 1965 FPS but **never created its swapchain**. There were
no AHB presentation frames. This result is INVALID, not a performance gain.
The harness now detects the specific swapchain errors both during execution
and after terminal exit; native exit0 cannot turn this into PASS. Real negative
control stops after ~1.1 s with presentation-initialization-failed, status -15.

Added bounded creation diagnostics to a separate WSI copy on C:, not to the
installed common runtime. Exact request:

```
MAVK_AHB_PLATFORM mode=2 presentId=0 presentWait=0 maintenance=0
MAVK_AHB_REQUEST flags=0 format=44 layers=1 sharing=0 customAllocator=0
```

No pNext chain. Format44 is VK_FORMAT_B8G8R8A8_UNORM, rejected by the current
RGBA8-only importer/factory. Do NOT loosen that guard or reinterpret BGRA as
RGBA; channel order is different. The D3D11 path uses format37/RGBA8 and works.

Extended the existing native AHB receiver with --format-support, reusing its
stock Mali initialization. Actual native Vulkan (not Mavk-reported features):

| Format | usage | VkResult | external features |
|---|---|---|---|
| RGBA8/37 | COLOR_ATTACHMENT (0x10) | 0 | 0x7 |
| RGBA8/37 | color+sample+transfer src/dst (0x17) | 0 | 0x7 |
| BGRA8/44 | COLOR_ATTACHMENT (0x10) | -11 | 0 |
| BGRA8/44 | combined 0x17 | -11 | 0 |

This is NO-GO for those **BGRA8 optimal-tiling AHB image parameters**, NOT for
ordinary BGRA Vulkan rendering, all possible formats or hardware Vulkan itself.
Allocation/render/export are not tested by this capability query. Ignore max
extent fields when the call fails. Probe SHA256:
7ba24a7e4739a994ad7f16dcb6dfbab0480b56ee6dd2ac7706f15b152c56dd42.

The standard AHB/Vulkan equivalence and external-image capability query contract:
https://docs.vulkan.org/spec/latest/chapters/memory.html
Public Android formats:
https://developer.android.com/ndk/reference/group/a-hardware-buffer
No undocumented numeric BGRA AHardwareBuffer allocation was attempted.

Next useful implementation decision: correct RGBA negotiation in the Zink/X11
consumer, or a general GPU BGRA->RGBA conversion before the existing AHB fence
release. The latter is an explicit GPU-copy compatibility fallback, NOT zero-copy.
Measure conversion and verify colors before advertising or enabling it. Do not
repeat fake FPS, swap channel interpretations, or rewrite the common transport.

## Storage incident / preserved state

Windows System log records repeated disk event153 on Disk1 during this work;
Get-Partition maps E: to Disk1, WDC WD10EZEX-22MFCA0. D: is Disk0, C: Disk2.
HealthStatus says Healthy but does not negate observed I/O retries. No disk
repair, format, firmware/security changes or unrelated process termination.

Stopped new E: writes. Small recovery copies on the separate C: disk:
Mavk/recovery/2026-10-24/wsi-ahb-source.tar (1341440 bytes),
droiddeck-changes.tar (348160 bytes), and explicit priority-source (454192 bytes).
Do not claim this backs up every external project/VM or all historical artifacts.

Latest diagnostic WSI source and build are now in
Mavk/recovery/2026-10-24/work-wsi and build-local/build on C:.
This copy adds only request diagnostics over the preserved f1b9370e source.
Diagnostic binary bf8e947b3ceee5f157373be3fbc1a1272ec265008f872bc4938e9a40d13b9a3d
was tested then replaced with original f1b9370e on Android. Recovery/build dirs
are gitignored. Build helper now accepts explicit source/output/temp overrides
so future builds need not write E:. SDK/toolchain on E: are still read-only inputs.
Small exact-request and SHM logs saved beside recovery files on C:.

Host exec handles27765 (optional signing sidecar),99177 (first input log E:),
22834 (first screenshot pull E:) may still be pending host I/O. Their native
device test process was confirmed absent before the successful stdout-only
repeat. Do not restart native workloads merely because those host writes hang.
Never kill the shared ADB server to clear an artifact writer.

Game profile/script remain restored (cmp verified). Session marker removed;
test session stopped. APK32 remains experimental, original system WSI/common
runtime unchanged, screen stay-on retained. Ubuntu/Windows/U-AVF untouched.
No commit/push. Strategic Mavk objective remains incomplete and active.
