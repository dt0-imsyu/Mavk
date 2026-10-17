"""Download public capability evidence, never install a GPU driver.

Report uploads are reference-device observations, not universal Turnip promises.
Preserves raw data and hashes; requires an empty/new evidence directory.
"""
import argparse
import hashlib
import json
import urllib.parse
import urllib.request
from pathlib import Path

def fetch(url):
    request = urllib.request.Request(url, headers={"User-Agent": "Mavk-capability-audit/1"})
    with urllib.request.urlopen(request, timeout=30) as response:
        return response.read()

def reports():
    q = {"draw": "1", "start": "0", "length": "100",
         "order[0][column]": "0", "order[0][dir]": "desc",
         "search[value]": "", "search[regex]": "false"}
    names = ["id", "device", "driver", "api", "vendor", "devicetype",
             "osname", "osversion", "osarchitecture", "compare"]
    for i, name in enumerate(names):
        for key, value in {"data": name, "name": "", "searchable": "true",
                           "orderable": "true", "search][value": "Turnip" if i == 1 else "",
                           "search][regex": "false"}.items():
            q[f"columns[{i}][{key}]"] = value
    for k in ("submitter devicelimit devicelimitvalue devicename displayname extension "
              "extensionname extensionfeature extensionproperty extensionpropertyvalue "
              "instanceextension instancelayer platform corefeature coreproperty "
              "corepropertyvalue core profile lineartilingformat optimaltilingformat "
              "bufferformat featureflagbit option apiversion age").split():
        q[f"filter[{k}]"] = ""
    return "https://vulkan.gpuinfo.org/api/internal/reports.php?" + urllib.parse.urlencode(q)

if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--report-id", type=int)
    a = p.parse_args()
    a.output.mkdir(parents=True, exist_ok=False)
    sources = [("reports.json", reports())] if a.report_id is None else [
        ("turnip-report.json", f"https://vulkan.gpuinfo.org/services/getreportjson.php?id={a.report_id}"),
        ("turnip-profile.json", f"https://vulkan.gpuinfo.org/api/v3/getprofile.php?id={a.report_id}")]
    manifest = []
    for name, url in sources:
        data = fetch(url)
        json.loads(data)
        (a.output / name).write_bytes(data)
        manifest.append({"file": name, "url": url, "sha256": hashlib.sha256(data).hexdigest()})
    (a.output / "provenance.json").write_text(json.dumps(manifest, indent=2))
    if a.report_id is None:
        report_list = json.loads(data)
        print(json.dumps(report_list, indent=2))
    else:
        print(json.dumps(manifest, indent=2))
