param([ValidateSet('info','gears')][string]$Mode='info')
$ErrorActionPreference='Stop'
$env:ADB='C:/Users/denis/AppData/Local/Android/Sdk/platform-tools/adb.exe'
$env:DROIDDECK_PACKAGE='com.droiddeck.launcher.mavk'
$directory='/data/user/0/com.droiddeck.launcher.mavk/files/mavk-zink-ui-25.1.9'
$program=if($Mode -eq 'info'){'exec /usr/bin/glxinfo -B'}else{'exec /usr/bin/timeout 15 /usr/bin/glxgears -info'}
$script='set -e; test -f "$MAVK_ZINK/lib/libgallium-25.1.9.so"; unset LIBGL_ALWAYS_SOFTWARE DROIDDECK_MAVK_UI_SOFTWARE; export LD_LIBRARY_PATH="$MAVK_ZINK/lib:$LD_LIBRARY_PATH" GALLIUM_DRIVER=zink MESA_LOADER_DRIVER_OVERRIDE=zink ZINK_DESCRIPTORS=lazy LIBGL_KOPPER_DRI2=true; '+$program
python E:/Mavk/work/DroidDeck-mavk/tools/droiddeckctl guest --timeout 25 --env "MAVK_ZINK=$directory" -- /bin/bash -c $script
