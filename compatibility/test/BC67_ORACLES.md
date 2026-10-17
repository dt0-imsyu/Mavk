# BC6H/BC7 oracle evidence (2026-10-17)

Production/fallback codec: unmodified iOrange/bcdec80859ed, MIT alternative.
First independent reference tried: detex cab11584d9be140602a66fd9a88ef0b99f08a97a.
It disagreed; do not use it as acceptance oracle for this integration.
Reproduction remains in experiments/bc67-detex-investigation.cpp (intentionally
failing investigation, NOT a passing regression test).

Two recorded counterexamples:

- BC7 mode1, block366580c6dcd497402e2388b7e6822b4a: byte2 bcdec64,detex0.
- BC6H unsigned mode0, block2487bd1fc06efb076d006c6b6bdc1030:
  pixel2 blue bcdec0.0000370144844,detex0.0000349879265(half0x024b).

Third BC7 reference richgel999/bc7enc_rdo b9438627eef73a1157e84201b6fa6eb2ffd6d9f0
agreed with bcdec across all tested BC7 modes. Header/source licenses preserved
in E:/Mavk/research/bc7decomp-b943862; test-only, not linked into Android APK.

Decisive independent acceptance gate: **Microsoft D3D11 WARP** on host Windows,
created through documented D3D11CreateDevice(D3D_DRIVER_TYPE_WARP), no UI/VM/ICD.
Actual compressed textures sampled with Texture2D.Load into structured float4
buffer, dispatch→staging readback. No Mavk decoder executes inside oracle shader.
BC7 checked2048 random valid blocks, all8 modes. BC6H unsigned3584+signed3584,
all14 modes per variant.589824 output components checked. Deterministic seed
includes both counterexamples above. BC7 tolerance1e-6 relative/1e-9 absolute in
normalized float; BC6H same tolerance, no loose half-ULP blanket waiver.

Observed terminal transcript (not a separately generated runtime report):

    MICROSOFT_WARP_BC_ORACLE_SLICE_PASS variant=0 blocks=2048
    MICROSOFT_WARP_BC_ORACLE_SLICE_PASS variant=1 blocks=3584
    MICROSOFT_WARP_BC_ORACLE_SLICE_PASS variant=2 blocks=3584
    BC67_MICROSOFT_WARP_ORACLE_PASS components=589824 jobs=3 NOT_ANDROID_GPU_OR_FULL_BC_API_PASS

Artifact E:/Mavk/artifacts/bc67-20261017/bc67-warp-oracle.exe;
source test/bc67-warp.cpp. This is a sampled deterministic corpus, NOT an
exhaustive all128-bit-block conformance proof. Does not prove Android sampling.

Separate independent goldens/bounds gate:3502 assertions,18 invalid inputs,
BC6H +/-65504 extrema, reserved modes, BC7 mode6 white, padded5..8 edges.
`bc67-bounds.cpp`, artifact bc67-bounds.exe. No third-party test oracle dependency.

Host references stay outside the Android runtime. No Adreno code used; no
driver/features spoofed, root/firmware/Windows-VM changes, commit or push.
