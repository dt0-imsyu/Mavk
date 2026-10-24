import ast
import os
from pathlib import Path
import socket
import struct
import sys
import tempfile


source = Path(sys.argv[1])
tree = ast.parse(source.read_text())
function = next(node for node in tree.body if isinstance(node, ast.FunctionDef) and node.name == "x_authority")
scope = dict(os=os, Path=Path, socket=socket, struct=struct)
exec(compile(ast.Module(body=[function], type_ignores=[]), str(source), "exec"), scope)
with tempfile.NamedTemporaryFile() as authority:
    os.environ["XAUTHORITY"] = authority.name
    fields = [b"", b"99", b"MIT-MAGIC-COOKIE-1", bytes(range(16))]
    data = struct.pack(">H", 65535) + b"".join(struct.pack(">H", len(field)) + field for field in fields)
    authority.write(data)
    authority.flush()
    assert scope["x_authority"]("99") == (fields[2], fields[3])
    for invalid in (data[:-1], b"\xff", data.replace(b"99", b"98", 1)):
        authority.seek(0)
        authority.truncate()
        authority.write(invalid)
        authority.flush()
        try:
            scope["x_authority"]("99")
        except OSError:
            pass
        else:
            raise AssertionError("Invalid authority accepted")
print("Xauthority cookie roundtrip / truncation / wrong display: PASS")
