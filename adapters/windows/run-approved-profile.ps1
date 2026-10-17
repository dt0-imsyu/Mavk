param([ValidateRange(210,1800)][int]$ObserverSeconds=900,
      [ValidatePattern('^[a-zA-Z0-9-]+$')][string]$EvidenceName='mavk-windows-compute-20261015')
$ErrorActionPreference='Stop'
$workspace=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$envNames=@('PSCI_CPU_ON','PSCI_RESERVE','PSCI_RUNTIME','PSCI_ZERO_CONTEXT','PSCI_ALL_CPUS','PSCI_PMU','PMU_COMMON_AB','PSCI_ENTRY_MARKER','PSCI_DEFER_RETURN','PSCI_PRE_START_ONLY','PSCI_STARTUP_WAIT','PSCI_CACHE_PUBLISH','PSCI_FINAL_STATE','PSCI_EXPLICIT_IPI_AB') | ForEach-Object {'UAVF_KD_'+$_}
$previous=@{}
try{
    foreach($name in $envNames+@('UAVF_WINDOWS_RAM_GOP','UAVF_KD_PSCI_POST_START_MINIMAL','UAVF_IDD_SOURCE_ONLY','UAVF_MAVK_BRIDGE_OPTIN','UAVF_MAVK_VALIDATION')){$previous[$name]=[Environment]::GetEnvironmentVariable($name,'Process')}
    foreach($name in $envNames){[Environment]::SetEnvironmentVariable($name,'1','Process')}
    $env:UAVF_WINDOWS_RAM_GOP='1';$env:UAVF_MAVK_BRIDGE_OPTIN='1';$env:UAVF_MAVK_VALIDATION='1'
    [Environment]::SetEnvironmentVariable('UAVF_KD_PSCI_POST_START_MINIMAL',$null,'Process')
    [Environment]::SetEnvironmentVariable('UAVF_IDD_SOURCE_ONLY',$null,'Process')
    & "$workspace/Non-root-Windows-VM/tools/windows-reassessment/android-owner/run-service-test.ps1" -EvidenceName $EvidenceName -ResumeWindowsInstallTarget -HoldRuntime -ObserverSeconds $ObserverSeconds -EmulateUserPmcc -QuietDriverNotifications -Candidate live-name-fix -FixedEightCpuGic -MatchHostCpus -KeepRunning
}finally{foreach($name in $previous.Keys){[Environment]::SetEnvironmentVariable($name,$previous[$name],'Process')}}
