#!/usr/bin/env python3
"""The iPad bundle must not enable portrait Warcraft III input layouts."""
import importlib.util
from pathlib import Path
import unittest

BUNDLE = Path(__file__).resolve().parents[1] / "bundle.py"
spec = importlib.util.spec_from_file_location("ipad_bundle", BUNDLE)
bundle = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bundle)


class IpadOrientationTests(unittest.TestCase):
    def test_ipad_and_default_orientations_are_landscape_only(self):
        options = bundle.app_orientation_info()
        expected = ["UIInterfaceOrientationLandscapeLeft",
                    "UIInterfaceOrientationLandscapeRight"]
        self.assertEqual(options["UISupportedInterfaceOrientations"], expected)
        self.assertEqual(options["UISupportedInterfaceOrientations~ipad"], expected)
        self.assertFalse(any("Portrait" in orientation
                             for values in options.values() for orientation in values))

    def test_orientation_values_are_independent_lists(self):
        info = bundle.app_orientation_info()
        info["UISupportedInterfaceOrientations"].clear()
        self.assertEqual(len(info["UISupportedInterfaceOrientations~ipad"]), 2)


if __name__ == "__main__":
    unittest.main()
