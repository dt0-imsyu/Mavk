"""Derive bounded signed/unsigned index fixtures, never execute unguarded."""
import argparse
from pathlib import Path
import struct

p = argparse.ArgumentParser(description=__doc__)
p.add_argument("fixture", type=Path)
p.add_argument("output", type=Path)
a = p.parse_args()
data = a.fixture.read_bytes()
assert 20 <= len(data) <= 4096 and len(data) % 4 == 0
original = list(struct.unpack(f"<{len(data)//4}I", data))
assert original[0] == 0x07230203
a.output.mkdir(exist_ok=False)
for name, value, signed in (("high", 1 << 32, False), ("valid", 0, False),
                            ("negative", (1 << 64) - 1, True)):
    words = original.copy()
    cursor, wide, changed = 5, None, 0
    while cursor < len(words):
        count, op = words[cursor] >> 16, words[cursor] & 65535
        assert count and cursor + count <= len(words)
        if op == 21 and count == 4 and words[cursor+2] == 64:
            wide = words[cursor+1]
            words[cursor+3] = int(signed)
        if op == 43 and count == 5 and words[cursor+1] == wide and words[cursor+3:cursor+5] == [0, 1]:
            words[cursor+3:cursor+5] = [value & 0xFFFFFFFF, value >> 32]
            changed += 1
        cursor += count
    assert changed == 1
    (a.output / f"{name}.spv").write_bytes(struct.pack(f"<{len(words)}I", *words))
