param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $out){throw 'Preserve prior build'}
$workspace=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$llvm=Join-Path $workspace 'tools/toolchains/llvm-mingw-20260826-clean-extract/llvm-mingw-20260826-ucrt-x86_64/bin'
$headers=Join-Path $env:LOCALAPPDATA 'Android/Sdk/ndk/28.2.13676358/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include'
New-Item -ItemType Directory -Path $out,"$out/include" | Out-Null
# Only Khronos headers; never mix Android libc headers into Windows C++ builds.
foreach($dir in @('vulkan','vk_video')){Copy-Item -LiteralPath "$headers/$dir" -Destination "$out/include/" -Recurse}
$shaderCompiler=Join-Path $env:LOCALAPPDATA 'Android/Sdk/ndk/28.2.13676358/shader-tools/windows-x86_64/glslc.exe'
foreach($shader in @('offscreen.vert','offscreen.frag','vertex-buffer.vert','matrix.vert','uniform.vert')){
    & $shaderCompiler --target-env=vulkan1.0 -mfmt=c "$PSScriptRoot/../test/$shader" -o "$out/$shader.inc"
    if($LASTEXITCODE){throw 'Independent app shader compilation'}
}
foreach($arch in @('aarch64','x86_64')){
    New-Item -ItemType Directory -Path "$out/$arch" | Out-Null
    & "$llvm/$arch-w64-mingw32-clang++.exe" -std=c++17 -O2 -Wall -Wextra -Werror -Wno-missing-field-initializers -I "$out/include" -shared -static '-Wl,--no-insert-timestamp' '-Wl,--build-id=none' "$PSScriptRoot/icd.cpp" -o "$out/$arch/mavk_icd.dll" -lws2_32
    if($LASTEXITCODE){throw "$arch ICD build failed"}
    Copy-Item -LiteralPath "$PSScriptRoot/mavk_icd.json" -Destination "$out/$arch/"
    & "$llvm/$arch-w64-mingw32-clang++.exe" -std=c++17 -O2 -Wall -Wextra -Werror -Wno-missing-field-initializers -I "$out/include" -I $out -static '-Wl,--no-insert-timestamp' '-Wl,--build-id=none' "$PSScriptRoot/../test/offscreen.cpp" -o "$out/$arch/vulkan-offscreen.exe"
    if($LASTEXITCODE){throw "$arch independent offscreen app build failed"}
    & "$llvm/$arch-w64-mingw32-clang++.exe" -std=c++17 -O2 -Wall -Wextra -Werror -Wno-missing-field-initializers -DTEST_VERTEX_BUFFER=1 -I "$out/include" -I $out -static '-Wl,--no-insert-timestamp' '-Wl,--build-id=none' "$PSScriptRoot/../test/offscreen.cpp" -o "$out/$arch/vulkan-vertex-buffer.exe"
    if($LASTEXITCODE){throw "$arch vertex-buffer API app build failed"}
    & "$llvm/$arch-w64-mingw32-clang++.exe" -std=c++17 -O2 -Wall -Wextra -Werror -Wno-missing-field-initializers -DTEST_VERTEX_BUFFER=1 -DTEST_INDEXED_BUFFER=1 -I "$out/include" -I $out -static '-Wl,--no-insert-timestamp' '-Wl,--build-id=none' "$PSScriptRoot/../test/offscreen.cpp" -o "$out/$arch/vulkan-indexed-buffer.exe"
    if($LASTEXITCODE){throw "$arch indexed-staging API app build failed"}
    & "$llvm/$arch-w64-mingw32-clang++.exe" -std=c++17 -O2 -Wall -Wextra -Werror -Wno-missing-field-initializers -DTEST_VERTEX_BUFFER=1 -DTEST_INDEXED_BUFFER=1 -DTEST_MATRIX=1 -I "$out/include" -I $out -static '-Wl,--no-insert-timestamp' '-Wl,--build-id=none' "$PSScriptRoot/../test/offscreen.cpp" -o "$out/$arch/vulkan-matrix.exe"
    if($LASTEXITCODE){throw "$arch matrix API app build failed"}
    & "$llvm/$arch-w64-mingw32-clang++.exe" -std=c++17 -O2 -Wall -Wextra -Werror -Wno-missing-field-initializers -DTEST_VERTEX_BUFFER=1 -DTEST_INDEXED_BUFFER=1 -DTEST_MATRIX=1 -DTEST_UNIFORM_BUFFER=1 -I "$out/include" -I $out -static '-Wl,--no-insert-timestamp' '-Wl,--build-id=none' "$PSScriptRoot/../test/offscreen.cpp" -o "$out/$arch/vulkan-uniform.exe"
    if($LASTEXITCODE){throw "$arch uniform API app build failed"}
}
& "$llvm/x86_64-w64-mingw32-clang++.exe" -std=c++17 -O2 -Wall -Wextra -Werror -Wno-missing-field-initializers -I "$out/include" -static "$PSScriptRoot/../test/push-state.cpp" -o "$out/push-state-test.exe" -lws2_32
if($LASTEXITCODE){throw 'Push-state CPU unit build failed'}
& "$out/push-state-test.exe"
if($LASTEXITCODE){throw 'Push-state CPU unit failed'}
& "$llvm/x86_64-w64-mingw32-clang++.exe" -std=c++17 -O2 -Wall -Wextra -Werror -Wno-missing-field-initializers -I "$out/include" -static "$PSScriptRoot/../test/readback-state.cpp" -o "$out/readback-state-test.exe" -lws2_32
if($LASTEXITCODE){throw 'Readback CPU unit build failed'}
& "$out/readback-state-test.exe"
if($LASTEXITCODE){throw 'Readback CPU unit failed'}
Get-FileHash "$out/aarch64/mavk_icd.dll"
& "$llvm/x86_64-w64-mingw32-clang++.exe" -std=c++17 -O2 -Wall -Wextra -Werror -Wno-missing-field-initializers -I "$out/include" -static "$PSScriptRoot/../test/uniform-state.cpp" -o "$out/uniform-state-test.exe" -lws2_32
if($LASTEXITCODE){throw 'Uniform descriptor unit build failed'}
& "$out/uniform-state-test.exe"
if($LASTEXITCODE){throw 'Uniform descriptor unit failed'}
