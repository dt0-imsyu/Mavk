# Android hardware-buffer image adapter

`ahb_image.c` imports a described, single-layer RGBA8 AHB into an exclusive,
optimal-tiling Vulkan image with dedicated memory. It is shared by the existing
glibc producer and independent native Android consumer, not an ICD or WSI layer.
No Android/Bionic system headers are included in glibc code. The public header is
C++17-compatible; compile the implementation as C11 and link it into a consumer.

Supply actual `AHardwareBuffer_describe` values and enabled Vulkan extension
dispatch. Output must initially be empty. On failure no image/memory ownership is
transferred. On success Vulkan memory retains its own AHB reference; the caller
still owns its original reference. Retire GPU and compositor use before destroy.
Destroy is idempotent for an empty object; no hidden queue/device waits occur.

Only RGBA8 2D, one layer/mip/sample and the requested color/sampled/transfer usage
subset are admitted. Protected and complete-mipmap allocations are rejected.
Actual importability, limits, AHB format features and memory type mask are checked.
No guesses about native-handle FD indices, modifiers or CPU memory mappings.

This adapter does not perform external ownership/layout transitions, transport,
fence handoff, acquire/present or expose any Vulkan feature/extension. Those remain
the responsibility of the consumer. In particular it is not yet a replacement
for the installed Wine/X11 WSI. A real swapchain must not report an image ready
until the display release dependency is represented in its acquire synchronization.

`ahb_image_test.c` checks mocked failure/cleanup paths only. Real GPU evidence is
the existing cross-ABI AHB pool regression (same helper linked independently on
both sides), not the mocked test.

Reference: https://docs.vulkan.org/spec/latest/chapters/memory.html#memory-external-android-hardware-buffer
