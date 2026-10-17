"""Native/Core + local Bridge gates only; never a Windows texture PASS."""
import argparse
import json
from pathlib import Path
p=argparse.ArgumentParser()
p.add_argument('--native',type=Path,required=True)
p.add_argument('--bridge',type=Path,required=True)
a=p.parse_args()
n=json.loads(a.native.read_text(encoding='utf-8'))
b=json.loads(a.bridge.read_text(encoding='utf-8'))
def exact(data,expected):
    assert all(type(data.get(k)) is type(v) and data[k]==v for k,v in expected.items()),data
validation=dict(device='Mali-G925-Immortalis MC12',validation_layers_enabled=True,
                synchronization_validation_requested=True,validation_callback_tests=1,
                validation_errors=0,validation_warnings=0)
exact(n,dict(validation,result='NATIVE_CORE_SAMPLED_MIPS_PASS',pixels_verified=12288,
             gpu_submissions=12,mip_levels=4,core_live_zero=True))
assert len(n['samples'])==12
assert {(s['seed'],s['mode']) for s in n['samples']}=={(seed,mode) for seed in (0,73) for mode in range(6)}
exact(b,dict(result='BRIDGE_LOCAL_GPU_PERSISTENT_TEXTURE_PASS',pixels_verified=2048,
             commands=34,negative_tests=16,uploaded_bytes=5592404,mip_levels=11))
exact(b['native'],dict(validation,result='BRIDGE_DESCRIPTOR_SESSION_PASS',gpu_submissions=3,
                     commands=34,clean_eof=True,core_live_zero_after_disconnect=True,
                     received_bytes=5599396,sent_bytes=9344))
print(json.dumps(dict(result='NATIVE_CORE_AND_LOCAL_BRIDGE_SAMPLED_PASS',
    scope='Native mip/filter pixel verification and persistent chunked texture bridge. NOT guest ICD, external textured app, zero-copy, performance or conformance proof.'),indent=2))
