# Guarded color-2D images connected to the actual loader layer

## Current result

Implemented original-shader color-2D reflection,typed real image/sampler leases,
common image materialization,and retained one-shot initialization prepended to
the application's actual vkQueueSubmit/vkQueueSubmit2 call. This supersedes the
previous checkpoint's missing layer connection. Public nullDescriptor stays
FALSE;actual Sarek D3D11CreateDevice still rejects that feature. NO D3D11 frame.

This is still the same pinned Sarek3.0.2 native ARM64 consumer in the DroidDeck
linuxfs-r9 subset/Termux debug UID,not a complete DroidDeck/Wine application run.

## Implementation and bounds

- Shared reflection now distinguishes Buffer from nonarray,non-MS,color-2D images
  and preserves float/signed/unsigned scalar type. Explicit storage formats must
  match;unknown storage formats/depth/other dimensions remain unadmitted.
- Private set/materializer resolution retains the actual combined sampler owner.
  Unsupported types fail closed rather than use a universal RGBA8 placeholder.
- Queue/family/protected flags are tracked. Private color images currently require
  ONE unprotected queue in the Core family. No cross-queue synchronization claim.
- Initialization is an extra first submit item in the SAME downstream call,not
  a separate stolen queue or blocking CPU submit. Original submit structs/pNext,
  wait/signal arrays and app fence are retained unchanged. Device-group/protected
  cases outside the profile reject instead of silently changing their meaning.
- One-shot command pools and init leases remain alive until device retirement.
  At most seven admitted format initialization batches can be retained. Capacity
  is reserved before calling the driver. Failed submits destroy the unqueued
  prelude/release reservations,without marking resources initialized. Driver-
  failure injection/retry has NOT yet been exercised;this is code/lifetime review.
- Existing shader/descriptor generations still survive command resubmission.
  No per-frame queue-idle/fence waits were added by Mavk.

## Evidence

Run15/16:actual shader fixture using Submit1/Submit2 respectively,with a private
diagnostic image-init trigger. Two real catalog images initialize exactly once;
65536 components/4 submissions/direct+indirect+resubmission+bounds PASS,gaps0,
all tracked cleanup0,Core liveZero1. Seeded images alone do not prove null reads.

Run18 (`E:/Mavk/artifacts/sarek-consumer-run18-20261018`) tests the actual null
image resolution branch through controlled private fault injection:

1. App makes valid ordinary Vulkan descriptor writes;native public feature
   queries remain false. MAVK_TEST_NULL_IMAGE changes only the private logical
   CbCr descriptor to null in the explicit diagnostic profile.
2. The original Sarek video-blit shader runs planar mode. Y remains valid so its
   size divisor is nonzero. Null CbCr query/fetch returns zero. Its own GLSL
   `color.rb=CbCr.gr; color.g=Y.r; color.a=1` with identity transform therefore
   independently predicts RGBA(0,64,0,255),not the colored nonnull chroma result.
3. Real Mali readback verifies the green result for full direct draw,resubmission
   and indirect draw. The separate bounded-UBO draw still predicts red(64,0,0,255)
   in2x2 and transparent clear outside. Total65536 components PASS.
4. Original SPIR-V ->shared guard compiler ->typed common resolver ->real native
   image/private descriptors ->one-shot Submit2 prelude all execute in one layer.
   One init batch/two images,cpuWaits0,gaps0,full cleanup0/Core liveZero1.

Diagnostic counter reports676456 requested record WORDS. This intentionally
heavy null workload exceeds the bounded32KiB record buffer;individual records
are truncated. Only the safe counter word is read for this proof. Do not report
that value as a verified full record count or a performance measurement.

Run17 was a failed test scenario,not a backend PASS:it nullified both Y and CbCr.
The actual Sarek shader divides by textureSize(Y),so zero Y size makes undefined
NaN arithmetic. The initial opaque-alpha expectation was also wrong for the
shader's nonplanar alpha propagation. Source inspection led to the well-defined
CbCr-only planar scenario above;guard implementation/feature admission unchanged.
Failure evidence is preserved. Test-only exception cleanup now uses the original
write-array lifetime,not the temporary injected array after leaving the try block.

Final-binary regressions:

- Run19:guarded,Submit1,NO null injection,original colored Sarek frame65536/4 PASS,
  572 diagnostic words from the bounded-UBO phase,one init batch,Core liveZero1.
- Run20:default tracking/native shaders,NO guarded mode/private image injection;
  49152 components/3 submissions/indirect+resubmission PASS. Default remains intact.
- Shared reflection CPU regression:6 Buffer numeric mappings,4 malformed cases,
  plus a genuine sampled2D mapping and Buffer/2D separation PASS.
- Earlier common image GPU1792/384 with backing-pixel readback/SyncVal0/0 remains
  the image catalog's validation evidence. The GLIBC runs above do NOT have a
  glibc validation layer;do not attribute Android SyncVal results to them.

Layer16 SHA256:
ba7ebb91758940ce1ef094d9ded571136fb544bf96c14bdcdfe7775928a6efb7

Consumer10 SHA256:
aff75a596cb33ba267f7476de234b8db5d3aa3b9337b8b4566253d3ad0a7db1b

## Next boundary toward actual D3D11

Bounded guarded color-2D integration is now proven;do not repeat this unchanged
fixture. Public nullDescriptor admission is still incomplete. Audit/implement
remaining required resource dimensions/views/depth/MS and shader query/atomic
forms,plus command/update coverage,without inventing feature bits. Also verify
submit failure/retry and nontrivial semaphore/pNext behavior before broadening
the experimental adapter. Native feature queries must not be changed just to
pass Sarek's device filter. Then rerun real D3D11CreateDevice at that changed gate.

The attempted Khronos feature-reference fetch this turn returned no contents
and was cancelled;it is NOT an authoritative contract audit or new restriction.
Do not infer unverified extra requirements from it. Inspect actual spec/source.

Sarek stays experimental;upstream DXVK compatibility and a real first D3D11 frame
remain the goal. Production U-AVF/Windows/Ubuntu/H264/stock Adreno unchanged.
No root/security changes,commit or push. Test processes stopped;goal incomplete.
