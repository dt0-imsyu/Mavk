param([Parameter(Mandatory=$true)][string]$IcdBuild,
      [Parameter(Mandatory=$true)][string]$LoaderBuild,
      [Parameter(Mandatory=$true)][string]$OutputDirectory,
      [ValidateSet('offscreen','vertex-buffer','indexed-buffer','matrix','renderheadless','window','uniform','triangle','sampled','texture')][string]$ApiTest='offscreen',
      [string]$ExternalBuild='')
$ErrorActionPreference='Stop'
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $out){throw 'Preserve previous bundle'}
New-Item -ItemType Directory -Path $out,"$out/payload" | Out-Null
$app=Join-Path $IcdBuild "aarch64/vulkan-$ApiTest.exe"
if($ApiTest -eq 'renderheadless'){
    $external=Get-Content -LiteralPath "$ExternalBuild/build-report.json" -Raw|ConvertFrom-Json
    if(!$external.source_unchanged -or $external.source_commit -ne '4ee2501c8280cf71d1f9ce231cd8ebd47b58adde'){throw 'Pinned unmodified external build required'}
    $app=Join-Path $ExternalBuild 'renderheadless.exe'
    if((Get-FileHash -LiteralPath $app).Hash -ne $external.exe_sha256){throw 'External executable identity'}
}
if($ApiTest -eq 'triangle'){
    $external=Get-Content -LiteralPath "$ExternalBuild/build-report.json" -Raw|ConvertFrom-Json
    if(!$external.source_unchanged -or $external.commit -ne '4ee2501c8280cf71d1f9ce231cd8ebd47b58adde'){throw 'Pinned unmodified triangle required'}
    $app=Join-Path $ExternalBuild 'triangle.exe'
    if((Get-FileHash $app).Hash -ne $external.exe_sha256){throw 'Triangle executable identity'}
}
$files=@{
    'mavk_icd.dll'=(Join-Path $IcdBuild 'aarch64/mavk_icd.dll')
    'mavk_icd.json'=(Join-Path $IcdBuild 'aarch64/mavk_icd.json')
    'vulkan-offscreen.exe'=$app
    'vulkan-1.dll'=(Join-Path $LoaderBuild 'loader/loader/vulkan-1.dll')
    'Vulkan-Loader-LICENSE.txt'='E:/Mavk/research/Vulkan-Loader-1.3.275/LICENSE.txt'
    'Vulkan-Headers-Apache-2.0.txt'='E:/Mavk/research/Vulkan-Headers-1.3.275/LICENSES/Apache-2.0.txt'
    'Vulkan-Headers-MIT.txt'='E:/Mavk/research/Vulkan-Headers-1.3.275/LICENSES/MIT.txt'
    'EXPERIMENTAL-ICD.txt'=(Join-Path $PSScriptRoot '../README.md')
}
if($ApiTest -eq 'texture'){
    $external=Get-Content -LiteralPath "$ExternalBuild/build-report.json" -Raw|ConvertFrom-Json
    if(!$external.source_unchanged -or $external.commit -ne '4ee2501c8280cf71d1f9ce231cd8ebd47b58adde' -or $external.asset_commit -ne 'a27c0e584434d59b7c7a714e9180eefca6f0ec4b'){throw 'Pinned unmodified textured app required'}
    $app=Join-Path $ExternalBuild 'texture.exe'
    if((Get-FileHash $app).Hash -ne $external.exe_sha256){throw 'Texture executable identity'}
    $files['vulkan-offscreen.exe']=$app
    foreach($stage in @('vert','frag')){$files["shaders/glsl/texture/texture.$stage.spv"]="$ExternalBuild/shaders/glsl/texture/texture.$stage.spv"}
    $asset="$ExternalBuild/assets/textures/metalplate01_rgba.ktx"
    if((Get-FileHash $asset).Hash -ne '38495B089915D38E2AE6CE14CDF872FF7D2DF6F79F3A1E5A7A7851CBE65882BD'){throw 'Original texture bytes required'}
    $files['assets/textures/metalplate01_rgba.ktx']=$asset
    $files['Sascha-Willems-MIT.txt']='E:/Mavk/research/texture-4ee2501/LICENSE.md'
    $files['KTX-LICENSE.txt']='E:/Mavk/research/texture-4ee2501/external/ktx/LICENSE.md'
    $files['KTX-NOTICE.txt']='E:/Mavk/research/texture-4ee2501/external/ktx/NOTICE.md'
    $files['UPSTREAM-BUILD.json']="$ExternalBuild/build-report.json"
    $files['GLM-LICENSE1.txt']='E:/Mavk/research/glm-1ad55c5016339b83b7eec98c31007e0aee57d2bf/util/conan-package/lib_licenses/LICENSE1.txt'
    $files['GLM-LICENSE2.txt']='E:/Mavk/research/glm-1ad55c5016339b83b7eec98c31007e0aee57d2bf/util/conan-package/lib_licenses/LICENSE2.txt'
    $files['ImGui-MIT.txt']='E:/Mavk/research/texture-4ee2501/external/imgui/LICENSE.txt'
}
if($ApiTest -eq 'renderheadless'){
    foreach($stage in @('vert','frag')){$files["shaders/glsl/renderheadless/triangle.$stage.spv"]="$ExternalBuild/shaders/glsl/renderheadless/triangle.$stage.spv"}
    $files['Sascha-Willems-MIT.txt']='E:/Mavk/research/renderheadless-4ee2501/LICENSE.md'
    $files['UPSTREAM-BUILD.json']="$ExternalBuild/build-report.json"
    $files['GLM-LICENSE1.txt']='E:/Mavk/research/glm-1ad55c5016339b83b7eec98c31007e0aee57d2bf/util/conan-package/lib_licenses/LICENSE1.txt'
    $files['GLM-LICENSE2.txt']='E:/Mavk/research/glm-1ad55c5016339b83b7eec98c31007e0aee57d2bf/util/conan-package/lib_licenses/LICENSE2.txt'
}
if($ApiTest -eq 'triangle'){
    foreach($stage in @('vert','frag')){$files["shaders/glsl/triangle/triangle.$stage.spv"]="$ExternalBuild/shaders/glsl/triangle/triangle.$stage.spv"}
    if(Test-Path "$IcdBuild/aarch64/vulkan-compute.exe"){$files['vulkan-compute.exe']="$IcdBuild/aarch64/vulkan-compute.exe"}
    $files['assets/README.txt']=(Join-Path $PSScriptRoot '../README.md')
    $files['Sascha-Willems-MIT.txt']='E:/Mavk/research/triangle-4ee2501/LICENSE.md'
    $files['ImGui-MIT.txt']='E:/Mavk/research/triangle-4ee2501/external/imgui/LICENSE.txt'
    $files['UPSTREAM-BUILD.json']="$ExternalBuild/build-report.json"
    $files['GLM-LICENSE1.txt']='E:/Mavk/research/glm-1ad55c5016339b83b7eec98c31007e0aee57d2bf/util/conan-package/lib_licenses/LICENSE1.txt'
    $files['GLM-LICENSE2.txt']='E:/Mavk/research/glm-1ad55c5016339b83b7eec98c31007e0aee57d2bf/util/conan-package/lib_licenses/LICENSE2.txt'
}
$manifest=@()
foreach($name in ($files.Keys|Sort-Object)){
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent "$out/payload/$name") | Out-Null
    Copy-Item -LiteralPath $files[$name] -Destination "$out/payload/$name"
    $manifest+= [ordered]@{name=$name;bytes=(Get-Item -LiteralPath "$out/payload/$name").Length;sha256=(Get-FileHash -LiteralPath "$out/payload/$name").Hash}
}
Compress-Archive -LiteralPath (Get-ChildItem -LiteralPath "$out/payload").FullName -DestinationPath "$out/mavk-icd-development.zip" -CompressionLevel Optimal
$zip=Get-Item -LiteralPath "$out/mavk-icd-development.zip"
if($zip.Length -gt 4194304){throw 'Bundle exceeds bounded development archive size'}
$report=[ordered]@{result='PRIVATE_DEVELOPMENT_BUNDLE_PREPARED_NOT_DELIVERED';bytes=$zip.Length;sha256=(Get-FileHash -LiteralPath $zip.FullName).Hash;loader_source='00893b9a03e526aec2c5bf487521d16dfa435229';headers_source='217e93c664ec6704ec2d8c36fa116c1a4a1e2d40';loader_unknown_extension_trampolines='C fallback; no unknown extensions in this test; not release-certified';files=$manifest}
[IO.File]::WriteAllText("$out/bundle-report.json",($report|ConvertTo-Json -Depth 5),[Text.UTF8Encoding]::new($false))
[pscustomobject]$report|Select-Object result,bytes,sha256
