# Live descriptor performance work

User priority: remove wasted work at unchanged graphics, not a quality reduction.
Do not assume100FPS is available without a measured result.

Installed APK remains source-built6c8680f5, runtime3bd128. Experimental layers
are per-Muck private selections only; guards remain on, GPU probe/CSE off.

## Measured control

Private CPU-counter layer19ce574ac0a61b9644fc4f563f7324db3a8ddb4a1dcd61d8df054c927bca1d9a:
same animated menu~6500guard calls/s,~6300native materializations/s,
snapshot39–40ms/s, materialize114–117ms/s, guard total175–180ms/s.
Useful metadata<1MB/s, but EACH new snapshot also allocates/zeros a32KiB log
even with MAVK_GUARD_REPORTS=0:~200MiB/s and thousands of GPU memory allocations.
Counters attribute CPU time, not GPU time or unique frames.

## First candidate: quiet diagnostic storage

Private layer64edd94ca5a62836907fc3139febec729eabd3949b56ece3b32abec7787db4d5:
MAVK_SHARED_DIAGNOSTICS=1 reuses one zero log per adapter ONLY when compiler
diagnostic writes are globally disabled. Reporting mode retains private logs.
Existing nested GPU consumer passes all4shared0/1 ×reports0/1 cases, CoreZero.
Actual Muck materialize87–91ms/s at~6200materializations/s; guard148–156ms/s.
Displayed menu352buffers/10.1s=34.8/s. CPU savings observed; no large FPS gain.
Log saved privately at files/mavk-perf/shared-diagnostics-game.log.

## Second candidate: immutable metadata arena

MAVK_DESCRIPTOR_ARENA=1 suballocates aligned slices from64KiB host-coherent pages.
Snapshots retain pages until command generations retire, including resubmission.
Descriptor ranges remain exact. Noncoherent memory falls back to dedicated
buffers, avoiding whole-allocation flush overlap with live slices.
Default remains OFF. Actual Mali tests pass all4shared/report modes plus the
2048-generation rollover/retention/replay case:131072checked words,canaries,
two submits,CoreZero. Actual Muck menu passes, diagnosticAllocations0 steady,
metadataArenaAllocations16–17/s at~6100–6200materializations/s (formerly one
metadata allocation per materialization). Materialize70–73ms/s observed versus
the control114–117ms/s. These CPU timing samples have differing thermals; do not
present them as a controlled GPU/game speedup. Display355buffers/10.1s=35.1/s:
no substantial FPS gain despite removal of demonstrable allocation waste.

Focused existing-consumer extension records2048distinct descriptor sets/ranges,
alternates valid/OOB reads into separate canary-protected output regions, then
replays the executable command buffer. This is needed to prove page rollover
and retention; not new Vulkan feature coverage or CTS.

One bounded NonWritable-only metadata test (no CSE) also did not produce a useful
gain:343buffers/10.1s=33.9/s at thermal status3. Keep annotation default unchanged.
GPU devfreq nodes exist but cur_freq/max_freq reads are Permission denied on
stock Android; no privileged workaround attempted, no GPU-utilization claim.

Private profile backup now RESTORED/INACTIVE; game quit, Steam open/minimized.
New build outputs on C: only; E: inputs
are read-only caches. Production U-AVF/Windows/Ubuntu/H264 unchanged.

Next useful GPU-side hypothesis is the use of SSBO reads for immutable guard
constants, not further resolution/quality reductions. A private UBO experiment
must preserve the exact guard semantics/layout and pass the same readback and
retention tests before touching a real game's configuration. No feature-bit
relaxation or unguarded game launch is justified by the profiling above.
