# Typed logical-null texel resources — 2026-10-18

## Progress

Shared Compatibility now includes bounded original-SPIR-V texel reflection and
a real device-owned null texel resource catalog. The opt-in guarded loader layer
merges per-stage types and selects fallback format by public set/binding. It
does NOT guess R32_UINT for float/signed descriptors or guess 2D image types.
Conflicting stage types or missing/unsupported null texel types reject the work.

Catalog admits R32_UINT/R32_SINT/R32_SFLOAT,checks actual native uniform+storage
texel format support,and creates a zeroed16-byte real buffer/view pair per type.
Valid native descriptor handles are retained by immutable snapshot ownership.
No per-frame copies,queue submissions or queue waits are added. A given format
is reused across null slots/kinds. Like the existing guarded profile,this is
serialized,single-family resource use;not complete multiqueue Vulkan admission.
Buffer-view functions were added to explicit Core downstream dispatch so layer
resources do not recurse through application-side loader trampolines.

Reflection is NOT a new validator. The layer runs the existing actual SPIR-V
compiler/validator first,then inspects the original typed descriptor schema.
Unsupported dimensions/widths/formats are absent from the admitted map. Arrays
and sampled-image wrappers are unwrapped with a bounded depth. Real image
fallbacks,unformatted broad texel semantics and full robustness2 remain open.

## Evidence / exact scope

Existing R32_UINT texel GPU test was reused with the common catalog replacing
hand-picked live-resource null substitutions:
`E:/Mavk/artifacts/null-texel-backing-regression1-20261018`.

- 2304 output/protected-sentinel values,1152 guards,two swapped null/live phases.
- Native sampled texel fetch +storage image-buffer load/store and OOB discard.
- Actual shader reflection matches both R32_UINT bindings.
- The same typed view/real owner survives both phases and unchanged snapshots.
- Core resources zero;SyncVal errors0/warnings0;strict existing verifier PASS.
- APK SHA256:
  500616389569194291d4e604aff3ed13f2e64725c6b3e5f988783495b1b35d17
- Fresh report SHA256:
  abb49fa96726534cd8ea201e5eb9bc3e66e02a27a539a4a739cfaf1f5e729f7c

Required numeric-type reflection regression:
`compatibility/test/texel-reflection.comp/.cpp`,compiled with existing NDK glslc
and host LLVM-MinGW;artifacts `E:/Mavk/artifacts/texel-reflection1-20261018`.
Six real shader declarations (uniform/storage x unsigned/signed/float) map to
the exact R32 native formats. An ordinary sampler2D is excluded. Four malformed
length/magic cases reject. PASS6/4. This is NOT proof of signed/float GPU execution;
the hardware readback test above remains explicitly R32_UINT.

Updated actual glibc layer built and passed Sarek shader execution regression:
`E:/Mavk/artifacts/sarek-consumer-run13-20261018`.
65536 components/4 submissions,direct+indirect+resubmission+bounds,
572 diagnostic words,gaps0/full tracked cleanup0. This fixture does not contain
null texel accesses;the common native GPU gate proves the catalog path separately.

Layer11 SHA256:
fbf81937d2bf38de743523333bc16f5cbdfa77e466a7c652aec55e834b9e54bd

## Still not achieved / next

Public nullDescriptor stays FALSE. Real Sarek D3D11CreateDevice still rejects it;
no D3D11 frame or full DroidDeck app/Wine-session PASS is claimed. Do not repeat
this unchanged feature filter alone. Typed image null backings with explicit
initialization/layout/queue ownership are the next resource boundary,followed
by required operation/command/update admission. Do not replace every image
with a generic RGBA8 fallback or steal an app queue for synchronous initialization.

Sarek remains experimental,opt-in. Current upstream DXVK compatibility is still
the strategic target;stock Adreno DXVK and production U-AVF/VM/H264 untouched.
Tests stopped;no commit/push/root/policy changes. Goal remains incomplete/active.
