param([string]$OutputDirectory='E:/Mavk/research/ui-4ee2501',
      [string]$ExistingSource='E:/Mavk/research/texture-4ee2501')
$ErrorActionPreference='Stop'
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $out){throw 'Preserve existing pinned source'}
$commit='4ee2501c8280cf71d1f9ce231cd8ebd47b58adde'
$tree=Invoke-RestMethod "https://api.github.com/repos/SaschaWillems/Vulkan/git/trees/${commit}?recursive=1"
$pins=@{};foreach($entry in $tree.tree){if($entry.type -eq 'blob'){$pins[$entry.path]=$entry.sha}}
$paths=@('LICENSE.md','shaders/glsl/base/uioverlay.vert','shaders/glsl/base/uioverlay.frag')
$paths+=@($pins.Keys | Where-Object {$_ -match '^external/imgui/[^/]+\.(cpp|h|txt)$'})
New-Item -ItemType Directory -Path $out | Out-Null
foreach($relative in $paths){
    if(!$pins.ContainsKey($relative)){throw "Missing upstream path: $relative"}
    $target=Join-Path $out $relative
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null
    $existing=Join-Path $ExistingSource $relative
    if(Test-Path -LiteralPath $existing){Copy-Item -LiteralPath $existing -Destination $target}
    else {Invoke-WebRequest "https://raw.githubusercontent.com/SaschaWillems/Vulkan/$commit/$relative" -OutFile $target}
    $data=[IO.File]::ReadAllBytes($target);$prefix=[Text.Encoding]::ASCII.GetBytes("blob $($data.Length)"+[char]0)
    $blob=[byte[]]::new($prefix.Length+$data.Length);$prefix.CopyTo($blob,0);$data.CopyTo($blob,$prefix.Length)
    if([Convert]::ToHexString([Security.Cryptography.SHA1]::HashData($blob)).ToLowerInvariant() -ne $pins[$relative]){throw "Upstream bytes mismatch: $relative"}
    [pscustomobject]@{path=$relative;git_blob=$pins[$relative];sha256=(Get-FileHash -LiteralPath $target).Hash}
}
