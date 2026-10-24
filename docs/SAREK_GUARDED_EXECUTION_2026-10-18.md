# Sarek shader execution through the standard API — 2026-10-18

## Result and scope

PASS: real pinned Sarek video-blit vertex/fragment shaders execute on Mali-G925
through the glibc Vulkan loader -> Mavk layer -> existing hybris/BC -> vendor
Vulkan chain. This supersedes run8's compile-only checkpoint.

**NOT a D3D11 frame or D3D11 device PASS.** The ordinary Vulkan startup fixture
uses the actual Sarek shader pair,not D3D11-generated commands. Afterwards the
same executable calls the real Sarek D3D11CreateDevice,which still rejects
nullDescriptor and returns 0x80004005. No requirement/feature bit was removed.

Runtime: exact DroidDeck linuxfs-r9 library subset in Termux debug UID/proot,
not the complete DroidDeck app/Wine session. No production app or VM changes.

## Implemented boundary

- Opt-in MAVK_LAYER_GUARDS=1. Default remains native tracking/passthrough.
- Exact native descriptor layouts retained,including stage flags and original
  push ranges;one additional private set. Shared NativeDescriptorAdapter can
  borrow these layouts with a real owner,without destroying them twice.
- Actual common-compiler SPIR-V replaces downstream native pipeline stages.
  Shader modules are temporary;pipelines/layouts and resource leases persist.
- Bind/draw/indexed draw/dispatch and basic indirect variants materialize common
  descriptor generations and bind the private guard set before execution.
- Primary command buffers retain generations until valid reset/free/pool
  retirement,not first fence completion:executable buffers can be resubmitted.
- Command end emits shader-write -> host-read barrier for diagnostic buffers.
  Diagnostics read only on the final retained generation's retirement,under the
  caller's normal Vulkan completion/external-synchronization obligations.
- Failed guarded admission marks the command invalid and rejects submit instead
  of silently drawing with stale/uninstrumented descriptors.

## Evidence

`E:/Mavk/artifacts/sarek-consumer-run10-20261018`:

```
VULKAN_SAREK_BLIT_PIXELS_PASS components=65536 submissions=4 resubmission=1 indirect=1 bounded=1
MAVK_LAYER_COMMAND_END guarded=1 draws=3 submissions=4 diagnosticWords=572 remainingCommands=0
```

Four readbacks:64x64 full color,direct command-buffer resubmission,the same
shader/color via vkCmdDrawIndirect,and a 2x2 scissored draw with UBO range16.
The last draw verifies the exact zeroed out-of-range-field output;572 diagnostic
words confirm actual guard execution. Valid draws record zero guard errors.
Tracked sets/resources/templates/shaders/layouts/pipelines all retire to zero,
gaps=0. Machine-readable layer-summary reports guarded_draw_readback_pass=true
and d3d11_frame_pass=false. Process20 is the subsequent expected Sarek rejection,
not failure of the preceding GPU draw proof.

Run9 was the first guarded direct-draw proof:49152 components/3 submissions,
resubmission and bounded readback,572 diagnostic words.

`E:/Mavk/artifacts/sarek-consumer-run11-20261018`:same current binaries with
guard mode disabled,49152 components/3 submissions,indirect/resubmission PASS,
native input unchanged by Mavk. Still the same real Sarek nullDescriptor gate.

Layer9 SHA256:
20a4195261b06c310cb6a694faa3db297375939d52caffc68c52f76397549b27

Consumer7 SHA256:
9483e72e82f7d9081c30a056756e7bff1132f5e51e6951f518339ada446618af

Shared-Core regression after borrowed-layout changes:
`E:/Mavk/artifacts/borrowed-layout-regression1-20261018`.
Existing separate image/sampler GPU gate:2048 components/256 guards,nearest+
bilinear,SyncVal0 warnings/0 errors,Core resources zero. Fresh report SHA256
8fea318408d27751d52d5d84e25a6c039b73f3659bf3db5fcb3d5a6fe410cd1b.
APK SHA256 4d0369fc1cd6715ed770c803856151995706fd4883c8bd66c27e9115776726e6.
This validation result applies to the Android regression,not the glibc run.

## Concrete remaining blocker / next work

nullDescriptor remains FALSE. Current API adapter's trusted resource resolver
accepts bound descriptors only;it does not yet supply typed native fallback
resources for logical null. Guarded mode does not forward/accept real consumer
null writes as a supported public feature. This is missing implementation,not
a vendor/hardware impossibility. Do not rerun this unchanged D3D11 gate alone.

Next:connect null-write/template/copy state to device-owned typed fallbacks and
guarded shader semantics,never forward unsupported native null handles. Match
image dimension/format/view/subresources and operation semantics;retain real
backings for recorded work. Complete required admission before advertising.
Other known API gaps:dynamic offsets/extended layouts,secondary commands,
indirect-count/byte-count commands,update-after-record/update-after-bind,
broader shader/image/atomic/format forms and complete feature accounting.
The current bounded primary-buffer proof is not full Vulkan robustness2.

Keep Sarek opt-in;stock Adreno DXVK untouched. Upstream DXVK gap closure remains
the strategic goal after the first real Sarek D3D11 frame. No commit/push.
