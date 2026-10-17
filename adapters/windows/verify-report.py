"""Validate the bounded Windows compute slice, not an ICD/display claim."""
import argparse
import json
from pathlib import Path


def verify(report, guest_text, guest_exit, graphics=False):
    expected = {
        'result': 'BRIDGE_DESCRIPTOR_SESSION_PASS',
        'device': 'Mali-G925-Immortalis MC12',
        'commands': 23,
        'received_bytes': 1000,
        'sent_bytes': 525056,
        'gpu_submissions': 8,
        'clean_eof': True,
        'core_live_zero_after_disconnect': True,
        'validation_layers_enabled': True,
        'synchronization_validation_requested': True,
        'validation_errors': 0,
        'validation_warnings': 0,
    }
    if graphics:
        expected.update(commands=3, received_bytes=112, sent_bytes=16496, gpu_submissions=1)
    failures = [key for key, value in expected.items()
                if type(report.get(key)) is not type(value) or report[key] != value]
    lines = set(guest_text.splitlines())
    markers = ('GPU_VERIFIED_PIXELS=3330', 'COMPLETED_COMMANDS=3',
               'MAVK_WINDOWS_VSOCK_GRAPHICS_PASS', 'PROBE_EXIT=0') if graphics else (
               'GPU_VERIFIED_WORDS=131072', 'COMPLETED_COMMANDS=23',
               'MAVK_WINDOWS_VSOCK_COMPUTE_PASS', 'PROBE_EXIT=0')
    for marker in markers:
        if marker not in lines:
            failures.append(marker)
    if guest_exit.strip() != 'EXIT=0':
        failures.append('process EXIT=0')
    if failures:
        raise ValueError('Missing/mismatched evidence: ' + ', '.join(failures))
    if graphics:
        return {'result': 'WINDOWS_AVF_MAVK_GRAPHICS_PASS', 'gpu_submissions': 1,
                'guest_verified_pixels': 3330,
                'scope': 'Trusted offscreen graphics slice; NOT Vulkan ICD, WDDM, DWM or ordinary app PASS'}
    return {'result': 'WINDOWS_AVF_MAVK_COMPUTE_PASS', 'gpu_submissions': 8,
            'guest_verified_words': 131072,
            'scope': 'Trusted compute slice; NOT Vulkan ICD, WDDM, DWM or graphics PASS'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--host-report', type=Path, required=True)
    parser.add_argument('--guest-stdout', type=Path, required=True)
    parser.add_argument('--guest-exit', type=Path, required=True)
    parser.add_argument('--graphics', action='store_true')
    args = parser.parse_args()
    guest_bytes = args.guest_stdout.read_bytes()
    guest_text = guest_bytes.decode('utf-16' if guest_bytes.startswith((b'\xff\xfe', b'\xfe\xff')) else 'utf-8-sig')
    exit_bytes = args.guest_exit.read_bytes()
    guest_exit = exit_bytes.decode('utf-16' if exit_bytes.startswith((b'\xff\xfe', b'\xfe\xff')) else 'utf-8-sig')
    print(json.dumps(verify(json.loads(args.host_report.read_text(encoding='utf-8')),
                            guest_text, guest_exit, args.graphics),
                     indent=2))
