# Shared runtime shader guard compiler — actual Android PASS

The guard pass previously ran as a host-side preparation tool. It now lives in
`compatibility/shaders/guards.cpp`,with a public portable `GuardCompiler` API.
The host CLI and native Android adapter use the same implementation. No Windows
VM boot, guest patch or production display change was needed.

## Executed proof

External pinned SPIRV-Tools v2022.4 +Headers and reviewed atomic patch are the
same as `compatibility/shaders/README.md`. Android NDK28.2.13676358,arm64-v8a,
android-29,Release/PIC,c++_static. Three archives link into only org.mavk.probe.
Its native DSO needs ordinary Android libvulkan/libandroid/libm/libdl/libc;
no additional shared compiler or C++ runtime installation. Upstream complete
licenses are included in APK assets.

Artifact directory: `E:/Mavk/artifacts/runtime-guards-native1-20261018`.
APK SHA256:
`4d8acbb09e06c5a7b7c6aaebffeb3479617989a3bde7206f4a72ecbfaa2d27c0`.

On the actual Mali-G925-Immortalis MC12,the APK compiled **original**3232-byte
atomic shader SPIR-V into9372 guarded bytes on device,not a prepared asset:

- Cold compile+validation:7.19138ms; one exact-word cache hit:0.000385ms.
  These are single observed calls,not sustained overhead/performance claims.
- Four malformed/already-instrumented input cases rejected without cache mutation.
- Existing shared descriptor frontend/materializer then executed that output:
  1536 verified words,1024 guarded invalid accesses,two GPU submissions.
- Actual null read-zero/write-discard,valid buffer/UBO/range and scalar integer
  atomicAdd result+side-effect feasibility passed. Core live allocations zero.
- Vulkan synchronization validation:0errors/0warnings. Strict acceptance:
  `harness/verify-null-buffers.py --atomics --shared-state --runtime-compiler`.
- Report `core-null-atomics.json` SHA256:
  `c9fc2d498b718fc661c2549335f8c7a2b7b29367d9805c0566f3483e1cfe04f2`.

Same APK independently re-ran the two-recorded-descriptor-generation/lifetime
gate:128words,resource retirement/cache and3negative cases PASS,SyncVal0/0.
Report `core-descriptor-generations.json` SHA256 remains
`7ea67d88bfd9739255930582db1f2f51c9b71ecd99db2952136de25968d8e537`.

Host cache gate:11assertions/5rejections PASS,including eviction while old shared
outputs remain alive,budget bypass and deterministic recompilation. Refactored
CLI validates and produces9372bytes SHA256
`a0ef7d95782e1c7f551cf0edf8fca918bb5cdd8cf8f00486094adc0c2720f381`,
identical to the previous guarded atomic fixture. Existing output is not replaced.

## Scope and next consumer work

Original set0/private set1,IndexAndInit ABI,Vulkan1.1 only. Serialized caller,
bounded exact-input LRU,immutable shared outputs. See compiler README for bounds.
This is not exhaustive shader admission,full nullDescriptor/robustness2,all shader
stages/operations,DXVK integration or a production performance result. Diagnostic
guard logging still adds invalid-access atomics. No native feature bit enabled.

Next:connect this reusable compilation path and descriptor generations to actual
consumer shader/layout/command entry points;extend semantics based on real
consumer requirements. The separately proven BC5 GPU candidate remains intact.
Do not remove Sarek admission checks before the underlying semantics exist.
