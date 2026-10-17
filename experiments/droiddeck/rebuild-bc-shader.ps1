param([Parameter(Mandatory=$true)][string]$SourceDirectory,[Parameter(Mandatory=$true)][string]$Output)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $Output){throw 'Fresh shader output required'}
$tools=Join-Path $env:LOCALAPPDATA 'Android/Sdk/ndk/28.2.13676358/shader-tools/windows-x86_64'
$source=Join-Path $SourceDirectory 'hybris/vulkan/compat/shaders'
& "$tools/glslc.exe" --target-env=vulkan1.0 -O0 "$source/bc_decode.comp" -o $Output
if($LASTEXITCODE){throw 'BC shader compilation failed'}
& "$tools/spirv-val.exe" --target-env vulkan1.0 $Output
if($LASTEXITCODE){throw 'BC shader validation failed'}
$data=[IO.File]::ReadAllBytes([IO.Path]::GetFullPath($Output))
$rows=[Collections.Generic.List[string]]::new()
for($at=0;$at -lt $data.Length;$at+=32){
    $words=for($offset=$at;$offset -lt [Math]::Min($at+32,$data.Length);$offset+=4){'0x{0:x8}' -f [BitConverter]::ToUInt32($data,$offset)}
    $rows.Add('    '+($words -join ', ')+',')
}
$text="// Generated from patched source. Includes MIT bc7.glsl/bc6h.glsl; see LICENSE.bcdec.`nstatic const uint32_t hybris_bc_decode_spv[] = {`n"+($rows -join "`n")+"`n};`n"
[IO.File]::WriteAllText((Join-Path $source 'bc_decode.inc'),$text,[Text.Encoding]::ASCII)
Get-FileHash -LiteralPath $Output
