# Concrete texel bounds bypass: optional image operands

Next upstream-DXVK P0 contract,not a new descriptor-shape expansion.
Pinned InstBindlessCheckPass::GenTexBuffCheckCode checked image operations only
when no optional operands were present. Valid explicit None and ZeroExtend
fetches thus had no texel-size check. Current glslc Vulkan1.2 INTEGER_TEXELS
output already contains ZeroExtend;this is not merely synthetic future syntax.

Host regression --texel-operands FAIL before fix/PASS after for masks0/8192:
five checks each,exactly one OpImageQuerySize and one fetch retaining its mask.
Input and guarded output spirv-val Vulkan1.2 PASS. Copied-pointer4checks,
copy-operands6checks/1legacy rejection andprior compiler25checks/9rejections
remain PASS. No unguarded OOB fixture was executed on GPU.

## Implementation/provenance

Change admits only absent mask or one no-ID mask containing
SignExtend/ZeroExtend/Nontemporal bits. Original instruction cloning retains
the operands. Scoped/memory-model forms remain outside this slice. This adds
no Vulkan feature claims. SignExtend/Nontemporal GPU proof not yet claimed.

Tracked patch:compatibility/shaders/spirv-tools-texel-operands.patch;reverse
git apply --check against modified source PASS. External research source before
SHA2567d1dca85572266470253a59587adb6a4273ae24cfd05b5c77508907271d8a58c,
afterea3af585a7f41689338f3acc03dc740e8ef1e8f56ea0238344899b0a000eadd3.
Existing custom atomic changes are preserved;do not apply to an unrelated
unpatched SPIRV-Tools tree and assume the same provenance.

build-texel-optimizer.sh compiles ONLY the affected ARM64 object and replaces
that single member in a fresh COPY of pinned optimizer archive. Archive member
count for inst_bindless_check_pass.cpp.o=1. Original archive untouched.
build-mavk-layer.sh accepts explicit MAVK_SPIRV_OPT_ARCHIVE;default remains
the original pinned optimizer. Frontend remains copied-pointer872c57ec.

Optimizer:texel-operands-optimizer-20261021/libSPIRV-Tools-opt.a
SHA2560d8a060712af37ad80fa6606946f368f8473e1e72afd558ef4f7e96a3ed4b2d2.
Layer:texel-bounds-layer-20261021/libVkLayer_mavk_compat.so
SHA256743d9d6c16cbff3b84e1089c7627199e0c1784805194b5a74b9f46a7ad703bde.
Consumer:texel-bounds-consumer-20261021/layer-execution
SHA25640178f3c1a89a4491a9026661eb8a9d7fdff80d51b3328be144ae4185455ffba.
All artifact paths rooted atE:/Mavk/artifacts/.

## GPU gate result

Reuse existing RGBA8 UNORM/UINT texel consumer with bounded coordinate set
0,63,-1,64,1073741824,nonzero view offset/whole-size with trailing bytes.
512components,positive in-range values and zero OOB results,input/padding
unchanged:PASS on Mali-G925. Prior21GPU cases PASS;22total,strictly checked
by verify-nested-buffer.py --texel-bounds. results.tar.gz in layer artifact
folder. CoreliveZero1/gaps0 throughout. Muck regression running for this
candidate,not yet counted. InstalledAPK19 and production unchanged.
No CTS/full robustness2/FPS claim.

## Integration gates

Headless Muck14542 mapped743d9d6c,D3D11device/swapchain/211565guarded commands,
no observed Mavk-gap/compile-error/device-loss markers. Saved muck-headless.log
SHA25602c580e9aa3c948620f625c46d468d43791956ad5f68cccc5156b56f9aae4c93.
Game stopped/override removed.

User unlocked Android;ordinary launch now READY/firstFrame=true. Normal
droiddeckctl launch1625450 with per-game candidate selected mapped743d9d6c in
Muck16093. Visible menu/animated clouds/grass,Settings click worked. Screenshots
candidate-visible-initial.png,candidate-settings.png and10sec candidate-visible.mp4
saved in layer artifacts. No obvious visual corruption in inspected screens.
SurfaceFlinger baselineAPK19 menu135frames/10.1sec=13.3screenupdates/s;
candidate194frames/15.1sec=12.8. Not equal-duration controlled performance A/B,
not proof every presented buffer is unique;do not advertise game FPS from this.
Overlay0fps clearly does not represent the observed changing output.
After normal client quit,Steam UI was visible too (steam-before-apk20.png).

This finite package is now being rolled into experimentalAPK20,not production.
Runtimebfc10a4c29a8c75d7040a0d36158a9d9abb589e382b6bb94bb1a25acfd07e4fb,
261other members identical. APK485other members identical,16KBalign/signature
verified with same devcerta40da80a. SignedAPKf186cb72816ea7e5b64c48779cc155d183c1e96e032d16632cbff0fc2d28ff7d
atE:/Mavk/artifacts/droiddeck-texel-apk20-20261021/DroidDeck-Mavk-texel-20261021.apk.
Install/common-runtime smoke check pending;APK19 preserved for rollback.

APK20 install-r SUCCESS;new normal Steam session2026-10-21-14-steam READY,
firstFrame=true. Visible Steam library screenshotsteam-common.png;CEF renderer
ANGLE/Mesa/Zink Vulkan1.3/Mali-G925 confirmed. New runtime bfc10a4c and layer
SHA743d9d6c confirmed INSIDE guest. All22common GPU cases PASS against that
actual runtime path,not candidate directory. common-results.tar.gz in APK20
folder. Both app/guest Muck DROIDDECK_MAVK_LAYER_PATH absent before ordinary
launch. Muck common-runtime smoke pending;no shortcut/per-game candidate needed.

Common-runtime Muck19767 mapped bfc10a4c/payload/libVkLayer_mavk_compat.so;
visible menu and Settings/input confirmed (muck-menu-common.png and
muck-settings-common.png). Initial muck-common.png is the transient black
startup frame,NOT the completed menu check. Saved muck-common.log inAPK20
folder:208875guarded commands,no observed Mavk errors,SHA256
d967459cb71c8d9c3eff0d303db39142828b546725830138cfe14998d3d13cc3.
Game quit through client;Steam left running. Source runtime assets synchronized
to bfc10a4c. Default common layer build now uses copied-pointer872c57ec frontend
and isolated optimizer0d8a0607 (supersedes the earlier default-pinned note).
APK19 rollback retained. Full game/uniqueFPS/CTS remain unproved.

Semantic reference:https://registry.khronos.org/SPIR-V/specs/unified1/SPIRV.html#Image_Operands
