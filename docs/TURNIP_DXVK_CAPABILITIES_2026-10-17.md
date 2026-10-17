# Turnip reference / Mali-G925 / Mavk compatibility

## Decision

Do not switch to a full DroidDeck/DXVK installation yet. The exact DXVK revision
pinned by the reviewed DroidDeck fork has **seven missing mandatory feature
checks on G925**, not merely a missing loader or Mavk forwarding flag. All other
46 mandatory feature/extension rows pass capability admission. This is a source
preflight, NOT an executed D3D11CreateDevice result. No capability was enabled,
spoofed or newly implemented by this audit.

Windows remains an active adapter. Its known-good original textured Vulkan app
and production Windows/Ubuntu/H264 remain unchanged. Guarded UI ICD source work
is checkpointed in STATE.md; it is unfinished, not deployed or advertised.

## Evidence and scope

Reference is an actual full public upload, not an invented universal Turnip
profile: **Turnip Adreno 750, Mesa 26.3.0-devel (git-8aa73b4b19)**, physical API
1.4.359, numeric driver version decoded by the database as26.2.99, SteamOS0.3.0
ARM64. These labels are both present in the upload, not silently reconciled.
Report52003: https://vulkan.gpuinfo.org/displayreport.php?id=52003
Raw: https://vulkan.gpuinfo.org/services/getreportjson.php?id=52003
Profile: https://vulkan.gpuinfo.org/api/v3/getprofile.php?id=52003

The uploader's full report includes core1.0..1.4 and extension features and
properties,190 device extensions,217 format entries (including unsupported
entries), memory and two queue families. It is a reference for THAT build/GPU/OS,
not all Turnip or a new locally executed Turnip test. Platform-specific WSI and
external-memory handles cannot be copied from Linux into Android blindly.
Full profile export is preserved alongside the raw upload; exported schema
coverage is not a claim that every possible format/image usage was queried.

Mali: fresh read-only `adb shell cmd gpu vkjson`, stock GPU service,2026-10-17.
Mali-G925-Immortalis MC12, API1.3.278, driver205524992,
v1.r49p1-03bet0.f8a98b506b89b21c80a508d70abfcb3f.152 extensions,240 format
entries,55 core1.0 /12 core1.1 /47 core1.2 /15 core1.3 feature fields; full core
limits, queried extension features/properties, memory and queues retained.
Identity and complete152-extension set match the earlier actual Mavk app-UID
compute report; this full dump itself runs in GPU-service context, not app UID.
An actual consumer must still query and enable features in its own context.
Core1.4 and structures not queried by the service are UNKNOWN, not zero.
No Surface was created; Android present-support queries are not supplied here.
Format feature bitmasks do not prove every image extent/usage/sample combination.

Raw/normalized profiles, full diff, source extracts and input hashes:
`E:/Mavk/artifacts/capability-audit-20261017/`.
Raw Turnip SHA256:f9eca7ae9951405e24d24a8e5ad985f75be8c229b3ffc3370434303a24c46541.
Mali SHA256:4420c7ffa9f4d9f1126165c8b9306710d912682762a4f552d8ba1083983fe588.
Reusable offline tooling: `Mavk/compatibility/`.

## Exact consumer, not the moving wiki

DroidDeck fork0916a538aff4c08f799a4bd9fa17dd3c98b01e99 pins DXVK
**25ca63f17f34bdc05a39873ee63907d3cbbfa030** (v3.1.1-27).
Its actual dxvk_instance.h requests Vulkan**1.3**, despite current DXVK wiki's
general3.x recommendation of1.4. G925 PASSES this exact API gate; upgrading an API
version string is neither necessary nor an acceptable fix.

Required rows are extracted from that commit's getFeatureList(), not guessed:
53 entries with require=true, including required extensions and extension feature
bits. Also require graphics queue and256 push-constant bytes. G925 has both.
Turnip reference passes all53 rows; Mali passes46, fails7, UNKNOWN0.
These are necessary admission checks, not complete DXVK correctness/format/WSI
or Wine/Proton validation. Later failure is still possible after satisfying them.

Primary source:
https://github.com/doitsujin/dxvk/blob/25ca63f17f34bdc05a39873ee63907d3cbbfa030/src/dxvk/dxvk_device_info.cpp
https://github.com/doitsujin/dxvk/blob/25ca63f17f34bdc05a39873ee63907d3cbbfa030/src/dxvk/dxvk_instance.h
https://github.com/doitsujin/dxvk/wiki/Driver-support

## Priority matrix

NATIVE below means actual stock driver support, not automatically Mavk guest
support or a new correctness PASS. PASSTHROUGH MISSING means native support
exists but the corresponding Mavk API/Bridge/ICD semantics are incomplete;
it is NOT permission to advertise it. EMULATION NEEDED is an unimplemented
compatibility requirement, not an existing working emulation.

| Priority / capability | Turnip750 | Mali-G925 | Classification / Mavk consequence |
|---|---|---|---|
| P0 robustBufferAccess2 | yes | no robustness2 extension | EMULATION NEEDED: complete bounds/robustness semantics, including alignment and shader accesses; robustBufferAccess1 is NOT equivalent |
| P0 nullDescriptor | yes | no | EMULATION NEEDED: real null reads/writes and resource behavior; dummy handles alone do not implement it |
| P0 textureCompressionBC | yes | no, BC1..BC7 formats131..146 all zero feature masks | EMULATION NEEDED: format-aware decode/storage/copy/mip/view/sRGB semantics; BC1-only must NOT enable the all-BC feature |
| P0 shaderClipDistance | yes | no, maxClipDistances0 | EMULATION NEEDED: shader/primitive clipping preserving interpolants; fragment discard is not equivalent |
| P0 shaderCullDistance | yes | no, maxCullDistances0 | EMULATION NEEDED: whole-primitive culling semantics; separate from clipping |
| P0 multiViewport | yes,16 | no,1 | EMULATION NEEDED: indexed viewport routing, scissors and draw/geometry semantics; blind draw duplication is insufficient |
| P0 fillModeNonSolid | yes | no | EMULATION NEEDED: correct wireframe/point raster, clipping/culling/edge/depth behavior; polygonMode=FILL is not a fallback |
| P1 required native shader/features | yes | geometry/tessellation, independent/dual-source blend, anisotropy, int8/16/64, descriptor indexing subfeatures yes | NATIVE for direct libhybris; PASSTHROUGH MISSING for full Mavk API/guest semantics |
| P1 modern scheduling/render | yes | dynamicRendering/synchronization2/maintenance4/5/6/depthClipEnable yes | NATIVE stock; PASSTHROUGH MISSING in guest API. depthClipEnable does not substitute shaderClipDistance |
| P1 timelineSemaphore | yes | yes | NATIVE bounded Core implementation/test exists; PASSTHROUGH MISSING in current Windows ICD/Bridge timeline API |
| P1 resources/command lifetime | full driver | full driver | Mavk has bounded buffer/image/descriptor/shader/graphics/compute slices, not arbitrary DXVK command replay; general resource/lifetime/barrier coverage is PASSTHROUGH MISSING |
| P1 Windows/Linux presentation | platform-specific | native Android swapchain | Mavk Windows WSI is implemented CPU readback/blit compatibility, not native Win32 surface; DroidDeck glibc/X11 WSI still untested |
| P2 transform feedback | yes | extension+feature present | NATIVE stock; PASSTHROUGH MISSING in Mavk. Optional in selected generic DXVK table, but needed by relevant D3D stream-output apps |
| P2 vertexPipelineStoresAndAtomics | yes | no | EMULATION NEEDED for higher D3D capabilities; selected feature-level source caps ordinary path at11_0 without it. Not a blocker to initial FL11_0 device by itself |
| P2 pipelineStatisticsQuery | yes | no | EMULATION NEEDED when actual application requires statistics, not immediate gate |
| Deferred sparse residency / float64 | sparse buffer yes / float64 no | no / no | UNSUPPORTED in current Mavk; no demonstrated general implementation. Do not chase for first D3D11 device |
| Deferred descriptor buffer / GPL | available | absent | UNSUPPORTED in current Mavk, optional optimization for this DXVK; ordinary descriptors/monolithic pipelines are the initial path |
| WDDM/DWM acceleration | not this profile | not this profile | UNSUPPORTED by Vulkan bridge alone; independent Windows-driver project |

## Selected limits and queue consequences

| Property | Turnip reference | Mali stock | Current Windows Mavk slice |
|---|---:|---:|---:|
| maxPushConstantsSize |256|256|64|
| maxBoundDescriptorSets |7|7|1|
| maxColorAttachments |8|8|bounded single-color scenes|
| maxViewports |16|1|1|
| maxComputeWorkGroupInvocations |2048|1024|256|
| maxImageDimension2D |16384|16384|1024|

Native queues: Mali family0 flags23=graphics+compute+transfer+protected,2queues,
timestampValidBits64. Protected capability is not required/enabled for Mavk.
Turnip family0 flags15=graphics+compute+transfer+sparse,1queue,present=true;
family1 sparse-only flags8,1queue,present=false,timestampValidBits48.
Neither additional queue count nor sparse queue is a first-device requirement.
Use one graphics+compute queue first; no unnecessary cross-queue ownership work.
The native1.3 version/limits must NOT be copied into the bounded Windows ICD:
it still reports a restricted experimental1.0 API and is not full Vulkan1.0 CTS.

## Minimum next implementation and PASS gates

1. Freeze exact DXVK pin +53-row admission manifest. Keep honest missing bits.
   Aim first for **D3D11CreateDevice requesting FL11_0**, not11_1/12_x or arbitrary
   games. The existing Mavk triangle/texture app PASS is not this PASS.
2. Direct DroidDeck Native path already avoids serializing all Vulkan through
   the tiny Windows ICD. Reuse its proven glibc/Bionic ABI component; do not
   rebuild it or widen the existing exactG715 workaround guard to G925.
3. Compatibility work starts with measured **BC decode + robustness/null
   resource semantics**, then clip/cull, viewport and wireframe. Each is a
   shared Compatibility module with isolated correctness tests before exposure.
   This is an ordering of implementation work, NOT a claim that the first two
   alone make DXVK admit the device. All seven mandatory checks must be real
   before claiming unmodified selected-DXVK support.
4. Separately forward required already-native capabilities through Mavk only
   as real consumers need them. Do not implement190 Turnip extensions. Android
   Feature Suite first; Windows loader/Bridge gate only when that API changes.
5. After admission: execute D3D11CreateDevice in actual DroidDeck UID/session,
   verify hardware renderer +requested feature level, then a small D3D11 draw
   with pixel verification, then WSI/present+copy/fence profiling. No game FPS
   or zero-copy claim before these results.

Quick legacy check: DXVK1.10.3 makes robustness2 optional, but still requires
wireframe, clip/cull distance and BC even at FL9_1, plus multiViewport at9_3.
Thus downgrading alone does not supply a quick honest FL11_0 PASS. It could
later reduce the initial compatibility implementation surface, but should be
chosen explicitly rather than silently replacing the project's DXVK version.
Source: https://github.com/doitsujin/dxvk/blob/v1.10.3/src/d3d11/d3d11_device.cpp

## Audit implementation / regression

`fetch-turnip-reference.py` downloads public raw/profile JSON and records hashes;
it does not download/install a driver. `audit-capabilities.py` normalizes raw
profiles, retains every observed feature/property/format/queue, produces the
full diff and extracts exact mandatory DXVK rows. Missing evidence is UNKNOWN.
No source from Adreno is compiled or copied into Mavk implementation.
Five offline parser regressions PASS. Profile53-row comparison PASS as an audit;
actual Mali admission result NO-GO7. No VM started, no production APK installed,
no Windows/Ubuntu data modifications, no commit/push.
