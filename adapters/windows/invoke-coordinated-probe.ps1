# Call only after observing the normal guest PowerShell prompt. Uses the existing
# authorized input/provider, not a new guest command or authentication channel.
param([Parameter(Mandatory=$true)][string]$TokenFile,
      [Parameter(Mandatory=$true)][string]$CommandFile,
      [Parameter(Mandatory=$true)][int]$ExpectedCid)
$ErrorActionPreference='Stop'
$workspace=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$adb=Join-Path $env:LOCALAPPDATA 'Android/Sdk/platform-tools/adb.exe'
$state=(& $adb shell /apex/com.android.virt/bin/vm list)-join "`n"
if($state -notmatch "cid: $ExpectedCid," -or $state -notmatch 'requesterUid: 10042,'){throw 'Expected isolated owner required'}
$command=[IO.File]::ReadAllText([IO.Path]::GetFullPath($CommandFile))
if($command.Length -gt 2048 -or !$command.StartsWith('$s=') -or !$command.Contains('MavkComputeProbe')){throw 'Prepared finite probe command required'}
$helper=Join-Path $workspace 'Non-root-Windows-VM/tools/windows-reassessment/send-diagnostic-windows-input.ps1'
$key=(Get-Content -LiteralPath $TokenFile -Raw).Trim()
if($key -notmatch '^[a-zA-Z0-9_-]+$'){throw 'Invalid owner token'}
# A producer reconnect can lose a modifier release from the preceding Win+R.
# Ordinary key-up events only; caller must still freshly verify console focus.
foreach($symbol in @(65515,65516,65507,65508,65513,65514,65505,65506)){
    $release=& $adb shell content call --uri content://com.example.uavfwinpetest.owner --method input_key --arg $key --extra "symbol:i:$symbol" --extra 'state:i:0'
    if(($release -join '') -notmatch 'channel_written=true'){throw 'Modifier release unavailable'}
}
& $helper -TokenFile $TokenFile -Text $command
$submitted=[DateTime]::UtcNow
try{& $helper -TokenFile $TokenFile -Symbol 65293}catch{
    # Guest producer can close on the secure-desktop transition; do not resend.
    'ENTER_CHANNEL_TRANSITION; no repeated input'
}
Start-Sleep -Seconds 7
$reply=& $adb shell content call --uri content://com.example.uavfwinpetest.owner --method mavk_probe --arg $key
[pscustomobject]@{submitted_utc=$submitted.ToString('o');request_utc=[DateTime]::UtcNow.ToString('o');owner_ack=($reply -join ' ');scope='Admission only; inspect actual reports'}
