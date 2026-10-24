"""Inspect actual Gallium trace draws; does not validate rendering correctness."""
import collections
import json
import os
from pathlib import Path
import struct
import sys
import xml.etree.ElementTree as ET


def value(node):
    if node.tag in ("arg", "member", "elem", "ret"):
        return value(node[0]) if len(node) else None
    if node.tag == "struct":
        return {child.get("name"): value(child) for child in node}
    if node.tag == "array":
        return [value(child) for child in node]
    if node.tag in ("uint", "int"):
        return int(node.text or "0")
    if node.tag == "float":
        return float(node.text or "0")
    if node.tag == "bool":
        return node.text == "1"
    if node.tag == "bytes":
        return bytes.fromhex(node.text or "")
    return node.text


def main():
    counts = collections.Counter()
    elements, buffers = {}, {}
    bound_elements, bound_buffers = [], []
    draws, mappings = [], []
    recent = collections.deque(maxlen=4)
    mapped = {}
    parse_warning = None
    try:
        for _, node in ET.iterparse(sys.argv[1], events=("end",)):
            if node.tag != "call":
                continue
            method = node.get("method")
            counts[method] += 1
            if method not in {"create_vertex_elements_state", "bind_vertex_elements_state",
                              "set_vertex_buffers", "buffer_subdata", "draw_vbo", "buffer_map"}:
                node.clear()
                continue
            args = {child.get("name"): value(child) for child in node.findall("arg")}
            if method == "buffer_map":
                record = {"args": args, "ret": value(node.find("ret"))}
                mapped[args["resource"]] = record
                if len(mappings) < 8:
                    mappings.append(record)
            elif method == "create_vertex_elements_state":
                elements[value(node.find("ret"))] = args["elements"]
            elif method == "bind_vertex_elements_state":
                bound_elements = elements.get(args["state"], [])
            elif method == "set_vertex_buffers":
                bound_buffers = args.get("buffers") or []
            elif method == "buffer_subdata":
                resource, offset, data = args["resource"], args["offset"], args["data"]
                if isinstance(data, bytes) and offset + len(data) <= 16 * 1024 * 1024:
                    target = buffers.setdefault(resource, bytearray())
                    if len(target) < offset + len(data):
                        target.extend(bytes(offset + len(data) - len(target)))
                    target[offset:offset + len(data)] = data
            elif method == "draw_vbo":
                recent.append({"call": node.get("no"), "info": args["info"],
                               "draws": args["draws"], "elements": list(bound_elements),
                               "bindings": list(bound_buffers)})
                if len(draws) >= 12:
                    node.clear()
                    continue
                attributes = []
                for attribute in bound_elements:
                    index = attribute["vertex_buffer_index"]
                    binding = bound_buffers[index] if index < len(bound_buffers) else {}
                    resource = binding.get("buffer.resource")
                    offset = binding.get("buffer_offset", 0) + attribute.get("src_offset", 0)
                    data = buffers.get(resource, b"")
                    chunk = data[offset:offset + 64]
                    attributes.append({"layout": attribute, "binding": binding,
                                       "captured_bytes": len(data), "head_hex": chunk.hex(),
                                       "head_float32": list(struct.unpack("<" + "f" * (len(chunk) // 4),
                                                                             chunk[:len(chunk) // 4 * 4]))})
                draws.append({"call": node.get("no"), "info": args["info"],
                              "draws": args["draws"], "attributes": attributes})
            node.clear()
    except ET.ParseError as error:
        parse_warning = str(error)
    memory_observation = None
    if len(sys.argv) == 3:
        pid = int(sys.argv[2])
        proc = Path("/proc") / str(pid)
        if proc.stat().st_uid != os.getuid() or proc.joinpath("comm").read_text().strip() != "DDNet.exe":
            raise SystemExit("Refusing a process other than own-UID DDNet.exe")
        ranges = []
        for line in proc.joinpath("maps").read_text().splitlines():
            region, permissions, *_ = line.split()
            if permissions[0] == "r":
                low, high = region.split("-")
                ranges.append((int(low, 16), int(high, 16)))
        memory_observation = []
        with proc.joinpath("mem").open("rb", buffering=0) as memory:
            for draw in recent:
                observation = {"call": draw["call"], "attributes": []}
                for attribute in draw["elements"]:
                    index = attribute["vertex_buffer_index"]
                    if index >= len(draw["bindings"]):
                        continue
                    binding = draw["bindings"][index]
                    mapping = mapped.get(binding.get("buffer.resource"))
                    if not mapping or not mapping["ret"]:
                        continue
                    offset = binding["buffer_offset"] + attribute["src_offset"]
                    box = mapping["args"]["box"]
                    address = int(mapping["ret"], 16) + offset - box["x"]
                    stride = attribute.get("src_stride", 0)
                    size = min(128, max(16, stride * 4))
                    if not (box["x"] <= offset and offset + size <= box["x"] + box["width"]):
                        continue
                    if not any(low <= address and address + size <= high for low, high in ranges):
                        continue
                    data = os.pread(memory.fileno(), size, address)
                    observation["attributes"].append({"layout": attribute, "binding": binding,
                        "address": hex(address), "hex": data.hex(),
                        "float32": list(struct.unpack("<" + "f" * (len(data) // 4), data[:len(data) // 4 * 4]))})
                memory_observation.append(observation)
    print(json.dumps({"trace": str(Path(sys.argv[1])), "parse_warning": parse_warning,
                      "calls": counts, "buffer_upload_resources": len(buffers),
                      "vertex_layouts": len(elements), "mappings": mappings,
                      "first_draws": draws, "recent_mapped_memory": memory_observation}, indent=2))


if __name__ == "__main__":
    main()
