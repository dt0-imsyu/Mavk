#!/system/bin/sh
set -eu
base=/data/user/0/com.droiddeck.launcher.mavk/files
log=$base/mavk-selection-normal/result.log
ready=0
for attempt in $(seq 1 80); do
    if test -f "$log" && grep -q WINDOW_PRESENT_RETURNED "$log" && ! grep -q WINDOW_STREAM_END "$log"; then
        ready=1
        break
    fi
    sleep .25
done
test "$ready" = 1
/system/bin/timeout 3 sh -c 'printf "move 2000 1200\nclick 2000 1200\nkey 30\n" > "$1"' sh "$base/.wayland-rt/test-input"
sleep .3
exec sh /data/local/tmp/mavk-geometry-control.sh python3 "$base/mavk-selection-normal/pointer-query.py"
