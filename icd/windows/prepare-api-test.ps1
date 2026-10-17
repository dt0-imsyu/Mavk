param([Parameter(Mandatory=$true)][string]$BundleDirectory,
      [ValidateRange(1,4)][int]$Attempt=3)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath($BundleDirectory)
$report=Get-Content -LiteralPath "$root/bundle-report.json" -Raw|ConvertFrom-Json
$hash=(Get-FileHash -LiteralPath "$root/mavk-icd-development.zip").Hash
if($hash -ne $report.sha256){throw 'Bundle identity mismatch'}
$command=@'
$ErrorActionPreference='Stop';$z=Join-Path $env:TEMP 'MavkIcdDevelopment1.zip';if((Get-FileHash $z).Hash -ne 'BUNDLE_HASH'){throw 'Bundle hash'};$r=Join-Path $env:TEMP 'MavkIcdApiATTEMPT';if(Test-Path $r){throw 'Preserve previous test'};Expand-Archive -LiteralPath $z -DestinationPath $r;$s=New-Object Diagnostics.ProcessStartInfo;$s.FileName=Join-Path $r 'vulkan-offscreen.exe';$s.UseShellExecute=$false;$s.CreateNoWindow=$true;$s.RedirectStandardOutput=$true;$s.RedirectStandardError=$true;$s.EnvironmentVariables['VK_DRIVER_FILES']=Join-Path $r 'mavk_icd.json';$s.EnvironmentVariables['VK_LOADER_DEBUG']='error,warn,driver';$p=New-Object Diagnostics.Process;$p.StartInfo=$s;[void]$p.Start();$o=$p.StandardOutput.ReadToEndAsync();$e=$p.StandardError.ReadToEndAsync();$done=$p.WaitForExit(60000);if(!$done){$p.Kill();$p.WaitForExit()};[IO.File]::WriteAllText((Join-Path $r 'stdout.txt'),$o.GetAwaiter().GetResult());[IO.File]::WriteAllText((Join-Path $r 'stderr.txt'),$e.GetAwaiter().GetResult());if($done){('EXIT='+$p.ExitCode)|Set-Content (Join-Path $r 'exit.txt')}else{'DEADLINE_60S'|Set-Content (Join-Path $r 'exit.txt')};$p.Dispose()
'@
$command=$command.Replace('BUNDLE_HASH',$hash).Replace('ATTEMPT',[string]$Attempt).Trim()
$errors=$null;$tokens=$null
[void][Management.Automation.Language.Parser]::ParseInput($command,[ref]$tokens,[ref]$errors)
if($errors.Count -or $command.Length -gt 2048){throw 'Finite manual command invalid'}
$file=[IO.File]::Open("$root/guest-api-command.txt",[IO.FileMode]::CreateNew,[IO.FileAccess]::Write)
try{$bytes=[Text.Encoding]::UTF8.GetBytes($command);$file.Write($bytes,0,$bytes.Length)}finally{$file.Dispose()}
[pscustomobject]@{result='MANUAL_API_TEST_PREPARED_NOT_EXECUTED';attempt=$Attempt;characters=$command.Length}
