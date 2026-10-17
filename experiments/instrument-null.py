"""Bounded feasibility only: upstream descriptor-init instrumentation.

Not a production nullDescriptor implementation. SPIRV-Tools' older pass omits
atomics/other pointer forms; it also writes validation records on null access.
Do not use this helper to advertise a Vulkan feature.
"""
import argparse
import pathlib
import struct
import subprocess


def instructions(words):
    if len(words) < 5 or words[0] != 0x07230203:
        raise ValueError("SPIR-V header required")
    offset = 5
    while offset < len(words):
        count, opcode = words[offset] >> 16, words[offset] & 65535
        if not count or offset + count > len(words):
            raise ValueError("Malformed SPIR-V instruction")
        yield offset, count, opcode
        offset += count


def read(path):
    raw = path.read_bytes()
    if len(raw) % 4 or len(raw) > 1024 * 1024:
        raise ValueError("Bounded word-aligned shader required")
    return list(struct.unpack(f"<{len(raw)//4}I", raw))


def check_source(original):
    # Reserve set1 for metadata; only set0 input is admitted by this PoC.
    for offset, count, opcode in instructions(original):
        # Includes extension floating-point atomics, not only core integer ops.
        if 227 <= opcode <= 242 or opcode in (318, 319, 5614, 5615, 6035):
            raise ValueError("UNSUPPORTED: atomic resource access requires a real guard implementation")
        if opcode == 71 and count == 4 and original[offset+2] == 34:
            if original[offset+3] != 0:
                raise ValueError("PoC requires original descriptor set0 only")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tools", required=True, type=pathlib.Path)
    parser.add_argument("--mode", choices=("buffers", "images"), default="buffers")
    parser.add_argument("input", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    args = parser.parse_args()
    if args.output.exists():
        raise ValueError("Preserve existing shader; use fresh output")
    original = read(args.input)
    check_source(original)
    temporary = args.output.with_suffix(".instrumented.spv")
    if temporary.exists():
        raise ValueError("Preserve previous intermediate")
    subprocess.run([str(args.tools / "spirv-opt.exe"), "--target-env=vulkan1.1",
                    "--inst-buff-oob-check" if args.mode == "buffers" else "--inst-desc-idx-check",
                    str(args.input), "-o", str(temporary)], check=True)
    words = read(temporary)
    relocated = []
    bindings = {}
    for offset, count, opcode in instructions(words):
        if opcode == 71 and count == 4:
            target, decoration, value = words[offset+1:offset+4]
            if decoration == 33:
                bindings[target] = value
            if decoration == 34 and value == 7:
                words[offset+3] = 1
                relocated.append(target)
    if len(relocated) != 2 or sorted(bindings.get(i) for i in relocated) != [0, 1]:
        raise ValueError("Expected exactly generated debug/input bindings0/1")
    args.output.write_bytes(struct.pack(f"<{len(words)}I", *words))
    subprocess.run([str(args.tools / "spirv-val.exe"), "--target-env", "vulkan1.1",
                    str(args.output)], check=True)
    print(f"Validated {args.mode} feasibility shader: {len(words)*4} bytes, reserved set1")


if __name__ == "__main__":
    main()
