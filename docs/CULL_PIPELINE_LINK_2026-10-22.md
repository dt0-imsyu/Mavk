# Experimental CullDistance pipeline interception

Existing vkCreateGraphicsPipelines hook optionally links original VS/FS through
common compileCullDistance,creates temporary native modules,and adds GS itself.
No caller-transformed shaders required. Shared compiler accepts explicit
Vulkan1.1/1.2/1.3 target and returns location budget. Existing guard compilation,
descriptor snapshots and shader/pipeline ownership remain the common path.
Tracking mode retains non-guarded source; guarded mode instruments linked stages.

MAVK_EXPERIMENTAL_CULL_PIPELINES=1 is OFF by default,cached per device.
Requires geometryShader already enabled by caller;no implicit feature enabling.
Native geometry component/vertex/total-output/location limits checked. Missing
dependency gives FEATURE_NOT_PRESENT and GAP before native generated pipeline.
PointSize mediation rejected,not advertised. Public feature queries unchanged.

PRIVATE DIAGNOSTIC PROFILE,not public Vulkan conformance:shaderCullDistance0
means a general Vulkan application cannot legitimately enable/use this public
feature yet. Diagnostic consumer supplies original unsupported builtin under
explicit experimental transformation. This is not upstream DXVK device PASS.

## Final candidate and evidence

Layer47de8943394347cf6bab24ff8abdbcc84ee8a520ae4802c29b41770573b3485d
in E:/Mavk/artifacts/cull-pipeline-final-layer-20261022/;
consumer620248fc0b005a6bd14bce9df31fcb2858e45e6adfae64c10ae81d14bbadbaca
in cull-link-descriptor-consumer-20261022/. Existing9d34b154 guardfrontend and
1f1f274a optimizer retained;common cull source linked separately into candidate.
APK25/runtime4de26844 stays installed,unchanged. No runtime asset update or APK
install,commit/push or production U-AVF change.

Four actual loader modes:descriptor-free/UBO VS,each tracking/guarded. Original
2stage VS/FS,CullDistance source,geometry1/publicCull0/request0. Exactly one
native2->3stage link per process.8cases/2planes/32768bytes each,131072total
compared with independent no-cull/no-GS reference. Whole-plane/mixed-sign/zero/
restore semantics PASS. UBO+guarded has3native-instrumented stages/publicSets1,
9guarded draws/9submissions. This exercises hook+metadata,not standalone SPIR-V.
Dependency negative:geometry disabled,explicit GAP,no CULL_LINK,CoreliveZero1.
Strict verify-cull-pipeline.py PASS. Host compiler7valid/5negative PASS.

Existing26GPU cases with flag OFF PASS74.177sec,strict no-event/no-diagnostic-
read gates. Not a performance delta. Candidate Muck30516 with flag absent:
actual candidate mapping,menu/Settings/input visually PASS,no observed gaps/
compile/device-loss markers. Regression ONLY,not Muck cull usage. Quit normally;
candidate override removed,Steam left running on installed APK25 baseline.

Artifacts:E:/Mavk/artifacts/cull-pipeline-gpu-20261022/.
link archivebd4d7bb73ae6ce5e24b0c535a3b83c84a16b8e069117bd6c0f3710df78a9fc44;
regressions478d46b3b227b57d6a8c49b11579b990798d6bb9f60b61c76896396c5a9adb43;
Muckc201e0a661f55299bcc6883d0451e47b53338d03ef23d6a6cdd3a7b50610cb3f.

## Remaining

Existing GS/TES/FS-cull/primitive-ID inputs,clip combinations,user output blocks,
specialization,XFB,multiview,PointSize,dynamic topology unadmitted. Point/line
compiler builds are host-only. Libraries/derivatives/custom allocators remain
outside experimental adapter profile. No Bridge/ICD/full feature/upstreamDXVK/
CTS/visible-FPS claim. Complete mandatory semantics before public feature bit;
do not spoof admission to make a device test pass. Next linked interface/stage
contracts or concrete consumer blocker,not repeated closed triangle tests.
