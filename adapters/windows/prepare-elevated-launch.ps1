param([Parameter(Mandatory=$true)][string]$OutputFile,
      [ValidateRange(1,16)][int]$Attempt=6,[switch]$Graphics,[switch]$FetchBundle,[switch]$Compact)
$ErrorActionPreference='Stop'
if($Graphics -and $FetchBundle){throw 'Choose one operation'}
if(Test-Path -LiteralPath $OutputFile){throw 'Preserve previous command artifact'}
# Only the existing disposable carrier and one temporary client are touched.
# Supply the finite script before normal UAC; no input into elevated consoles.
$script=@'
$ErrorActionPreference='Stop'
$root=Join-Path $env:TEMP 'MavkComputeProbe'
if(Test-Path $root){throw 'Preserve previous probe'}
New-Item -ItemType Directory -Path $root|Out-Null
try {
 $p=@(Get-CimInstance Win32_Volume|ForEach-Object{$f=$_.DeviceID+'IDDWORK\idd-motion-source.exe';if(Test-Path -LiteralPath $f){$f}})
 if($p.Count -ne 1){throw 'Carrier count'}
 $b=[IO.File]::ReadAllBytes($p[0]);if($b.Length -ne 7680){throw 'Carrier length'}
 $exe=Join-Path $root 'probe.exe';[IO.File]::WriteAllBytes($exe,$b[0..7167])
 if((Get-FileHash $exe).Hash -ne 'C9DEC0BAF7D07C79C849F323DB69F97C23A72E9C3FCA9236377705C16E663568'){throw 'Client hash'}
 & $exe *> (Join-Path $root 'stdout.txt')
 $code=$LASTEXITCODE
 ('EXIT='+$code)|Set-Content (Join-Path $root 'exit.txt')
}catch{($_|Out-String)|Set-Content (Join-Path $root 'error.txt')}
finally{if(Test-Path (Join-Path $root 'probe.exe')){Remove-Item -LiteralPath (Join-Path $root 'probe.exe')}}
'@
if($Attempt -gt 1){$script=$script.Replace('MavkComputeProbe','MavkComputeProbe'+$Attempt)}
if($Graphics){$script=$script.Replace('C9DEC0BAF7D07C79C849F323DB69F97C23A72E9C3FCA9236377705C16E663568','7D3A6E29488F7C14BB503E4BB832FF41362322D46DF4124CDDF68549117FBDDF')}
if($FetchBundle){$script=$script.Replace('C9DEC0BAF7D07C79C849F323DB69F97C23A72E9C3FCA9236377705C16E663568','8574B919EAE74F51C0FF8EEA047BE67698709E4BF7B4B34997FD9773387B809E')}
if($Compact){
    if(!$FetchBundle){throw 'Compact preparation scoped to fixed development artifact fetch'}
    $script=@'
$ErrorActionPreference='Stop';$r="$env:TEMP\MavkComputeProbeNUMBER";if(Test-Path $r){throw 'exists'};md $r|Out-Null;$f=@(gcim Win32_Volume|%{$_.DeviceID+'IDDWORK\idd-motion-source.exe'}|?{Test-Path -LiteralPath $_});if($f.Count-ne 1){throw 'carrier'};$b=[IO.File]::ReadAllBytes($f[0]);if($b.Length-ne 7680){throw 'size'};$x="$r\probe.exe";[IO.File]::WriteAllBytes($x,$b[0..7167]);if((Get-FileHash $x).Hash-ne '8574B919EAE74F51C0FF8EEA047BE67698709E4BF7B4B34997FD9773387B809E'){throw 'hash'};& $x *> "$r\stdout.txt";('EXIT='+$LASTEXITCODE)|Set-Content "$r\exit.txt";Remove-Item -LiteralPath $x
'@
    $script=$script.Replace('NUMBER',[string]$Attempt)
}
$single=($script -replace '\r?\n',';') -replace ';\s*}', '}' -replace '{\s*;', '{'
$literal=$single.Replace("'","''")
$command='$s='''+$literal+'''; Start-Process powershell.exe -Verb RunAs -ArgumentList @(''-NoProfile'',''-NonInteractive'',''-EncodedCommand'',[Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($s)))'
if($Compact){$command="Start-Process powershell.exe -Verb RunAs -ArgumentList '-NoProfile -NonInteractive -EncodedCommand "+[Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($single))+"'"}
if($command.Length -gt 2048){throw 'Existing input channel bound exceeded'}
$errors=$null;$tokens=$null
[void][Management.Automation.Language.Parser]::ParseInput($command,[ref]$tokens,[ref]$errors)
if($errors.Count){throw 'Outer command syntax invalid'}
[void][Management.Automation.Language.Parser]::ParseInput($single,[ref]$tokens,[ref]$errors)
if($errors.Count){throw 'Guest script syntax invalid'}
[IO.File]::WriteAllText([IO.Path]::GetFullPath($OutputFile),$command,[Text.UTF8Encoding]::new($false))
[pscustomobject]@{result='FINITE_UAC_COMMAND_PREPARED_NOT_EXECUTED';characters=$command.Length;path=$OutputFile}
