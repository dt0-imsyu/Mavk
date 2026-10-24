"""Recompile one isolated compositor source; read cached objects, write only a fresh output folder."""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import subprocess

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('source', type=Path)
p.add_argument('output', type=Path)
a = p.parse_args()
build = Path('E:/Mavk/work/DroidDeck-mavk/app/.cxx/Debug/5a1m492x/arm64-v8a')
baseline = Path('E:/Mavk/work/DroidDeck-mavk/app/build/intermediates/cxx/Debug/5a1m492x/obj/arm64-v8a/libdroiddeckwayland.so')
assert hashlib.sha256(baseline.read_bytes()).hexdigest() == 'bfc199e4bae1f5c151114c499aabf9addb4abdd839ace0c73ca46af9371cfb1d'
assert a.source.is_file() and not a.output.exists()
a.output.mkdir(parents=True)
out = a.output.resolve()
env = dict(os.environ, TMP=str(out), TEMP=str(out))
split = ctypes.windll.shell32.CommandLineToArgvW
split.argtypes = [ctypes.c_wchar_p, ctypes.POINTER(ctypes.c_int)]
split.restype = ctypes.POINTER(ctypes.c_wchar_p)
ctypes.windll.kernel32.LocalFree.argtypes = [ctypes.c_void_p]

def argv(command):
    count = ctypes.c_int()
    data = split(command, ctypes.byref(count))
    if not data:
        raise OSError('CommandLineToArgvW failed')
    try:
        return [data[i] for i in range(count.value)]
    finally:
        ctypes.windll.kernel32.LocalFree(data)

def norm(value):
    return value.replace('\\', '/').lower()

entries = json.loads((build / 'compile_commands.json').read_text())
entry, = [v for v in entries if norm(v['file']).endswith('/waylandcomp/src/compositor.c')]
compile_args = argv(entry['command'])
old_object = compile_args[compile_args.index('-o') + 1]
compile_args[compile_args.index('-o') + 1] = str(out / 'compositor.c.o')
sources = [i for i, value in enumerate(compile_args) if norm(value) == norm(entry['file'])]
assert len(sources) == 1
compile_args[sources[0]] = str(a.source.resolve())
subprocess.run(compile_args, cwd=build, env=env, check=True)
ninja = 'C:/Users/denis/AppData/Local/Android/Sdk/cmake/3.22.1/bin/ninja.exe'
commands = subprocess.check_output([ninja, '-C', str(build), '-t', 'commands', str(baseline)], text=True)
command = commands.splitlines()[-1]
prefix, suffix = 'cmd.exe /C "cd . && ', ' && cd ."'
assert command.startswith(prefix) and command.endswith(suffix)
link_args = argv(command[len(prefix):-len(suffix)])
assert norm(link_args[link_args.index('-o') + 1]) == norm(str(baseline))
link_args[link_args.index('-o') + 1] = str(out / 'libdroiddeckwayland.so')
objects = [i for i, value in enumerate(link_args) if norm(value) == norm(old_object)]
assert len(objects) == 1
link_args[objects[0]] = str(out / 'compositor.c.o')
subprocess.run(link_args, cwd=build, env=env, check=True)
library = out / 'libdroiddeckwayland.so'
print('NATIVE_RELINK_PASS sha256=' + hashlib.sha256(library.read_bytes()).hexdigest())
