# Concrete buffer pointer alias bypass

Validated copied-pointer-buffer.spvasm (Vulkan1.1) creates three buffer
AccessChains,then OpCopyObject pointer aliases before its read/two stores.
Existing compiler --nested test FAIL before fix:private bounds metadata was
absent. This is not a speculative missing feature or a game-specific case.
The unfixed shader was NOT executed on GPU.

Common AggregateBuffers prepass now removes identical Uniform/StorageBuffer
pointer CopyObjects BEFORE aggregate lowering and access-chain combination.
It propagates NonUniform if present,removes obsolete alias decorations/names,
replaces uses with the same pointer and leaves application arithmetic intact.
Other storage classes/OpPhi/OpSelect pointer semantics are not newly supported.
NonUniform GPU proof remains separate and is not claimed by this fixture.

Host after fix:--nested4checks PASS;copy-operands6checks/1legacy rejection,
prior compiler25checks/9rejections,static-use14checks/7rejections PASS.
Independent spirv-val of guarded result PASS. ARM frontend:
E:/Mavk/artifacts/copied-pointer-frontend-20261021/libmavk-shader-guards.a
SHA256872c57ec1327b50e00ee7346492f94d36b6cd366178503449f8ba9d4dd9e08c4.
GPU PASS with layer804d9271cb8a62a76f8c3a8c61d40d267c83b1ed5a2bd395b46e3322a87f322b:
read-zero/write-suppression fixture plus20prior scenarios,strictly verified by
verify-nested-buffer.py --copied-pointer. Six aggregate forms/1152words also
PASS;gaps0/Corezero. results.tar.gz andvariants-results.tar.gz in
E:/Mavk/artifacts/copied-pointer-layer-20261021/.
Real Muck process9528 mapped this candidate;D3D11 device featurelevel11_0
created and81534guarded commands observed. Saved muck-headless.log SHA256
6b592c1f8f06bde49bd857017d16847ccaf0c2d34e727d5cd8d4dd69e5f0d6b9.
No Mavk-gap/guard-compilation-error/device-loss/failing-case markers found.
Game stopped with its existing isolated prefix wineserver;only its diagnostic
layer override removed from app+guest settings. Steam/commonAPK19 left running.
This is headless integration evidence,NOT visible game/FPS/full-game PASS.
No feature-bit changes/CTS/APK rollout/commit/push. InstalledAPK19 unchanged.
