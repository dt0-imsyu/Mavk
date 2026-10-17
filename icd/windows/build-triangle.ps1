param([string]$SourceRoot='E:/Mavk/research/triangle-4ee2501',
      [string]$GlmRoot='E:/Mavk/research/glm-1ad55c5016339b83b7eec98c31007e0aee57d2bf',
      [Parameter(Mandatory=$true)][string]$IcdBuild,
      [Parameter(Mandatory=$true)][string]$LoaderBuild,
      [Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $out){throw 'Preserve prior application build'}
$commit='4ee2501c8280cf71d1f9ce231cd8ebd47b58adde'
if((Get-FileHash "$SourceRoot/examples/triangle/triangle.cpp").Hash -ne '96FF619230B0CAE0C5B6D489085611C8F32B79E58F075FEC654336EDAD047717'){throw 'Pinned triangle identity mismatch'}
if((Get-FileHash 'E:/Mavk/research/glm-1ad55c5.tar.gz').Hash -ne 'CEC07D19A13DF44030D0A3FAC920303AA3BB72ADD52A3B80BFEB3187E71D7015'){throw 'Pinned GLM identity mismatch'}
# Verify every downloaded source byte against the immutable upstream git tree.
$tree=Invoke-RestMethod -Uri "https://api.github.com/repos/SaschaWillems/Vulkan/git/trees/${commit}?recursive=1"
$pins=@{};foreach($entry in $tree.tree){if($entry.type -eq 'blob'){$pins[$entry.path]=$entry.sha}}
$manifest=@()
foreach($file in Get-ChildItem -LiteralPath $SourceRoot -Recurse -File){
    $relative=[IO.Path]::GetRelativePath($SourceRoot,$file.FullName).Replace('\','/')
    if(!$pins.ContainsKey($relative)){throw "Unknown upstream source: $relative"}
    $bytes=[IO.File]::ReadAllBytes($file.FullName);$prefix=[Text.Encoding]::ASCII.GetBytes("blob $($bytes.Length)"+[char]0)
    $blob=[byte[]]::new($prefix.Length+$bytes.Length);$prefix.CopyTo($blob,0);$bytes.CopyTo($blob,$prefix.Length)
    $sha=[Convert]::ToHexString([Security.Cryptography.SHA1]::HashData($blob)).ToLowerInvariant()
    if($sha -ne $pins[$relative]){throw "Upstream blob mismatch: $relative"}
    $manifest+=@{path=$relative;git_blob=$sha;sha256=(Get-FileHash $file.FullName).Hash}
}
$workspace=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$compiler=Join-Path $workspace 'tools/toolchains/llvm-mingw-20260826-clean-extract/llvm-mingw-20260826-ucrt-x86_64/bin/aarch64-w64-mingw32-clang++.exe'
$glslc=Join-Path $env:LOCALAPPDATA 'Android/Sdk/ndk/28.2.13676358/shader-tools/windows-x86_64/glslc.exe'
New-Item -ItemType Directory -Path $out,"$out/shaders/glsl/triangle","$out/assets" | Out-Null
[IO.File]::WriteAllText("$out/source-manifest.json",(@{commit=$commit;files=$manifest}|ConvertTo-Json -Depth 5))
$sources=@("$SourceRoot/examples/triangle/triangle.cpp")
foreach($name in @('vulkanexamplebase','VulkanTools','VulkanDebug','VulkanDevice','VulkanBuffer','VulkanSwapChain','VulkanUIOverlay')){$sources+="$SourceRoot/base/$name.cpp"}
foreach($name in @('imgui','imgui_draw','imgui_widgets')){$sources+="$SourceRoot/external/imgui/$name.cpp"}
# Build definitions only; no application/framework/shader edits or ABI substitutes.
& $compiler -std=c++20 -O2 -DNDEBUG -Wno-ignored-pragmas -DVK_USE_PLATFORM_WIN32_KHR '-DVK_EXAMPLE_SHADERS_DIR="./shaders/"' '-DVK_EXAMPLE_ASSETS_DIR="./assets/"' -I "$IcdBuild/include" -I $GlmRoot -I "$SourceRoot/base" -I "$SourceRoot/external/ktx/include" -I "$SourceRoot/external/ktx/other_include" -ffunction-sections -fdata-sections -static '-Wl,--gc-sections' '-Wl,--no-insert-timestamp' @sources "$LoaderBuild/loader/loader/libvulkan-1.dll.a" -o "$out/triangle.exe" -luser32 -lshell32 -lgdi32 -lshcore -lwinmm
if($LASTEXITCODE){throw 'Unmodified ARM64 triangle build failed'}
foreach($stage in @('vert','frag')){
    & $glslc --target-env=vulkan1.0 "$SourceRoot/shaders/glsl/triangle/triangle.$stage" -o "$out/shaders/glsl/triangle/triangle.$stage.spv"
    if($LASTEXITCODE){throw 'Upstream shader build failed'}
}
$report=@{result='UPSTREAM_TRIANGLE_ARM64_BUILD_ONLY_NOT_RUNTIME_PASS';commit=$commit;source_unchanged=$true;exe_sha256=(Get-FileHash "$out/triangle.exe").Hash;scope='No guest, pixels, FPS or compatibility PASS'}
[IO.File]::WriteAllText("$out/build-report.json",($report|ConvertTo-Json))
$report
