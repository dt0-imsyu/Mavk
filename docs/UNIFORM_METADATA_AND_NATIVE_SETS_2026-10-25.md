# Bounded metadata and descriptor performance experiments

User priority: improve resource use at unchanged rendering quality. No quality,
resolution, CPU model, production APK or U-AVF changes in these experiments.
Per-Muck profiles select private libraries and retain explicit restore preimages.
GPU timestamp probe and shader CSE remain OFF. No new feature bits or CTS claim.

## Uniform metadata: functional, no useful FPS gain

Resumed from GLM's logs and checked actual files/device binary hashes. The first
std140 scalar-array candidate `ea88fd472d5ab4803ab7ee2ab121ee316b48cc018928ee7795725a168f8d8ec2`
passed GLM's retained 2048-generation GPU readback but expanded each word to
16 bytes. Its initial real menu sample was 22.8 buffers/s, not an improvement.

Replaced that storage with a std140 uvec4 array: four ABI words per 16-byte slot,
word index split into vector/lane. No uniformBufferStandardLayout feature needed.
Clone only the private input block; preserve application types and guard logic.
Reserve a bounded extra uniform binding only within conservative native limits;
otherwise retain SSBO. Validate rewritten SPIR-V before creating native shaders.

First packed attempt exposed duplicate OpTypeVector in real DXVK modules. The
validator rejected it; black output at 54.8 buffer submissions/s is explicitly
NOT a performance PASS. Fixed by reusing existing uint4 type. Added the same
case to the existing nested consumer, not a new benchmark framework.

Corrected candidate:
`0d7508d73163e69270bb5460ad8b10e96a56a0e5c67d4f6a98985bab83ebb954`.
Actual Mali-G925 passes all four shared-diagnostic/report configurations, existing
uint4 case, and 2048 retained generations/replay with 131072 checked words and
canaries, CoreZero. Four shader-report configurations are our own tests, not CTS.

Real Muck animated menu: 703 SurfaceFlinger game buffers/20.1s = 34.9/s, visible
game counter about35, gaps0. This is the previous baseline level, not60 FPS.
Creative seed12345 forward scene, same original quality: candidate123/10.1s =
12.1/s; control arena/SSBO123/10.1s =12.2/s. Both have the same 75 intervals near
86ms and comparable 74–78ms intervals. No useful gain; do not promote UBO default.
Creative is used to prevent player death interrupting the repeatable scene;
the earlier death-screen31.7/s sample is NOT gameplay performance.

Private logs/readbacks/screenshots reside in
`artifacts-local/uniform-packed2-20261025/`. Initial failed packed experiment is
retained separately in `artifacts-local/uniform-packed-20261025/`.

## Ordinary public set passthrough: isolated experiment

Creative scene records about11349 materializations/s (about900 per frame),
metadata arena32 pages/s, guard recording183–186ms/s including snapshot59–61ms
and materialization95ms. Allocation work is real, but CPU timings alone cannot
prove it accounts for the remaining frame time.

The first direct-public-set attempt failed the existing nested GPU oracle,
including a Mali group fault. It was never selected for Muck. Root cause is
concrete: guarded updateSets deliberately stores logical state only, so original
native sets are uninitialized. Merely rebinding them is invalid. Core cleanup
still reached liveZero; do not repeat that binary.

The corrected opt-in implementation mirrors eligible ordinary non-null writes
only AFTER logical validation, at the original application's update point.
Any null/extended write or descriptor copy destination
permanently removes that set's eligibility until it is freed. Those paths keep
the established typed snapshot implementation. Native sampler heaps keep their
existing independent semantics. At draw time require every selected value bound
and every ordinary set eligible. Retain resource leases, immutable guard metadata,
dynamic-offset validation and command-generation lifetime in both paths.
Seal native contents after the first recorded use: a later logical update falls
back without changing the original bytes referenced by earlier commands. Native
set/pool lifetime still follows the application's Vulkan lifetime requirements.

This remains default OFF (`MAVK_NATIVE_BOUND_SETS=1` is private only). Its GPU
regression/game result is recorded below after testing. Moving native updates
out of prepareExecution can shift CPU attribution, so lower guardWallMs alone
must NOT be described as an end-to-end speedup.

First corrected candidate c8e321c3f1c7b848c40731d268123cf2c7b41b6025b8b0b1cdb077c028080e81
passed four report/share cases, retention and nine existing consumer regressions
(routing/dynamic/resubmit/inflight/empty/unused/selected/immutable/images). Real
Muck selected it correctly, but nativePublic draws stayed0. It does not justify
any speedup claim. DXVK's descriptor-template entry point is an important missing
eligible route. A follow-up reuses the frontend's already-decoded/validated
template writes through an optional callback, instead of decoding them twice.
Those writes must pass the same mirroring/sealing/null checks as ordinary writes.

Template-aware candidate `68586264ff98fba005dd869155500b522c890219327da5325f5a9a46955ec0e9`
passes the same actual GPU matrix and nine regressions, including immutable
sampler/template execution, CoreZero/gaps0. Added focused assertions to the
existing descriptor-template unit test: callback observes committed decoded
values, and never runs for truncated data or a rejected resource. The ARM64
binary ran on the tablet: DESCRIPTOR_TEMPLATES_PASS checks=19 negatives=19.

## Actual game outcome and checkpoint

Template-aware path actually executes: menu roughly6400 native-public draws out
of6440; forward creative scene roughly11460 out of11473. Thus most public set
allocations/rewrites in materialization are bypassed, while the private guard set
and metadata snapshots remain. Native public updates are still necessary at the
application's original update point; this is not elimination of ALL descriptor
work. Null fallback and immutable metadata semantics remain intact.

Menu356/10.1s =35.2 SurfaceFlinger game buffers/s. Same creative seed12345 camera
and quality:262/20.1s =13.0/s, game counter13. Prior control12.2/s was hotter
(thermal3 versus2); do not call this a proven6.6% performance improvement.
Final log gaps/pipeline failures/Mali group errors0. Sampled frames show normal
moving rendering; intermittent diagonal artifacts are NOT declared fixed.

This experiment removes specific duplication but does NOT explain/solve the
dominant remaining frame time. Do not keep polishing metadata variants/pools for
60 FPS. Next useful work must isolate shader/driver GPU cost versus engine/CPU
work at unchanged quality, using a concrete consumer and safe observability.
The earlier GPU timestamp probe that caused group faults remains prohibited by
our current experiment plan; no privileged GPU-utilization claim is available.

Private artifacts: `artifacts-local/native-public4-20261025/` contains exact layer,
all14 GPU run outputs (four share/report modes, retention, nine regressions),
ARM64 template test, same-view screenshots, pacing JSON, full game/thermal logs.
Installed APK/runtime remains6c8680f5/3bd128. All experiment flags default OFF;
the Muck profile is restored/inactive, Muck quit for cooldown, Steam retained.
USB stay-awake is enabled at the user's request. No commit was pushed, no U-AVF,
Windows/Ubuntu/H264 or firmware changes. All new outputs written to C:.
