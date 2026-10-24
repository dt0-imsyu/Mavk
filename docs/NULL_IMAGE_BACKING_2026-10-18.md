# Device-owned guarded color-2D null images — 2026-10-18

## Implemented and verified

Common `compat/null_images.hpp` now provides real device-owned image/view leases
for guarded logical-null color-2D descriptors. Existing native image regression
uses this catalog instead of substituting unrelated live app images for nulls.
Combined descriptors retain the actual sampler registry lease,not a dummy token.

Explicit format admission currently covers RGBA8_UNORM and R32/RGBA32 scalar
float/signed/unsigned formats. Native image-format properties for actual combined
sampled/storage/transfer usage are checked before creating a 1x1 color image.
Hardware execution below proves RGBA8 sampled and R32_UINT storage only;other
formats are NOT declared hardware-tested. Depth/MS/array/cube/other dimensions
remain outside this explicit subset. No general image or Vulkan feature claim.

Initialization has explicit recording/submission ownership:UNDEFINED ->clear
zero ->GENERAL,with transfer-write ->shader-access dependency. No hidden submit,
queue idle or CPU wait. Caller records before use on ONE externally synchronized
queue,marks submitted only after successful queue submission,and keeps the batch/
command alive until completion. Submitted means queue-ordered,not GPU-completed.
Move-only reservations prevent concurrent duplicate initialization booking;
discarding an unsubmitted batch releases its reservation for retry. Never
resubmit an initialization command. The adapter must supply the actual lifetime
and queue-ordering contract;the catalog does not pretend to solve it globally.

## Evidence

`E:/Mavk/artifacts/null-image-backing-regression2-20261018`:

- Existing combined-sampler/storage-image gate,1792 original output/sentinel
  values,384 guard events,two swapped live/null phases.
- Two real fallback images initialized once;second phase schedules zero new
  initializations. Existing immutable native snapshot cache remains unchanged.
- Additional actual GPU readback checks both backing pixels remain zero after
  guarded stores. Real live image writes and original guard counts still match.
- Core live resources zero;Synchronization Validation errors0/warnings0.
- Strict existing verify-null-images.py --shared-state PASS;probe stopped.
- APK SHA256:
  1c7f8f18698ce175fbb23f2382903dafcc899020ace56e618d37b98f23aefe6d
- Fresh report SHA256:
  58bc8fbecdeec87cd1e8947c4eeb15cfccdca510a745098e607872abbca24e09

Initial regression1 never executed GPU work:the selected historical instrumented
shader used private set7,while the current materializer uses private set1.
Validation correctly rejected pipeline creation. Replaced the artifact with
the exact previous native-gate set1 shader;no implementation/oracle weakening.
Failure evidence preserved in regression1. Build preflight now rejects wrong
fixed-guard descriptor-set artifacts BEFORE creating output/building/installing.
Both correct-input acceptance and wrong-input rejection tested;the deliberately
invalid output directory was not created. This is a schema preflight,not full
SPIR-V validation.

Core dispatch gained actual downstream image-format query and clear-color calls.
Actual glibc Sarek shader regression after that dispatch change:
`E:/Mavk/artifacts/sarek-consumer-run14-20261018`.
65536 components/4 submissions,indirect/resubmission/bounds,572 diagnostic words,
gaps0/all tracked cleanup0. This does NOT exercise the new image catalog in the
layer:the common GPU gate above covers that implementation separately.

Layer12 SHA256:
fef7b02e1e540c730b9402c32cc0a1ff1d01134df8e46f8b6a847bf45a4a644e

## Remaining integration / next exact step

The loader layer STILL rejects logical-null images at materialization. It is not
yet connected to this common image catalog. Next:reflect admitted color image
types from validated original SPIR-V;choose compatible resources with real
sampler leases;prepend retained one-shot initialization commands to the caller's
successful queue submissions without stealing a queue or waiting CPU-side.
Preserve waits/signals/pNext and app fences;retry failed submits correctly;
reject unsupported cross-queue/family use rather than pretend it is synchronized.
Then verify the actual layer boundary,not just this common-module gate.

Further dimensions/formats/query/atomic and command/update semantics remain open.
nullDescriptor stays FALSE. Real Sarek D3D11CreateDevice still stops at that gate;
NO D3D11 frame/device or full DroidDeck app/Wine session PASS. Do not rerun the
unchanged device gate alone. No production/VM/H264/stock Adreno changes,root,
commit or push. Strategic goal remains active and incomplete.
