# Experimental shared shader guards

`tool.cpp` links the public SPIRV-Tools Optimizer API, enabling descriptor-index,
descriptor-init, buffer-range and texel-buffer checks in **one** pass. Metadata
is set1, bindings0(log)/1(input). Unlike stacking CLI passes, this does not
re-instrument another pass's generated metadata/debug resources. Only original
set0 is currently accepted. Outputs must validate for Vulkan1.1.

External dependency: KhronosGroup/SPIRV-Tools **v2022.4**, commit
`eb0a36633d2acf4de82588504f951ad0f2cecacb`; SPIRV-Headers from its DEPS,
`85a1ed200d50660786c1a88d9166e871123cce39`.
Both preserve upstream Apache2 licenses. Not vendored wholesale into Mavk.

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
all atomics, all shader stages/texture operations and production cache/lifetime
integration remain separate requirements. Debug logging still costs atomics
on invalid accesses; no production overhead claim.

Build using SDK CMake/Ninja and host llvm-mingw (not Android target compiler):

1. Obtain exact external source/header commits above into a healthy disk path.
2. `git apply --check <Mavk>/compatibility/shaders/buffer-atomics.patch`, apply.
3. Configure this CMake project with `MAVK_SPIRV_TOOLS_SOURCE=<external source>`,
   C/C++ host compilers, Ninja path and PYTHON_EXECUTABLE. Tests/executables of
   upstream are off; this builds the two libraries and small Mavk tool only.
4. Build target `mavk-shader-tool`. Run `mavk-shader-tool input.spv fresh.spv`.
5. Test on actual Mali via the existing isolated harness; source/build success
   alone is NOT runtime PASS. See STATE.md for authoritative measured outcome.

Legacy `experiments/instrument-null.py` retains stock NDK pass behavior for
already-proven buffer/image gates and rejects atomics. It is not the new tool.
