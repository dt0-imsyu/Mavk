#!/bin/bash
set -eu
export MAVK_BENCH_RESIZE=1
exec /bin/bash /data/user/0/com.droiddeck.launcher.mavk/files/mavk-selection-normal/run-base.sh "$@"
