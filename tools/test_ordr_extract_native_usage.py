#!/usr/bin/env python3
import importlib.util
import pathlib
import unittest

path = pathlib.Path(__file__).with_name("ordr_extract_native_usage.py")
spec = importlib.util.spec_from_file_location("ordr_extract_native_usage", path)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

class ExtractionTests(unittest.TestCase):
    def test_ignores_comments_and_strings(self):
        source = '''
        // DzFake(1)
        /* DzBlocked(2) */
        call DzFrameShow(f, true)
        set s = "DzNotReal(3)"
        call DzFrameShow(f, false)
        call JNStringCount(s)
        '''
        self.assertEqual(module.extract_calls(source),
                         {"DzFrameShow": 2, "JNStringCount": 1})

    def test_escaped_quote(self):
        source = 'set s = "text \\" DzFake(0)"\ncall EXPauseUnit(u, true)'
        self.assertEqual(module.extract_calls(source), {"EXPauseUnit": 1})

if __name__ == "__main__":
    unittest.main()
