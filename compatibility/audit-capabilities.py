"""Offline capability comparison and source-pinned DXVK admission, not a driver.

Missing query evidence is UNKNOWN, never false. No capability is advertised or
enabled by this tool. Driver admission is necessary, not a D3D11 device PASS.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path

DXVK_PIN = "25ca63f17f34bdc05a39873ee63907d3cbbfa030"

def read(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))

def flatten(value, prefix=""):
    result = {}
    if isinstance(value, dict):
        for key, child in value.items():
            result.update(flatten(child, prefix + key + "."))
    else:
        result[prefix.rstrip(".")] = value
    return result

def normalize(raw, android=False):
    if android:
        raw = raw["devices"][0]
    feature_groups = {"core10": raw["features"]}
    property_groups = {"core10": raw["properties"]}
    for ver in ("11", "12", "13", "14"):
        if "core" + ver in raw:
            feature_groups["core" + ver] = raw["core" + ver].get("features", {})
            property_groups["core" + ver] = raw["core" + ver].get("properties", {})
    ext_features, ext_properties = {}, {}
    if android:
        for ext, entries in raw.items():
            if not ext.startswith("VK_"):
                continue
            for struct, fields in entries.items():
                if "Features" in struct:
                    ext_features[ext] = dict(ext_features.get(ext, {}), **fields)
                else:
                    ext_properties.setdefault(ext, {})[struct] = fields
    else:
        for row in raw.get("extended", {}).get("devicefeatures2", []):
            ext_features.setdefault(row["extension"], {})[row["name"]] = row["supported"]
        for row in raw.get("extended", {}).get("deviceproperties2", []):
            ext_properties.setdefault(row["extension"], {})[row["name"]] = row["value"]
    formats = {}
    for number, values in raw["formats"]:
        # Drop convenience 'supported' flags: compare actual usage bitmasks.
        formats[str(int(number))] = {k: int(v, 0) if isinstance(v, str) and v.startswith("0x")
                                    else int(v) for k, v in values.items() if k != "supported"}
    return {"identity": {k: raw["properties"][k] for k in
                        ("deviceName", "apiVersion", "driverVersion", "vendorID", "deviceID")},
            "core_features": feature_groups, "core_properties": property_groups,
            "extension_features": ext_features, "extension_properties": ext_properties,
            "extensions": {e["extensionName"]: int(e["specVersion"]) for e in raw["extensions"]},
            "formats_by_vk_format_number": formats, "queues": raw["queues"], "memory": raw["memory"]}

def extension_names(header):
    pairs = re.findall(r"(\w+)\s*= vk::makeExtension\((VK_\w+)_EXTENSION_NAME\)", header)
    return {alias: macro for alias, macro in pairs}

def resolve_extension(alias, mapping, all_extensions):
    macro = mapping[alias]
    key = macro.replace("_", "").lower()
    candidates = [e for e in all_extensions if e.replace("_", "").lower() == key]
    # Even if both devices lack it, extension identity is still deterministic.
    if not candidates:
        vendor, name = macro[3:].split("_", 1)
        return "VK_" + vendor + "_" + name.lower()
    assert len(candidates) == 1, candidates
    return candidates[0]

def requirements(source, header, profiles):
    # Restrict extraction to the actual table, not macro declarations/comments.
    table = source.split("std::vector<DxvkDeviceCapabilities::FeatureEntry> DxvkDeviceCapabilities::getFeatureList()", 1)[1]
    table = table.split("return {{", 1)[1].split("#undef ENABLE_FEATURE", 1)[0]
    mapping = extension_names(header)
    known_extensions = set().union(*(p["extensions"] for p in profiles))
    rows = []
    for kind, args in re.findall(r"(ENABLE_FEATURE|ENABLE_EXT_FEATURE|ENABLE_EXT)\(([^)]*)\)", table):
        parts = [v.strip() for v in args.split(",")]
        if parts[-1] != "true":
            continue
        owner = parts[0]
        name = parts[1] if kind != "ENABLE_EXT" else None
        extension = None if kind == "ENABLE_FEATURE" else resolve_extension(owner, mapping, known_extensions)
        values = []
        for profile in profiles:
            if extension is not None:
                if extension not in profile["extensions"]:
                    value = False
                elif name is None:
                    value = True
                else:
                    value = profile["extension_features"].get(extension, {}).get(name)
            else:
                group = "core10" if owner == "core.features" else owner.replace("vk", "core")
                value = profile["core_features"].get(group, {}).get(name)
            values.append(None if value is None else bool(value))
        rows.append({"kind": kind, "feature": name, "extension": extension,
                     "owner": owner, "turnip": values[0], "mali": values[1],
                     "mali_class": "NATIVE" if values[1] is True else
                     "EMULATION NEEDED" if values[1] is False else "UNKNOWN",
                     "mavk_guest": "PASSTHROUGH MISSING" if values[1] is True else
                     "EMULATION NEEDED" if values[1] is False else "UNKNOWN"})
    assert len(rows) >= 40, "Unexpected source table; review extraction, do not guess"
    return rows

def differences(turnip, mali):
    a, b = flatten(turnip), flatten(mali)
    return {k: {"turnip": a.get(k), "mali": b.get(k)} for k in sorted(a.keys() | b.keys())
            if a.get(k) != b.get(k)}

def audit(folder, output, consumer=None, revision=DXVK_PIN):
    output.mkdir(parents=True, exist_ok=False)
    turnip = normalize(read(folder / "turnip750/turnip-report.json"))
    mali = normalize(read(folder / "mali-gpu-service.json"), True)
    consumer = folder if consumer is None else consumer
    source = (consumer / "dxvk_device_info.cpp").read_text()
    header = (consumer / "dxvk_device_info.h").read_text()
    instance = (consumer / "dxvk_instance.h").read_text()
    api = re.search(r"DxvkVulkanApiVersion = VK_API_VERSION_(\d)_(\d)", instance)
    assert api
    rows = requirements(source, header, [turnip, mali])
    required_api = (int(api[1]) << 22) | (int(api[2]) << 12)
    gates = {"source_pin": revision, "required_api": f"{api[1]}.{api[2]}",
             "mali_api_gate": mali["identity"]["apiVersion"] >= required_api,
             "required_feature_rows": rows,
             "mali_missing": [r for r in rows if r["mali"] is False],
             "mali_unknown": [r for r in rows if r["mali"] is None],
             "turnip_missing": [r for r in rows if r["turnip"] is False],
             "turnip_unknown": [r for r in rows if r["turnip"] is None],
             "push_constants_required": 256,
             "mali_push_constants": mali["core_properties"]["core10"]["limits"]["maxPushConstantsSize"],
             "d3d11_runtime_executed": False,
             "notes": ["NATIVE means driver-advertised, not a new correctness PASS.",
                       "PASSTHROUGH MISSING is the bounded Mavk guest API, not libhybris direct native.",
                       "EMULATION NEEDED is unimplemented work, not advertised support.",
                       "Missing observations stay UNKNOWN; no made-up zero values."]}
    assert mali["identity"]["vendorID"] == 0x13b5
    assert "Mali-G925" in mali["identity"]["deviceName"]
    assert "Turnip" in turnip["identity"]["deviceName"]
    results = {"turnip-normalized.json": turnip, "mali-normalized.json": mali,
               "all-capability-differences.json": differences(turnip, mali), "dxvk-admission.json": gates}
    for name, value in results.items():
        (output / name).write_text(json.dumps(value, indent=2), encoding="utf-8")
    files = [folder / "mali-gpu-service.json", folder / "turnip750/turnip-report.json",
             folder / "turnip750/turnip-profile.json", consumer / "dxvk_device_info.cpp",
             consumer / "dxvk_device_info.h", consumer / "dxvk_instance.h"]
    (output / "inputs.json").write_text(json.dumps([
        {"path": str(f), "sha256": hashlib.sha256(f.read_bytes()).hexdigest()} for f in files], indent=2))
    summary = {"required_rows": len(rows), "mali_missing": [r["feature"] or r["extension"] for r in gates["mali_missing"]],
               "mali_unknown": len(gates["mali_unknown"]), "turnip_missing": len(gates["turnip_missing"]),
               "turnip_unknown": len(gates["turnip_unknown"]), "api_gate": gates["mali_api_gate"],
               "extensions": [len(turnip["extensions"]), len(mali["extensions"])],
               "format_rows": [len(turnip["formats_by_vk_format_number"]), len(mali["formats_by_vk_format_number"])]}
    print(json.dumps(summary, indent=2))

if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--artifacts", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--consumer-source", type=Path)
    p.add_argument("--consumer-revision")
    args = p.parse_args()
    if bool(args.consumer_source) != bool(args.consumer_revision):
        p.error("Consumer source and pinned revision must be supplied together")
    if args.consumer_revision and not re.fullmatch(r"[0-9a-f]{40}", args.consumer_revision):
        p.error("Full pinned consumer commit required")
    audit(args.artifacts, args.output, args.consumer_source, args.consumer_revision or DXVK_PIN)
