param([Parameter(Mandatory=$true)][string]$OutputDirectory,[switch]$Graphics,[switch]$FetchBundle,[switch]$ChunkedBundle)
$ErrorActionPreference='Stop'
if($Graphics -and $FetchBundle){throw 'Choose one finite guest operation'}
if($ChunkedBundle -and !$FetchBundle){throw 'Chunked archive requires explicit fetch mode'}
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $out){throw 'Preserve old artifacts, choose fresh output directory'}
$workspace=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$llvm=Join-Path $workspace 'tools/toolchains/llvm-mingw-20260826-clean-extract/llvm-mingw-20260826-ucrt-x86_64/bin'
New-Item -ItemType Directory -Path $out | Out-Null
foreach($arch in @('aarch64','x86_64')){
    $defines=@();if($Graphics){$defines+= '-DMAVK_GRAPHICS_PROBE=1'}
    if($FetchBundle){$defines+='-DMAVK_BUNDLE_FETCH=1'}
    if($ChunkedBundle){$defines+='-DMAVK_BUNDLE_CHUNKED=1'}
    & "$llvm/$arch-w64-mingw32-clang.exe" @defines -Oz -Wall -Wextra -Werror -fno-builtin -fno-stack-protector -nostdlib '-Wl,--entry,mavk_entry' '-Wl,--no-insert-timestamp' '-Wl,--build-id=none' '-Wl,--gc-sections' "$PSScriptRoot/bridge_probe.c" -o "$out/mavk-bridge-$arch.exe" -lws2_32 -lkernel32
    if($LASTEXITCODE){throw "Guest bridge $arch compile failed"}
    & "$llvm/llvm-strip.exe" --strip-all "$out/mavk-bridge-$arch.exe"
    if($LASTEXITCODE){throw 'Strip failed'}
}
& "$out/mavk-bridge-x86_64.exe" --wire-selftest
if($LASTEXITCODE){throw 'Actual client wire self-test failed'}
Get-FileHash "$out/mavk-bridge-aarch64.exe"
Get-Item "$out/mavk-bridge-aarch64.exe" | Select-Object Length
