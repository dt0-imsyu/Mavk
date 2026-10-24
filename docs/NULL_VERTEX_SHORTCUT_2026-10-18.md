# Null vertex bindings: short experiment closed

Result: **NO-GO for zero-buffer + ordinary robustBufferAccess shortcut**.
This is a specification-level rejection, not a failed GPU measurement. No
vertex implementation or extra synthetic GPU fixture was added.

The [Vulkan shader specification](https://github.com/KhronosGroup/Vulkan-Docs/blob/main/chapters/shaders.adoc)
allows ordinary robust out-of-bounds vertex accesses to return values from the
underlying allocation or several zero/default patterns. A lucky zero result on
one Mali run cannot establish exact null-binding semantics. Missing vertex
components also have specified defaults, not universally all-zero components.

The [pipeline specification](https://github.com/KhronosGroup/Vulkan-Docs/blob/main/chapters/pipelines.adoc)
requires robustBufferAccess2 support for ACCESS_2 vertex robustness (VUID06933).
Native pipelineRobustness alone therefore does not bypass the absent feature.

Next: user-authorized local Sarek admission diagnostic, NOT broader theoretical
null coverage. Keep native queried/enabled nullDescriptor FALSE. The optional
consumer admission flag does not establish valid Vulkan or game compatibility.
Original real Sarek binaries are retained in
`E:/Mavk/artifacts/sarek-pre-null-probe-20261018`; previous run24 also preserves
the exact original payload. Revert the small diagnostic patch and rebuild, or
use the original binaries. No production/VM changes, commit or push.
