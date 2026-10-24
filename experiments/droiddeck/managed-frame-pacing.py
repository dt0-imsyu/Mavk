"""Read-only IL inspection of frame-rate/VSync assignments in an owned test game.

Requires external dnfile0.18.0/dncil1.0.2. Never executes or patches the assembly.
Static assignments do not prove which branch runs in a live scene.
"""
import argparse
import hashlib
import json
from pathlib import Path

import dnfile
from dncil.cil.body.reader import read_method_body_from_bytes
from dncil.clr.token import Token

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('assembly', type=Path)
p.add_argument('--method', action='append', default=[])
a = p.parse_args()
raw = a.assembly.read_bytes()
pe = dnfile.dnPE(data=raw)
assert pe.net and pe.net.mdtables, 'Managed assembly required'
tables = pe.net.mdtables
methods, fields = {}, {}
for type_row in tables.TypeDef.rows:
    for index in type_row.MethodList:
        methods[index.row_index] = str(type_row.TypeName) + '::' + str(index.row.Name)
    for index in type_row.FieldList:
        fields[index.row_index] = str(type_row.TypeName) + '::' + str(index.row.Name)


def resolve(value):
    if not isinstance(value, Token):
        return str(value) if value is not None else ''
    if value.table == 6:
        return methods.get(value.rid, str(value))
    if value.table == 4:
        return fields.get(value.rid, str(value))
    if value.table == 10:
        row = tables.MemberRef.rows[value.rid-1]
        parent = row.Class.row
        owner = getattr(parent, 'TypeName', getattr(parent, 'Name', '?'))
        return str(owner) + '::' + str(row.Name)
    if value.table == 0x70:
        return repr(pe.net.user_strings.get(value.rid).value)
    return str(value)


report, errors = [], []
for number, method in enumerate(tables.MethodDef.rows, 1):
    if not method.Rva:
        continue
    name = methods[number]
    try:
        body = read_method_body_from_bytes(pe.get_data(method.Rva))
    except Exception as error:
        errors.append(dict(method=name, error=str(error)))
        continue
    lines, hits = [], []
    for index, ins in enumerate(body.instructions):
        operand = resolve(ins.operand)
        lines.append(f'{ins.offset:04x} {ins.opcode.name} {operand}'.rstrip())
        if any(needle in operand for needle in ('set_targetFrameRate', 'set_vSyncCount',
                                                'get_targetFrameRate', 'get_vSyncCount')):
            hits.append(index)
    requested = any(pattern.lower() in name.lower() for pattern in a.method)
    if hits or requested:
        selected = set(range(len(lines))) if requested else {
            j for i in hits for j in range(max(0, i-8), min(len(lines), i+5))}
        report.append(dict(method=name, il=[lines[i] for i in sorted(selected)]))
print(json.dumps(dict(sha256=hashlib.sha256(raw).hexdigest(), findings=report,
                      parse_errors=errors, executed=False), indent=2))
