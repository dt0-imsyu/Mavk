# Shared Vulkan loader adapter — tracking and opt-in guarded execution

Runtime controls (read once at device creation):`MAVK_LAYER_TRACE=1` enables
per-command/update/template/secondary event messages;default is off. Errors and
teardown counters remain visible. `MAVK_GUARD_REPORTS=0` independently disables
shader diagnostic stores and retirement readback,not access guards.
`MAVK_LAYER_PROFILE=1` enables bounded timing summaries;not visible-FPS telemetry.
TRACE_POLICY_2026-10-22.md records controlled recording-cost evidence.
The adapter description below began2026-10-18;current implemented/deferred
scope is in docs/STATE.md and docs/BUFFER_PACKAGE_CHECKPOINT_2026-10-21.md.

VK_LAYER_MAVK_compat is a real explicit glibc Vulkan layer connecting the existing
Core borrowed-device context and VulkanDescriptorUpdates frontend. It does NOT
implement or advertise nullDescriptor yet;native feature queries are unchanged.

Hooks:instance/device lifecycle,physical enumeration/groups,buffers/images/views/
samplers/R32 texel views with native ownership,descriptor layouts/sets/pool
retirement,ordinary writes/copies and core/KHR update templates. Shader/layout/
pipeline hooks retain bounded source and compile guards via common GuardCompiler.
Default tracking forwards original native pipeline input. MAVK_LAYER_GUARDS=1
instead creates actual guarded native shader stages,augmented exact layouts,
and materialized public/private descriptor bindings before direct/basic indirect
draw or dispatch. This experimental mode is not public feature admission.
Foreign handles use explicit downstream dispatch,not global loader trampolines.
Default non-layer Core callers still use normal Vulkan loader entry points.

In default mode native operations pass through unchanged. Unsupported shadow schemas or allocator
contracts emit MAVK_LAYER_GAP,not emulation claims. Failed updates invalidate
affected shadow sets. Custom buffer allocation callbacks stay native/untracked;
the layer cannot extend caller-owned allocator lifetime without a contract.
Tracked buffers have actual native RAII ownership,not placeholder lease tokens.

Guarded primary command buffers keep real native pipelines/layouts/descriptors
alive through recording and resubmission,until valid reset/free. The caller must
complete GPU use before those operations,as Vulkan requires. Failed admission
rejects guarded work rather than silently using unguarded/stale descriptors.

Guarded public updates are logical only;actual native updates are emitted during
materialization. Common device-owned logical-null UBO/SSBO backing is connected,
and R32 texel/color-2D backings are connected. Types are reflected from validated
original shaders,not guessed. Other image/texel shapes/forms reject. Feature
queries remain false. Color-2D initialization is a retained one-shot prelude in
the same Submit/Submit2 call,with a single unprotected queue/family restriction.
Original wait/signal/pNext/fence inputs stay unchanged;no CPU waits are added.
MAVK_TEST_IMAGE_INIT and MAVK_TEST_NULL_IMAGE are diagnostic fault-injection
switches only,not production feature admission or normal application behavior.

Not ready:broader image/texel forms/full feature admission,extended/immutable layouts,
variable counts/dynamic offsets,full image/texel format semantics,secondary and
indirect-count commands,descriptor update-after-record semantics. No complete
nullDescriptor,D3D11-device or game compatibility claim.
Evidence:docs/GLIBC_LAYER_INTEGRATION_2026-10-18.md. Continue with the SAME real
Sarek consumer,not unrelated Suite cases. Keep nullDescriptor off until correct.
Latest evidence:docs/SAREK_GUARDED_EXECUTION_2026-10-18.md.
Current color-2D/queue proof:docs/NULL_IMAGE_LAYER_2026-10-18.md.
