# Independent DeepSeek copy: evidence review, not merged code

Read the complete report and run-all.cmd from ../Mavk-DeepSeek-Test. Original
layer.cpp, guards.cpp, texel_atomics.hpp, descriptors.hpp and core.cpp hashes
match that copy. Saved regression.log contains the named compiler gates. No
full suite or native-null UB run was repeated; no test files copied wholesale.

Saved vertex binaries independently decoded and asserted: va/vb=(112..115),
vc=(0,0,0,0), vd_native=(120..123), vd=(0,0,0,0). This supports instrumentation
liveness on the saved run. D deliberately narrows metadata, not the native
descriptor: it is not layer-hook or real descriptor-emission proof. Standalone
output-SPIR-V execution must remain distinct from installed Mavk integration.
The original APK22 already has separate real-loader/common-runtime and visible
Steam/Muck evidence; DeepSeek's omission does not negate those earlier gates.

One verifier weakness found: gpu-regress-vertex-guard.py prints
D_GUARD_NOT_METADATA_DRIVEN on D failure but still prints PASS cases=A,B,C and
exits0. D's saved binary data pass, but ALL_DONE cannot alone certify D. Fix
that assertion if adopting this test later. Keep this copy intact for provenance.

The report's Int64ImageEXT/Vulkan1.2 rejection is a fixture target/extension
qualification, not proof that 64-bit image atomics are fundamentally impossible
on all Vulkan1.2 implementations. No native G925 extension/feature support or
Mavk 64-bit implementation is inferred. R32i remains compiler-only there. CTS
and performance remain untested. Broad “nothing wrong with Mavk” applies only
to the isolated cases, not every image operation or complete Vulkan features.

Separate current issue: real APK22 layer reproduces incorrect missing-alpha
substitution on OOB R32_UINT imageLoad: phase0/word3 got0 expected1. Existing
atomic-zero tests do not exercise vector image component substitution. The new
storage-texel-components.comp captures full uvec4, unlike earlier .r-only gate.
This does not contradict DeepSeek's atomic results; no atomic result changes.
