param(
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [ValidatePattern('^sarek-native[1-9][0-9]?$')][string]$Slot='sarek-native1',
    [string]$BuildDirectory='E:/Mavk/artifacts/sarek-native-build1-20261018',
    [string]$ConsumerDirectory='E:/Mavk/artifacts/sarek-native-consumer11-20261018',
    [switch]$MavkLayer,
    [switch]$GuardedLayer,
    [switch]$Submit2Test,
    [switch]$NullImageTest,
    [switch]$RetrySubmitTest,
    [switch]$DiagnosticNullAdmission,
    [switch]$LegacySpirvDiagnostic,
    [string]$LayerDirectory='E:/Mavk/artifacts/mavk-glibc-layer17-20261018'
)
$ErrorActionPreference='Stop'
if($GuardedLayer -and !$MavkLayer){throw 'Guarded mode requires Mavk layer'}
if($Submit2Test -and !$GuardedLayer){throw 'Submit2 regression requires guarded mode'}
if($NullImageTest -and !$GuardedLayer){throw 'Null image regression requires guarded mode'}
if($RetrySubmitTest -and !$GuardedLayer){throw 'Submit retry regression requires guarded mode'}
if($DiagnosticNullAdmission -and (!$GuardedLayer -or $NullImageTest -or $RetrySubmitTest)){throw 'Diagnostic admission requires guarded mode without fixture fault injection'}
if($LegacySpirvDiagnostic -and !$DiagnosticNullAdmission){throw 'Legacy SPIR-V selection is restricted to explicit diagnostic admission'}
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $out){throw 'Fresh evidence directory required'}
$inputs=@{
    'sarek-d3d11-consumer'="$ConsumerDirectory/sarek-d3d11-consumer"
    'libdxvk_d3d11.so'="$BuildDirectory/src/d3d11/libdxvk_d3d11.so.0.30002"
    'libdxvk_dxgi.so.0'="$BuildDirectory/src/dxgi/libdxvk_dxgi.so.0.30002"
    'libSDL3.so.0'='E:/Mavk/artifacts/sarek-native-source-20261018/sdk/usr/lib/libSDL3.so.0.4.16'
    'libVkLayer_hybris_compat.so'='E:/Mavk/artifacts/hybris-bc5-layer1-20261018/rg32f-build/libVkLayer_hybris_compat.so'
    'Sarek-LICENSE.txt'='E:/Mavk/artifacts/sarek-native-source-20261018/source/LICENSE'
}
if($MavkLayer){
    $inputs['libVkLayer_mavk_compat.so']="$LayerDirectory/libVkLayer_mavk_compat.so"
    $inputs['VkLayer_mavk_compat.json']="$PSScriptRoot/../../adapters/vulkan-layer/VkLayer_mavk_compat.json"
    $inputs['SPIRV-Tools-LICENSE.txt']='E:/Mavk/artifacts/shader-guards-glibc1-20261018/licenses/SPIRV-Tools-LICENSE.txt'
    $inputs['SPIRV-Headers-LICENSE.txt']='E:/Mavk/artifacts/shader-guards-glibc1-20261018/licenses/SPIRV-Headers-LICENSE.txt'
}
foreach($inputFile in $inputs.Values){if(!(Test-Path -LiteralPath $inputFile) -or (Get-Item -LiteralPath $inputFile).Length -eq 0){throw "Not built: $inputFile"}}
New-Item -ItemType Directory -Path "$out/payload" | Out-Null
$checks=foreach($entry in ($inputs.GetEnumerator() | Sort-Object Key)){
    Copy-Item -LiteralPath $entry.Value -Destination "$out/payload/$($entry.Key)"
    "$((Get-FileHash -LiteralPath $entry.Value).Hash.ToLowerInvariant())  $($entry.Key)"
}
[IO.File]::WriteAllLines("$out/payload/SHA256SUMS",$checks,[Text.UTF8Encoding]::new($false))
tar -cf "$out/payload.tar" -C "$out/payload" .
if($LASTEXITCODE){throw 'Payload packaging failed'}
$adb=Join-Path $env:LOCALAPPDATA 'Android/Sdk/platform-tools/adb.exe'
& $adb push "$out/payload.tar" /data/local/tmp/mavk-sarek-native-payload.tar
if($LASTEXITCODE){throw 'Payload delivery failed'}
& $adb push "$PSScriptRoot/sarek-control.sh" /data/local/tmp/mavk-sarek-control.sh
if($LASTEXITCODE){throw 'Control delivery failed'}
& $adb shell run-as com.termux /data/data/com.termux/files/usr/bin/bash /data/local/tmp/mavk-sarek-control.sh $Slot $(if($Submit2Test){'guarded2'}elseif($GuardedLayer){'guarded'}elseif($MavkLayer){'mavk'}else{'hybris'}) $([int][bool]$NullImageTest) $([int][bool]$RetrySubmitTest) $([int][bool]$DiagnosticNullAdmission) $([int][bool]$LegacySpirvDiagnostic) 2>&1 | Tee-Object "$out/run.log"
$code=$LASTEXITCODE
$remote="/data/data/com.termux/files/home/mavk-droiddeck-poc-20261015/$Slot"
foreach($name in @('stdout.log','stderr.log')){
    & $adb shell run-as com.termux cat "$remote/$name" | Out-File -LiteralPath "$out/$name" -Encoding utf8
}
$consumerLog=Get-Content -LiteralPath "$out/stdout.log" -Raw
$layerLog=Get-Content -LiteralPath "$out/stderr.log" -Raw
if($code -eq 0 -and $layerLog -match 'proot info: vpid 1: terminated with signal (\d+)'){$code=128+[int]$Matches[1]}
$guardMatch=[regex]::Match($layerLog,'MAVK_LAYER_COMMAND_END guarded=1 draws=(\d+) submissions=(\d+) diagnosticWords=(\d+) remainingCommands=0')
$pixelPass=$consumerLog.Contains('VULKAN_SAREK_BLIT_PIXELS_PASS components=65536 submissions=4 resubmission=1 indirect=1 bounded=1')
$cleanupPass=$layerLog -match 'remainingSets=0 remainingBuffers=0 templates=1 remainingImages=0 remainingViews=0 remainingSamplers=0 remainingTexels=0 remainingTemplates=0' -and $layerLog -match 'remainingShaders=0 remainingPipelineLayouts=0 remainingPipelines=0 shaderBytes=0 pipelineBytes=0'
$guardPass=[bool]$GuardedLayer -and $pixelPass -and $guardMatch.Success -and $guardMatch.Groups[1].Value -eq '3' -and $guardMatch.Groups[2].Value -eq '4' -and [uint64]$guardMatch.Groups[3].Value -gt 0 -and $cleanupPass -and !$layerLog.Contains('MAVK_LAYER_GAP')
$imageInitPass=([regex]::Matches($layerLog,'MAVK_LAYER_IMAGE_INIT images=2 prelude=1 cpuWaits=0').Count -eq 1) -and $layerLog.Contains('MAVK_LAYER_IMAGE_END retainedPreludes=1') -and $layerLog.Contains('MAVK_LAYER_CORE_END liveZero=1')
$submitApi=if($Submit2Test){2}else{1}
$guardPass=$guardPass -and $imageInitPass -and $consumerLog.Contains("VULKAN_SUBMIT_API=$submitApi")
if($NullImageTest){$guardPass=$guardPass -and $consumerLog.Contains('VULKAN_TEST_NULL_IMAGE=1') -and $layerLog.Contains('MAVK_LAYER_NULL_INJECTION imageSlots=1')}
$retryPass=([regex]::Matches($layerLog,'MAVK_LAYER_IMAGE_TEST_REJECT beforeDriver=1').Count -eq 1) -and $consumerLog.Contains('VULKAN_IMAGE_SUBMIT_RETRY_PASS count=1')
if($RetrySubmitTest){$guardPass=$guardPass -and $retryPass}
[ordered]@{requested=[bool]$MavkLayer;loaded=$layerLog.Contains('MAVK_LAYER_INSTANCE mode=tracking');core_connected=$layerLog.Contains('MAVK_LAYER_DEVICE sharedCore=1 descriptorFrontend=1');common_update=$layerLog.Contains('MAVK_LAYER_UPDATE common=1');gaps=$layerLog.Contains('MAVK_LAYER_GAP');guarded_requested=[bool]$GuardedLayer;guarded_draw_readback_pass=$guardPass;private_image_init_pass=$imageInitPass;null_image_fault_injection=[bool]$NullImageTest;submit_retry_requested=[bool]$RetrySubmitTest;submit_retry_pass=$retryPass;submit_api=$submitApi;nullDescriptor_advertised=$false;d3d11_frame_pass=$false} | ConvertTo-Json | Out-File "$out/layer-summary.json" -Encoding utf8
[ordered]@{exit_code=$code;vulkan_instance_pass=$consumerLog.Contains('VULKAN_CREATE_INSTANCE=0');vulkan_device_pass=$consumerLog.Contains('VULKAN_PREFLIGHT=PASS');d3d11_device_pass=$consumerLog.Contains('SAREK_DEVICE_HRESULT=0x00000000');d3d11_clear_readback_pass=$consumerLog.Contains('SAREK_D3D11_CLEAR_READBACK_PASS components=16384');d3d11_draw_readback_pass=$consumerLog.Contains('SAREK_D3D11_DRAW_READBACK_PASS components=16384');d3d11_texture_readback_pass=($code -eq 0 -and $consumerLog.Contains('SAREK_D3D11_TEXTURE_READBACK_PASS components=16384 texels=16'));d3d11_point_sampling_pass=$consumerLog.Contains('SAREK_D3D11_FILTER_READBACK_PASS mode=0 components=16384');d3d11_bilinear_sampling_pass=($code -eq 0 -and $consumerLog.Contains('SAREK_D3D11_FILTER_READBACK_PASS mode=1 components=16384'));diagnostic_admission=[bool]$DiagnosticNullAdmission;vulkan_correctness_proven=$false;runtime='real pinned DXVK-Sarek native ARM64 + DroidDeck linuxfs-r9 subset + hybris';context='Termux debug UID,not complete DroidDeck/Wine session';production_changed=$false;slot=$Slot} | ConvertTo-Json | Out-File "$out/summary.json" -Encoding utf8
Get-Content "$out/summary.json"
if($DiagnosticNullAdmission){
    [ordered]@{diagnostic_only=$true;nullDescriptor_advertised=$false;optional_float_controls2_disabled=[bool]$LegacySpirvDiagnostic;vulkan_correctness_proven=$false;production_eligible=$false} | ConvertTo-Json | Out-File "$out/diagnostic-admission.json" -Encoding utf8
}
exit $code
