param(
    [Parameter(Mandatory=$true)][string]$Program,
    [string[]]$ProbeArguments = @(),
    [string[]]$ProbeEnvironment = @(),
    [string]$LayerName = 'mavk-guard-cse',
    [switch]$GuardCse,
    [switch]$UseInstalledLayer,
    [switch]$Mesa,
    [switch]$ShellSuite,
    [switch]$PrivateAhbWsi,
    [switch]$BundledAhbWsi,
    [switch]$AhbPresent,
    [switch]$NativeSarekWindow,
    [switch]$XcbAhbProxy,
    [switch]$BgraBlit,
    [ValidateSet('1280x800','1920x1200','2560x1600')][string]$XcbTestExtent = '2560x1600',
    [ValidateRange(5,30)][int]$WindowSeconds = 8,
    [ValidateSet('mavk-wsi-ahb','mavk-wsi-xcb','mavk-wsi-resize','mavk-wsi-pointer')][string]$PrivateWsiDirectory = 'mavk-wsi-ahb',
    [string]$AhbWsiHash,
    [string]$Adb = 'C:/Users/denis/AppData/Local/Android/Sdk/platform-tools/adb.exe',
    [string]$Serial = 'R52Y9072A2R',
    [string]$RuntimeHash = '7549290e1a5b9685b6bbc29c1c58088a6cb24aa65d12c85e8903cc83196e56fa',
    [string]$SystemHash = '8a17ef888360ab1a61b58ee0a5984f5ab1ad9dbabbd8cbcd12b4a52de52be042'
)
$ErrorActionPreference = 'Stop'
$base = '/data/user/0/com.droiddeck.launcher.mavk/files'
if ($LayerName -notmatch '^mavk-[a-z0-9-]+$' -or $RuntimeHash -notmatch '^[0-9a-f]{64}$' -or $SystemHash -notmatch '^[0-9a-f]{64}$') { throw 'Invalid private test/runtime selection' }
if (!$Program.StartsWith("$base/", [StringComparison]::Ordinal) -or $Program.Contains('..') -or $Program -notmatch '^/[a-zA-Z0-9_./-]+$') { throw 'App-private probe required' }
$runtime = "$base/system_vulkan/$SystemHash"
$layerRuntime = if ($UseInstalledLayer) { "$base/mavk-runtime/$RuntimeHash" } else { "$base/$LayerName" }
$layer = "$layerRuntime/payload"
$libraries = "$base/linuxfs/usr/lib:$runtime/lib"
$environment = @(
    "VK_DRIVER_FILES=$runtime/icd.json", "VK_ICD_FILENAMES=$runtime/icd.json",
    "VK_LAYER_PATH=$layer", 'VK_INSTANCE_LAYERS=VK_LAYER_MAVK_compat:VK_LAYER_HYBRIS_compat',
    'HYBRIS_ANDROID_SDK_VERSION=36', 'HYBRIS_VULKAN_HAL=/vendor/lib64/hw/vulkan.mali.so',
    "HYBRIS_LINKER_DIR=$runtime/lib/libhybris/linker",
    'HYBRIS_LD_LIBRARY_PATH=/system/lib64:/system_ext/lib64:/vendor/lib64/egl:/vendor/lib64/hw:/vendor/lib64',
    'MAVK_LAYER_GUARDS=1', 'MAVK_GUARD_REPORTS=0', 'MAVK_LAYER_TRACE=0',
    'MAVK_LAYER_PROFILE=0', 'MAVK_GPU_PROBE=0', 'MAVK_METADATA_READ_ONLY=0',
    "MAVK_GUARD_CSE=$([int]$GuardCse.IsPresent)", "MAVK_EXPECT_LAYER_PATH=$layer/libVkLayer_mavk_compat.so"
)
foreach ($entry in $ProbeEnvironment) {
    if ($entry -notmatch '^MAVK_AHB_[A-Z0-9_]+=[A-Za-z0-9_./-]+$') { throw 'Only explicit AHB probe parameters are accepted' }
}
$environment += $ProbeEnvironment
if ($PrivateAhbWsi) {
    if (!$UseInstalledLayer -or $Mesa -or $ShellSuite -or $AhbWsiHash -notmatch '^[0-9a-f]{64}$') {
        throw 'Private WSI requires installed layer, explicit hash and direct consumer'
    }
    $wsi = if ($BundledAhbWsi) { "$layerRuntime/presentation/ahb" } else { "$base/$PrivateWsiDirectory" }
    $observed = & $Adb -s $Serial shell run-as com.droiddeck.launcher.mavk sha256sum "$wsi/libVkLayer_window_system_integration.so"
    if ($LASTEXITCODE -or !$observed.StartsWith($AhbWsiHash, [StringComparison]::Ordinal)) {
        throw 'Private WSI hash mismatch'
    }
    $environment += @("VK_LAYER_PATH=${wsi}:$layer",
        'VK_INSTANCE_LAYERS=VK_LAYER_window_system_integration:VK_LAYER_MAVK_compat:VK_LAYER_HYBRIS_compat',
        'MAVK_WSI_AHB_ALLOC_ONLY=1', "MAVK_EXPECT_WSI_PATH=$wsi/libVkLayer_window_system_integration.so")
    if ($AhbPresent) {
        $environment += @('MAVK_WSI_AHB_PRESENT=1', "XDG_RUNTIME_DIR=$base/mavk-window/run", 'WAYLAND_DISPLAY=wayland-0')
    }
}
elseif ($AhbPresent -or $BundledAhbWsi) { throw 'AHB presentation requires the private WSI candidate' }
if ($BgraBlit) {
    if (!$PrivateAhbWsi -or !$AhbPresent) { throw 'BGRA GPU conversion requires the private AHB WSI' }
    $environment+='MAVK_WSI_AHB_BGRA_BLIT=1'
}
if ($NativeSarekWindow) {
    $allowedWindow = $Program -eq "$base/mavk-wsi-ahb/sarek-window" -or
        ($XcbAhbProxy -and $PrivateWsiDirectory -in @('mavk-wsi-xcb','mavk-wsi-pointer') -and $Program -eq "$base/$PrivateWsiDirectory/sarek-window")
    if (!$PrivateAhbWsi -or !$AhbPresent -or !$allowedWindow) {
        throw 'Native Sarek window is bounded to the private AHB presentation consumer'
    }
    $bundle="$base/mavk-window/payload"
    $hashes=@{
        'libdxvk_d3d11.so'='42d3e4827459d959bbb0bfa94ba203a2b6fd660e47d8607aaae588b0f157404a'
        'libdxvk_dxgi.so.0'='b2155710f0850680a7bef39b3c65deb6145af03343b881694a07ffb7da916da9'
        'libSDL3.so.0'='4cf3e1fa515d99c7bb2636723519747c3d8022e58c98c0d6a0f455d46dc4441c'
    }
    foreach($name in $hashes.Keys) {
        $observed=& $Adb -s $Serial shell run-as com.droiddeck.launcher.mavk sha256sum "$bundle/$name"
        if($LASTEXITCODE -or !$observed.StartsWith($hashes[$name],[StringComparison]::Ordinal)) {
            throw "Native Sarek dependency hash mismatch: $name"
        }
    }
    $libraries="${bundle}:$libraries"
    $environment+=@('SDL_VIDEODRIVER=wayland','DXVK_WSI_DRIVER=SDL3','DXVK_LOG_LEVEL=info',
        "DXVK_LOG_PATH=$base/mavk-wsi-ahb",'MAVK_SAREK_NULL_PROBE=1','MAVK_SAREK_LEGACY_SPIRV=1',
        'HYBRIS_BC_TEXTURES=dxvk','MAVK_SAREK_BC_FORMATS=1',
        'MAVK_BENCH_WIDTH=1920','MAVK_BENCH_HEIGHT=1200',"MAVK_BENCH_SECONDS=$WindowSeconds",'MAVK_BENCH_VSYNC=1')
}
if ($XcbAhbProxy) {
    if (!$NativeSarekWindow -or !$AhbPresent -or $PrivateWsiDirectory -notin @('mavk-wsi-xcb','mavk-wsi-pointer')) {
        throw 'XCB proxy is a private native Sarek/session rendering-only experiment'
    }
    $environment+=@('MAVK_WSI_XCB_AHB_PROXY=1','SDL_VIDEODRIVER=x11','DISPLAY=:99',
        "XAUTHORITY=$base/mavk-session/xauthority", "XDG_RUNTIME_DIR=$base/.wayland-rt",
        "MAVK_BENCH_WIDTH=$($XcbTestExtent.Split('x')[0])","MAVK_BENCH_HEIGHT=$($XcbTestExtent.Split('x')[1])")
}
if ($Mesa) {
    $libraries = "$base/mavk-runtime/$RuntimeHash/mesa25/lib:$libraries"
    $environment += @('EGL_PLATFORM=surfaceless', 'GALLIUM_DRIVER=zink', 'MESA_LOADER_DRIVER_OVERRIDE=zink', 'ZINK_DESCRIPTORS=lazy', 'HYBRIS_BC_TEXTURES=dxvk')
}
if ($ShellSuite) {
    $environment += @("MAVK_ELF_LOADER=$base/linuxfs/usr/lib/ld-linux-aarch64.so.1", "MAVK_ELF_LIBRARIES=$libraries",
        "DROIDDECK_MAVK_RUNTIME=$layerRuntime", "MAVK_BUFFER_TEST_ROOT=$base/mavk-aggregate",
        'MAVK_BUFFER_COMMON=1', 'MAVK_TEXEL_VIEW_COMPONENTS=1', 'MAVK_TEXEL_COMPONENTS=1',
        'MAVK_STORAGE_ATOMIC=1', 'MAVK_STORAGE_TEXEL=1')
    & $Adb -s $Serial shell run-as com.droiddeck.launcher.mavk env @environment /system/bin/sh $Program @ProbeArguments
} else {
    & $Adb -s $Serial shell run-as com.droiddeck.launcher.mavk env @environment "$base/linuxfs/usr/lib/ld-linux-aarch64.so.1" --library-path $libraries $Program @ProbeArguments
}
if ($LASTEXITCODE -ne 0) { throw "Headless probe failed: $LASTEXITCODE" }
