# Finite buffer/texel package checkpoint

This is a practical upstream-DXVK P0 implementation slice,NOT closure of the
whole robustness2 feature. Stop expanding rare descriptor shapes to perfect one
subsystem. Guarded storage-texel atomics now have a bounded implementation and
GPU proof below; this does not close general image-atomic compatibility.

| Area | Classification | Evidence and remaining scope |
|---|---|---|
| Nested/copy buffer pointers | EMULATED slice | Canonicalize chains/identical pointers;GPU read-zero/write-suppression. Phi/select/physical provenance not newly admitted |
| Offset math | EMULATED slice | Generated32bit overflow saturation,64bit narrowing checks,mixed signedness bitcast fix;application math preserved |
| Composite access/copy | EMULATED slice | Struct/fixed-array/column-major lowering,common memory masks,plain/two-mask CopyMemory;6forms1152words. Scoped copies/spec-sized arrays/row-major andNonUniform GPU proof open |
| Uniform texel fetch | EMULATED bounds + NATIVE format | RGBA8UNORM/UINT,nonzero offsets/trailingbytes,positive/negative/large coordinates,None/ZeroExtend;512components. Not every format/flag |
| Storage texel read/write | EMULATED bounds + NATIVE R32_UINT |4texel view within32element allocation;64words,zero reads/discarded writes/positive data/sentinels. Candidatec2a12a21 |
| Integer buffer atomics | Existing EMULATED slice | Earlier32bit null/valid atomicAdd gates recorded in RUNTIME_SHADER_COMPILER_2026-10-18.md;not repeated or expanded here |
| Vertex fetch | NATIVE tested slice |2float formats/3firstVertex/1536components,with/withoutMavk. Partial/packed/integer/instanced/indexed/dynamic-size contract not fully proved |
| Storage-texel atomics | EMULATED bounds + NATIVE tested R32_UINT operations | ca87ea54: add/exchange/CAS/read;5phases320words,valid/OOB/negative/diagnostic-null/restored. R32i and other admitted integer op/stage combinations not GPU-proven; other image dimensions/widths/pointer forms explicitly reject |
| Update-after-bind | Mixed/native selected slice |Actual Sarek sampler heap preserved;general guarded UAB deferred,not complete |

All Mavk evidence above is custom host/GPU/integration testing. Vulkan CTS has
NOT run. Raw-driver/native conformance is not evidence for transformed shaders.
InstalledAPK20 passed22common GPU cases and visibleSteam/Muck at menu level.
Candidatec2a12a21 passed23GPU cases plus6aggregate forms and visibleMuck;
APK21 is now installed with8de92183 and ALL23common GPU cases PASS. Visible
Steam/hardwareZink/Mali and normal Muck menu/Settings/input PASS with new
runtime,816873guarded commands,no observed Mavk errors. APK20 rollback retained.
Atomic candidateca87ea54 also passed prior23cases and6forms1152words;visible
Muck menu/Settings/input PASS with mapped candidate. STORAGE_ATOMICS_NEXT_2026-10-21.md
records exact sources/archives and installation status. No broad feature bit
was enabled by this proof.
No all-games claim. STORAGE_TEXEL_2026-10-21.md has exact provenance.

Keep robustBufferAccess2/nullDescriptor/textureCompressionBC aggregate feature
claims unchanged until their entire mandatory contract is implemented/proved.
Native passthrough remains preferred;no new CPU-copy layer was added.
