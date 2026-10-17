"""Parser/rejection tests only. Fixtures are NOT valid Vulkan shader modules."""
import importlib.util
import pathlib
import unittest

spec = importlib.util.spec_from_file_location("instrument_null", pathlib.Path(__file__).with_name("instrument-null.py"))
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
header = [0x07230203, 0x00010300, 0, 20, 0]


class SourceGate(unittest.TestCase):
    def test_plain_set_zero(self):
        module.check_source(header + [(4 << 16) | 71, 1, 34, 0])

    def test_reserved_or_other_sets(self):
        for index in (1, 6, 7, 0xFFFFFFFF):
            with self.subTest(index=index), self.assertRaisesRegex(ValueError, "set0"):
                module.check_source(header + [(4 << 16) | 71, 1, 34, index])

    def test_atomic_rejection(self):
        for opcode in (*range(227, 243), 318, 319, 5614, 5615, 6035):
            with self.subTest(opcode=opcode), self.assertRaisesRegex(ValueError, "atomic"):
                module.check_source(header + [(1 << 16) | opcode])

    def test_malformed(self):
        for words in ([], [0]*5, header+[0], header+[(3 << 16) | 71, 1]):
            with self.subTest(words=words), self.assertRaises(ValueError):
                module.check_source(words)


if __name__ == "__main__":
    unittest.main()
