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

    def test_native_signatures_preserve_types_and_line_numbers(self):
        source = ('// private text is not exported\n'
                  'native EXPauseUnit takes unit u,boolean flag returns nothing\r\n'
                  'constant native JNUse takes nothing returns boolean\n'
                  'native StockNative takes string value returns integer\n')
        self.assertEqual(module.extract_declarations(source), {
            'EXPauseUnit': {'parameters': [{'type': 'unit', 'name': 'u'},
                                          {'type': 'boolean', 'name': 'flag'}],
                            'returns': 'nothing', 'line': 2},
            'JNUse': {'parameters': [], 'returns': 'boolean', 'line': 3},
        })

    def test_native_declarations_ignore_strings_and_block_comments(self):
        source = ('/*\nnative DzFake takes nothing returns integer\n*/\n'
                  'set s = "native JNFake takes nothing returns string"\n'
                  'native DzSyncData takes string prefix, string data returns nothing // comment\n')
        self.assertEqual(module.extract_declarations(source), {
            'DzSyncData': {'parameters': [{'type': 'string', 'name': 'prefix'},
                                         {'type': 'string', 'name': 'data'}],
                           'returns': 'nothing', 'line': 5},
        })

if __name__ == "__main__":
    unittest.main()
