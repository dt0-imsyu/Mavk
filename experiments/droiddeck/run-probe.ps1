param([Parameter(Mandatory=$true)][string]$OutputDirectory,[ValidateSet('stage-and-run','proot-control','bc-control')][string]$Script='stage-and-run',[switch]$BcCompatibilityLayer,[ValidateSet('original','baseline','rg32f','linear')][string]$BcLayerBuild='original')
$ErrorActionPreference='Stop'
if($BcCompatibilityLayer -and $Script -ne 'bc-control'){throw 'BC compatibility switch requires BC probe'}
if($BcLayerBuild -ne 'original' -and (!$BcCompatibilityLayer -or $Script -ne 'bc-control')){throw 'BC layer build requires BC compatibility probe'}
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $out){throw 'Preserve existing evidence, choose fresh directory'}
New-Item -ItemType Directory -Path $out | Out-Null
$adb=Join-Path $env:LOCALAPPDATA 'Android/Sdk/platform-tools/adb.exe'
$info=[Diagnostics.ProcessStartInfo]::new($adb)
$info.UseShellExecute=$false;$info.CreateNoWindow=$true
$info.RedirectStandardOutput=$true;$info.RedirectStandardError=$true
foreach($argument in @('shell','run-as','com.termux','/data/data/com.termux/files/usr/bin/bash',"/data/local/tmp/mavk-droiddeck-$Script.sh")){[void]$info.ArgumentList.Add($argument)}
if($BcCompatibilityLayer){[void]$info.ArgumentList.Add($(switch($BcLayerBuild){'original'{'compat'}'baseline'{'rebuilt'}'rg32f'{'rg32f'}'linear'{'linear'}}))}
$process=[Diagnostics.Process]::new();$process.StartInfo=$info
try{
    [void]$process.Start();$stdout=$process.StandardOutput.ReadToEndAsync();$stderr=$process.StandardError.ReadToEndAsync()
    if(!$process.WaitForExit(90000)){$process.Kill();throw 'Bounded PoC observer timeout; inspect guest process before retry'}
    $text=$stdout.GetAwaiter().GetResult();$errors=$stderr.GetAwaiter().GetResult()
    [IO.File]::WriteAllText((Join-Path $out 'stdout.log'),$text)
    [IO.File]::WriteAllText((Join-Path $out 'stderr.log'),$errors)
    $summary=[ordered]@{exit_code=$process.ExitCode;offscreen_pass=$text.Contains('GLIBC_HYBRIS_G925_OFFSCREEN_PASS');bc_constant_pass=($process.ExitCode -eq 0 -and $text.Contains('GLIBC_HYBRIS_G925_BC_CONSTANT_PASS'));bc_linear_pass=($process.ExitCode -eq 0 -and $text.Contains('GLIBC_HYBRIS_G925_BC_LINEAR_PASS'));bc_compatibility_layer=[bool]$BcCompatibilityLayer;bc_layer_build=$BcLayerBuild;runtime='DroidDeck linuxfs-r9 library subset + pinned4073873 ICD; compatibility layer build selected separately';context='Termux debug UID/runas_app, NOT DroidDeck app UID or complete session';production_changed=$false}
    [IO.File]::WriteAllText((Join-Path $out 'summary.json'),($summary|ConvertTo-Json))
    $text;$errors;$summary|ConvertTo-Json
}finally{$process.Dispose()}
