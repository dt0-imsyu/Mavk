# Real Sarek consumer integration — 2026-10-18

## Measured result

Real DXVK-Sarek3.0.2,b6653094a2b8f3b824ffcf8f50aaa3b3da41263b,
ARM64 Linux native build. Unmodified D3D11 implementation,not a mock factory or
custom Mavk command. Consumer calls standard Vulkan loader and then actual
`D3D11CreateDevice`,requesting hardware/feature level11_0. If successful it performs
an offscreen D3D11 clear/copy/map with16384 component checks. That final workload
has NOT executed yet because device creation currently fails.

Runtime: exact DroidDeck linuxfs-r9 glibc/library subset,pinned hybris ICD4073873,
isolated proot under Termux debug UID. This is NOT a full DroidDeck app UID,
Wine game session,Windows guest,swapchain or visible D3D11 frame PASS.

| Stage | Actual result |
|---|---|
| ICD/standard Vulkan loader | PASS |
| vkCreateInstance | VK_SUCCESS |
| Physical device | Mali-G925-Immortalis MC12,vendor0x13b5,non-CPU |
| Basic graphics vkCreateDevice | VK_SUCCESS |
| Original Sarek D3D11CreateDevice | 0x80004005;BC aggregate feature gate |
| Opt-in patched Sarek BC admission | All14 D3D formats PASS |
| Patched Sarek D3D11CreateDevice | 0x80004005;nullDescriptor gate |
| D3D11 device / first frame / game | NOT PASSED |

`E:/Mavk/artifacts/sarek-consumer-run1-20261018` preserves original artifacts/logs.
`E:/Mavk/artifacts/sarek-consumer-run2-20261018` preserves patched artifacts/logs.
Both are bounded runs,exit20,not hangs. No VM or production U-AVF modifications.
The SDL offscreen headless-surface warning is not the blocking error:Vulkan
instance creation and Mali enumeration subsequently succeed. Android library
lookup warnings likewise did not prevent actual native device creation.

## Concrete fix already tested

`experiments/droiddeck/sarek-bc-format-admission.patch` is an opt-in alteration of
Sarek's admission logic,enabled only by `MAVK_SAREK_BC_FORMATS=1`. Each of14 exact
D3D BC identities must actually report sampled image,linear filter and transfer
source/destination flags. It does NOT set textureCompressionBC true or request
that unsupported aggregate feature at vkCreateDevice. Missing formats still
reject the adapter. Existing rebuilt hybris BC5 RG32F layer handles actual API
operations;prior native sample/filter gates cover it. This is not full BC family
conformance or a performance claim.

Actual patched Sarek log:

```
Mavk D3D BC admission: 14 formats PASS; aggregate feature unchanged
Skipping: Device does not support required feature 'nullDescriptor' (extension: VK_EXT_robustness2)
D3D11CreateDevice: Failed to create a DXGI factory
```

Patched artifacts SHA256:

- libdxvk_d3d11.so:a7f69367d28d73e33434bb54b8add107445522a02648c87694d3622141a21e66
- libdxvk_dxgi.so.0:ab5500ed0486af13f38f42a76059e37f478fd0665a19a828abd503092c8ff65c
- BC layer:179870fcf9e1cdfe0495d829df547b53bd950c63e2981ff162b53a4d02687654
- Consumer:733f6dfb6f994a04311bd5186864d08bf125871a286b4df1f1a9070a9e53f576
- SDL3:4cf3e1fa515d99c7bb2636723519747c3d8022e58c98c0d6a0f455d46dc4441c

## Remaining integration boundary and cost

Later checkpoint:docs/GLIBC_LAYER_INTEGRATION_2026-10-18.md records a real loaded
tracking-stage Mavk layer and Core dispatch fix. The following describes the
run2 boundary before that integration;shader/command/null semantics remain OPEN.

This is an unfinished shared standard-API integration,not a new vendor/hardware
NO-GO. Mavk's guarded descriptor modules work through explicit trusted calls in
native gates. The loaded glibc Vulkan chain currently contains hybris/BC,but no
Mavk loader layer translating the consumer's descriptor/shader/command lifecycle
into these modules. Therefore Sarek correctly observes nullDescriptor=false.
Changing that bit or deleting the requirement would hide the actual missing
semantics and is NOT an acceptable fix.

Next integration must target the same real consumer,not more unrelated Suite cases:

1. Loadable shared Mavk Vulkan layer with per-instance/device dispatch and Core's
   borrowed native device context;track resources/layouts/pools/sets and feed
   ordinary writes,copies and templates to the existing trusted frontend.
2. Pipeline-layout/shader hooks plus command-buffer bind/submit integration.
   Reserve the guard set,materialize immutable generations,and retain resources
   until GPU completion. Do not silently redirect only a subset of command APIs.
3. Admit actual Sarek SPIR-V/stages/descriptor forms. Current compiler/gates use
   a bounded Vulkan1.1 experimental ABI and selected operations;full image/atomic/
   format/dynamic semantics are not proven. Only advertise nullDescriptor when
   its required semantics are implemented,then rerun this consumer and fix the
   next actual blocker.

Cost:multiple integration milestones across loader dispatch,command lifetime and
shader semantics,not a one-line admission fix. The existing CPU/GPU modules can
be reused;there is no evidence requiring a new VM transport or a DroidDeck rewrite.
Do not estimate successful completion from synthetic guard PASS alone.

## Reproduction and scope

Source/dependency preparation and GCC/Meson cross tooling scripts live in
`experiments/droiddeck`;all downloaded tools and build output stay on E.
prepare-sarek-native-source.ps1's ExperimentalD3dBcAdmission switch applies only
the scoped patch to a fresh pinned source tree. Build without it for baseline.
The nested dxbc-spirv SPIRV-Headers pin is included;no apt packages were installed
to C's WSL filesystem. Source licenses remain in the external source trees;
Sarek code is zlib/libpng,not Mavk-authored original code.

run-sarek.ps1 packages actual ELF outputs with SHA256 checks,uses a fresh isolated
slot,collects stdout/stderr and distinguishes native Vulkan device PASS from
D3D11 device/clear PASS. Existing evidence is never overwritten. Subsequent source
or layer changes should rerun this real consumer rather than grow a synthetic suite.

Sarek remains an experimental Mali backend option. Preserve DroidDeck's stock
DXVK for supported Adreno devices. After first real Sarek frame,compare current
upstream DXVK requirements precisely and implement feasible missing semantics
in common Compatibility. Do not equate DXVK with WDDM,device creation with game
compatibility,or offscreen clear/readback with windowed presentation.

No commit/push authorized;none performed for these changes.
