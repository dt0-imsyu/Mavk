# Used bindings within used sets — isolated bounded GPU PASS

## Verified device result (supersedes preparation below)

Final layer SHA256219b5bf0249ea2fea2ce919f4e6b72473cce6f28dcc11a4b7eb29e485790c064
in `E:/Mavk/artifacts/mavk-selected-normalized-20261021/`.
Consumer SHA2562e18fb218ce7d6d485b846fe161e9de4b1f3a4c3e8863a72cb457aa476333c08.
Probe SHA2566fa6d2b2b844b4c36cdb55f835ce3c184525a3f2123f586738cf1b041912030d.
Guard frontend archive SHA2564cc479a3b381f333733c46bfeba777f8ef705e689ec01f0175cbe9a022d4d773.

Actual Mali-G925 results, `E:/Mavk/artifacts/grouped-reflection-20261021/`:

- `final-results.tar.gz`: strict selected512words/native=guarded, seven unwritten
  unused bindings,eleven dynamic offsets. Used array element1 unwritten rejects
  with exactly one expected gap,zero submissions/Corezero.
- Strict dynamic, empty, unused and command-routing verifiers PASS. Missing used
  set rejects as expected; no generic gaps0 claim includes expected negatives.
- Direct immutable oracle192components/3phases/5submits PASS, gaps0/Corezero.
- Valid GROUPED immutable shader: native matching-sampler control PASS192;
  protected copy/template/layout oracle PASS192, gaps0/Corezero.
- Fresh Core5gates/13rows, null2D/array/volume/cube, generation/template and
  BC123/45/67/texture lifecycle bounded gates PASS, validation0warnings/0errors
  in those probe reports. Not guarded-layer SyncVal, CTS or full extension PASS.
- Host10descriptor suites PASS. Compiler25assertions/9rejections and reflection
  formats6/negatives10/groups11/equivalentGroupedViews1 PASS.

Two general bugs discovered, preserved with pre-fix evidence:

1. Shared reflection ignored grouped DescriptorSet/Binding decorations. Valid
   grouped fixture failed before the fix. Reflection now resolves groups and
   rejects conflicts/nesting/member misuse without broadening admitted formats.
2. Pinned SPIRV-Tools bindless pass needs direct decorations. Protected grouped
   GPU output previously stayed0xCCCCCCCC while native control passed. Normalize
   only modules containing groups before instrumentation. The pinned flatten
   pass itself spins on OpMemberName; strip original debug first, then flatten.
   Original nonsemantic debug is removed, not resource/shader semantics. Direct
   modules skip both passes. Final actual GPU oracle proves corrected output.

Earlier affa9866 reflection-only layer and0b32e738 candidate are NOT final.
`selected-bindings-gpu-20261021/pre-flatten-results.tar.gz` includes the expected
grouped failure and must not be labeled a complete PASS archive.

## Common experimental rollup

Isolated Muck PID11333 mapped candidate219b5bf0:141349protected executions,
gaps0/device-lost0 at capture. No visible-FPS or exhaustive game claim.
Game stopped, candidate override removed. APK17 now installed in place:
SHA2561c5fa1ce7cdd32f95303a206ad639faad878afee819803de4f2dd928ec05c731.
Runtime acda20c73d2b56d2fe06bd6c3d47c0fcbc485e6fdf24b2f45b85adc8a4975b94.
Only layer+manifest changed,261runtime members and485other APK members unchanged;
same test signing certificate. Fork assets updated to this exact runtime.
Installed common selected oracle512words PASS/Corezero. Muck PID12065 mapped
COMMONacda20c7,25059protected executions/gaps0/device-lost0 at capture; game
then stopped. Steam normal desktop hardware renderer ANGLE/Zink/Mali confirmed.
APK16/d1ad retained on E: as rollback. No data clear/uninstall/commit/push.

One independent Big Picture
Steam startup failed with exit139 on that unchanged baseline, not candidate.
Normal desktop Steam UI works; Muck integration and rollup are complete above.
Original suspend preference restored absent/default manual.
No visible-FPS, broad nullDescriptor/UAB, CTS or production support claim.

## Historical offline preparation (NOT current status)

This package is SOURCE/CANDIDATE ONLY. Installed common APK16/runtime d1ad4522
is unchanged. Do not label the following GPU cases PASS or promote the layer
until the tablet is available and the real-loader oracle/regressions complete.

## Shared mechanism

- `snapshotUsedBindings` requires sorted unique selected (set,binding) keys.
  It validates initialization and live ownership of EVERY element in each used
  binding. It omits unused bindings, including unwritten/destroyed resources;
  it does not create fake null resources or admit partially-bound arrays.
- Cache keys include set identity, generation, binding selection and metadata
  ABI. Weak source-set identities prevent handle-reuse ABA; resource liveness is
  checked before reuse. No-op descriptor writes preserve the cached generation.
- Pipeline preparation selects resources by entry-point name/stage/call tree.
  Image/texel reflection is also filtered per selected stage, avoiding conflicts
  from declarations unused by that stage.
- Native layout allocation is distinct from the selected shader-resource list.
  Pool budgeting retains every declared binding in allocated native sets.
  Dynamic slots retain all offsets in Vulkan order, including unused bindings;
  only selected resources undergo range/liveness resolution. Unused offsets
  still undergo alignment/count checks. No offset is applied twice.
- Ordinary native sampler-heap passthrough remains separate and unchanged.
  General UAB, variable descriptor counts and partial arrays are not admitted.

Sources: [descriptor sets](https://docs.vulkan.org/spec/latest/chapters/descriptorsets.html),
[dynamic binding valid usage](https://docs.vulkan.org/refpages/latest/refpages/source/vkCmdBindDescriptorSets.html).

## Matrix

| Case | Driver semantics | New protected status |
|---|---|---|
| Unused unwritten binding in a used set | Native permitted within applicable valid usage | Implemented; CPU PASS, GPU NOT RUN |
| Used binding with an unwritten array element | Not admitted without appropriate indexing semantics | CPU rejects; GPU negative prepared, NOT RUN |
| Unused binding references destroyed resource | Does not make a used binding valid/invalid | CPU skips unused resource; GPU NOT RUN |
| Used binding references destroyed resource | Invalid | CPU rejects before cached snapshot reuse |
| Native allocation includes unused image/sampler/dynamic bindings | Entire layout must fit pool | Implemented; GPU NOT RUN |
| Dynamic slots before/between used bindings | Offsets follow complete native layout | CPU slot mapping PASS; GPU NOT RUN |
| Caller overwrites offset arrays after bind | Values consumed at bind | Existing path retained; expanded GPU case NOT RUN |
| WHOLE_SIZE with zero offsets for used buffers | Native bounded valid case | Expanded GPU regression NOT RUN |
| BufferRanges and IndexAndInit metadata, holes/set remapping | Shared bounded ABI | CPU PASS |
| New feature/extension advertisement | None justified | Unchanged; nullDescriptor remains false |

## Actual results

- Fresh host `selected-bindings`:12checks/16negative cases PASS.
- All nine existing descriptor host suites rebuilt against changed headers and
  passed (ten suites total). Output: `E:/Mavk/artifacts/selected-bindings-host-20261021/`.
- Existing prior APK16 unused-set evidence still passes the extended verifier;
  this is verifier regression, NOT execution of the candidate.
- ARM64 glibc layer/consumer and Android probe compile. No GPU, game, performance,
  synchronization-validation or CTS PASS for this package. ADB currently lists
  no devices; no app/guest was restarted to compensate for missing observation.

## Prepared minimal device test

`E:/Mavk/artifacts/selected-bindings-gpu-ready-20261021/` is an isolated payload,
not a runtime replacement. Use app-private `files/mavk-selected` and the existing
guest command channel, with `MAVK_DESCRIPTOR_TEST_ROOT` set to that exact path.
The existing `unused-descriptor-control.sh` now accepts this path and modes:

1. Native then guarded `--selected`: same four dispatches,512words on G925,
   four used bindings/seven used descriptors; seven unused bindings (buffers,
   sampled/storage images,sampler),eleven TOTAL dynamic offsets. None of the
   unused bindings is initialized. Pool sizes cover the full layout.
2. Guarded `--unwritten-used`: require the marker for rejected unwritten array
   element1, exactly one expected compatibility gap,zero draws/submissions and
   Corezero. Never execute this invalid input natively.
3. Strict verifier `verify-layer-execution.py REPORT_DIR --selected`, with
   native/guarded/unwritten stdout+stderr. It requires matching actual outputs
   and the negative gate, not just a successful process exit.
4. Regression `--dynamic`, `--empty`, `--unused`, immutable sampler
   failure/control/correction and command routing, followed by fresh existing
   Core/image/descriptor/BC gates using the newly compiled probe.
5. One isolated Muck integration; only then consider common runtime rollup.

Layer SHA0b32e738f33ce0f13e817baae47c4f2b50d0024fde7699b3045bccdfcedbed47.
Final consumer SHA2e18fb218ce7d6d485b846fe161e9de4b1f3a4c3e8863a72cb457aa476333c08.
Probe SHA0688126f22495088c33be40293a3f0e32bb348826114db0ba24c3a29b316b98c.
Earlier `mavk-selected-candidate` and `layer-selected-consumer` builds are NOT
the final test inputs; use verified/final directories and the prepared bundle.
No commit/push, no production U-AVF/Windows/Ubuntu changes.

## Independent D4 audit / next dependency

`commands.hpp` captures native generations during record and retains them until
reset/free (including secondary references); queueSubmit/queueSubmit2 validate
recording state and arrange null-image preludes, not late descriptor snapshots.
`createLayout` only admits the pre-existing exact native sampler-heap UAB form;
the common logical registerSet rejects extended layouts. Thus current generation
tests must NOT be used as proof of general update-after-bind semantics.

Before widening UAB: test sequential resubmission, multiple in-flight users,
reset/free retirement and submit failure independently; define how native
descriptor contents and private guard metadata observe the SAME late update.
Re-recording pending buffers, queue-idle waits or guessing flags is not a fix.
Do not implement/admit the next functional package before this one's GPU gates.

Storage maintenance: five old unsigned/aligned U-AVF build intermediates moved
from C: to `E:/Mavk/archives/project-intermediates-20261021/`,1,725,272,223bytes.
Hashes match, `moved-files.csv` records original paths. Recoverable, not deleted;
signed release APKs, source, VM images/data and Golden baselines left in place.

Additional recoverable offload after C: again fell below50GB: old diagnostic
`build-logs/windows-post-linux-reassessment-20261004/live-candidate.wim` moved
to `E:/Mavk/archives/windows-diagnostic-intermediates-20261021/`,646799491bytes,
source/destination SHA match recorded in moved-files.csv. This is an old build
intermediate, not installed Windows/Ubuntu data or the production boot image.
C: returned to50.1GB decimal; future old-diagnostic reuse must use archive path.
