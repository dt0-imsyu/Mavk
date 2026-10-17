# Native baseline harness (M0, not a complete Mavk backend)

Isolated debug APK `org.mavk.probe`; no network, storage or VM permissions.
It does not depend on or replace either U-AVF APK. No system settings, driver,
SELinux, Vulkan layer injection or global GPU properties are modified.

Build with installed Android SDK36/NDKr28b/JBR17+:

```powershell
./build.ps1 -OutputDirectory E:/Mavk/artifacts/native-compute-<fresh-run>
```

Build output must be a fresh directory. APK uses the existing local Android
debug signing key; it is not release signed and must not be published as a
production backend. Shaders are compiled with the installed NDK glslc.

The initial workload selects an ARM non-CPU Vulkan device, runs128 compute
dispatches writing65536 words with varying seeds, performs an explicit shader
write→host-read barrier and a bounded fence wait, invalidates noncoherent memory
where needed, and checks every result on CPU. GPU timestamps are used only
when the selected queue reports support. Optional device features/extensions
are queried but **none are enabled** for this workload.

Retrieve the app-owned report after the worker completes:

```powershell
adb shell run-as org.mavk.probe cat files/native-compute.json
```

Save the report before another run (the app intentionally overwrites only its
own previous report). Force-stop/start **only org.mavk.probe** for clean-context
repeats. Do not use broad package/process termination or start a guest VM.

## Limits

`NATIVE_COMPUTE_CORRECTNESS_PASS` is not a graphics/Surface test, conformance
certificate, guest Vulkan bridge, WDDM acceleration, DXVK compatibility or
Mavk-overhead result. This tiny workload cannot predict game/desktop FPS.
`gpu_ms` measures its command span, not CPU scheduling/fence wakeup or scanout.
No validation layer is enabled in the first build; allocator/lifetime stress,
validation, graphics and Surface lifecycle remain separate required tests.

Reference for barrier/readback semantics:
[Khronos synchronization examples](https://docs.vulkan.org/guide/latest/synchronization_examples.html).
Future validation should be app-local, following the
[Android validation-layer guide](https://developer.android.com/ndk/guides/graphics/validation-layer),
without changing global device configuration.
