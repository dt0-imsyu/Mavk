"""Add one explicit no-argument Image Operands mask to generated texel fixtures."""
import argparse
import struct
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("source", type=Path)
parser.add_argument("destination", type=Path)
parser.add_argument("--mask", required=True, choices=("none", "zeroextend"))
args = parser.parse_args()
assert not args.destination.exists(), "Preserve prior artifacts"
raw = args.source.read_bytes()
assert len(raw) % 4 == 0
words = list(struct.unpack(f"<{len(raw)//4}I", raw))
assert words[0] == 0x07230203 and words[1] >= 0x00010400
out = words[:5]
p, changed = 5, 0
while p < len(words):
    count, opcode = words[p] >> 16, words[p] & 65535
    assert count and p + count <= len(words)
    inst = words[p:p+count]
    if opcode == 95:  # OpImageFetch; result type/id,image,coordinate only.
        mask = 0 if args.mask == "none" else 8192
        assert count == 5 or (count == 6 and inst[5] == mask)
        if count == 5:
            inst[0] += 1 << 16
            inst.append(mask)
        changed += 1
    out.extend(inst)
    p += count
assert changed == 1
args.destination.write_bytes(struct.pack(f"<{len(out)}I", *out))
print(f"IMAGE_OPERAND_FIXTURE mask={args.mask} operations={changed}")
