# Buffer aggregates: bounded shared lowering, not full robustness2

The pinned pass explicitly checks only descriptor initialization for arrays,
structs and matrices. Direct loads from the buffer variable could also miss its
AccessChain recognizer. This leaves whole-object operations without byte bounds.

The shared AggregateBuffers pass lowers buffer composites before access-chain
combining and existing bounds instrumentation. Struct/array/matrix members are
recursively loaded or extracted/stored as scalar/vector references. The original
Vulkan strides, member offsets and matrix-layout decorations remain on the
canonical global access chain. Memory Volatile/Nontemporal bits are retained;
Aligned becomes1 (safe conservative guarantee after splitting an object).
NonUniform pointer/load decorations are propagated; broad bindless admission
does not follow from that implementation detail.

Budgets:depth32,1024 members/leaves per operation,existing bounded module/output
limits. Specialization-sized arrays, pointer leaves and scoped memory operands
are explicitly rejected rather than silently claiming safe access. Native
Function/Private/Workgroup memory is not subjected to buffer descriptor guards.

## Actual evidence for candidate9c9d6680

- Basic whole-struct (two vec2 members):three descriptor-range phases verify
  zero for an unreadable member, suppressed out-of-range writes and full copy.
 192input/output words checked.17 prior GPU scenarios alsoPASS,gaps0/Corezero.
- Four concrete forms:whole struct,fixed array,column-major matrix,
  Volatile|Aligned|Nontemporal load/store. Three phases each,768checked words.
  Input values are normal FP32 bit patterns as well as valid integer data;
  matrix checks do not rely on denormal preservation. AllPASS/gaps0/Corezero.
- Input/output spirv-val Vulkan1.1 and compiler assertionsPASS.
- No CTS/SyncVal/nonuniform/row-major/full robustness2/FPS claim.

Artifacts under E:/Mavk/artifacts/:

- aggregate-frontend-20261021/libmavk-shader-guards.a
  SHA25662cb42d1668f9bca699086a3c7dee8393c846501580013acd8b92823357b4e4f
- aggregate-layer-20261021/libVkLayer_mavk_compat.so
  SHA2569c9d66804e1036e9a99ff7056dab6f5c623c099851b131a7c03206e5bd082cd7
  results.tar.gz:basic case+17regressions.
- aggregate-variants-consumer-20261021/layer-execution
  SHA2566fe6b92c85d3f6ff8a71f2436507de4951f950fe2eabd1a403b7cbdbb408e046
  results.tar.gz:four aggregate forms,three ranges each.

This candidate is NOT installed. Installed APK19/runtime73ec2df4 remains the
verified nested/overflow/wide-index baseline. Its common17GPU scenariosPASS;
Android keyguard prevents visible Steam/Muck verification. Temporary USB stay-on
and session suspend preferences were restored. No production U-AVF changes.

## Historical plain-copy implementation checkpoint

Plain OpCopyMemory involving a descriptor buffer is lowered to a typed load and
store, then the SAME aggregate/range path. Memory-operand CopyMemory forms are
explicitly unsupported for now,not ignored. Host fixture compiles with four
bounds checks; hardware verification is still pending. Do not count this as
candidate9c9d evidence or a common runtime update.

## CopyMemory hardware result — candidate b46cfb43

Plain buffer OpCopyMemory now PASS on Mali through the same typed/aggregate
path:partial-read zero,partial-write suppression,full valid transfer.19 total
cases (copy+aggregate+17prior) PASS,gaps0/Corezero. Additional five-form run
(struct,array,col-major matrix,memory flags,copy),three ranges each,960words
PASS. No VM/Android unlock needed for these offscreen GPU checks.

E:/Mavk/artifacts/aggregate-copy-layer-20261021/ contains results.tar.gz and
variants-results.tar.gz,strictly verified with verify-nested-buffer.py --copy
and --aggregate-variants-only --copy. Layer SHA256
b46cfb43cc66bff9209821b1964eccfbef743aa7ceea0e368adef982fb651fa5;
frontend aggregate-copy-frontend-20261021/libmavk-shader-guards.a SHA256
6e330b19796c326dc7d61891a51e030dea4b4721f7681da78e68f3157a66773b.
Copy shader SHA256689f9beb4f62b8b49cfc6fce7450b7e4d074eb4f9c1e50910516ad70357cd12e.

Finite aggregate package checkpoint:basic scalar/vector members,struct/fixed
array/col-major matrix lowering and plain copy implemented; common load/store
memory flags preserved. Scoped copy/memory forms,spec-sized arrays,row-major
GPU proof,NonUniform GPU proof and full vertex/texel robustness remain open.
Keep robustness2 unadvertised. CTS not run. Do not expand rare combinations
instead of the next concrete missing contract/real consumer.

InstalledAPK19 remains unchanged; do NOT silently update runtime/source assets
to this candidate before application regression. Steam CDP hardware renderer is
proved, but physical display is behind Android keyguard. No full-game/FPS claim.

## Real consumer regression and Vulkan target correction

Candidate b46cfb43 was loaded by real Muck process7391 via the existing Steam
IPC and per-game experimental layer selector. D3D11 device and2560x1600 BGRA8
swapchain created;202826 protected commands observed,without guard compilation
errors,Mavk gaps or device-loss markers. Evidence:
E:/Mavk/artifacts/aggregate-copy-layer-20261021/muck-headless.log.
This is HEADLESS execution evidence,not a visible frame/FPS/full-game PASS.
The game was stopped and per-game override removed;APK19/common unchanged.

Next bounded correction: Vulkan1.2 instances now select SPV_ENV_VULKAN_1_2
instead of falling back to1.1 in BOTH admission and descriptor static-use.
Vulkan11/13 enum values remain0/1;new Vulkan12 value2 preserves existing ABI.
CopyMemory now preserves one shared or two distinct common memory-access masks
(Volatile/Aligned/Nontemporal);scoped/unknown masks still explicitly reject.
Alignment of lowered aggregate leaves is conservatively1.

Host SPIR-V1.4 two-mask fixture validated for Vulkan1.2. Compiler regression:
6assertions/1expected rejection (legacy1.1),2source loads with mask6 and2target
stores with mask3,alignment1. Prior compiler25assertions/9rejections and
static-use14checks/7rejections PASS. Guarded output independently spirv-val
PASS. ARM frontend9339061f915d86aab5b73904a398993c267f8824203dea3fd12f57b63f5ec3cc.
GPU check for THIS candidate remains pending;do not substitute b46 evidence.

## Vulkan1.2/copy operand hardware result — c41b2f2d

Layer c41b2f2d2f7e5467ac9da806b4454b1ce9cc0130de3a2bf490775bddb9b2e32c
with frontend9339061f PASS on Mali.20cases (newSPIR-V1.4/two-mask case plus
19prior) verified by verify-nested-buffer.py --copy-operands. Six-form run
checked1152words with partial-read zero,partial-write suppression and valid
copy;gaps0/Corezero. Artifacts:
E:/Mavk/artifacts/copy-operands-layer-20261021/results.tar.gz and
variants-results.tar.gz. Initial45second harness timeout interrupted ONLY
the regression tail;new case had finished PASS. Tail resumed with explicit
--regressions-only and completed27seconds,then strict combined verification.
No device hang or dropped failing case. InstalledAPK19 unchanged.

Finite aggregate/copy package now complete for this scope. Next concrete P0
question: actual native vertex input bounds behavior,which descriptor guards
do not intercept. A bounded two-format/three-firstVertex test uses native
robustBufferAccess before considering any new emulation. Test is diagnostic;
it cannot establish the whole robustness2 feature or justify advertising it.
