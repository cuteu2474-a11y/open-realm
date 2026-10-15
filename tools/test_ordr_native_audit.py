#!/usr/bin/env python3
"""Regression tests for the offline ORDR extension coverage audit."""
import importlib.util
import pathlib
import unittest

MODULE_PATH = pathlib.Path(__file__).with_name("ordr_native_audit.py")
spec = importlib.util.spec_from_file_location("ordr_native_audit", MODULE_PATH)
audit_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit_module)


class NativeCoverageTests(unittest.TestCase):
    def test_registry_only_matches_function_entries(self):
        source = '''
            { "DzFrameShow", DzFrameShow },
            { "EXPauseUnit", PauseUnit },
            // { "DzFake", DzFake }
        '''
        names = audit_module.registry_names(source)
        self.assertIn("DzFrameShow", names)
        self.assertIn("EXPauseUnit", names)

    def test_only_extension_calls_count(self):
        usage = {"map": "fixture.w3x", "native_calls": {
            "DzFrameShow": 4, "JNStringCount": 2,
            "EXPauseUnit": 1, "CreateUnit": 12, "DzUnused": 0,
        }}
        result = audit_module.audit(usage, {"DzFrameShow", "EXPauseUnit"})
        self.assertEqual(result["required_count"], 3)
        self.assertEqual(result["registered_count"], 2)
        self.assertEqual(result["missing_by_static_frequency"], {"JNStringCount": 2})

    def test_missing_sorted_by_frequency(self):
        result = audit_module.audit({"native_calls": {
            "DzLow": 1, "DzHigh": 5, "DzMid": 3,
        }}, set())
        self.assertEqual(list(result["missing_by_static_frequency"]),
                         ["DzHigh", "DzMid", "DzLow"])


if __name__ == "__main__":
    unittest.main()
