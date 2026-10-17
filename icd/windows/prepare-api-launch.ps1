param([Parameter(Mandatory=$true)][string]$OutputFile,
      [Parameter(Mandatory=$true)][ValidatePattern('^[A-Fa-f0-9]{64}$')][string]$BundleSha256,
      [ValidateRange(1,4)][int]$Attempt=2)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputFile){throw 'Preserve previous command'}
# Prepared text only; execute through the unchanged existing manual console.
$command=@'
$ErrorActionPreference='Stop';$z=Join-Path $env:TEMP 'MavkIcdDevelopment1.zip';if((Get-FileHash $z).Hash -ne 'ZIP_HASH'){throw 'Bundle hash'};$r=Join-Path $env:TEMP 'MavkIcdApiNUMBER';if(Test-Path $r){throw 'Preserve previous test'};Expand-Archive -LiteralPath $z -DestinationPath $r;$s=New-Object Diagnostics.ProcessStartInfo;$s.FileName=Join-Path $r 'vulkan-offscreen.exe';$s.UseShellExecute=$false;$s.CreateNoWindow=$true;$s.RedirectStandardOutput=$true;$s.RedirectStandardError=$true;$s.EnvironmentVariables['VK_DRIVER_FILES']=Join-Path $r 'mavk_icd.json';$s.EnvironmentVariables['VK_LOADER_DEBUG']='error,warn,driver';$p=New-Object Diagnostics.Process;$p.StartInfo=$s;[void]$p.Start();$o=$p.StandardOutput.ReadToEndAsync();$e=$p.StandardError.ReadToEndAsync();$done=$p.WaitForExit(60000);if(!$done){$p.Kill();$p.WaitForExit()};[IO.File]::WriteAllText((Join-Path $r 'stdout.txt'),$o.GetAwaiter().GetResult());[IO.File]::WriteAllText((Join-Path $r 'stderr.txt'),$e.GetAwaiter().GetResult());if($done){('EXIT='+$p.ExitCode)|Set-Content (Join-Path $r 'exit.txt')}else{'DEADLINE_60S'|Set-Content (Join-Path $r 'exit.txt')};$p.Dispose()
'@
$command=$command.Replace('ZIP_HASH',$BundleSha256.ToUpperInvariant()).Replace('NUMBER',[string]$Attempt)
$errors=$null;$tokens=$null;[void][Management.Automation.Language.Parser]::ParseInput($command,[ref]$tokens,[ref]$errors)
if($errors.Count -or $command.Length -gt 2048){throw 'Existing manual channel command invalid'}
[IO.File]::WriteAllText([IO.Path]::GetFullPath($OutputFile),$command,[Text.UTF8Encoding]::new($false))
[pscustomobject]@{result='STANDARD_API_COMMAND_PREPARED_NOT_EXECUTED';characters=$command.Length}
