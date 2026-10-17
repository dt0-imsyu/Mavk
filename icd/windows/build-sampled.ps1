param([Parameter(Mandatory=$true)][string]$OutputDirectory,
      [string]$Headers='E:/Mavk/artifacts/windows-icd-uniform-build3-20261016/include',
      [string]$LoaderBuild='E:/Mavk/artifacts/vulkan-loader-arm64-build1-20261015')
$ErrorActionPreference='Stop'
$out=[IO.Path]::GetFullPath($OutputDirectory);if(Test-Path $out){throw 'Preserve earlier candidate'}
New-Item -ItemType Directory -Path $out | Out-Null
$workspace=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$llvm=Join-Path $workspace 'tools/toolchains/llvm-mingw-20260826-clean-extract/llvm-mingw-20260826-ucrt-x86_64/bin'
$core=Join-Path $workspace 'Mavk/core/include'
$glslc=Join-Path $env:LOCALAPPDATA 'Android/Sdk/ndk/28.2.13676358/shader-tools/windows-x86_64/glslc.exe'
foreach($stage in @('vert','frag')){
    & $glslc --target-env=vulkan1.0 -mfmt=c "$workspace/Mavk/harness/shaders/sampled-scene.$stage" -o "$out/sampled-scene.$stage.inc"
    if($LASTEXITCODE){throw "Sampled $stage shader build"}
}
foreach($arch in @('aarch64','x86_64')){
    New-Item -ItemType Directory -Path "$out/$arch" | Out-Null
    & "$llvm/$arch-w64-mingw32-clang++.exe" -std=c++17 -O2 -Wall -Wextra -Werror -Wno-missing-field-initializers -DMAVK_EXPERIMENTAL_WIN32_WSI -DMAVK_EXPERIMENTAL_SAMPLED -I $Headers -I $core -shared -static '-Wl,--no-insert-timestamp' "$PSScriptRoot/icd.cpp" -o "$out/$arch/mavk_icd.dll" -lws2_32 -lgdi32 -luser32 -ldwmapi
    if($LASTEXITCODE){throw "$arch sampled candidate build"}
    Copy-Item "$PSScriptRoot/mavk_icd.json" "$out/$arch/"
}
& "$llvm/aarch64-w64-mingw32-clang++.exe" -std=c++17 -O2 -Wall -Wextra -Werror -Wno-missing-field-initializers -I $Headers -I $out -static '-Wl,--no-insert-timestamp' "$PSScriptRoot/../test/sampled.cpp" "$LoaderBuild/loader/loader/libvulkan-1.dll.a" -o "$out/aarch64/vulkan-sampled.exe"
if($LASTEXITCODE){throw 'Standard Vulkan sampled consumer build'}
foreach($test in @('sampled-resource-state','sampled-descriptor-state')){
    & "$llvm/x86_64-w64-mingw32-clang++.exe" -std=c++17 -O2 -Wall -Wextra -Werror -Wno-missing-field-initializers -I $Headers -I $core -static "$PSScriptRoot/../test/$test.cpp" -o "$out/$test.exe" -lws2_32
    if($LASTEXITCODE){throw "$test build"}
    & "$out/$test.exe";if($LASTEXITCODE){throw "$test CPU gate"}
}
Get-FileHash "$out/aarch64/mavk_icd.dll"
