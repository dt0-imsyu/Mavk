"""One cold diagnostic cluster, exact before/after journal and readback."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path

p=argparse.ArgumentParser()
p.add_argument('--adb',required=True)
p.add_argument('--uavf',type=Path,required=True)
p.add_argument('--plan',type=Path,required=True)
p.add_argument('--output',type=Path,required=True)
p.add_argument('--rollback',action='store_true')
a=p.parse_args()
assert not a.output.exists()
a.output.mkdir(parents=True)
plan=json.loads((a.plan/'plan.json').read_text())
assert plan['result']=='COLD_EXISTING_SLOT_PREFLIGHT_PASS' and not plan['applied']
assert plan['package']=='com.example.uavfwinpetest' and plan['disk']=='files/windows-diagnostic.img'
assert plan['path']=='/IDDWORK/idd-motion-source.exe' and plan['bytes']==8192 and plan['file_bytes']==7680
before=(a.plan/'before.bin').read_bytes();candidate=(a.plan/'candidate.bin').read_bytes()
sha=lambda b:hashlib.sha256(b).hexdigest()
assert len(before)==len(candidate)==8192
assert sha(before)==plan['before_sha256']=='3381bd9574ddecd6a6674d99b5b5be81e506393746c66460082e2bde35f4320d'
assert sha(candidate)==plan['candidate_sha256']
assert (plan['input_client_sha256'],sha(candidate)) in {
    ('3670bbf1af842707460103b0bb6473a1a8cc63d05b9770d1dbc43f0ff64ccae9',
     '5733130201fc1af0c8c5c73fb96792eb4e4794b53b54f5da8c39b0e1ac37c942'),
    ('99ef3264b1cc5d7917517a946fb0333ce1d084e4704a293eb4ff2b83f718ad9a',
     '5b7ec1bb0ff6c6c4c6db8f017adf3d1bbe2f5481a8bd62aec7503cd6ce70f580'),
    ('406ed9735b7eea2df8472bcd51b2495ecaad08a6b6303759f38960d3b0ec990b',
     '4c655ecf72d4968d94d09bc81cbfad1ab1bc348fd06ac0a108d63c0bf9b0be2b'),
    ('c9dec0baf7d07c79c849f323db69f97c23a72e9c3fca9236377705c16e663568',
     '5b255db45ba7ca370d98c980684f3b8d0176c9beedd5ca0b540e7aaa8e299ff5'),
    ('7d3a6e29488f7c14bb503e4bb832ff41362322d46df4124cddf68549117fbddf',
     '7d1323107f71c90bd1731a359c581dfbf2fc97b8970e2b66d65ad8ddfd576470'),
    ('8574b919eae74f51c0ff8eea047be67698709e4bf7b4b34997fd9773387b809e',
     '32bae66ffeb77547fb1b350b03859e0f12f67a6f67a2c31d5ac8fc5f07d17529'),
}
spec=importlib.util.spec_from_file_location('mavk_carrier_guard',a.uavf/'tools/windows-reassessment/isolated-device-bcd.py')
g=importlib.util.module_from_spec(spec);spec.loader.exec_module(g)
fat=g.DeviceFat(a.adb,a.output,expected_efi_sha=g.RAM_GOP_HANDOFF_AUDIT_EFI_SHA)
current,chain=fat.file(plan['path'])
assert len(chain)==1 and chain[0]==plan['cluster'] and fat.offset(chain[0])==plan['offset'] and len(current)==7680
old,new=(candidate,before) if a.rollback else (before,candidate)
observed=fat.read(plan['offset'],8192)
assert observed==old,'Unknown/changed carrier preimage; no write'
report={'result':'NOT_WRITTEN','rollback':a.rollback,'offset':plan['offset'],'bytes':8192,'before_sha256':sha(old),'after_sha256':sha(new)}
try:
    fat.write_bcd(new,chain)
    assert fat.read(plan['offset'],8192)==new
    assert fat.file(plan['path'])[0]==new[:7680]
    report['result']='CARRIER_EXACT_READBACK_PASS'
except Exception:
    fat.cold()
    fat.write_bcd(old,chain)
    assert fat.read(plan['offset'],8192)==old
    report['result']='FAILED_AND_RESTORED'
    raise
finally:
    (a.output/'report.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report))
