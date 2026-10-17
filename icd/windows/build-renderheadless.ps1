param([string]$SourceRoot='E:/Mavk/research/renderheadless-4ee2501',
      [string]$GlmRoot='E:/Mavk/research/glm-1ad55c5016339b83b7eec98c31007e0aee57d2bf',
      [Parameter(Mandatory=$true)][string]$IcdBuild,
      [Parameter(Mandatory=$true)][string]$LoaderBuild,
      [Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $out){throw 'Preserve prior external application build'}
$expected=@{
    'renderheadless.cpp'='3353EAF467E223BBC34EB6A9D41D16A0D02CB7DAC17D8D6909D6219F0E810D64'
    'base/VulkanTools.h'='23B6AADD68CF12E0C81767FCD22D66B91105A6DF82BF32FD325F237E4F5E3DF6'
    'base/VulkanTools.cpp'='F1038CDFC54984AA2B0830DA62A5FBFA85EFE44206CB88114B86C4CBA53A0A37'
    'base/VulkanInitializers.hpp'='7D34143B0B999156A100888758171752D394466E96FC18002E9058E50CAC77CC'
    'base/CommandLineParser.hpp'='1F1463C09C29A4879D805A6AEE73CA256941F9B37829B33C7770332D98386C73'
    'shaders/glsl/renderheadless/triangle.vert'='688A972EC8AFD994C06157FF18D7927FF2D819A506779CA1BFC3D76B8E9F54C7'
    'shaders/glsl/renderheadless/triangle.frag'='F6546106FD9A46C6D251CAE30131627B60902F72F71F409B8E6F903093E8B11B'
}
foreach($name in $expected.Keys){if((Get-FileHash -LiteralPath "$SourceRoot/$name").Hash -ne $expected[$name]){throw "Upstream identity mismatch: $name"}}
if((Get-FileHash -LiteralPath 'E:/Mavk/research/glm-1ad55c5.tar.gz').Hash -ne 'CEC07D19A13DF44030D0A3FAC920303AA3BB72ADD52A3B80BFEB3187E71D7015'){throw 'Pinned GLM archive identity'}
$workspace=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$compiler=Join-Path $workspace 'tools/toolchains/llvm-mingw-20260826-clean-extract/llvm-mingw-20260826-ucrt-x86_64/bin/aarch64-w64-mingw32-clang++.exe'
$glslc=Join-Path $env:LOCALAPPDATA 'Android/Sdk/ndk/28.2.13676358/shader-tools/windows-x86_64/glslc.exe'
New-Item -ItemType Directory -Path $out,"$out/shaders/glsl/renderheadless" | Out-Null
# Build configuration only. No edits to upstream application, shaders or helpers.
& $compiler -std=c++20 -O2 -DNDEBUG -Wno-ignored-pragmas '-DVK_EXAMPLE_SHADERS_DIR="./shaders/"' -I "$IcdBuild/include" -I $GlmRoot -I "$SourceRoot/base" -static '-Wl,--gc-sections' '-Wl,--no-insert-timestamp' "$SourceRoot/renderheadless.cpp" "$SourceRoot/base/VulkanTools.cpp" "$LoaderBuild/loader/loader/libvulkan-1.dll.a" -o "$out/renderheadless.exe" -luser32 -lshell32
if($LASTEXITCODE){throw 'Unmodified ARM64 application compile'}
foreach($stage in @('vert','frag')){
    & $glslc --target-env=vulkan1.0 "$SourceRoot/shaders/glsl/renderheadless/triangle.$stage" -o "$out/shaders/glsl/renderheadless/triangle.$stage.spv"
    if($LASTEXITCODE){throw 'Unmodified upstream shader compilation'}
}
$report=[ordered]@{result='UPSTREAM_ARM64_BUILD_ONLY_NOT_RUNTIME_PASS';source_commit='4ee2501c8280cf71d1f9ce231cd8ebd47b58adde';source_sha256=$expected['renderheadless.cpp'];glm_commit='1ad55c5016339b83b7eec98c31007e0aee57d2bf';source_unchanged=$true;resolution='1024x1024 upstream default';exe_sha256=(Get-FileHash "$out/renderheadless.exe").Hash;shader_path='./shaders/';scope='No guest execution, pixel or performance PASS'}
[IO.File]::WriteAllText("$out/build-report.json",($report|ConvertTo-Json),[Text.UTF8Encoding]::new($false))
[pscustomobject]$report
