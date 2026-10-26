"""Only safe, current-session crash metadata should enter simulator artifacts."""
import json
import os
import pathlib
import subprocess
import sys
import tempfile
import unittest

TOOL = pathlib.Path(__file__).resolve().parents[1] / "crash_report_summary.py"


class CrashReportTests(unittest.TestCase):
    def test_json_ips_classifies_exception_and_ignores_old_report(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            body = '{"exception":{"type":"EXC_BAD_ACCESS","signal":"SIGSEGV"},' \
                   '"termination":{"namespace":"SIGNAL","code":11},' \
                   '"privateField":"must never be exported"}'
            contents = '{"app_name":"Warcraft3","bundleID":"com.example.warcraft3","bug_type":"309"}\n' + body
            newer = root / "Warcraft3-2026.ips"
            older = root / "Warcraft3-2025.ips"
            newer.write_text(contents, encoding="utf-8")
            older.write_text(contents, encoding="utf-8")
            os.utime(newer, (4000, 4000))
            os.utime(older, (1000, 1000))
            result = subprocess.run([sys.executable, str(TOOL), "--root", str(root),
                                     "--bundle-id", "com.example.warcraft3", "--launched-at", "2000"],
                                    capture_output=True, text=True, check=False)
            self.assertEqual(result.returncode, 0, result.stderr)
            summary = json.loads(result.stdout)
            self.assertEqual(summary["matching_report_count"], 1)
            self.assertEqual(summary["signature_counts"], {"EXC_BAD_ACCESS": 1, "SIGSEGV": 1})
            self.assertEqual(summary["termination_namespaces"], {"SIGNAL": 1})
            self.assertEqual(summary["bug_types"], {"309": 1})
            self.assertNotIn("privateField", result.stdout)

    def test_unknown_or_malformed_report_has_no_guessed_signature(self):
        with tempfile.TemporaryDirectory() as directory:
            file = pathlib.Path(directory) / "Warcraft3-2026.ips"
            file.write_text('{"bundleID":"com.example.warcraft3"}\nnot valid json', encoding="utf-8")
            result = subprocess.run([sys.executable, str(TOOL), "--root", directory,
                                     "--bundle-id", "com.example.warcraft3", "--launched-at", "0"],
                                    capture_output=True, text=True, check=False)
            self.assertEqual(result.returncode, 0, result.stderr)
            summary = json.loads(result.stdout)
            self.assertEqual(summary["matching_report_count"], 1)
            self.assertEqual(summary["signature_counts"], {})


if __name__ == "__main__":
    unittest.main()
