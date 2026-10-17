param([Parameter(Mandatory=$true)][string]$Bundle,[Parameter(Mandatory=$true)][string]$Report)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $Report){throw 'Preserve previous evidence'}
$root=[IO.Path]::GetFullPath($Bundle)
$info=[Diagnostics.ProcessStartInfo]::new((Join-Path $root 'x86_64/vulkan-offscreen.exe'))
$info.UseShellExecute=$false;$info.CreateNoWindow=$true
$info.RedirectStandardOutput=$true;$info.RedirectStandardError=$true
$info.Environment['VK_DRIVER_FILES']=Join-Path $root 'x86_64/mavk_icd.json'
$info.Environment['VK_LOADER_DEBUG']='error,warn,driver'
$process=[Diagnostics.Process]::new();$process.StartInfo=$info
try{
    [void]$process.Start();$stdout=$process.StandardOutput.ReadToEndAsync();$stderr=$process.StandardError.ReadToEndAsync()
    if(!$process.WaitForExit(10000)){$process.Kill();throw 'Host loader smoke timeout'}
    $out=$stdout.GetAwaiter().GetResult();$err=$stderr.GetAwaiter().GetResult()
    $pass=$process.ExitCode -eq 1 -and $err.Contains('MAVK_ICD_CREATE_INSTANCE') -and $err.Contains('MAVK_ICD_INSTANCE_TRANSPORT=Viosock access') -and $err.Contains('Found ICD manifest file')
    $reportData=[ordered]@{result=$(if($pass){'HOST_X64_LOADER_DISCOVERY_ENTRY_PASS_TRANSPORT_UNAVAILABLE'}else{'FAIL'});scope='Host Windows x64 loader smoke only. NOT ARM64 guest ICD/device/render PASS.';exit_code=$process.ExitCode;stdout=$out;stderr=$err;icd_sha256=(Get-FileHash (Join-Path $root 'x86_64/mavk_icd.dll')).Hash}
    [IO.File]::WriteAllText([IO.Path]::GetFullPath($Report),($reportData|ConvertTo-Json -Depth 4),[Text.UTF8Encoding]::new($false))
    if(!$pass){throw 'Host loader smoke did not match expected boundary'}
    $reportData.result
}finally{$process.Dispose()}
