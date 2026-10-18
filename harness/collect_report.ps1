param(
    [Parameter(Mandatory=$true)][ValidatePattern('^(feature-suite|native-compute|core-comparison|core-sampled|core-timeline|core-reuse|core-raster|core-descriptor-generations|core-bc(-channels|67|-texture)?|core-null-(buffers|images|atomics|texels)|core-ui(-layers|-bridge)?|bridge-probe|mavk-bridge-report|graphics-lifecycle|graphics-[0-9]+)\.json$')][string]$ReportName,
    [Parameter(Mandatory=$true)][string]$OutputFile,
    [ValidateSet('org.mavk.probe','com.example.uavfwinpetest')][string]$Package='org.mavk.probe'
)
$ErrorActionPreference='Stop'
if(($Package -eq 'com.example.uavfwinpetest') -ne ($ReportName -eq 'mavk-bridge-report.json')){throw 'Report/package scope mismatch'}
$adb=Join-Path $env:LOCALAPPDATA 'Android/Sdk/platform-tools/adb.exe'
$output=[IO.Path]::GetFullPath($OutputFile)
if(Test-Path -LiteralPath $output){throw 'Preserve prior evidence; choose fresh output filename'}
if(!(Test-Path -LiteralPath ([IO.Path]::GetDirectoryName($output)))){throw 'Output parent must already exist'}
$before=(& $adb shell run-as $Package sha256sum "files/$ReportName") -join ''
if($LASTEXITCODE -or $before -notmatch '^([0-9a-f]{64})\s'){throw 'App-owned report not ready'}
$expected=$Matches[1]
# Capture this debuggable APK's report via its existing authorized run-as UID.
# No broad storage permission, private Binder, root or filesystem policy change.
$info=[Diagnostics.ProcessStartInfo]::new($adb)
$info.UseShellExecute=$false;$info.CreateNoWindow=$true
$info.RedirectStandardOutput=$true;$info.RedirectStandardError=$true
foreach($argument in @('exec-out','run-as',$Package,'cat',"files/$ReportName")){[void]$info.ArgumentList.Add($argument)}
$process=[Diagnostics.Process]::new();$process.StartInfo=$info
$memory=[IO.MemoryStream]::new()
try {
    [void]$process.Start()
    $copy=$process.StandardOutput.BaseStream.CopyToAsync($memory)
    $errors=$process.StandardError.ReadToEndAsync()
    if(!$process.WaitForExit(10000)){$process.Kill();throw 'Read-only report capture timed out; APK not stopped'}
    [void]$copy.GetAwaiter().GetResult()
    if($process.ExitCode){throw "Report capture failed: $($errors.GetAwaiter().GetResult())"}
    $bytes=$memory.ToArray()
} finally {$memory.Dispose();$process.Dispose()}
if($bytes.Length -lt 20 -or $bytes.Length -gt 1024*1024){throw 'Report outside bounded size'}
$document=[Text.Encoding]::UTF8.GetString($bytes)|ConvertFrom-Json
if(!$document.result){throw 'Not a completed probe report'}
$hash=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes)).ToLowerInvariant()
$after=(& $adb shell run-as $Package sha256sum "files/$ReportName") -join ''
if($LASTEXITCODE -or !$after.StartsWith($expected+' ') -or $hash -ne $expected){throw 'Report changed during read; no artifact written'}
$file=[IO.File]::Open($output,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write)
try {$file.Write($bytes,0,$bytes.Length)} finally {$file.Dispose()}
if((Get-FileHash -LiteralPath $output).Hash.ToLowerInvariant() -ne $expected){throw 'Artifact readback failed'}
[pscustomobject]@{result=$document.result;bytes=$bytes.Length;sha256=$expected;path=$output}
