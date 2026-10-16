"""Regression gates for ORDR JASS parsing; no proprietary map is committed."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
JASS = ROOT / "build/bin/jass"


class ORDRParserTests(unittest.TestCase):
    def test_escaped_quotes_and_inline_slashes(self):
        if not JASS.exists():
            self.skipTest("Build the standalone interpreter with make jass-tool")
        script = (
            'globals\n'
            '  string message = "한글 \\"인용\\" // 실제 주석 아님"\n'
            'endglobals\n'
            'function main takes nothing returns nothing\n'
            'endfunction\n'
        )
        with tempfile.TemporaryDirectory() as folder:
            filename = Path(folder) / "quotes.j"
            filename.write_text(script, encoding="utf-8")
            result = subprocess.run([str(JASS), str(filename)],
                                    capture_output=True, text=True, timeout=15)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    @unittest.skipUnless(os.environ.get("ORDR_JASS_PATH"),
                         "Private map test: set ORDR_JASS_PATH to extracted war3map.j")
    def test_private_map_parse(self):
        if not JASS.exists():
            self.skipTest("Build make jass-tool first")
        result = subprocess.run([str(JASS), os.environ["ORDR_JASS_PATH"]],
                                capture_output=True, text=True, timeout=60)
        self.assertNotIn("Parser Error", result.stderr)
        self.assertNotIn("error parsing function", result.stderr)
        # The standalone CLI deliberately provides no game native host;
        # reaching InitHashtable is therefore not gameplay validation.
        self.assertIn("unknown function: InitHashtable", result.stderr)


if __name__ == "__main__":
    unittest.main()
