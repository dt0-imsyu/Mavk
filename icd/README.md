# Experimental Windows offscreen ICD

This is an incomplete research implementation, not a conformant Vulkan 1.0/1.3
driver, production installation, general game runtime or WDDM/DWM acceleration.
The manifest's 1.0 version selects the initial loader API contract; it is not a
conformance claim. Its virtual limits deliberately describe the implemented
slice, not all native G925 capabilities. No conformance version is advertised.

## Vertical slice

2026-10-16 opt-in sampled ICD Windows runtime gate PASS: standard API consumer
verified3072 pixels across3 indexed draws,11mip RGBA1024 texture, cached reuse
and changed content. Actual Mali5GPU/29commands,EXIT0,SyncVal0/0,clean teardown.
Requires MAVK_EXPERIMENTAL_SAMPLED; bounded UBO+combined sampler set, not general
texture support or conformance. Unmodified external textured example is next.

2026-10-16 standard compute API runtime gate PASS: two application SPIR-V
dispatches on Mali,8192 verified words plus descriptor guards, EXIT0; host
4commands/2GPU,zero validation errors/warnings,clean EOF and Core teardown.
This is bounded1D compute with one set0/binding0 SSBO<=512KiB and push<=64B,
not general compute support or conformance. Evidence and next external-app gate
are recorded in docs/STATE.md; original-slice descriptions below are historical.

Standard Windows `vulkan-1.dll` → application-local manifest → ARM64 ICD → existing
4057 app-owned AVF stream → Mavk Core → native Mali. No system-wide registry or
driver installation. Use `VK_DRIVER_FILES` only for the one test process.

Implemented source: instance/physical-device discovery, one graphics queue,
bounded image/readback-buffer allocations, shader modules, pipeline/layout,
one render pass/draw/copy command buffer, synchronous submit/fence, MapMemory.
Only RGBA8, 16..512 dimensions, one sample/layer/mip, descriptor-free shaders,
three vertices using VertexIndex, full viewport/scissor, 16-byte push constants,
no blending, depth, samplers, swapchains, extensions or semaphores.
This paragraph describes the original tested slice, not the latest source.
The 2026-10-16 candidate adds bounded indexed draws, staging buffer copies,
64-byte per-draw push constants, D32 depth, culling, up to16 draws/pass,
1024-square colour readback and a separate linear-image copy submission.
Native, CPU and unmodified upstream headless Windows runtime gates passed.
See STATE.md for
the distinction between tested guest functionality and candidate source.
Vertex-buffer extension source/build: one vertex-rate binding, up to4 float
attributes, stride<=64, up to4096 non-indexed vertices. Native Mali transport
test passed; Windows API vertex-buffer runtime also passed on2026-10-16:
3330 verified pixels,EXIT0,one real Mali submission,zero SyncVal errors/warnings.
This covers the tested padded stride16/attribute offset4/binding offset16 path,
not exhaustive vertex formats, arbitrary applications or performance.
Unsupported configurations must fail explicitly, not render a fixed triangle.

Memory is copied, not shared/zero-copy. Image allocation and pipeline creation
are currently lazy on the host; compilation failure can surface at submission.
Only a trusted finite test guest is supported. Structural SPIR-V checks are not
a shader-security sandbox or full semantic validator. The initial blocking WSP
transport still requires the external finite test-process deadline; no production
cancellation/concurrency claim. Custom allocation callbacks are not implemented.

`test/offscreen.cpp` depends only on Windows + Vulkan, dynamically uses the
standard loader, and has no Mavk protocol/API calls. Its own blue fragment shader
differs from the verified Bridge diagnostic's orange shader. A correct blue
3330-pixel readback is necessary to distinguish actual app shader execution from
substitution. This test passed in the ARM64 guest through the standard loader:
3330 blue pixels verified by Windows, process exit 0.
It is an independent API consumer in this source tree, not a third-party app/game.

## Evidence as of 2026-10-15

- Windows compute Bridge: PASS, 8 GPU jobs,131072 verified words.
- Windows trusted graphics Bridge: PASS,3330 verified pixels,zero validation errors.
- ARM64/x64 ICD and standalone app: build PASS (see latest STATE for exact build).
- Host x64 standard loader finds manifest, negotiates and calls ICD CreateInstance.
  No host Viosock exists: expected initialization failure, **not device/render PASS**.
- ARM64 standard loader finds DLL and reaches remote DeviceIdentity; first run
  then rejected missing mandatory sparse-format query entrypoint. Fixed in source
  with correct zero sparse-format enumeration; second runtime check PASS.
- Physical-device enumeration and offscreen standard API render: PASS on Mali-G925.
  Three bridge commands, one hardware GPU submission, zero validation errors or
  warnings, clean EOF and zero remaining Core resources. Guest readback:3330
  blue pixels, EXIT=0. Strict host/guest verifier passed after clean shutdown.
- Evidence: E:/Mavk/artifacts/windows-icd-api-run2-20261015/{host-api-report.json,guest-api-cold/}.

Current Windows manual input/UAC method, firmware, CPU/SMP, H.264, Ubuntu and P7
remain unchanged. Temporary diagnostic carrier restored with exact readback.
No commit/push. Next milestone is a concrete independent application requirement
and the smallest additional API slice it needs, not repeating this passed test.

Loader contract reference:

2026-10-16: indexed/staging standard API test PASS in Windows ARM64 through
the same loader/ICD/Bridge on Mali-G925:3330 blue pixels verified,EXIT0,
one GPU submission,SyncVal0errors/0warnings,clean EOF and Core live zero.
Test covers padded vertices,nonzero vertex/index bindings,firstIndex and two
staging copies into virtual device-local memory. Copies currently execute in
guest shadow memory; native buffers are uploaded lazily at draw. This is not
zero-copy,performance,third-party application or Vulkan conformance evidence.
Exact reports and rollback identities are in docs/STATE.md.

https://github.com/KhronosGroup/Vulkan-Loader/blob/main/docs/LoaderDriverInterface.md

## 2026-10-16 external app and experimental Win32 presentation

Vertex UBO descriptors now passed a real Windows/Mali test: set0/binding0,
192B matrices, nonzero descriptor offset, mapped writes after command recording,
3906 verified pixels and EXIT0, zero synchronization-validation errors/warnings.
This remains a bounded trusted-consumer API slice, not general descriptors.
An unmodified upstream windowed triangle passed with source/blob verification:
24 actual Mali frames per run in IMMEDIATE/FIFO; FIFO process EXIT0,zero SyncVal
errors/warnings and clean Core teardown. Its RGB triangle window was observed
on Android through unchanged H264. Genuine bounded compute pipeline/dispatch
support is separately tested (8192 words), not just an advertised queue flag.
The image-view identity/explicit-RGBA equivalence fix is covered by CPU tests.
This is one third-party window app PASS, not general WSI/game compatibility,
DWM acceleration,zero-copy or sustained/unique visible FPS proof.

Unmodified pinned SaschaWillems renderheadless passed in Windows ARM64 at1024²:
standard loader/ICD,three indexed RGB triangles,MVP/depth/cull,EXIT0 and3624
independent analytic pixel samples. Host SyncVal0errors/0warnings,cleanEOF and
Core live zero. This is one external offscreen app,not general compatibility.

`build-wsi.ps1` enables `MAVK_EXPERIMENTAL_WIN32_WSI` in a separate DLL build;
default offscreen builds do not advertise WSI. Own standard Vulkan Win32
consumer passed12 distinct moving images320×240,acquire/submit/present/binary
semaphores,EXIT0; the window was observed through existing Android/H264 output.
Rendering really runs on Mali. Presentation downloads RGBA pixels,swizzles on
the guest CPU and blits to its Win32 window. No new Android transport/decoder,
no display driver installation,no DWM acceleration or zero-copy claim.

Restricted WSI slice:2 images,RGBA8,16..1024 client extents,one local-process
HWND,opaque/exclusive,color-attachment usage,FIFO(DwmFlush) or immediate mode.
Synchronous single-queue work only; future/unsignaled semaphore waits and broad
asynchronous submission are unsupported. Host hidden-window units cover state,
resize/retirement/lost surface and budget teardown; not exhaustive WSI testing.
This is not a conformant implementation of all advertised extension semantics.
No visible Android FPS or performance target is established by these12frames.
