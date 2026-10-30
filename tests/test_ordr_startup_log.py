#!/usr/bin/env python3
"""Regression tests for the ORDR log milestone verifier."""
import pathlib
import subprocess
import sys
import tempfile
import unittest

TOOL = pathlib.Path(__file__).resolve().parents[1] / "tools" / "ordr" / "check_startup_log.py"

def log(*entries):
    return "".join(f"0 ms  [{event:<20}]  {detail}\n" for event, detail in entries)

class StartupCheckTests(unittest.TestCase):
    def verify(self, content, expected):
        with tempfile.TemporaryDirectory() as directory:
            filename = pathlib.Path(directory) / "ordr-jass.log"
            filename.write_text(content, encoding="utf-8")
            process = subprocess.run(
                [sys.executable, str(TOOL), str(filename)],
                capture_output=True, text=True, check=False
            )
            self.assertEqual(process.returncode, expected, process.stdout + process.stderr)
            return process.stdout

    def test_complete_sequence(self):
        txt = log(
            ("FLOW_ENTER", "function=main via=coroutine"),
            ("FLOW_RETURN", "function=main hit=1 calls=19871"),
            ("MAP_LOAD_AFTER_JASS", "G_StartScripts returned"),
            ("MAP_LOAD_AFTER_LOADING_FRAME", "loading frame returned"),
            ("MAP_LOAD_READY", "G_LoadMap completed"),
            ("FRAME_AFTER_JASS", "frame=1 time_ms=20"),
        )
        self.assertIn("VERIFIED", self.verify(txt, 0))

    def test_realistic_main_only_is_not_success(self):
        txt = log(
            ("FLOW_ENTER", "function=main via=coroutine"),
            ("FLOW_RETURN", "function=main hit=1 calls=19871"),
            ("RESUME_EXIT", "steps=15306 yielded=0 done=1"),
        )
        output = self.verify(txt, 2)
        self.assertIn("MAP_LOAD_AFTER_JASS", output)
        self.assertIn("NOT VERIFIED", output)

    def test_loading_frame_incomplete(self):
        txt = log(
            ("FLOW_RETURN", "function=main hit=1"),
            ("MAP_LOAD_AFTER_JASS", "returned"),
        )
        self.assertIn("MAP_LOAD_READY", self.verify(txt, 2))

    def test_runtime_error_not_ignored(self):
        txt = log(
            ("FLOW_RETURN", "function=main hit=1"),
            ("MAP_LOAD_AFTER_JASS", "returned"),
            ("MAP_LOAD_AFTER_LOADING_FRAME", "returned"),
            ("MAP_LOAD_READY", "ready"),
            ("FRAME_AFTER_JASS", "frame=1"),
            ("JASS_RUNTIME_ERROR", "unimplemented native"),
        )
        self.assertIn("Runtime errors", self.verify(txt, 2))

    def test_out_of_order_is_not_success(self):
        txt = log(
            ("FLOW_RETURN", "function=main hit=1"),
            ("MAP_LOAD_READY", "ready"),
            ("MAP_LOAD_AFTER_JASS", "late event"),
            ("MAP_LOAD_AFTER_LOADING_FRAME", "late event"),
            ("FRAME_AFTER_JASS", "frame=1"),
        )
        self.assertIn("Out of order", self.verify(txt, 2))

if __name__ == "__main__":
    unittest.main()
