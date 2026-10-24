# Device-owned logical-null buffer backing — 2026-10-18

Progress toward real Sarek nullDescriptor admission,NOT the complete feature.

## Implementation

Common `compat/null_buffers.hpp` supplies one lazy device-owned 16-byte buffer
with UBO+SSBO usage,zeroed/flushed host-visible memory,and real shared ownership.
It accepts only logical-null UBO/SSBO requests. No native null buffer handles,
queue waits,per-draw backing allocation or fabricated resource leases. Context
must outlive all leases;callers serialize resolution. Shader guards remain
mandatory:the backing alone does not implement read-zero/write-discard.

The opt-in guarded glibc layer connects this resolver to its common native
descriptor materializer. Ordinary writes/copies and template updates now update
logical public state only in guarded mode;only valid materialized descriptors
are sent to Mali. The default tracking/native update path is unchanged.
Private guarded frontend accepts logical null,but public capability queries
remain unchanged. Non-buffer nulls fail closed at materialization. No ordinary
application is claimed compatible with nullDescriptor yet.

## Evidence

Reused the existing multiset buffer/atomic GPU regression,not a new Suite case.
`E:/Mavk/artifacts/null-buffer-backing-regression1-20261018`:

- 1536 verified output/sentinel words,1024 guard events,two metadata phases.
- Shared logical API writes ->native guarded UBO/SSBO/atomic execution.
- The same real fallback handle/lease is reused across both kinds and phases.
- Explicit host readback after GPU completion verifies all16 backing bytes remain
  zero:guarded writes/atomics did not leak into the shared buffer.
- Core live resources zero;Synchronization Validation errors0/warnings0.
- Strict existing verifier passed with --atomics --shared-state
  --runtime-compiler --multi-sets. Probe stopped afterwards.
- APK SHA256:
  7ed8b5c0067a42210a025b3184731d03d527114bae462046ae28390500ac4f95
- Fresh report SHA256:
  cf8d74153abba008454d4c2521375bfac8ac2844073685f4834e309b50860afe

The normal non-null Sarek shader execution regression after changing public
descriptor forwarding also passed:
`E:/Mavk/artifacts/sarek-consumer-run12-20261018`.
65536 components/4 submissions,indirect+resubmission+bounds,572 guard diagnostic
words,gaps0/full tracked cleanup0. This tests the actual updated glibc layer,
but DOES NOT exercise null writes from an ordinary Vulkan application,as the
feature remains unavailable. Its subsequent real D3D11CreateDevice still fails
at the same required nullDescriptor gate. Do not count it as a D3D11 PASS.

Layer10 SHA256:
4af4f3d599f76a52e989d2063cc6aa788ef9d6f3f0601870673d3c1e09dcb325

## Exact next boundary

Typed image/texel null backings and shader-operation admission remain missing.
They cannot be substituted blindly with RGBA8/R32 resources for every shader:
dimension,array/MS mode,numeric type,format,view and sampler ownership matter.
Image initialization/layout/synchronization must be explicit without stealing
an application's queue or adding per-frame waits. Shader queries/atomics and
all unsupported command/update semantics also need coverage before public
feature advertising. Do not flip the bit just to pass Sarek's device filter.

This is continued implementation work,not an external/vendor blocker. Keep
the strategic goal active. Production U-AVF/VMs/Adreno DXVK unchanged;no
root,policy changes,commit or push.
