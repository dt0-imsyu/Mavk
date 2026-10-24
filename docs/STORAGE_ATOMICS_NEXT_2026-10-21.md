# Guarded storage texel atomics — 2026-10-21

## Installed APK22 — final bounded-package checkpoint, 2026-10-22

APK SHA6525353cd72c22cf750705a7ca2c1407c58f190944fbd57876c633e24c042dfd,
runtime1e859af69e957fc6e5f59f7b1a5785b8037fc1cd8684a56b5c22797873d659ca.
Install-r SUCCESS;all24 COMMON GPU gates PASS,42.437sec,strict actual-runtime
mapping verification PASS. Archive common-results.tar.gz SHA
513a1be8efdba085a5734261094d2631ad6eba0f9c3186cbbf008f42624900b9.
APK folder:E:/Mavk/artifacts/droiddeck-atomic-apk22-20261022/.
Normal Muck30542:actual1e859af6 payload mapping,menu/Settings/input visible PASS,
225068guarded commands,no observed compile/gap/device-loss errors. Log archive
muck-common.tar.gz SHAc06ec080aaefde118493fb7e024e48bc6f155fb93c01d30c5728173c309f906b.
Game quit normally;Steam running;per-game override absent. Source assets and
default guard/optimizer archive pins synchronized. APK21 retained rollback.
Only layer/manifest changed in runtime;261other members unchanged. APK only
runtime/version assets changed;485other members unchanged,16KB/signature PASS.
No production U-AVF/VM modifications,commit,push,feature-bit or CTS claims.
Do not grow this into an exhaustive atomic-variant project;choose next concrete
cross-subsystem P0/P1 after this finite checkpoint.

## Current result: isolated GPU PASS, not installed

New candidate layer ca87ea54dfbcc676bacee2bf1bea7f5b556cb1facfebbea988722e6377004238
uses frontend 8b1d0db1bbe9b08fef2274169bef1cc8b906cbd99e105e7d0f18748b5c7b13f5
and optimizer 1040dcb55009032e7e08ceaa7b04ed187be5a162201776ab3dbd46230653d182.
Consumer 837c82130620d1956a9171137092bb6e6d4960504cf1c6397992d182839a325d.
No installed runtime/default archive replacement yet; APK21 remains baseline.

Actual Mali-G925 GPU execution passed five phases: valid add/exchange/CAS/read,
positive OOB, negative OOB, diagnostic null, restored valid. All 320 target/result
words matched; invalid atomics returned zero without modifying target or sentinels.
Five dispatches/submissions; gaps=0, Core liveZero=1. This null phase exercises
internal experimental compatibility, not a claim that nullDescriptor is exposed
as a supported Vulkan feature. Elapsed 2.474 seconds for the complete gate.
Prior 23 GPU scenarios also passed on this exact candidate (58.268 seconds).
Artifacts: E:/Mavk/artifacts/texel-atomic-gpu-20261021/results.tar.gz and
regressions.tar.gz; extracted reports and combined strict verifier PASS retained.

Implementation is real atomic execution, not load/store substitution. New
ValidateTexelAtomics admits a bounded Buffer/R32i/R32ui/32-bit integer slice and
explicitly rejects unimplemented image-pointer forms. Extended pinned optimizer
recognizes the descriptor pointer separately, clones ImageTexelPointer only in
the valid branch, and queries size only after initialization checking. Original
atomic scope/semantics/value/comparator remain intact. Host structural regression
passes 22 assertions for four atomics/four size queries and guarded branches;
prior compiler regressions pass. CTS NOT RUN; only R32_UINT compute execution is
GPU-proven here, not all accepted integer operations, formats or shader stages.
No full robustness2/nullDescriptor/ordinary-DXVK support claim follows from this.

Reproducible external-source delta:
compatibility/shaders/spirv-tools-texel-atomics.patch, based on exact backed-up
pre-atomic source below. Reverse git apply --check --ignore-space-change PASS
(research sources have CRLF). Use after existing texel-operands/buffer patches.
New archive/layer builds explicitly pair both new archives. Installed/default
archive pins remain unchanged until real-consumer integration is verified.

## Historical prerequisite and implementation plan

Installed baseline stays APK21c5f68760/runtime8de92183/layerc2a12a21,23common
GPU cases and visible Steam/Muck menu+Settings/input PASS. Production untouched.
Buffer package finite checkpoint complete;fullrobustness2 remains unproved.

## New native gate actually executed

Existing storage-texel consumer reused with all-valid coordinates and native
R32_UINT storage-texel-atomic format feature explicitly checked. Four hardware
operations:add,exchange,compare-swap,add0/read.64target/result words matched.
Only VK_LAYER_HYBRIS_compat enabled;LD_DEBUG confirms its init and absence of
Mavk layer init. No Mavk transformation,unguarded OOB or null access in this run.
Elapsed1.076sec. Native Mali-G925 hardware renderer selected,not CPU.

Consumer SHA25648329a97f55eab01734140e0c89de0e7079cc6fd86c3ba91fd1fce1a07083bbe.
Artifacts E:/Mavk/artifacts/storage-atomic-native-20261021/:
layer-execution,results.tar.gz,results/atomic-native.stdout/.stderr.
Sources:storage-texel-atomic.comp,existing layer-storage-texel.hpp atomic mode;
control storage-atomic-native-control.sh. This proves native execution ONLY,
not Mavk image atomic support. No feature bit enabled/advertised.

## Original implementation boundary (now addressed above)

GuardCompiler word preflight explicitly rejects opcode60 OpImageTexelPointer.
Pinned InstBindlessCheckPass handles integer BUFFER atomics227..242 only when
pointer is AccessChain. Atomic image pointer is instead OpImageTexelPointer
with an UniformConstant image-variable pointer,coordinate and sample operands.
Existing CloneOriginalReference assumes image-based references start from a
loaded image object;that assumption must not be applied to an atomic pointer.

Next proper implementation:explicit atomic-texel reference analysis,clone its
ImageTexelPointer INSIDE the valid descriptor branch,then image-load/query-size
INSIDE initialized branch and coordinate bounds check before original atomic.
Preserve original scope/semantics/value/comparator and atomic result/side-effect;
invalid branch returns0 without executing atomic. Do not substitute load/store
or marker-based fake operations. Keep unsupported image dimensions/widths/forms
explicitly rejected. Start native-supported R32i/R32ui Buffer integer forms;
no claim for all2D/MS/64bit/float/image-atomic semantics from that slice.

Relevant external source before atomic changes:
inst_bindless_check_pass.cpp SHAea3af585a7f41689338f3acc03dc740e8ef1e8f56ea0238344899b0a000eadd3
inst_bindless_check_pass.h SHAfc18c09f54f06b4feee77cd87f4b095a4f6af0684ac838ff2a01abdad080bc66.
Both are inE:/Mavk/research/spirv-tools-v2022.4/source/opt/.
The above hashes describe the backed-up source/header before atomic changes.
Keep default verified optimizer0d8a0607/frontendf0a0c057 and build isolated
new archive/layer for implementation. Existing build-texel-optimizer.sh can
compile affected object into a fresh archive copy;preserve all prior patches.
