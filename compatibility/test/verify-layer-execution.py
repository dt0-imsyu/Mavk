"""Real-loader command-routing evidence, not Vulkan conformance or FPS."""
import argparse
import re
from pathlib import Path

p = argparse.ArgumentParser(description=__doc__)
p.add_argument("reports", type=Path)
mode = p.add_mutually_exclusive_group()
mode.add_argument("--dynamic", action="store_true")
mode.add_argument("--empty", action="store_true")
mode.add_argument("--unused", action="store_true")
mode.add_argument("--selected", action="store_true")
mode.add_argument("--resubmit", action="store_true")
mode.add_argument("--inflight", action="store_true")
mode.add_argument("--texel", action="store_true")
mode.add_argument("--images", action="store_true")
a = p.parse_args()
if a.inflight:
    a.resubmit = True
required = () if a.dynamic or a.empty or a.unused or a.selected or a.resubmit or a.texel or a.images else ("vkCmdDispatchBase", "vkCmdDispatchBaseKHR", "vkCmdDrawIndirectCount",
            "vkCmdDrawIndirectCountKHR", "vkCmdDrawIndexedIndirectCount",
            "vkCmdDrawIndexedIndirectCountKHR")
outputs = []
for mode, guarded in (("native", 0), ("guarded", 1)):
    out = (a.reports / f"{mode}.stdout").read_text()
    err = (a.reports / f"{mode}.stderr").read_text()
    assert "HARDWARE Mali-G925-Immortalis MC12" in out
    assert "CANDIDATE_LAYER_MAPPED " in out
    lines = out.splitlines()
    for command in required:
        assert any(line == f"GPU_PASS {command}" or
                   line == f"GPU_PASS {command} count0/count1 offsets16/12" for line in lines), command
    if a.images:
        assert "IMAGE_SUBRESOURCE_GPU_PASS views=16 components=256 mips=3 layers=2 mutableSrgb=1 paddedRows=1 afterRecordHostWrite=1 compressedCopy=1" in lines
        assert f"MAVK_LAYER_COMMAND_END guarded={guarded} draws={16 if guarded else 0} submissions=18" in err
        if guarded:
            assert err.count("MAVK_LAYER_EXECUTE guarded=1 sets=2 generations=1\n") == 16
    elif a.texel:
        assert "TEXEL_FORMAT_GPU_PASS formats=2 components=512 elements=64 wholeSize=1 trailingBytes=2 offsetNonzero=1" in lines
        assert f"MAVK_LAYER_COMMAND_END guarded={guarded} draws={2 if guarded else 0} submissions=2" in err
        if guarded:
            assert err.count("MAVK_LAYER_EXECUTE guarded=1 sets=2 generations=1\n") == 2
    elif a.dynamic or a.empty or a.unused or a.selected or a.resubmit:
        tail = ' unusedBindings=7 dynamicOffsets=11' if a.selected else ' unusedNonemptySlots=1' if a.unused else ' emptySlots=1' if a.empty else ''
        assert re.search(r"^DYNAMIC_DESCRIPTOR_GPU_PASS dispatches=4 dynamicDescriptors=7 sets=2 checkedWords=[1-9][0-9]* partialRebind=1 consumedImmediately=1 wholeSize=1" + tail + r"$", out, re.M)
        assert f"MAVK_LAYER_COMMAND_END guarded={guarded} draws={4 if guarded else 0} submissions={4 if a.empty or a.unused or a.resubmit else 2}" in err
        if a.resubmit:
            assert re.search(r"^REUSABLE_COMMAND_GPU_PASS submissions=4 replays=2 checkedWords=[1-9][0-9]* resetAfterCompletion=1 descriptorUpdatesAfterRecord=0$", out, re.M)
        if a.inflight:
            assert "INFLIGHT_COMMAND_GPU_PASS blockedSubmissions=2 fencesInitiallyUnsignaled=2 hostTimelineRelease=1 simultaneousUse=1" in out.splitlines()
        if a.empty:
            assert re.search(r"^EMPTY_LAYOUT_GPU_PASS mixedEmptySlots=1 allEmptySlots=3 unboundEmptySet=1 zeroCountReserved=1 comparedComponents=1024 redPixels=[1-9][0-9]* blackPixels=[1-9][0-9]*$", out, re.M)
        if a.unused:
            assert re.search(r"^UNUSED_LAYOUT_GPU_PASS mixedNonemptySlots=1 allUnusedNonemptySlots=3 unboundUnwrittenSet=1 comparedComponents=1024 redPixels=[1-9][0-9]* blackPixels=[1-9][0-9]*$", out, re.M)
        if guarded:
            assert err.count(f"MAVK_LAYER_EXECUTE guarded=1 sets={4 if a.empty or a.unused or a.selected else 3} generations=1\n") == 4
    else:
        assert "LAYER_EXECUTION_GPU_PASS words=512 components=8192 dispatchCases=2 drawCases=8 coreLiveZero=1" in lines
        assert f"MAVK_LAYER_COMMAND_END guarded={guarded} draws={12 if guarded else 0} submissions=12" in err
    assert "gaps=0" in err and "MAVK_LAYER_CORE_END liveZero=1" in err
    assert "MAVK_LAYER_GAP" not in err and "FAIL " not in out
    outputs.append(out)
assert outputs[0] == outputs[1], "Native and guarded outcomes differ"
if a.images:
    print("IMAGE_SUBRESOURCE_NATIVE_GUARDED_PASS BC1RGBA sixteen mip/layer/views; no fullBC/CTS/SyncVal claim")
    raise SystemExit(0)
if a.texel:
    print("RGBA8_TEXEL_NATIVE_GUARDED_PASS UNORM/UINT512components; no typed-null/all-formats/CTS claim")
    raise SystemExit(0)
if a.unused:
    negative = (a.reports / "missing.stdout").read_text()
    negative_err = (a.reports / "missing.stderr").read_text()
    assert "STATIC_USED_SET_REJECTION_PASS missingSet=2 submissions=0" in negative.splitlines()
    assert negative_err.count("MAVK_LAYER_GAP operation=guarded execution reason=guarded descriptor compatibility\n") == 1
    assert "gaps=1" in negative_err and "MAVK_LAYER_COMMAND_END guarded=1 draws=0 submissions=0" in negative_err
    assert "MAVK_LAYER_CORE_END liveZero=1" in negative_err and "FAIL " not in negative
if a.selected:
    negative = (a.reports / "unwritten.stdout").read_text()
    negative_err = (a.reports / "unwritten.stderr").read_text()
    assert "STATIC_USED_BINDING_REJECTION_PASS unwrittenArrayElement=1 submissions=0" in negative.splitlines()
    assert negative_err.count("MAVK_LAYER_GAP operation=guarded execution reason=statically used descriptor unwritten\n") == 1
    assert "gaps=1" in negative_err and "MAVK_LAYER_COMMAND_END guarded=1 draws=0 submissions=0" in negative_err
    assert "MAVK_LAYER_CORE_END liveZero=1" in negative_err and "FAIL " not in negative
print("INFLIGHT_COMMAND_NATIVE_GUARDED_PASS two timeline-blocked submissions/reset; no late-update/UAB/CTS claim" if a.inflight else "REUSABLE_COMMAND_NATIVE_GUARDED_PASS two fence-completed replays/reset; no in-flight/UAB/CTS claim" if a.resubmit else "SELECTED_BINDINGS_NATIVE_GUARDED_PASS seven unwritten unused bindings/eleven dynamic offsets; no CTS/SyncVal/FPS claim" if a.selected else "UNUSED_LAYOUT_NATIVE_GUARDED_PASS nonempty unbound sets; no CTS/SyncVal/FPS claim" if a.unused else "EMPTY_LAYOUT_NATIVE_GUARDED_PASS mixed gap and all-empty descriptorless draw; no CTS/SyncVal/FPS claim" if a.empty else "DYNAMIC_DESCRIPTOR_NATIVE_GUARDED_PASS seven dynamic descriptors/two sets/four dispatches; retained generation and WHOLE_SIZE; no CTS/SyncVal/FPS claim" if a.dynamic else "LAYER_EXECUTION_NATIVE_GUARDED_PASS words=512 components=8192; AMD unavailable, no CTS/SyncVal/FPS claim")
