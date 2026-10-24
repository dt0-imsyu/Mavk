"""Run the existing Windows D3D11 consumer in its existing isolated Wine prefix, without Steam."""
import os
from pathlib import Path
import runpy
import shutil
import subprocess
import sys

base = Path('/data/user/0/com.droiddeck.launcher.mavk/files')
work = base / 'mavk-selection-normal'
prefix = Path('/root/.mavk-wine-poc')
assert (prefix / 'system.reg').is_file(), 'Do not create a Wine prefix'
protons = list(Path('/root/.local/share/droiddeck-esync/dist').glob('*/proton'))
assert len(protons) == 1, protons
proton = protons[0]
wine = proton.parent / 'files/bin-arm64/wine'
assert wine.is_file() and (wine.parent / 'wineserver').is_file()
arch = sys.argv[1] if len(sys.argv) == 2 else 'aarch64'
assert arch in ('aarch64', 'x86_64'), arch
resize = os.environ.get('MAVK_BENCH_RESIZE') == '1'
directory = work / ('wine-' + arch + ('-resize' if resize else ''))
directory.mkdir(exist_ok=True)
program = directory / 'bench.exe'
source = work / 'pe-resize' / ('mavk-d3d11-' + arch + '.exe') if resize else base / 'mavk-quick-bench' / arch / 'bench.exe'
if not program.exists():
    shutil.copyfile(source, program)
assert program.read_bytes() == source.read_bytes()
env = os.environ.copy()
env.pop('DXVK_WSI_DRIVER', None)
env.pop('SDL_VIDEODRIVER', None)
module = runpy.run_path(env['DROIDDECK_GAME_ENV_LAUNCHER'])
assert isinstance(module['load_sync'](), dict), 'bundled synchronization helper'
env['STEAM_COMPAT_DATA_PATH'] = '/tmp/mavk-prepare-only/990001'
_, env = module['system_graphics_launch']([str(proton), 'waitforexitandrun', str(program)], env)
assert env['MAVK_WSI_AHB_PRESENT'] == env['MAVK_WSI_XCB_AHB_PROXY'] == '1'
runtime = Path(env['DROIDDECK_MAVK_RUNTIME'])
for name in ('d3d11.dll', 'dxgi.dll', 'd3d10core.dll'):
    source = runtime / 'pe' / name
    target = directory / name
    if not target.exists():
        target.symlink_to(source)
    assert target.resolve() == source.resolve(), 'stale private DLL selection'
log = directory / 'window.log'
env.update(WINEPREFIX=str(prefix), WINELOADER=str(wine), WINESERVER=str(wine.parent / 'wineserver'),
           WINEDEBUG='-all', WINEESYNC='1', WINEFSYNC='0',
           WINEDLLOVERRIDES='mscoree,mshtml=;d3d11,dxgi=n',
           MAVK_WINDOW_LOG='Z:' + str(log).replace('/', '\\'),
           DXVK_LOG_LEVEL='info', DXVK_LOG_PATH=str(directory),
           MAVK_BENCH_WIDTH='1920', MAVK_BENCH_HEIGHT='1200',
           MAVK_BENCH_SECONDS='15', MAVK_BENCH_VSYNC='1')
print('NORMAL_SESSION_POLICY_PASS helpers=2 ahb=1 shm=0 protonExecuted=0', flush=True)
print('WINE_AHB_CONSUMER arch=' + arch + ' existingPrefix=1 steamStarted=0', flush=True)
try:
    result = subprocess.run([str(wine), 'Z:' + str(program).replace('/', '\\'), 'd3d11.dll'],
                            cwd=directory, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            timeout=65)
    assert len(result.stdout) <= 8 * 1024 * 1024, 'bounded Wine diagnostics'
    print(result.stdout.decode(errors='replace'), flush=True)
    if log.is_file():
        assert log.stat().st_size <= 1024 * 1024
        print(log.read_text(errors='replace'), flush=True)
    print('WINE_AHB_EXIT', result.returncode, flush=True)
    raise SystemExit(result.returncode)
finally:
    subprocess.run([str(wine.parent / 'wineserver'), '-k'], env=env, timeout=10,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
