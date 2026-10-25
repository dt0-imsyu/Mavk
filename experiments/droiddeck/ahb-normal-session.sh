#!/bin/bash
set -eu
base=/data/user/0/com.droiddeck.launcher.mavk/files
work=$base/mavk-selection-normal
exec >"$work/result.log" 2>&1
trap 'status=$?; printf "%s\n" "$status" >"$work/done"' EXIT
test "${DROIDDECK_GAME_ENV_LAUNCHER:-}" = "${DROIDDECK_MAVK_RUNTIME:?}/integration/droiddeck-game-env"
case "${DROIDDECK_MAVK_PRESENTATION:-}" in
 ahb-bgra)
  test "${DROIDDECK_MAVK_XCB_AHB_WSI:-}" = "$DROIDDECK_MAVK_RUNTIME/presentation/ahb"
  test -f "${XDG_RUNTIME_DIR:?}/mavk-explicit-ahb.enable"
  echo "NORMAL_SESSION_AHB_READY resolution=${BL_WIDTH:-unknown}x${BL_HEIGHT:-unknown} runtime=$DROIDDECK_MAVK_RUNTIME" ;;
 x11-shm)
  test -z "${DROIDDECK_MAVK_XCB_AHB_WSI:-}"
  test ! -e "${XDG_RUNTIME_DIR:?}/mavk-explicit-ahb.enable"
  echo "NORMAL_SESSION_SHM_READY resolution=${BL_WIDTH:-unknown}x${BL_HEIGHT:-unknown} runtime=$DROIDDECK_MAVK_RUNTIME" ;;
 *) exit 2 ;;
esac
bundle=$base/mavk-window/payload
export SDL_VIDEODRIVER=x11 DXVK_WSI_DRIVER=SDL3 DXVK_LOG_LEVEL=info
export DXVK_LOG_PATH=$work MAVK_SAREK_NULL_PROBE=1 MAVK_SAREK_LEGACY_SPIRV=1
export HYBRIS_BC_TEXTURES=dxvk MAVK_SAREK_BC_FORMATS=1
export MAVK_BENCH_WIDTH=1920 MAVK_BENCH_HEIGHT=1200 MAVK_BENCH_SECONDS=20 MAVK_BENCH_VSYNC=1
export MAVK_LAYER_GUARDS=1 MAVK_GUARD_REPORTS=0
python3 "$work/consumer.py" "$@"
echo NORMAL_SESSION_D3D11_DONE
