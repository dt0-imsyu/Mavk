"""Check routing against the installed Proton source and actual ARM64X payload."""
import importlib.util
from pathlib import Path
import runpy
import sys

root = Path(sys.argv[1])
proton = Path(sys.argv[2])
wrapper = runpy.run_path(str(root / "run-proton.py"))
spec = importlib.util.spec_from_file_location("mavk_proton", root / "proton_d3d11.py")
adapter = importlib.util.module_from_spec(spec)
spec.loader.exec_module(adapter)
_, resolve = adapter.prepare(proton, root / "pe", wrapper["check_machine"], wrapper["check_arm64x"])


class ProtonLayout:
    def __init__(self, host):
        self.host = host

    def arch_pe_dir(self, directory, is32):
        return "/native/" + directory + "/" + ("i386-windows" if is32 else self.host) + "/"


native = ProtonLayout("aarch64-windows")
for name in adapter.NAMES:
    assert resolve(native, name, False) == str(root / "pe" / (name + ".dll"))
    assert resolve(native, name, True) == "/native/wine/dxvk/i386-windows/" + name + ".dll"
assert resolve(native, "d3d9", False) == "/native/wine/dxvk/aarch64-windows/d3d9.dll"
try:
    resolve(ProtonLayout("x86_64-windows"), "d3d11", False)
except ValueError:
    pass
else:
    raise AssertionError("unsupported 64-bit runtime accepted")
print("MAVK_PROTON_ROUTING_PASS real_proton=1 real_arm64x=3 prefix32_preserved=1 wrong64_rejected=1 stock_d3d9_preserved=1")
