# Enabled standard UBO layout — 2026-10-23

## Real consumer and fix

Official glmark2 GLES jellyfish through Mesa 25.1.9/Zink rejected its first
graphics pipeline in staticDescriptorUses: a Uniform Block array had ArrayStride4,
which the default SPIRV-Tools validator treated as invalid std140. The native
device was actually created with uniformBufferStandardLayout enabled. Merely
selecting the Vulkan1.2 validation environment does not enable that option.

The layer now captures the ENABLED feature from Vulkan12 or its separate feature
structure. That immutable device policy reaches static-use validation and the
guard compiler's input, optimizer and output validators. Defaults remain strict.
No shader data layout is rewritten, feature advertised, guard disabled or driver
feature silently enabled. ScalarBlockLayout is not included in this change.
The separate experimental cull-link compiler remains strict; this is not a claim
of packed-UBO admission through every cull lowering path.

## Hardware evidence

Tab S11 SM-X736B, Mali-G925; private candidate SHA256
83e4a6d9e1ebb284b10bd3c1c67d95d0be816626564234185884767e6387b910.

- Actual standard Vulkan device feature enabled in glmark2, printed by layer.
- Reused nested-buffer GPU oracle with an actual std430 Uniform block:
  previous f195 layer rejects; new layer passes valid read2779096485 and OOB
  read-zero, with out-of-range output write suppressed,31 output sentinels and
  all32 input words intact. Descriptor range4, allocation128 bytes.
- All12 existing guarded GPU regression modes pass; CoreLiveZero.
- ARM64 compiler/static-use checks19/rejections9, including strict feature-off
  rejection, feature-on guarded output validation and cache reuse. These are
  compiler checks, not GPU or CTS evidence.
- Real jellyfish/terrain/shadow scenes complete through guarded Zink without
  gaps, with all common resources retired. A fresh fullscreen1920x1080 desktop
  reaches roughly60 benchmark frames/s; independent713 Surface buffer updates
  over12.1s=58.8/s. Buffer updates do not prove unique image identity.

## Isolation / limits

Only files/mavk-uniform-layout contains the staged layer. APK25/common runtime,
U-AVF/Windows/Ubuntu unchanged. The first resumed desktop was visually frozen;
its109–115 benchmark FPS are NOT a visible-output result. After a normal desktop
session restart the test was actually visible. Initial1280x720 desktop/windowed
runs are excluded from native1080p comparison. Fullscreen output was then set to
1920x1080 for the valid run; the previously absent displayResolution.lxqt setting
was removed afterwards. No user game or save data changed.

This is a compatibility fix, not a demonstrated game FPS improvement or full
Vulkan/robustness conformance. CTS was not run. Matching native benchmark results
have presentation/driver/compiler differences; do not convert60vs364/223/1468
directly into a percentage of native GPU power.

## Artifacts

E:/Mavk/artifacts/uniform-layout-{frontend,consumer,layer}-20261023 and
glmark2-20261023 contain binaries,shader fixtures,old/new logs and gpu-evidence.tar.
Source: shader_guards.hpp,static_use.cpp,guards.cpp,layer.cpp; existing nested
consumer extended with an opt-in UBO mode, default regression behavior retained.
No commit/push or installed product binary replacement.
