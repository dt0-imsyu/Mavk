param([Parameter(Mandatory=$true)][string]$OutputDirectory,
      [Parameter(Mandatory=$true)][string]$VerifiedOffscreenBuild)
$ErrorActionPreference='Stop'
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $out){throw 'Preserve prior build'}
$workspace=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$llvm=Join-Path $workspace 'tools/toolchains/llvm-mingw-20260826-clean-extract/llvm-mingw-20260826-ucrt-x86_64/bin'
New-Item -ItemType Directory -Path $out | Out-Null
$headers=Join-Path ([IO.Path]::GetFullPath($VerifiedOffscreenBuild)) 'include'
if(!(Test-Path -LiteralPath "$headers/vulkan/vulkan.h")){throw 'Existing verified Vulkan header build required'}
foreach($arch in @('aarch64','x86_64')){
    New-Item -ItemType Directory -Path "$out/$arch" | Out-Null
    & "$llvm/$arch-w64-mingw32-clang++.exe" -std=c++17 -O2 -Wall -Wextra -Werror -Wno-missing-field-initializers -DMAVK_EXPERIMENTAL_WIN32_WSI -I $headers -shared -static '-Wl,--no-insert-timestamp' '-Wl,--build-id=none' "$PSScriptRoot/icd.cpp" -o "$out/$arch/mavk_icd.dll" -lws2_32 -lgdi32 -luser32 -ldwmapi
    if($LASTEXITCODE){throw "$arch opt-in WSI ICD build failed"}
    Copy-Item -LiteralPath "$PSScriptRoot/mavk_icd.json" -Destination "$out/$arch/"
    & "$llvm/$arch-w64-mingw32-clang++.exe" -std=c++17 -O2 -Wall -Wextra -Werror -Wno-missing-field-initializers -I $headers -I $VerifiedOffscreenBuild -static '-Wl,--no-insert-timestamp' '-Wl,--build-id=none' "$PSScriptRoot/../test/window.cpp" -o "$out/$arch/vulkan-window.exe" -lgdi32 -luser32
    if($LASTEXITCODE){throw "$arch standard Vulkan window consumer build failed"}
}
& "$llvm/x86_64-w64-mingw32-clang++.exe" -std=c++17 -O2 -Wall -Wextra -Werror -Wno-missing-field-initializers -I $headers -static "$PSScriptRoot/../test/wsi-state.cpp" -o "$out/wsi-state-test.exe" -lws2_32 -lgdi32 -luser32 -ldwmapi
if($LASTEXITCODE){throw 'WSI CPU unit build failed'}
& "$out/wsi-state-test.exe"
if($LASTEXITCODE){throw 'WSI CPU unit failed'}
Get-FileHash "$out/aarch64/mavk_icd.dll"
