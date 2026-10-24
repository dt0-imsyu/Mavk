# D4 descriptor lifetime — bounded command reuse, not update-after-bind

Reuses the real-loader dynamic-descriptor consumer, seven dynamic descriptors,
two sets, actual Mali-G925 and the installed common experimental layer219b5bf0.
No new renderer/harness, production change or feature advertisement.

Contract source: [Vulkan command-buffer lifecycle](https://docs.vulkan.org/spec/latest/chapters/cmdbuffers.html).
The ordinary Core CommandBatch is deliberately ONE_TIME_SUBMIT. The new test
uses explicit standard Vulkan begin/reset on that existing buffer instead of
silently changing Core behavior for all clients.

| Case | Current result / boundary |
|---|---|
| Reusable executable buffer, two resubmits after fences | Native/protected PASS; identical512word oracle,4total submissions,zero gaps/Corezero |
| Reset only after completion | Included in reusable PASS; native generations retire on reset,not first completed fence |
| Two pending uses of same SIMULTANEOUS_USE buffer | Native/protected PASS; both submitted before host release, identical final512word oracle |
| Host release of timeline gate | Both fences checked NOT_READY before release,not guessed overlap from fast submissions; native timeline then releases both |
| Descriptor/resource updates while pending | Not attempted/admitted by this package |
| Ordinary descriptor update after recording | Does not become update-after-bind; invalidation rules must be respected |
| General UAB/partial/variable counts | Remain rejected except existing exact native sampler heap |
| Multiple queues/ownership transfer | NOT tested |
| Secondary invalidation/reset interactions | Existing retention path reviewed, not covered by new primary reuse oracle |
| Submit failure/retry | Existing null-image pre-driver test separate; no driver-failure simulation added |

Sequential evidence `E:/Mavk/artifacts/layer-resubmit-consumer-20261021/`:
consumer SHAfd99f8c30118bbf6301fbcfba8f613ca698c6203d9efd0b9918cb2bb967693ec,
`reusable-results.tar.gz`, strict `verify-layer-execution.py --resubmit` PASS.
Outputs include sentinel checks after every replay,not just successful submits.
No guarded SyncVal/CTS/performance claim.

In-flight evidence `E:/Mavk/artifacts/layer-inflight-consumer-20261021/`:
consumer SHAe0e045e8a396c37fe5fdeb8d48514e00297d586e871f22bd7c75bfb9acb66f74,
`inflight-results.tar.gz`, strict `verify-layer-execution.py --inflight` PASS.
Native timeline feature is queried/enabled only in this consumer mode. The
reused command carries SIMULTANEOUS_USE and an explicit compute write-to-write
dependency for its shared output; no host access until both fences complete.
Both queued replays share one immutable generation; their combined final output
is checked, not claimed as two independently timestamped/visible frames. Reset
occurs after both completions. No runtime/Core implementation change needed;
existing retention is correct for these bounded cases.

General late updates still need one coherent model for native descriptors AND
private guard metadata. Capturing at record then forwarding late writes only
to original public sets would leave the protected clone stale. Mutating one
clone shared by pending submissions is also not an implementation. Do not
enable indexing bits by loosening admission checks or use queue-idle waits as
a hidden compatibility workaround.
