"""Offline parser/admission regressions, not Vulkan execution tests."""
import importlib.util
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location("audit", Path(__file__).with_name("audit-capabilities.py"))
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)

class Capabilities(unittest.TestCase):
    def test_unknown_is_not_false(self):
        self.assertEqual(audit.differences({"feature": False}, {}),
                         {"feature": {"turnip": False, "mali": None}})

    def test_numbers_are_not_booleanized(self):
        self.assertEqual(audit.flatten({"formats": {"37": {"mask": 513025}}}),
                         {"formats.37.mask": 513025})

    def test_queue_arrays_preserved(self):
        queues = [{"queueFlags": 23, "queueCount": 2}]
        self.assertEqual(audit.flatten({"queues": queues}), {"queues": queues})

    def test_robustness_spelling(self):
        self.assertEqual(audit.resolve_extension("extRobustness2",
            {"extRobustness2": "VK_EXT_ROBUSTNESS_2"}, {"VK_EXT_robustness2"}), "VK_EXT_robustness2")

    def test_missing_extension_name_still_retained(self):
        self.assertEqual(audit.resolve_extension("khrSwapchain",
            {"khrSwapchain": "VK_KHR_SWAPCHAIN"}, set()), "VK_KHR_swapchain")

if __name__ == "__main__":
    unittest.main()
