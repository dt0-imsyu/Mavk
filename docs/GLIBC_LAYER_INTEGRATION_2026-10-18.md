# glibc Vulkan layer integration — 2026-10-18

## Actual PASS,limited scope

Real ARM64 VK_LAYER_MAVK_compat loaded by DroidDeck's glibc Vulkan Loader above
hybris/BC. It connects shared Core and descriptor frontend,NOT null emulation.
All native feature queries remain unchanged. This is Termux debug UID/proot,
not a full DroidDeck/Wine game session.

E:/Mavk/artifacts/sarek-consumer-run6-20261018 proves:

```
MAVK_LAYER_INSTANCE mode=tracking nullDescriptor=not-advertised
MAVK_LAYER_DEVICE sharedCore=1 descriptorFrontend=1 mode=tracking
MAVK_LAYER_UPDATE common=1 writes=1 copies=1
MAVK_LAYER_DEVICE_END commonWrites=1 commonCopies=1 gaps=0 remainingSets=0 remainingBuffers=0
```

The same real Sarek consumer's startup creates/binds a buffer,allocates two
descriptor sets,writes/copies them,frees/resets the pool and destroys resources.
Vulkan instance/device and standard lifecycle PASS. Sarek then creates another
instance through Mavk,passes14 BC formats,and rejects nullDescriptor. D3D11
device/frame remain NOT PASSED. No feature bit bypass or disabled requirement.

Layer ELF SHA2569a1b4c3ce91eb093e4ba1df5cd584bf2402cea6e357ec0d71b40060df716f975.
Consumer SHA2560b3a4b5c59d7484335d4e85e3d143de72a8e49f4d30bc34a8ff0cd8ce6485ee7.
Manifest SHA2560c1a48e54a4f75d0104b2a30e79e40ac45a48390f4e914ba15d23cc14d99dbf2.
ELF/build:E:/Mavk/artifacts/mavk-glibc-layer2-20261018.

## Concrete failures fixed

Run3 did NOT load Mavk;loader tracing in run4 proved implementation_version must
be a string in this manifest. Fixed it;these runs are NOT layer PASS,preserved.
Run5 actually loaded Mavk but Core's global vkGetPhysicalDeviceMemoryProperties
aborted with an invalid physicalDevice. Layer handles belong to the DOWNSTREAM
chain,not application-side loader trampolines. This was not a GPU/VM failure.

Added common VulkanDispatch:normal loader addresses for existing callers,or
explicit next-layer instance/device functions. Core resource/submission operations
and NativeDescriptorAdapter now use it;layout/snapshot destruction keeps the same
dispatch. This also avoids recursive tracking of internal guard/fallback resources.
Run6 verifies the fix. No VM,firmware,production U-AVF changes.

## Required existing GPU regression

Because common Core/descriptor dispatch changed,rebuilt the diagnostic APK and
reran the EXISTING separate-image/sampler gate with synchronization validation.
No new Feature Suite case. E:/Mavk/artifacts/core-dispatch-regression1-20261018:

- APK4ff38f72b746da36b83213c5e13b67c34b5d8dac7cade15f4ad7c19571e4d0a4.
- core-separate-samplers.fresh.json SHA256
  8fea318408d27751d52d5d84e25a6c039b73f3659bf3db5fcb3d5a6fe410cd1b.
- 2048components/256guards,nearest+linear,Corezero,SyncVal0/0 PASS.
- To exclude stale persisted output,the old app-owned report was renamed to an
  explicit backup and the gate rerun before fresh capture. Probe stopped.

glibc tracking itself did NOT use a glibc validation layer;do not misattribute
Android SyncVal results to that separate loader-chain run.

## Next real integration boundary

Extend this adapter with existing resource/template modules,shader/pipeline
instrumentation,command bind/submit and GPU-completion lifetime. Bounded compiler
gates do not establish every nullDescriptor operation or Sarek's Vulkan1.3 shader
forms. Keep the feature false until implemented. Target the same real Sarek
consumer;do not replace this task with isolated feature demos or admission hacks.
No commit/push,production APK,Windows/Ubuntu/display/storage changes.
