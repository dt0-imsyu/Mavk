# Storage texel bounds and mixed signed offset correction

After completing APK20 visual/common smoke,extend the SAME P0 contract to
R32_UINT storage texel read/write,not another descriptor shape or game-specific
workaround. Shader and consumer reuse Core/loader/layer.

Buffer128bytes (32uints),view16bytes (4texels). Read17/-1 must return0;write17
must not modify its real allocation sentinel. Write0 then image barrier/read0
must return5A17C0DE;read1 must returnoriginalA5000001. Check32target+32result
words including all untouched sentinels. Native R32 storage texel queried.

## Actual host blocker found and fixed

Valid glslc Vulkan1.2 shader generated mixed signed/unsigned byte-offset math.
The saturation transform rewrote it to invalid UMulExtended(uint4,int0).
spirv-val rejected before GPU execution. This is shared instrumentation,not
Mali/Vulkan ABI/game failure. InstalledAPK20 is not claimed to pass this case.

SaturateGuardOffsets now normalizes same-width32-bit operands via bit-preserving
unsigned Bitcast before its generated extended arithmetic. Application math
unchanged;other widths reject instead of implicit narrowing. Host fixture now
compiles/validates. Exact structural gate:5ImageQuerySize,3ImageRead preserving
ZeroExtend,2ImageWrite. Constant negative texel index folds safely tozero,
explaining why no fourth read/query survives. Fiveassertions PASS.
Prior texel5checks,copy6/1,nested4,static14/7,compiler25/9 PASS.

Frontend:signed-offset-frontend-20261021/libmavk-shader-guards.a
SHA256f0a0c0571feac87876bb20963021b23f20bdb458a9119352c36e22e976ffd84d.
Layer storage-texel-layer-20261021 SHA256
c2a12a217264df60aab53bfed2087c8d7028250d473e0f6147bd5dbf1388b79e.
Consumer7d83fd14a45c3d0cd8a220598f21cca640e33c3edc454993b74cd1721699d186.
GPU PASS on Mali:64words with view-bound read-zero/write-suppression and valid
read/write,all sentinels preserved. Prior22cases also PASS;strict verifier
--storage-texel. results.tar.gz SHA256
524de1e2a60cb9b9dfa15ed8fb1a11e6d853c816f6525c3d9090e38a01534150.
Six aggregate forms/1152words PASS in variants-results.tar.gz.
All artifact paths rootedE:/Mavk/artifacts/.
MAVK_STORAGE_TEXEL=1 explicitly enables the new gate in existing control;
absence of the flag preserves APK20's22-case regression scope.
No new APK installed/feature advertised/CTS claim for this candidate.

## Real consumer and rollup

Ordinary Muck launch22735 with isolated candidate mapped c2a12a21. Visible menu
and Settings/input confirmed (muck-visible.png,muck-settings.png);440371guarded
commands,no observed Mavk-gap/compile-error/device-loss markers. Saved
muck-visible.log SHA2562041124ed234249cf77bb6e259f5e7630f7291e9c43682c3eb25e674eff13094.
Game quit through client;per-game override removed before rollout.

APK21 being prepared:runtime8de921837db9d2196dd4aa2d28d1653404d8dc003db4bc3ca59ef80a252dfe60,
only layer/manifest changed,261other runtime members identical. Source assets
stillAPK20 until install/common smoke. No full robustness2/CTS/uniqueFPS claim.

## Verified installed APK21

SignedAPK SHA256c5f68760f714d3fb8b3fc62c081b2f7468c1061159285408722b004936ee0335
atE:/Mavk/artifacts/droiddeck-storage-apk21-20261021/DroidDeck-Mavk-storage-20261021.apk.
Same devcertificate,16KBalignment/signature PASS,485other APK members identical.
Install-r SUCCESS,data preserved. Normal session2026-10-21-15-steam READY and
firstFrame=true. All23COMMON GPU cases PASS using actual8de92183 payload path;
common-results.tar.gz saved inAPK21folder. Visible Steam library/hardware
ANGLE/Zink/Mali confirmed. Normal Muck26715 mapped8de92183 payload,visible menu
andSettings/input PASS;816873guarded commands,no observed Mavk errors. Log
saved compressed asAPK21/muck-common.tar.gz. Game quit throughclient,Steam
left running;no per-game layer override. Source assets now8de92183,default
frontendf0a0c057 + optimizer0d8a0607. APK20 retained as rollback.
Menu-level integration only;not full-game completion or uniqueFPS/conformance.
