"""Read only Muck's saved pacing settings; never launch Steam or change a prefix."""
import argparse
import hashlib
import json
import re
import subprocess
import xml.etree.ElementTree as ET

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--adb', required=True)
p.add_argument('--serial', required=True)
a = p.parse_args()
registry = ('files/linuxfs/root/.local/share/Steam/steamapps/compatdata/1625450/'
            'pfx/user.reg')
raw = subprocess.check_output([a.adb, '-s', a.serial, 'exec-out', 'run-as',
                               'com.droiddeck.launcher.mavk', 'cat', registry])
assert len(raw) <= 8*1024*1024, 'Registry read budget'
text = raw.decode('utf-8')
match = re.search(r'(?m)^"save_h2088338340"=hex:((?:[0-9a-fA-F]{2},?|\\\r?\n[ \t]*)+)', text)
assert match, 'Known Muck PlayerPrefs save value missing'
encoded = bytes.fromhex(''.join(re.findall('[0-9a-fA-F]{2}', match[1])))
xml = encoded.rstrip(b'\x00').decode('utf-8')
assert '<!DOCTYPE' not in xml, 'Unexpected XML declaration'
root = ET.fromstring(xml)
assert root.tag == 'PlayerSave'
keys = ('fpsLimit', 'vSync', 'fullscreen', 'shadowQuality', 'shadowResolution',
        'shadowDistance', 'antiAliasing', 'textureQuality', 'bloom')
result = {key: root.findtext(key) for key in keys}
print(json.dumps(dict(settings=result, saved_value_sha256=hashlib.sha256(encoded).hexdigest(),
                      read_only=True, steam_started=False, runtime_branch_verified=False), indent=2))
