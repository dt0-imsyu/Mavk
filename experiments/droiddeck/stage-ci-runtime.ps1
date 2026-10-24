param(
    [Parameter(Mandatory=$true)][string]$Apk,
    [Parameter(Mandatory=$true)][string]$Repository,
    [Parameter(Mandatory=$true)][string]$EvidenceDirectory,
    [string]$ExpectedCommit='0916a538aff4c08f799a4bd9fa17dd3c98b01e99'
)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath($Repository)
$head=(& git -C $repo rev-parse HEAD).Trim()
if($LASTEXITCODE -or $head -ne $ExpectedCommit){throw 'CI payload requires the exact reviewed source commit'}
if(Test-Path -LiteralPath $EvidenceDirectory){throw 'Fresh evidence directory required'}
Add-Type -AssemblyName System.IO.Compression.FileSystem
$package=[IO.Compression.ZipFile]::OpenRead([IO.Path]::GetFullPath($Apk))
$records=[Collections.Generic.List[object]]::new()
try{
    foreach($entry in $package.Entries){
        $name=$entry.FullName
        if($name.EndsWith('/')){continue}
        if($name -match '(^/|\\|(^|/)\.\.(/|$))'){throw 'Unsafe APK member'}
        $relative=$null
        if($name.StartsWith('assets/')){
            $asset=$name.Substring(7)
            if($asset -eq 'pulseaudio.tzst' -or $asset -match '^graphics_driver/adrenotools-(turnip25\.1\.0-quest1|turnip26\.3\.0-a6xx|turnip26\.3\.0-quest2)\.tzst$'){
                $relative='app/build/prepared-assets/'+$asset
            }else{$relative='app/src/main/assets/'+$asset}
        }elseif($name -match '^lib/arm64-v8a/(libproot\.so|libproot-loader\.so|libdirectaudiorelay\.so)$'){
            $relative='app/src/main/jniLibs/arm64-v8a/'+$Matches[1]
        }
        if(!$relative){continue}
        $target=[IO.Path]::GetFullPath((Join-Path $repo $relative))
        if(!$target.StartsWith($repo+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Extraction escaped checkout'}
        $stream=$entry.Open();$hash=[Security.Cryptography.SHA256]::Create()
        try{$digest=[Convert]::ToHexString($hash.ComputeHash($stream)).ToLowerInvariant()}finally{$stream.Dispose();$hash.Dispose()}
        $exists=Test-Path -LiteralPath $target
        if($exists){if((Get-FileHash -LiteralPath $target).Hash.ToLowerInvariant() -ne $digest){throw "Existing file differs; preserved: $relative"}}
        else{New-Item -ItemType Directory -Path ([IO.Path]::GetDirectoryName($target)) -Force | Out-Null;[IO.Compression.ZipFileExtensions]::ExtractToFile($entry,$target,$false)}
        $records.Add([ordered]@{member=$name;target=$relative;sha256=$digest;reused=$exists})
    }
}finally{$package.Dispose()}
New-Item -ItemType Directory -Path $EvidenceDirectory | Out-Null
[ordered]@{source_commit=$head;apk_sha256=(Get-FileHash -LiteralPath $Apk).Hash.ToLowerInvariant();ci_run=38693863397;runtime_inputs=$records} | ConvertTo-Json -Depth 5 | Out-File (Join-Path $EvidenceDirectory 'ci-runtime-origin.json') -Encoding utf8
Write-Output "Staged/verified $($records.Count) runtime files; upstream runtime verification is still required."
