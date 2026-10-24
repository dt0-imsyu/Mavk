# Format component substitution: concrete FAIL -> GPU PASS

## Boundary and source

Vulkan robust texel-buffer reads outside the view return zero, followed by
format component substitution. Missing alpha in R/RG/RGB therefore becomes1.
Null descriptor reads are a separate rule and remain all-zero. Integer atomics
return scalar results and must not receive image component substitution.
Source: https://docs.vulkan.org/spec/latest/chapters/shaders.html#shaders-robust-buffer-access2
No whole robustness2/nullDescriptor feature or CTS claim is made.

Old storage-texel fixture examined only imageLoad(...).r, which could not detect
wrong alpha. New storage-texel-components.comp captures full uvec4 while reusing
the real layer-execution consumer and existing storage-texel resources.
Actual installed APK22/common runtime1e859af6 FAIL reproduced:
`storage texel result mismatch phase=0 word=3 got=0 expected=1`.
Baseline is protected Mavk execution, not native undefined OOB/null access.

## Implementation

Tracked spirv-tools-texel-components.patch applies after texel-atomics.patch to
pinned SPIRV-Tools. GenCheckCode accepts an optional invalid-value ID; existing
descriptor/init/buffer/atomic callers retain their all-zero default. Only
GenTexBuffCheckCode supplies typed(0,0,0,1) for known non-alpha storage formats.
Typed float1 or integer1 uses the original result component type. RGBA formats
retain zero alpha on OOB. Valid image operations/masks remain original. No
additional CPU pixel copies, waits, layer feature claims or app-specific rules.

Unknown image formats (including ordinary uniform texel fetch) cannot infer
actual VkBufferView format from SPIR-V alone. That remaining component-mask
contract is NOT solved here and needs actual view-format metadata/specialization.
R64 forms and general non-buffer image robustness remain outside this patch.
Other known accepted format/stage branches are not GPU-certified by one R32 test.

## Actual candidate gates

Layer8470144ca2503bd02ba6a3c9aec7d6943a99789d44810a85caaed393f2266dc2
uses verified frontend8b1d0db1 and new optimizer
1f1f274af886d448621d70696dbf0d42bc756d1a62efe10ba129cd77d9858401.
Consumerd3b6ce30221774d4186e19634f3f059943e06df4939df1eec214dfb2d0ffadc1.
GPU R32_UINT:valid/OOB/negative components,diagnostic-null allzero,restored valid;
192target/result words correct,all sentinels retained;3dispatches/submissions,
gaps0/CoreliveZero1. Isolated complete gate2.083sec,baseline2.138sec;these are
test wall times,NOT performance comparison/FPS. All25 bounded GPU scenarios
PASS on candidate61.834sec;6aggregate forms/1152words PASS15.229sec. Host
new compiler gate5assertions,old25assertions/9rejections,atomic22assertions,
copy6/1,texel5,storage5 all PASS. Guarded output spirv-val Vulkan1.2 PASS.

Artifacts:E:/Mavk/artifacts/texel-components-gpu-20261022/:
baseline.tar.gz SHA69f820e2204b8f1486815549c6e2ded17fe6968ff6fde610f85996e94f449042;
candidate.tar.gz SHAe5ed79361f7be9e46f2a401be288a21ef1172476fa0f0a087fc7bcdc032ada58;
regressions.tar.gz SHA6d4328dcdca8f9ce19969e8fec927ed9f2bd3289b03ded387f19debbdcd50dc6.
Strict verifier --texel-components checks actual candidate mapping and all25.

Backed-up pre-change CPP SHAba3c46a69fdaae77acafa1bc9e6582975850ae8603b50a10f41fb588e2ed5571,
header045b14ceadf0326804e7021df75d9a212efd057604b36b5dbe23cf7ce5589d9c,
inE:/Mavk/artifacts/texel-components-source-base-20261022/.
New patch reverse git apply --check --ignore-space-change PASS;research CRLF.
Installed APK22/default optimizer/runtime assets remain unchanged for now.
Real consumer gate PASS:Muck1096 mapped candidate8470144c,visible menu/Settings/
input,176678guarded commands,no observed gap/compile/device-loss errors. Saved
muck-components.tar.gz SHA3bc01f24ccff191d0fa0a39a769f574ae3c41e629f1b33b6b9e4df78aee3053f,
screenshots muck-candidate.png/muck-settings.png and6form reports retained.
Game quit normally and diagnostic override removed;Steam open on APK22.
No unique-FPS/full-game claim or new APK rollup. Default verified atomic optimizer
1040dcb5/frontend8b1d0db1 remain pinned;new optimizer1f1f274a is explicit only.
