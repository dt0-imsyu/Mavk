"""Experimental ARM64X D3D11 payload selection for the existing Proton wrapper."""
from pathlib import Path
import os
import sys


NAMES = ("d3d10core", "d3d11", "dxgi")


def prepare(proton, directory, check_machine, check_arm64x):
    directory = Path(directory)
    if not directory.is_absolute():
        raise ValueError("Mavk payload directory must be absolute")
    for name in NAMES:
        dll = directory / (name + ".dll")
        check_machine(dll, 0xaa64)
        check_arm64x(dll)
    # Optional PE32 payload is independent of the ARM64X D3D11 implementation.
    # Never route a 32-bit process to ARM64 DLLs, or claim the stock DXVK supports Mali.
    i386 = os.environ.get("MAVK_SAREK_I386_PAYLOAD")
    if not i386 and (directory / "i386").is_dir():
        i386 = str(directory / "i386")
    if i386:
        i386 = Path(i386)
        if not i386.is_absolute():
            raise ValueError("Mavk i386 payload directory must be absolute")
        for name in ("d3d9", "dxgi"):
            check_machine(i386 / (name + ".dll"), 0x14c)
    source = Path(proton).read_text(encoding="utf-8")
    for is32 in ("False", "True"):
        original = f'g_proton.arch_pe_dir("wine/dxvk", {is32}) + f + ".dll"'
        if source.count(original) != 1:
            raise ValueError("unsupported Proton DXVK copy layout")
        source = source.replace(original, f'_mavk_payload_path(g_proton, f, {is32})', 1)
    marker = "            # check whether any prefix config has changed"
    if source.count(marker) != 1:
        raise ValueError("unsupported Proton prefix configuration layout")
    source = source.replace(marker, '            prefix_info += "\\nmavk-sarek-arm64x-d3d11-v1"\n' + marker, 1)

    reported = set()

    def resolve(proton_instance, name, is32):
        original = Path(proton_instance.arch_pe_dir("wine/dxvk", is32))
        if is32 and i386 and name in ("d3d9", "dxgi"):
            return str(i386 / (name + ".dll"))
        if name not in NAMES:
            return str(original / (name + ".dll"))
        if is32:
            if original.name not in reported:
                print("Mavk: preserving stock 32-bit DXVK for Proton prefix setup; "
                      "32-bit Mavk D3D11 is not implemented", file=sys.stderr)
                reported.add(original.name)
            return str(original / (name + ".dll"))
        if original.name != "aarch64-windows":
            raise ValueError("Mavk Sarek currently supports ARM64/ARM64EC D3D11 only; "
                             f"selected Proton architecture is {original.name}")
        return str(directory / (name + ".dll"))

    return compile(source, str(proton), "exec"), resolve
