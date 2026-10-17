param([string]$SourceRoot='E:/Mavk/research/texture-4ee2501',
      [string]$GlmRoot='E:/Mavk/research/glm-1ad55c5016339b83b7eec98c31007e0aee57d2bf',
      [string]$Headers='E:/Mavk/artifacts/windows-icd-uniform-build3-20261016/include',
      [string]$LoaderBuild='E:/Mavk/artifacts/vulkan-loader-arm64-build1-20261015',
      [Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$out=[IO.Path]::GetFullPath($OutputDirectory);if(Test-Path $out){throw 'Preserve existing build'}
$commit='4ee2501c8280cf71d1f9ce231cd8ebd47b58adde'
$tree=Invoke-RestMethod "https://api.github.com/repos/SaschaWillems/Vulkan/git/trees/${commit}?recursive=1"
$pins=@{};foreach($entry in $tree.tree){if($entry.type -eq 'blob'){$pins[$entry.path]=$entry.sha}}
$manifest=@()
foreach($file in Get-ChildItem $SourceRoot -Recurse -File){
    $relative=[IO.Path]::GetRelativePath($SourceRoot,$file.FullName).Replace('\','/')
    if($relative.StartsWith('assets/')){continue}
    $bytes=[IO.File]::ReadAllBytes($file.FullName);$prefix=[Text.Encoding]::ASCII.GetBytes("blob $($bytes.Length)"+[char]0)
    $blob=[byte[]]::new($prefix.Length+$bytes.Length);$prefix.CopyTo($blob,0);$bytes.CopyTo($blob,$prefix.Length)
    $hash=[Convert]::ToHexString([Security.Cryptography.SHA1]::HashData($blob)).ToLowerInvariant()
    if(!$pins.ContainsKey($relative) -or $hash -ne $pins[$relative]){throw "Upstream source mismatch: $relative"}
    $manifest+=@{path=$relative;git_blob=$hash}
}
$asset="$SourceRoot/assets/textures/metalplate01_rgba.ktx"
if((Get-FileHash $asset).Hash -ne '38495B089915D38E2AE6CE14CDF872FF7D2DF6F79F3A1E5A7A7851CBE65882BD'){throw 'Original asset mismatch'}
$workspace=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$llvm="$workspace/tools/toolchains/llvm-mingw-20260826-clean-extract/llvm-mingw-20260826-ucrt-x86_64/bin"
$glslc="$env:LOCALAPPDATA/Android/Sdk/ndk/28.2.13676358/shader-tools/windows-x86_64/glslc.exe"
New-Item -ItemType Directory -Path $out,"$out/shaders/glsl/texture","$out/assets/textures" | Out-Null
$objects=@()
foreach($name in @('texture','checkheader','filestream','memstream','swap','hashlist','hashtable','errstr')){
    & "$llvm/aarch64-w64-mingw32-clang.exe" -std=c99 -O2 -DKTX_STATIC -I "$SourceRoot/external/ktx/include" -I "$SourceRoot/external/ktx/other_include" -ffunction-sections -fdata-sections -c "$SourceRoot/external/ktx/lib/$name.c" -o "$out/$name.o"
    if($LASTEXITCODE){throw "Original KTX $name build"};$objects+="$out/$name.o"
}
$sources=@("$SourceRoot/examples/texture/texture.cpp")
foreach($name in @('vulkanexamplebase','VulkanTools','VulkanDebug','VulkanDevice','VulkanBuffer','VulkanSwapChain','VulkanUIOverlay')){$sources+="$SourceRoot/base/$name.cpp"}
foreach($name in @('imgui','imgui_draw','imgui_widgets')){$sources+="$SourceRoot/external/imgui/$name.cpp"}
& "$llvm/aarch64-w64-mingw32-clang++.exe" -std=c++20 -O2 -DNDEBUG -DKTX_STATIC -Wno-ignored-pragmas -DVK_USE_PLATFORM_WIN32_KHR '-DVK_EXAMPLE_SHADERS_DIR="./shaders/"' '-DVK_EXAMPLE_ASSETS_DIR="./assets/"' -I $Headers -I $GlmRoot -I "$SourceRoot/base" -I "$SourceRoot/external/ktx/include" -I "$SourceRoot/external/ktx/other_include" -ffunction-sections -fdata-sections -static '-Wl,--gc-sections' '-Wl,--no-insert-timestamp' @sources @objects "$LoaderBuild/loader/loader/libvulkan-1.dll.a" -o "$out/texture.exe" -luser32 -lshell32 -lgdi32 -lshcore -lwinmm
if($LASTEXITCODE){throw 'Unmodified ARM64 texture build'}
foreach($stage in @('vert','frag')){& $glslc --target-env=vulkan1.0 "$SourceRoot/shaders/glsl/texture/texture.$stage" -o "$out/shaders/glsl/texture/texture.$stage.spv";if($LASTEXITCODE){throw 'Original texture shader build'}}
Copy-Item $asset "$out/assets/textures/"
$report=@{result='UPSTREAM_TEXTURE_ARM64_BUILD_ONLY_NOT_RUNTIME_PASS';commit=$commit;source_unchanged=$true;asset_commit='a27c0e584434d59b7c7a714e9180eefca6f0ec4b';asset_sha256=(Get-FileHash $asset).Hash;exe_sha256=(Get-FileHash "$out/texture.exe").Hash;files=$manifest}
[IO.File]::WriteAllText("$out/build-report.json",($report|ConvertTo-Json -Depth 5))
$report|Select-Object result,commit,exe_sha256
