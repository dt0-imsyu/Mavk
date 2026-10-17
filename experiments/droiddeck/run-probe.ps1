param([Parameter(Mandatory=$true)][string]$OutputDirectory,[ValidateSet('stage-and-run','proot-control')][string]$Script='stage-and-run')
$ErrorActionPreference='Stop'
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $out){throw 'Preserve existing evidence, choose fresh directory'}
New-Item -ItemType Directory -Path $out | Out-Null
$adb=Join-Path $env:LOCALAPPDATA 'Android/Sdk/platform-tools/adb.exe'
$info=[Diagnostics.ProcessStartInfo]::new($adb)
$info.UseShellExecute=$false;$info.CreateNoWindow=$true
$info.RedirectStandardOutput=$true;$info.RedirectStandardError=$true
foreach($argument in @('shell','run-as','com.termux','/data/data/com.termux/files/usr/bin/bash',"/data/local/tmp/mavk-droiddeck-$Script.sh")){[void]$info.ArgumentList.Add($argument)}
$process=[Diagnostics.Process]::new();$process.StartInfo=$info
try{
    [void]$process.Start();$stdout=$process.StandardOutput.ReadToEndAsync();$stderr=$process.StandardError.ReadToEndAsync()
    if(!$process.WaitForExit(90000)){$process.Kill();throw 'Bounded PoC observer timeout; inspect guest process before retry'}
    $text=$stdout.GetAwaiter().GetResult();$errors=$stderr.GetAwaiter().GetResult()
    [IO.File]::WriteAllText((Join-Path $out 'stdout.log'),$text)
    [IO.File]::WriteAllText((Join-Path $out 'stderr.log'),$errors)
    $summary=[ordered]@{exit_code=$process.ExitCode;offscreen_pass=$text.Contains('GLIBC_HYBRIS_G925_OFFSCREEN_PASS');runtime='DroidDeck linuxfs-r9 library subset + pinned4073873 libhybris';context='Termux debug UID/runas_app, NOT DroidDeck app UID or complete session';production_changed=$false}
    [IO.File]::WriteAllText((Join-Path $out 'summary.json'),($summary|ConvertTo-Json))
    $text;$errors;$summary|ConvertTo-Json
}finally{$process.Dispose()}
