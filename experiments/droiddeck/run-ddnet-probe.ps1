param([Parameter(Mandatory=$true)][string]$Name,[string]$Trace='',
 [string[]]$ExtraEnvironment=@(),
 [string[]]$GameOptions=@(),
 [ValidateSet('amd64','arm64')][string]$Architecture='amd64',
 [ValidateSet('Vulkan','OpenGL')][string]$Backend='Vulkan',
 [switch]$PrivateZink,
 [switch]$VertexSnapshot,
 [switch]$Validation)
$ErrorActionPreference='Stop'
if($Name -notmatch '^[a-z0-9-]{1,48}$'){throw 'Invalid test label'}
if($Trace -and $Trace -notmatch '^[a-z0-9-]+\.so$'){throw 'Invalid diagnostic helper'}
$env:ADB='C:/Users/denis/AppData/Local/Android/Sdk/platform-tools/adb.exe'
$env:DROIDDECK_PACKAGE='com.droiddeck.launcher.mavk'
$own='/data/user/0/com.droiddeck.launcher.mavk/files'
$game="$own/mavk-games/DDNet-20.1.1-win64"
$executable=if($Architecture -eq 'arm64'){"$own/mavk-games/DDNet-arm64/DDNet.exe"}else{"$game/DDNet.exe"}
$argsList=@('E:/Mavk/work/DroidDeck-mavk/tools/droiddeckctl','guest','--timeout','60','--cwd',$game)
$environment=@(
 'STEAM_COMPAT_CLIENT_INSTALL_PATH=/root/.local/share/Steam',
 'STEAM_COMPAT_DATA_PATH=/root/.local/share/droiddeck/mavk-proton/4294967000',
 'WINEESYNC=0','WINEFSYNC=0','WINEDLLOVERRIDES=vulkan-1=b',
 'DROIDDECK_MAVK_NATIVE_VULKAN=1',"MAVK_PROBE_LOG=$own/mavk-window/ddnet-$Name.log"
)
if($Trace){$environment+="LD_PRELOAD=$own/mavk-window/$Trace"}
$environment += $ExtraEnvironment
if($VertexSnapshot){
 $PrivateZink=$true
 $environment += @('MAVK_TRACE_VERTEX_SNAPSHOT=true','GALLIUM_TRACE=/dev/null',
  "GALLIUM_TRACE_TRIGGER=$own/mavk-window/vertex-never.trigger",'GALLIUM_TRACE_NIR=0')
}
if($Validation){
 $environment += @('VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation:VK_LAYER_HYBRIS_compat',
  'DROIDDECK_HYBRIS_LAYERS=VK_LAYER_KHRONOS_validation:VK_LAYER_HYBRIS_compat',
  'DROIDDECK_HYBRIS_SESSION_LAYERS=VK_LAYER_HYBRIS_compat','VK_LAYER_DUPLICATE_MESSAGE_LIMIT=3',
  'VK_LOADER_DEBUG=error,warn,layer')
}
foreach($line in $environment){$argsList+=@('--env',$line)}
$launch='exec /usr/local/bin/droiddeck-game-env "$@" > "$MAVK_PROBE_LOG" 2>&1'
if($PrivateZink){
 $zink=if($VertexSnapshot){"$own/mavk-zink-vertex-observer"}else{"$own/mavk-zink-ui-25.1.9"}
 $launch='unset DROIDDECK_MAVK_UI_MESA DROIDDECK_MAVK_UI_PREVIOUS_DESCRIPTORS LIBGL_ALWAYS_SOFTWARE; export LD_LIBRARY_PATH='+$zink+'/lib:$LD_LIBRARY_PATH GALLIUM_DRIVER=zink MESA_LOADER_DRIVER_OVERRIDE=zink ZINK_DESCRIPTORS=lazy LIBGL_KOPPER_DRI2=true; '+$launch
}
if($Validation){
 $launch='export LD_LIBRARY_PATH='+$own+'/mavk-validation:$LD_LIBRARY_PATH VK_LAYER_PATH='+$own+'/mavk-validation:$VK_LAYER_PATH; '+$launch
}
$argsList+=@('--','/bin/bash','-c', $launch,
 'mavk-ddnet','/root/.local/share/Steam/steamapps/common/Proton Experimental (ARM64)/proton',
 'waitforexitandrun',$executable,"gfx_backend $Backend",'gfx_fullscreen 0',
 'gfx_screen_width 1280','gfx_screen_height 720',"logfile mavk-ddnet-$Name.log")
$argsList += $GameOptions
python @argsList
& $env:ADB shell "run-as com.droiddeck.launcher.mavk tail -n 38 files/mavk-window/ddnet-$Name.log"
