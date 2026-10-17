"""Check unmodified pinned application's PPM against its projected geometry."""
import argparse
import json
import math
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--host', type=Path, required=True)
p.add_argument('--guest', type=Path, required=True)
a = p.parse_args()
host = json.loads(a.host.read_text(encoding='utf-8'))
expected = dict(result='BRIDGE_DESCRIPTOR_SESSION_PASS', device='Mali-G925-Immortalis MC12',
                commands=8, gpu_submissions=1, clean_eof=True,
                core_live_zero_after_disconnect=True, validation_layers_enabled=True,
                synchronization_validation_requested=True, validation_errors=0,
                validation_warnings=0, channel='app-owned AVF connectVsock4057')
assert all(type(host.get(k)) is type(v) and host[k] == v for k, v in expected.items()), host
assert host['sent_bytes'] >= 4194304
stdout = (a.guest/'stdout.txt').read_text(encoding='utf-8-sig')
assert 'Running headless rendering example' in stdout
assert 'GPU: Mavk experimental / Mali-G925-Immortalis MC12' in stdout
assert 'Framebuffer image saved to headless.ppm' in stdout
assert 'Finished. Press enter to terminate...' in stdout
assert (a.guest/'exit.txt').read_text(encoding='utf-8-sig').strip() == 'EXIT=0'
assert 'MAVK_ICD_CREATE_INSTANCE' in (a.guest/'stderr.txt').read_text(encoding='utf-8-sig')
data = (a.guest/'headless.ppm').read_bytes()
header = b'P6\n1024\n1024\n255\n'
assert data.startswith(header) and len(data) == len(header)+1024*1024*3
pixels = memoryview(data)[len(header):]
# Upstream vertices RGB and three translations, GLSL default, perspective60deg.
# All vertices of each triangle have the same clip W, hence linear barycentrics.
f = 1/math.tan(math.pi/6)
triangles = []
for tx, z in [(-1.5, -4.0), (0.0, -2.5), (1.5, -4.0)]:
    triangles.append(([(f*(x+tx)/(-z), f*y/(-z)) for x, y in [(1, 1), (-1, 1), (0, -1)]], -z))
checked = 0
classes = [0, 0, 0, 0]
for y in range(8, 1024, 16):
    for x in range(8, 1024, 16):
        px, py = 2*(x+.5)/1024-1, 2*(y+.5)/1024-1
        winner, depth, weights, edge = 3, math.inf, None, False
        for index, (vertices, distance) in enumerate(triangles):
            (ax, ay), (bx, by), (cx, cy) = vertices
            denominator = (by-cy)*(ax-cx)+(cx-bx)*(ay-cy)
            r = ((by-cy)*(px-cx)+(cx-bx)*(py-cy))/denominator
            g = ((cy-ay)*(px-cx)+(ax-cx)*(py-cy))/denominator
            b = 1-r-g
            # Omit rasterization edges; do not accept arbitrary missing interiors.
            if min(abs(r), abs(g), abs(b)) < .015:
                edge = True
            if min(r, g, b) >= 0 and distance <= depth:
                winner, depth, weights = index, distance, (r, g, b)
        if edge:
            continue
        color = [0, 0, 51] if winner == 3 else [round(c*255) for c in weights]
        actual = list(pixels[(y*1024+x)*3:(y*1024+x)*3+3])
        assert all(abs(v-e) <= 2 for v, e in zip(actual, color)), (x, y, winner, actual, color)
        checked += 1
        classes[winner] += 1
assert checked >= 3000 and all(n >= 20 for n in classes), (checked, classes)
print(json.dumps(dict(result='PINNED_UPSTREAM_WINDOWS_VULKAN_MALI_HEADLESS_PASS',
    source_commit='4ee2501c8280cf71d1f9ce231cd8ebd47b58adde', resolution=[1024, 1024],
    sampled_pixels=checked, visible_samples_per_triangle_and_background=classes,
    scope='One unmodified external offscreen application. NOT conformance, WSI, game, DWM or performance PASS.'), indent=2))
