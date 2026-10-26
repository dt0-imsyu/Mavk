# Bounded shader A/B: closed without a material FPS improvement

The user requested finishing the existing shader experiment and stopping FPS
research if it did not provide a substantial gain. The same unchanged heavy
Muck scene was measured reference / native-eligible diagnostic / reference:

| Mode | Android displayed game buffers/s |
| --- | ---: |
| Guarded reference A | 12.5 |
| Native-eligible diagnostic B | 12.7 |
| Guarded reference A2 | 12.3 |

Evidence: `artifacts-local/native-shader-ab4-20261025/`, including the three
pacing JSONs/screenshots and final-ab-log.txt. The variant executed about11559
graphics and26 compute commands/s; these counters are NOT FPS. Sampled output
was visually normal and the capture had no new Mali/pipeline/gap errors. This
does not establish universal visual correctness or measured unique gameplay FPS.

The variation is too small to justify promoting this weaker diagnostic path.
Native alternatives rely only on application-enabled native robustness1 for
eligible fully-bound resources; they do not establish robustness2 semantics.
The experiment stays private and default OFF. Muck was quit and its temporary
performance profile restored. No new shader/FPS experiments should be started
from this result; retain the previously committed CPU/metadata improvements.

CPU-only causal tracing found the dominant sampled waits after earlier GPU
submissions returned, not delayed CPU submission entry. This includes GPU,
driver and scheduling costs; it is not a hardware utilization measurement or
proof that a specific shader causes the remaining low performance.

Next practical direction is a different real-game demonstration and common
runtime/API coverage. HL2 startup exposed native FEX and PE32 D3D9 routing gaps;
see PE32_D3D9_2026-10-25.md. No claim of60FPS or a resolved diagonal-artifact
root cause is made.
