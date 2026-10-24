# Experimental shared shader guards

`spirv-tools-texel-components.patch` follows the atomic patch: OOB reads of
known non-alpha storage-texel formats get typed(0,0,0,1); descriptor-null results
stay zero. Unknown image formats still need runtime view-format information.
R32_UINT GPU FAIL-before/PASS-after and25 bounded cases recorded in
docs/TEXEL_COMPONENT_SUBSTITUTION_2026-10-22.md. Not full feature/CTS support.

Latest bounded extension: `texel_atomics.hpp` plus
`spirv-tools-texel-atomics.patch` (apply after existing buffer/texel patches).
It instruments integer Buffer/R32i/R32ui image-pointer atomic references using
descriptor initialization and view-size checks, preserving native atomics and
memory operands. Unsupported pointer/dimension/format forms reject explicitly.
R32_UINT compute GPU proof covers add/exchange/CAS/read, valid/OOB/negative/null/
restored phases, 320 checked words. This is not all image atomics, full
robustness2/nullDescriptor or CTS. See STORAGE_ATOMICS_NEXT_2026-10-21.md.
Host test: `mavk-shader-guards-test --texel-atomic <original.spv>`;
isolated GPU control: `storage-atomic-guarded-control.sh`.
The current compiler supports explicitly selected Vulkan11/12/13 targets;
historical Vulkan11-only descriptions below describe earlier checkpoints.

`GuardCompiler` in `include/mavk/compat/shader_guards.hpp` is shared by the host
CLI and Android adapters. `tool.cpp` is only a file-input/output frontend.
The implementation links the SPIRV-Tools Optimizer API, enabling descriptor-index,
descriptor-init, buffer-range and texel-buffer checks in **one** pass. Metadata
uses bindings0(log)/1(input) in one private set. Unlike stacking CLI passes,
this does not re-instrument another pass's generated metadata/debug resources.
The default CLI accepts only original set0 and reserves set1. Native callers
can configure public sets before the private guard set. Outputs must validate
for Vulkan1.1.

External dependency: KhronosGroup/SPIRV-Tools **v2022.4**, commit
`eb0a36633d2acf4de82588504f951ad0f2cecacb`; SPIRV-Headers from its DEPS,
`85a1ed200d50660786c1a88d9166e871123cce39`.
Upstream SPIRV-Tools Apache2 and SPIRV-Headers MIT-style licenses are preserved.
Not vendored wholesale into Mavk.

`buffer-atomics.patch` is Mavk's minimal source modification:

- Admits core integer buffer atomic instructions227..242 into the existing
  pointer/descriptor/range analysis, which already guards cloned instructions
  and produces zero for invalid result-bearing accesses. Scope/memory operands
  of valid atomics remain unchanged. Does not admit image atomic pointers.
- Includes `<type_traits>` explicitly: Clang23/libc++ no longer supplies
  aligned_storage transitively via `<memory>`. Pure build compatibility fix.

The wrapper rejects physical/pointer-arithmetic/image-atomic and extension
float-atomic forms it cannot support. This is not an exhaustive opcode/support
verifier. Native features remain closed. Runtime descriptor state, complete
pointer analysis, buffer-overflow arithmetic, component-granular robustness2,
all atomics, all shader stages/texture operations and production consumer/lifetime
integration remain separate requirements. Debug logging still costs atomics
on invalid accesses; no production overhead claim.

`GuardCompilerOptions::reportErrors=false` retains guarded accesses and fallback
results, but omits generated diagnostic-report calls using the pinned optimizer
IR API. Only generated function IDs (above the original module bound) with the
instrumenter's exact report-name prefix are eligible. Dead functions and unused
entry-point interfaces are removed; a surviving private log binding rejects the
module. Metadata binding1 stays active. This permits read-only vertex guards on
devices without vertexPipelineStoresAndAtomics; it does not emulate application
vertex stores/atomics or advertise that feature. Default reporting is unchanged.
SPIR-V1.6 needs explicit unused-interface cleanup, unlike older fixtures.

Build using SDK CMake/Ninja and host llvm-mingw (not Android target compiler):

1. Obtain exact external source/header commits above into a healthy disk path.
2. `git apply --check <Mavk>/compatibility/shaders/buffer-atomics.patch`, apply.
3. Configure this CMake project with `MAVK_SPIRV_TOOLS_SOURCE=<external source>`,
   C/C++ host compilers, Ninja path and PYTHON_EXECUTABLE. Tests/executables of
   upstream are off; this builds the two libraries and small Mavk tool only.
4. Build target `mavk-shader-tool`. Run `mavk-shader-tool input.spv fresh.spv`.
5. Test on actual Mali via the existing isolated harness; source/build success
   alone is NOT runtime PASS. See STATE.md for authoritative measured outcome.

Host target `mavk-shader-guards-test` takes two different original SPIR-V fixture
paths (atomic and image gate originals). It checks exact cache reuse, eviction,
retained ownership, deterministic recompilation, byte-budget bypass and rejection
of malformed/already-instrumented input. The fixtures are compiled from the
existing feasibility shader sources, not embedded expected compiler output.

Legacy `experiments/instrument-null.py` retains stock NDK pass behavior for
already-proven buffer/image gates and rejects atomics. It is not the new tool.

## Runtime compiler/cache contract

`GuardCompiler::compile` accepts an original word vector and returns immutable
shared ownership of validated guarded words. Both input and output are validated
for Vulkan1.1; this is not yet a general Vulkan1.2/1.3 DXVK shader compiler.
`GuardCompilerOptions::publicSetCount` is immutable for each compiler/cache,
defaults to1,and supports1..7 public sets. Public indices must be below this
count; the private guard set index equals the count (`guardSetIndex()`). Caller
must reserve the same index in NativeDescriptorAdapter and respect actual
maxBoundDescriptorSets. There is no automatic native-capability or pipeline
layout negotiation. IndexAndInit metadata ABI remains fixed. Unsupported forms
reject rather than being silently passed through.
The opcode filter is deliberately not an exhaustive feature-admission boundary.

Callers serialize access. The bounded LRU defaults to16 entries/8MiB of retained
original+guarded words. Hits compare exact original words, not just a hash.
Eviction releases cache ownership only; returned shared words remain valid.
Outputs larger than the configured cache budget are returned without caching.
Input is limited to1MiB, output to4MiB after optimization; these limits do not
bound optimizer transient allocations or execution time. No background threads,
per-frame compilation, Vulkan handles or hidden GPU waits are introduced.

Android builds use the same CMake target `mavk-shader-guards` with the NDK
toolchain,arm64-v8a/android-29,c++_static and position-independent archives.
`harness/build.ps1 -GuardCompilerBuild <build> -RuntimeAtomicShader <original.spv>`
links the three static archives and includes the original upstream licenses.
The isolated harness's `runtimeGuards` intent compiles the original atomic shader
on device,checks warm reuse and rejected inputs,then executes the existing
null/buffer/range/atomic GPU gate. Acceptance requires the strict verifier's
`--atomics --shared-state --runtime-compiler` options, not build success alone.

## Runtime diagnostic policy

`GuardCompilerOptions::reportErrors=false` retains access checks and zero/skip
behavior but omits generated diagnostic writes. The CLI accepts `--no-reports`.
The experimental layer can select this immutable per-device policy with
`MAVK_GUARD_REPORTS=0`; feature queries and native stores/atomics bits are not
changed. Reporting remains the default until the consumer A/B is verified.
Use report-enabled GPU coverage oracles for access counts, and report-disabled
pixel/live-write/null-write oracles to verify the quiet path. No-report operation
is not a reason to weaken null semantics or advertise incomplete robustness2.
