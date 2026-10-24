#!/usr/bin/python3
"""Run the existing D3D11 consumer in its isolated, already-created Wine prefix."""
import json
import os
from pathlib import Path
import runpy
import subprocess
import sys

arch, width, height = sys.argv[1:]
assert arch in ("aarch64", "x86_64")
root = Path("/data/user/0/com.droiddeck.launcher.mavk/files/mavk-quick-bench")
prefix = Path("/root/.mavk-wine-poc")
assert (prefix / "system.reg").is_file(), "Do not create a new prefix for this benchmark"
protons = list(Path("/root/.local/share/droiddeck-esync/dist").glob("*/proton"))
assert len(protons) == 1, protons
proton = protons[0]
module = runpy.run_path("/usr/local/bin/droiddeck-game-env")
env = os.environ.copy()
config = json.loads(Path("/root/.config/droiddeck/game-environment.json").read_text())
env = module["apply_config"](env, config, "1625450", {})
env["STEAM_COMPAT_DATA_PATH"] = "/tmp/mavk-benchmark/990001"
program = root / arch / "bench.exe"
_, env = module["system_graphics_launch"]([str(proton), "waitforexitandrun", str(program)], env)
wine = proton.parent / "files/bin-arm64/wine"
env.update(WINEPREFIX=str(prefix), WINELOADER=str(wine),
           WINESERVER=str(wine.parent / "wineserver"), WINEDEBUG="-all",
           WINEESYNC="1", WINEFSYNC="0", WINEDLLOVERRIDES="mscoree,mshtml=;d3d11,dxgi=n",
           MAVK_BENCH_WIDTH=width, MAVK_BENCH_HEIGHT=height, MAVK_BENCH_SECONDS="15",
           MAVK_BENCH_VSYNC="0", MAVK_LAYER_PROFILE="0", MAVK_GUARD_REPORTS="0",
           MAVK_GPU_PROBE="0", DXVK_LOG_LEVEL="info", DXVK_LOG_PATH=str(root),
           MAVK_WINDOW_LOG="Z:" + str(root / (arch + "-" + width + ".log")).replace("/", "\\"))
runtime = Path(env["DROIDDECK_MAVK_RUNTIME"])
for name in ("d3d11.dll", "dxgi.dll", "d3d10core.dll"):
    source = runtime / "pe" / name
    target = program.parent / name
    if target.is_symlink() and target.readlink() != source:
        target.unlink()
    if not target.exists():
        target.symlink_to(source)
print("BENCH_LAUNCH", arch, width, height, "existing-isolated-prefix", flush=True)
with (root / (arch + "-" + width + ".stderr.log")).open("w") as errors:
    status = subprocess.run([str(wine), "Z:" + str(program).replace("/", "\\"), "d3d11.dll"],
                            env=env, cwd=program.parent, stderr=errors, timeout=50).returncode
print("BENCH_EXIT", status, flush=True)
log = root / (arch + "-" + width + ".log")
if log.exists():
    print(log.read_text(errors="replace"), flush=True)
raise SystemExit(status)
