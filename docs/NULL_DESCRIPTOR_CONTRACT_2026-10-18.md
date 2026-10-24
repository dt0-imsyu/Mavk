# Remaining nullDescriptor contract and submit rollback — 2026-10-18

## Decision

Do NOT expose nullDescriptor yet. Static descriptors plus color-2D tests do not
cover its actual contract. There is no new vendor blocker:the missing work is
Mavk semantics/API coverage. Do not repeat the proven green-frame fixture or
the unchanged Sarek device filter instead of implementing that work.

The pinned real Sarek device-info source requires nullDescriptor,but marks
robustBufferAccess2 and robustImageAccess2 optional (required=false). Therefore
do not expand this immediate task into implementing BOTH entire robustness2
features. Keep their bits truthful and distinct from nullDescriptor semantics.

## Primary-source audit

Read directly from Khronos Vulkan-Docs upstream on2026-10-18. Unlike the previous
cancelled rendered-reference fetch,the raw source was successfully retrieved.
These are specification requirements,not assumptions based on other drivers.

- [Feature definition](https://github.com/KhronosGroup/Vulkan-Docs/blob/main/chapters/features.adoc):
  nullDescriptor controls valid null resource/view descriptors. It is separate
  from the two robust-access2 feature bits.
- [Descriptor updates](https://github.com/KhronosGroup/Vulkan-Docs/blob/main/chapters/descriptorsets.adoc):
  null loads return zero;stores/atomics have no effect. Covers applicable buffer,
  texel-buffer and image descriptors,including dynamic buffer descriptor types.
  Null buffer offset must be0 and range VK_WHOLE_SIZE. This current frontend
  already validates that contract for its static buffer subset.
- The same source's VUID00325 still requires a real sampler when not immutable.
  VUID07683 forbids null input-attachment image views;do NOT add a fictitious
  requirement to emulate null input attachments or null sampler handles.
- [Vertex bindings](https://github.com/KhronosGroup/Vulkan-Docs/blob/main/chapters/fxvertex.adoc):
  null vertex bindings are part of the feature. Memory-sourced values are zero;
  missing components retain normal(0,0,1) filling. Missing alpha is NOT universally
  zero. This is a real gap:the current layer has no null-vertex fetch emulation.
- [Index binding](https://github.com/KhronosGroup/Vulkan-Docs/blob/main/chapters/drawing.adoc):
  null index bindings interact with maintenance6;when that feature allows the
  null binding and nullDescriptor is enabled,every index is zero. Treat this as
  conditional on enabled/advertised features,not a demand to implement every new
  device-address extension appearing in today's spec. Current layer has no fix.
- Acceleration-structure/tensor null behavior is also conditional on the relevant
  extensions. Inspect actual enabled capability profile before adding tasks;
  don't invent Mali support or silently advertise unsupported Mavk operations.

### Exact remaining implementation map

| Area | Current evidence | Remaining before broad admission |
|---|---|---|
| Static UBO/SSBO | Native GPU null/range/atomic gate,real backing | Dynamic descriptors/offsets,broader admitted access forms |
| Texel buffers | R32_UINT GPU;U/S/float reflection | Remaining required formats/operations/image atomics |
| Color2D | Actual Sarek CbCr-null guarded pixel proof | Other legal shapes/views/MS/depth and required query/atomic forms |
| Sampler ownership | Real sampler leases in common test | Immutable sampler layout/schema ownership in actual layer |
| Vertex binding | Not implemented | Null binding hooks plus exact zero/fill fetch semantics |
| Conditional index binding | Not implemented | Honor maintenance6+nullDescriptor enabled contract |
| Shader coverage | Selected forms tested;unsupported reject | OpImageTexelPointer/image atomics explicitly rejected in compiler |
| Command/update API | Primary/direct/basic indirect;recorded leases | Dynamic/secondary/indirect-count,late updates and remaining compatibility |

Rows describe current incomplete implementation,not seven new fake feature bits.
Next work should target these real gaps,preferably the resource/operation forms
needed by the actual consumer. First real D3D11 frame remains unmet.

## Submit rollback implementation/test

Added private MAVK_TEST_SUBMIT_RETRY fault injection:reject exactly one fully
prepared image prelude BEFORE calling the driver,return OUT_OF_HOST_MEMORY.
The normal local destruction path must release its command pool and reservation.
The same original application command buffer/fence is then retried once;no extra
fence reset or VM restart. Test flag is off in normal mode;no driver OOM induced.

The [command-buffer source](https://github.com/KhronosGroup/Vulkan-Docs/blob/main/chapters/cmdbuffers.adoc)
explicitly requires resource/synchronization state preservation for
vkQueueSubmit OUT_OF_HOST/DEVICE_MEMORY;otherwise DEVICE_LOST is required.
That supports the rollback design,but this controlled test is NOT a measurement
of Mali's actual failure behavior and does not claim recovery from DEVICE_LOST.

Actual latest evidence:

- `E:/Mavk/artifacts/sarek-consumer-run21-20261018`:Submit1+CbCr fault+one pre-driver
  rejection/retry PASS. Green/bounds pixels65536/4,one successful init batch,
  gaps0/all cleanup/Core liveZero1.
- `E:/Mavk/artifacts/sarek-consumer-run22-20261018`:same through Submit2 PASS.
- Both record `VULKAN_IMAGE_SUBMIT_RETRY_PASS count=1`;exactly one test rejection.
  Failed prelude was not retained or marked initialized;successful retry retained
  exactly one. No CPU wait was added by Mavk.
- Run23:retry/null-injection flags OFF,normal guarded colored frame65536/4 PASS.
- Run24:default tracking/native shader frame49152/3 PASS. No forced rejection.
- All then invoke actual Sarek D3D11CreateDevice,which still fails at the truthful
  nullDescriptor feature gate. Exit20 is expected for that final stage,not an
  overall D3D11 PASS. This remains the DroidDeck-library subset/Termux UID.
- glibc tests do not enable SyncVal;previous Android validation results must not
  be misattributed. Diagnostic record truncation limits from NULL_IMAGE_LAYER
  still apply to the intentionally heavy null workload.

Layer17 SHA256:
fcea9184fa9afe20ec12ff25352586131f31cdb0fef6c5907c44dbf6ae7117cf

Consumer11 SHA256:
681b7c9e799d10431dd4e6b38cc9757d05c8b0b17dc0ca834be14ec219211273

Nontrivial timeline/binary semaphore and submit-pNext tests remain open;original
structures are preserved by code review,but do not claim complete runtime proof.
No production/VM/storage/stock Adreno changes,root,commit or push. Goal active.
