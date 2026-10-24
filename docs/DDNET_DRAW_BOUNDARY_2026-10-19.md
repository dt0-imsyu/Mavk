# DDNet actual-draw boundary

## Proven

- Both official Windows ARM64 and AMD64 DDNet20.1.1 reach a real menu through
  Wine -> private Mesa25.1.9 Zink -> native Mali-G925. Both have corrupt glyphs
  and geometry. This is NOT a playable-game or DXVK PASS.
- The game's own F10 screenshot is already corrupt before Android composition.
  Nearby Android capture is a different animated frame; no bit-exact pairing.
- A no-Mavk run retains only hybris/Zink, verified in actual process mappings,
  and still corrupts the menu. This does not prove the remaining component.
- AMD64 DDNet reports WASAPI sound initialization success; ARM64 reports missing
  float conversion. Audible sound and latency remain unverified.

## Actual resource observation

Gallium's built-in single-frame trace records43 draw calls. Its trigger does not
preserve earlier vertex-layout creation or persistent-map byte writes. A short
startup trace supplies layouts: 2D position R32G32_FLOAT, packed color
R8G8B8A8_UNORM, UV R32G32_FLOAT, stride20 for ordinary menu draws. Some initial
probe geometry uses stride36. Separate attribute upload spans are expected;
large offsets between bindings alone do not prove a bad stride.

Reading known mapped buffers through own-UID `/proc/PID/mem` opens successfully
but returns EIO for the relevant range. Do not infer a driver limitation or bad
vertex bytes from EIO. No root, ptrace bypass or physical-memory mapping used.

A bounded, isolated Mesa observer samples bytes inside the renderer process at
set_vertex_buffers from currently active map records. First12 calls only,
up to160 bytes/binding, no resource writes or feature changes. Observed position
and UV samples are plausible, including repeated corners of six-vertex quads.
Packed RGBA interpreted as float can be NaN: this is NOT evidence of corruption.
This is a small CPU-side sample, not proof of every byte or GPU-side correctness.

## Artifacts and rollback

- E:/Mavk/artifacts/zink-ui-25.1.9-20261019/ddnet-frame.xml (single frame),
  ddnet-start.xml (120772624 bytes, copied to E:), ddnet-start-summary.json.
- E:/Mavk/artifacts/zink-vertex-observer-20261019/ddnet-vertex-snapshot.log.
- Observer libgallium SHA256
  af5540a390ac2de9ebe415c5560a92f8fcb7d79492e041f4ba2243c5c42b4e63.
- experiments/droiddeck/zink-vertex-observer.patch is reversible against the
  pinned Mesa25.1.9 source. C/H source restored to archive hashes AFTER building.
  Build-tree binary still observer until next rebuild; normal staged libraries,
  APK11 and Steam default never replaced by observer. Device observer directory
  files/mavk-zink-vertex-observer is used only with probe -VertexSnapshot.
- Removed generated device ddnet-start.xml (host copy retained) and failed
  ddnet-mapped.xml (~685MiB, no successful memory result); no user files removed.

## Next discriminating step

Vulkan-side observer now captured expected vertex formats103 (R32G32_SFLOAT),
37 (R8G8B8A8_UNORM), offsets matching the Gallium uploads, source_stride20.
dynamic_input=0, dynamic_stride=1: CmdBindVertexBuffers2 supplies the stride;
pipeline_stride0 is therefore legal, NOT itself a bug. Log ddnet-vk-bindings.log,
observer binary SHA709da7f984a6036f431ade9507916926f720b039b15f7f545c4b899c4580ef6c.
An isolated source-gated equivalent static-stride A/B compiled and ran:
dynamic_stride=0, pipeline_stride20, source_stride20, state0. Loading-screen
glyphs AND the later language menu remain corrupt. NO-GO; not integrated.
Evidence ddnet-static-stride.log, ddnet-static-stride.png, ddnet-static-menu.png.
Working staged libraries unchanged. Static-stride source patch retained only
as a reproducer, not a production workaround.

The next observer copied three160-byte vertex ranges to staging before map/read.
Pinned Zink resource_copy_region -> zink_copy_buffer uses vkCmdCopyBuffer, NOT
CPU memcpy. All returned zeros. Explicit pipe flush/fence completes successfully,
but a separately uploaded control buffer in the SAME Zink path also reads zero.
Therefore this does NOT prove the original vertex data is invisible to the GPU:
readback/observer placement remains unvalidated. No memory-policy fix justified.
Logs ddnet-gpu-vertex-copy.log and ddnet-gpu-fence-control.log; no FPS conclusion.

### Independent Vulkan copy control

Reused existing glibc probe initialization in hybris_buffer_copy.c; run through
the normal /usr/lib/libvulkan.so.1 loader and same app-owned ICD/compat layer,
not Termux or a VM. Actual device Mali-G925. Each case checks40 uint32 words
after vkCmdCopyBuffer, fence, and required mapped-memory invalidation.

| Source/destination flags | Bind offset | Binding API | Result |
| --- | --- | --- | --- |
| 0x7 / 0x7 | 0 | BindBufferMemory | 40/40 |
| 0x7 / 0xb | 0 | BindBufferMemory | 40/40 |
| 0x7 / 0xb | 65536 | BindBufferMemory | 40/40 |
| 0x7 / 0xb | 65536 | BindBufferMemory2 | 40/40 |

0xb is cached but NOT coherent. `cached-implicit` still invalidates by the real
feature flags; it repeats the explicit cached case, NOT an independent implicit
coherence PASS. No fake HOST_COHERENT advertising. Broad native copy/offset/API
failure is contradicted by these results; Zink allocation/readback differences
remain to be captured. Helper SHA256
dedf9d1d06b15b5d58f6ebe4bc23e9779af109f58396e98181fbb8f35a0f1241.
All four observer-modified Mesa C/H/CPP/HPP files restored to exact archive
hashes after builds; patches/binaries retained separately, stable stage intact.

Capture exact allocation/binding/map/invalidate parameters of the failing Zink
staging readback and compare with the passing native copy. Do not interpret
vertex GPU visibility until the Zink readback control itself passes. Keep this
bounded; no more generic copy/offset sweeps or full startup XML captures.
Do not restart GL-version, packed-vertex or generic debug-flag sweeps. Preserve
the working accelerated Steam UI. No production U-AVF/Windows/Ubuntu changes.
