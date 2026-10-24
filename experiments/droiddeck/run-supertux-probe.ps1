param([ValidatePattern('^[a-z0-9-]{1,40}$')][string]$Name='d3d11')
$ErrorActionPreference='Stop'
$env:ADB='C:/Users/denis/AppData/Local/Android/Sdk/platform-tools/adb.exe'
$env:DROIDDECK_PACKAGE='com.droiddeck.launcher.mavk'
$own='/data/user/0/com.droiddeck.launcher.mavk/files'
$game="$own/mavk-games/SuperTux-portable/bin"
$probeArgs=@('E:/Mavk/work/DroidDeck-mavk/tools/droiddeckctl','guest','--timeout','60','--cwd',$game)
$probeEnv=@(
 'STEAM_COMPAT_CLIENT_INSTALL_PATH=/root/.local/share/Steam',
 'STEAM_COMPAT_DATA_PATH=/root/.local/share/droiddeck/mavk-proton/4294967000',
 'WINEESYNC=0','WINEFSYNC=0','WINEDLLOVERRIDES=vulkan-1=b',
 'SDL_RENDER_DRIVER=direct3d11','DXVK_LOG_LEVEL=info',
 "DXVK_LOG_PATH=$own/mavk-window", "MAVK_PROBE_LOG=$own/mavk-window/supertux-$Name.log"
)
foreach($line in $probeEnv){$probeArgs+=@('--env',$line)}
$probeArgs+=@('--','/bin/bash','-c',
 'exec /usr/local/bin/droiddeck-game-env "$@" > "$MAVK_PROBE_LOG" 2>&1',
 'mavk-supertux','/root/.local/share/Steam/steamapps/common/Proton Experimental (ARM64)/proton',
 'waitforexitandrun',"$game/supertux2.exe",'--renderer','sdl')
python @probeArgs
& $env:ADB shell "run-as com.droiddeck.launcher.mavk tail -n 32 files/mavk-window/supertux-$Name.log"
