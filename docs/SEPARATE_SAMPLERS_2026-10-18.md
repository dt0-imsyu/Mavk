# Separate image/sampler consumer gate — 2026-10-18

Status: feasibility PASS on Mali-G925; not complete nullDescriptor support.
Ordinary Vulkan write structs feed the shared registry/update/binder/native
descriptor adapter. An original shader is compiled by the shared runtime guard
compiler. Public sampled images/output are set0,samplers set1,guard metadata set2.

Two phases exchange live/null images and nearest/linear samplers. A varied8x8
RGBA8 source and fractional coordinates distinguish actual filtering;CPU oracle
checks all RGBA channels,including zero alpha for logical null. Sampler descriptors
need a real owning sampler and reject logical null writes. Image-only descriptor
fields are not incorrectly required for sampler writes.

## Evidence

`E:/Mavk/artifacts/separate-samplers-native2-20261018`:

- APK SHA256 `b0cd6f8970890f0ae9c73cbea3122692cff95996bf63159bb800863361420289`.
- `core-separate-samplers.json` SHA256
  `8fea318408d27751d52d5d84e25a6c039b73f3659bf3db5fcb3d5a6fe410cd1b`:
  2048 components,256 guards,two filters,two phases;strict verifier PASS,
  synchronization validation0 errors/0 warnings,Core live resources zero.
- Same APK `core-null-texels.json` SHA256
  `abb49fa96726534cd8ea201e5eb9bc3e66e02a27a539a4a739cfaf1f5e729f7c`:
  texel regression2304 values/1152 guards PASS.
- Same APK `core-null-images.json` SHA256
  `58bc8fbecdeec87cd1e8947c4eeb15cfccdca510a745098e607872abbca24e09`:
  combined/storage image regression1792 values/384 guards PASS.
- Sampler CPU gate9 checks/7 rejection checks PASS.

First native run completed phase0 pixel comparisons but diagnostic parser assumed
10 words per guard record. Actual combined compiler emitted11 words. Parser now
accepts only known10/11-word records and checks shader/stage/error/count fields;
pixel/filter expectations were not weakened to fix this instrumentation issue.
The first failed report remains preserved separately in native1 artifacts.

## Limits and next work

This gate covers static array indices,RGBA8 2D images and nearest/linear textureLod
with clamp-to-edge. It does not establish all image dimensions/formats,all graphics
stages,all dynamic descriptor forms or general robustness2. No feature bits added.
The tested modules are not yet exported as a glibc Vulkan loader layer.

Per latest priority,do not add unrelated synthetic gates next. Run the real pinned
DXVK-Sarek native ARM64/glibc consumer,record actual instance/device/D3D11 failures,
then integrate only blocking standard API paths. Keep full DroidDeck/Wine/game
claims distinct from a native offscreen D3D11 device/clear/readback.
