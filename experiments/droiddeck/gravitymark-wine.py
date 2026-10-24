#!/usr/bin/python3
"""Launch unmodified GravityMark through installed Sarek, without Steam."""
import json
import os
from pathlib import Path
import runpy
import subprocess
import sys
import time

program = Path(sys.argv[1]).resolve(strict=True)
assert program.name.lower() == "gravitymark.exe"
assert str(program).startswith("/data/user/0/com.droiddeck.launcher.mavk/files/mavk-gravitymark/")
prefix = Path("/root/.mavk-wine-poc")
assert (prefix / "system.reg").is_file()
protons = list(Path("/root/.local/share/droiddeck-esync/dist").glob("*/proton"))
assert len(protons) == 1
proton = protons[0]
module = runpy.run_path("/usr/local/bin/droiddeck-game-env")
env = os.environ.copy()
xservers = subprocess.check_output(["pgrep", "-x", "Xwayland"], text=True).split()
assert len(xservers) == 1, xservers
for entry in Path("/proc/" + xservers[0] + "/environ").read_bytes().split(b"\0"):
    key, separator, value = entry.partition(b"=")
    if separator and (key.decode().startswith(("DROIDDECK_", "HYBRIS_", "FEX_", "VK_", "BL_"))
                      or key in (b"DISPLAY", b"XAUTHORITY", b"XDG_RUNTIME_DIR", b"WAYLAND_DISPLAY", b"LD_LIBRARY_PATH")):
        env[key.decode()] = value.decode()
env["DROIDDECK_MAVK_RUNTIME"] = "/data/user/0/com.droiddeck.launcher.mavk/files/mavk-runtime/4de2684454d0ba9f1f80ac37e9fc9d37d2dd04be444897622ae8518fc4bb13c0"
env["DROIDDECK_MAVK_GUEST_ROOT"] = "/data/user/0/com.droiddeck.launcher.mavk/files/linuxfs"
env["DROIDDECK_MAVK_LAYER_PATH"] = env["DROIDDECK_MAVK_RUNTIME"] + "/payload"
env["DROIDDECK_SYSTEM_VULKAN_DXVK"] = "managed"
env["DROIDDECK_SYSTEM_VULKAN_PRESENTATION"] = "x11-shm"
env["DROIDDECK_HYBRIS_DXVK"] = "dxvk-mali"
env = module["apply_config"](env, json.loads(
    Path("/root/.config/droiddeck/game-environment.json").read_text()), "1625450", {})
env["STEAM_COMPAT_DATA_PATH"] = "/tmp/mavk-benchmark/990001"
_, env = module["system_graphics_launch"]([str(proton), "waitforexitandrun", str(program)], env)
wine = proton.parent / "files/bin-arm64/wine"
env.update(WINEPREFIX=str(prefix), WINELOADER=str(wine),
           WINESERVER=str(wine.parent / "wineserver"), WINEDEBUG="-all",
           WINEESYNC="1", WINEFSYNC="0", WINEDLLOVERRIDES="mscoree,mshtml=;d3d11,dxgi=n",
           MAVK_LAYER_PROFILE="0", MAVK_GUARD_REPORTS="0", MAVK_GPU_PROBE="0",
           DXVK_LOG_LEVEL="info", DXVK_LOG_PATH=str(program.parent))
runtime = Path(env["DROIDDECK_MAVK_RUNTIME"])
for name in ("d3d11.dll", "dxgi.dll", "d3d10core.dll"):
    source = runtime / "pe" / name
    target = program.parent / name
    if not target.exists():
        target.symlink_to(source)
    assert target.resolve() == source.resolve()
control = env.get("MAVK_GRAVITY_CONTROL") == "native"
if control:
    assert "-vk" in sys.argv[2:] and "-d3d11" not in sys.argv[2:]
    env["VK_INSTANCE_LAYERS"] = ":".join(
        layer for layer in env.get("VK_INSTANCE_LAYERS", "").split(":")
        if layer != "VK_LAYER_MAVK_compat")
    print("GRAVITYMARK_NATIVE_CONTROL layers=" + env["VK_INSTANCE_LAYERS"], flush=True)
print("GRAVITYMARK_LAUNCH", program, sys.argv[2:], "Steam not required", flush=True)
log = program.parent / ("mavk-native-control.log" if control else "mavk-wine.log")
with log.open("w") as output:
    process = subprocess.Popen([str(wine), "Z:" + str(program).replace("/", "\\"), *sys.argv[2:]],
                               env=env, cwd=program.parent, stdout=output, stderr=subprocess.STDOUT)
    start = time.monotonic()
    while process.poll() is None:
        contents = log.read_text(errors="replace")
        reason = "GPU-fault" if "GROUP_ERROR_FATAL" in contents else None
        if time.monotonic() - start > (30 if control else 240):
            reason = "bounded-control" if control else "timeout"
        if reason:
            print("GRAVITYMARK_STOP reason=" + reason, flush=True)
            subprocess.run([env["WINESERVER"], "-k"], env=env, timeout=10, check=False)
            break
        time.sleep(0.5)
    status = process.wait(timeout=10)
print("GRAVITYMARK_EXIT", status, flush=True)
raise SystemExit(status)
