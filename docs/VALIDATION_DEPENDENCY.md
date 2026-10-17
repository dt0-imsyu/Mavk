# Harness-only validation dependency

Official KhronosGroup/Vulkan-ValidationLayers release `vulkan-sdk-1.4.363.0`:
https://github.com/KhronosGroup/Vulkan-ValidationLayers/releases/tag/vulkan-sdk-1.4.363.0

Android ZIP (32,748,819 bytes):
https://github.com/KhronosGroup/Vulkan-ValidationLayers/releases/download/vulkan-sdk-1.4.363.0/android-binaries-1.4.363.0.zip

GitHub release asset digest, verified on download:
`sha256:9a3b8e32cb3330406dd4f3651594201635baee4528da8e2a5300e4f55e1a9cc7`.
Extracted arm64-v8a `libVkLayer_khronos_validation.so`:
`sha256:db7447027e28db95584e9fde8a57cc1f28c2dd09defcbe2f2b76ee2bbc65bf09`.

Archives/library/build artifacts remain under E:/Mavk/artifacts, not committed.
Do not relabel this third-party component as Mavk-owned code. This is an internal
debug dependency; any redistribution needs its original licensing/notices and
the upstream release/source dependency audit, not a new Mavk license applied to it.

`harness/build.ps1 -ValidationLibrary <absolute .so path>` packages it only in
the test APK. The default build does not package it. Activity `validation=true`
explicitly requests the layer; missing layer is a failure, not a silent fallback.
Instance settings enable `validate_sync`; debug callbacks count diagnostics
through device/resource teardown. No global GPU properties/settings modified.

An intentional record-time WAW hazard tests SyncVal activation. Its command
buffer is reset/discarded and never submitted to GPU. Expected diagnostics are
separated from actual workload errors. This does not cover every Vulkan hazard,
host memory race, unsupported feature or correctness of the driver itself.

References:
https://developer.android.com/ndk/guides/graphics/validation-layer
https://vulkan.lunarg.com/doc/view/latest/windows/synchronization_usage.html
