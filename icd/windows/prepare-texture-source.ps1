param([string]$OutputDirectory='E:/Mavk/research/texture-4ee2501',
      [string]$FrameworkSource='E:/Mavk/research/triangle-4ee2501')
$ErrorActionPreference='Stop'
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path $out){throw 'Preserve prior immutable source'}
$commit='4ee2501c8280cf71d1f9ce231cd8ebd47b58adde'
$assetsCommit='a27c0e584434d59b7c7a714e9180eefca6f0ec4b'
$tree=Invoke-RestMethod "https://api.github.com/repos/SaschaWillems/Vulkan/git/trees/${commit}?recursive=1"
$pins=@{};foreach($entry in $tree.tree){if($entry.type -eq 'blob'){$pins[$entry.path]=$entry.sha}}
New-Item -ItemType Directory -Path $out | Out-Null
Copy-Item "$FrameworkSource/*" $out -Recurse
$paths=@('examples/texture/texture.cpp','shaders/glsl/texture/texture.vert','shaders/glsl/texture/texture.frag')
$paths+=@($pins.Keys | Where-Object {$_ -match '^external/ktx/(lib/|include/|other_include/|LICENSE.md$|NOTICE.md$)'})
foreach($path in $paths){
    $target=Join-Path $out $path
    if(!(Test-Path $target)){
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null
        Invoke-WebRequest "https://raw.githubusercontent.com/SaschaWillems/Vulkan/$commit/$path" -OutFile $target
    }
}
function BlobHash([string]$path){
    $bytes=[IO.File]::ReadAllBytes($path);$prefix=[Text.Encoding]::ASCII.GetBytes("blob $($bytes.Length)"+[char]0)
    $blob=[byte[]]::new($prefix.Length+$bytes.Length);$prefix.CopyTo($blob,0);$bytes.CopyTo($blob,$prefix.Length)
    [Convert]::ToHexString([Security.Cryptography.SHA1]::HashData($blob)).ToLowerInvariant()
}
$manifest=@()
foreach($file in Get-ChildItem $out -Recurse -File){
    $relative=[IO.Path]::GetRelativePath($out,$file.FullName).Replace('\','/')
    if(!$pins.ContainsKey($relative) -or (BlobHash $file.FullName) -ne $pins[$relative]){throw "Immutable upstream mismatch: $relative"}
    $manifest+=@{path=$relative;git_blob=$pins[$relative]}
}
$assetTree=Invoke-RestMethod "https://api.github.com/repos/SaschaWillems/Vulkan-Assets/git/trees/${assetsCommit}?recursive=1"
$asset=$assetTree.tree | Where-Object {$_.path -eq 'textures/metalplate01_rgba.ktx'}
if($asset.sha -ne '6f3f6123acee50d203174a93a4853856b4d915c6' -or $asset.size -ne 1398236){throw 'Pinned asset identity'}
New-Item -ItemType Directory -Path "$out/assets/textures" | Out-Null
$assetPath="$out/assets/textures/metalplate01_rgba.ktx"
Invoke-WebRequest "https://raw.githubusercontent.com/SaschaWillems/Vulkan-Assets/$assetsCommit/textures/metalplate01_rgba.ktx" -OutFile $assetPath
if((BlobHash $assetPath) -ne $asset.sha){throw 'Asset bytes mismatch'}
[pscustomobject]@{result='IMMUTABLE_TEXTURE_SOURCE_READY_NOT_RUNTIME_PASS';commit=$commit;files=$manifest.Count;asset_commit=$assetsCommit;asset_bytes=$asset.size;asset_sha256=(Get-FileHash $assetPath).Hash}
