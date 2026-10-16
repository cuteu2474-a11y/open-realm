"""Exercise the shipped C ORDR regex helper without SDL/Warcraft assets.

This extracts the *actual* helper from api_ordr_strings.h and compiles it in
isolation; a Python reimplementation would not catch regressions in the game.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "games/warcraft-3/game/api/api_ordr_strings.h"


class ORDRNativeRegexTests(unittest.TestCase):
    def test_compiled_engine_regex(self):
        compiler = shutil.which("cc")
        if not compiler:
            self.skipTest("C compiler unavailable")
        source = HEADER.read_text(encoding="utf-8")
        first = source.index("static unsigned ORDR_Utf8Codepoint(")
        last = source.index("static uint32_t JNStringCount(")
        impl = source[first:last]
        harness = (
            "#include <assert.h>\n"
            "#include <stdbool.h>\n"
            "#include <stdint.h>\n"
            "#include <limits.h>\n"
            "#include <regex.h>\n"
            "#include <string.h>\n"
            "typedef char const *cstring_t;\n"
            + impl
            + '\nint main(void) {\n'
            '  assert(ORDR_RegexCount("aaa", "a") == 3);\n'
            '  assert(ORDR_RegexCount("aaa", "^a") == 1);\n'
            '  assert(ORDR_RegexCount("aaa", "a$") == 1);\n'
            '  assert(ORDR_RegexCount("abc", "$") == 1);\n'
            '  assert(ORDR_RegexCount("", "^$") == 1);\n'
            '  assert(ORDR_RegexCount("한국어", "[ㄱ-ㅣ가-힣]") == 3);\n'
            '  assert(ORDR_RegexCount("English", "[ㄱ-ㅣ가-힣]") == 0);\n'
            '  assert(ORDR_RegexCount("abcabc", "abc") == 2);\n'
            '  assert(ORDR_RegexCount("abc", "[") == 0);\n'
            '  return 0;\n}\n'
        )
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "test.c"
            exe = Path(tmp) / "test_native_regex"
            src.write_text(harness, encoding="utf-8")
            built = subprocess.run(
                [compiler, "-std=c11", "-Wall", "-Wextra", "-Werror", str(src), "-o", str(exe)],
                capture_output=True, text=True, timeout=30,
            )
            self.assertEqual(built.returncode, 0, built.stdout + built.stderr)
            executed = subprocess.run([str(exe)], capture_output=True, text=True, timeout=15)
            self.assertEqual(executed.returncode, 0, executed.stdout + executed.stderr)


if __name__ == "__main__":
    unittest.main()
