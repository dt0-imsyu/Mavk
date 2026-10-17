# Active development order — 2026-10-17

User's newest attachment and direct authorization supersede older ordering.
Private **dt0-imsyu/Mavk** uploads are authorized. No U-AVF commits/push or
public releases are authorized by that instruction. Preserve remote license.

## 1. DroidDeck / real D3D11 through DXVK-Sarek (current)

- [x] Pinned Turnip/Mali/DXVK capability comparison; Sarek leaves BC+nullDescriptor.
- [x] BC1–BC7 CPU correctness and bounded native GPU upload/sampling gates.
- [x] Common BC mip/update/copy shadow+dirty lifecycle; native partial upload/sRGB.
- [x] Null SSBO load/store +UBO load guarded feasibility, no feature advertising.
- [x] Scalar SSBO range read-zero/write-discard feasibility on native Mali;
      same metadata phases,1536 values/896 guards,SyncVal0/0. Full feature OPEN.
- [x] Null combined-sampler texelFetch and R32_UINT storage image load/store
      feasibility1792 words/384 guards,SyncVal0/0; all image semantics OPEN.
- [x] Configurable combined shader pass and minimal integer-buffer atomic
      extension; actual32-bit atomicAdd null/valid result+side-effect PASS.
- [x] Shared descriptor state with explicit unwritten/null/bound distinction,
      resource leases,checked updates/copies,immutable cached snapshots/both ABIs;
      integrated into native descriptors and buffer/atomic/image GPU gates.
- [x] Shared native Vulkan materializer with immutable set/metadata generations,
      retained resources/fallbacks,unchanged-generation cache; actual two-recorded
      generation execution/lifetime gate on Mali,SyncVal0/0.
- [x] Bounded Vulkan-structure update/copy frontend and trusted resource registry;
      ordered transactional writes/copies,consecutive bindings,resource retirement.
      Not loader exports or full Vulkan update-after-bind semantics.
- [ ] Full Vulkan image/format/view/subresource adapter, execution-time integration.
- [ ] Correct null images/texel buffers/arrays/atomics and required robustness2.
- [ ] Actual Vulkan device +D3D11CreateDevice +swapchain +correct first frame.
- [x] Reuse proof: existing hybris compat layer executes compressed-image API
      upload+sampling for14 D3D BC identities on G925,896constant components.
      BC5 UNORM/SNORM lack FILTER_LINEAR;full DXVK format admission NOT passed.
- [ ] DroidDeck ABI/ICD integration and real D3D11 application, then small game.
- [ ] Measure CPU/GPU/frame time/overhead; compare PanVK only if available/same device.

Do not claim clip/cull removal in Sarek is emulation. Do not advertise unsupported
features. Native Core PASS != complete ICD/D3D11/device/game PASS. Prefer shared
implementations over a demonstration-only consumer. No new Windows boot for each
Core function. Stop deep DroidDeck investigation only on an evidenced serious
blocker after meaningful alternatives; retain logs/code and return to Windows.

## 2. Windows IddCx after D3D11 success or justified DroidDeck suspension

Reproducible signed-driver installation, startup/reconnect/recovery, integration
with existing H.264→MediaCodec,1920×1200,resolution changes,input latency,safe
fallback. Respect Code Integrity; experimental driver !=production readiness.
IddCx is a display driver, not a hardware WDDM GPU driver.

## 3. U-AVF Windows workspace product integration

UI/lifecycle/start/stop/reboot; supported persistence; keyboard/mouse/touch,
network/audio/clipboard/display; resource/disk/error handling. Preserve Ubuntu.
Then resume advanced Windows GPU/ICD/Bridge/WDDM research independently.

## Invariants

Stock Android,no root/unlock/flash/SELinux changes. Production Windows8CPU,
Ubuntu/P7,H.264,storage stay unchanged. No repeat of closed gfxstream/Venus
branches without new evidence. Tests/artifacts on healthy E; preserve C free
space and tablet ~50GiB. Avoid long cosmetic reports or synthetic polishing.
Current detailed evidence/blocker: STATE.md and compatibility docs.
