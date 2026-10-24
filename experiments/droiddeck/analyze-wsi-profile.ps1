param([Parameter(Mandatory=$true)][string[]]$Logs, [int]$Last = 20)
$ErrorActionPreference = 'Stop'
foreach ($log in $Logs) {
    $rows = @(Get-Content -LiteralPath $log | Where-Object { $_ -match '^MAVK_WSI_PROFILE seconds=' } | Select-Object -Last $Last | ForEach-Object {
        $row = @{}
        foreach ($field in [regex]::Matches($_, '(\w+)=([0-9]+(?:\.[0-9]+)?)')) {
            $row[$field.Groups[1].Value] = [double]::Parse($field.Groups[2].Value, [Globalization.CultureInfo]::InvariantCulture)
        }
        $row
    })
    if (!$rows.Count) { throw "No WSI profile records in $log" }
    $sum = @{}
    foreach ($row in $rows) { foreach ($key in $row.Keys) { $sum[$key] += $row[$key] } }
    $result = [ordered]@{ log = $log; records = $rows.Count; seconds = $sum.seconds; frames = $sum.frames
        sourcePresentsPerSecond = [math]::Round($sum.frames / $sum.seconds, 3); uniqueContentVerified = $false }
    foreach ($key in 'gpuWaitMs','xSyncMs','mapMs','copyMs','pacingMs','xQueueMs') {
        $result[$key + 'PerFrame'] = [math]::Round($sum[$key] / $sum.frames, 4)
    }
    $result.readbackMiBPerSecond = [math]::Round($sum.copiedBytes / 1MB / $sum.seconds, 2)
    [pscustomobject]$result | ConvertTo-Json
}
