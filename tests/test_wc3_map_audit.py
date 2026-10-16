"""Tests for the bounded Warcraft III campaign-map audit."""

from __future__ import annotations

import importlib.util
import contextlib
import io
import json
import os
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DOTA_LOOSE = ROOT / "data/Warcraft III/Maps/DotA v6.83dAI PMV 1.42 EN.w3x"
sys.dont_write_bytecode = True  # exec_module below must not leave tools/__pycache__ behind
SPEC = importlib.util.spec_from_file_location("wc3_map_audit", ROOT / "tools/wc3_map_audit.py")
AUDIT = importlib.util.module_from_spec(SPEC)
assert SPEC.loader
SPEC.loader.exec_module(AUDIT)


def cstring(text: str) -> bytes:
    return text.encode() + b"\0"


def w3i_fixture(name: str, title: str, subtitle: str) -> bytes:
    data = struct.pack("<III", 18, 0, 0)
    data += b"".join(cstring(text) for text in (name, "author", "description", "players"))
    data += bytes(48 + 8 + 4 + 1 + 4)
    data += b"".join(cstring(text) for text in ("loading text", title, subtitle))
    return data


class MapMetadataTest(unittest.TestCase):
    def test_wts_zero_id_and_loading_title_are_resolved(self):
        wts = "\ufeffSTRING 0\n{\nChapter Five\n}\nSTRING 1\n{\nMarch of the Scourge\n}\n"
        name = AUDIT.parse_w3i_name(w3i_fixture("Human05", "TRIGSTR_000", "TRIGSTR_001"), wts, "fallback")
        self.assertEqual(name, "Chapter Five — March of the Scourge")

    def test_map_name_is_used_without_loading_titles(self):
        self.assertEqual(AUDIT.parse_w3i_name(w3i_fixture("Human05", "", ""), "", "fallback"), "Human05")


class DiagnosticTest(unittest.TestCase):
    def test_repeated_entity_errors_are_compacted(self):
        output = """
SLK: failed to load 'UI\\SoundInfo\\Music.slk'
WC3 CreepSleep: ACsp TargetArt missing; using canonical sleep art for unit 2
WC3 CreepSleep: ACsp TargetArt missing; using canonical sleep art for unit 9
SV_FindIndex: pool full start=32 max=256 name=a.mdx
SV_FindIndex: pool full start=32 max=256 name=b.mdx
JASS runtime error: unimplemented native: EnumItemsInRect
JASS runtime error: unimplemented native: EnumItemsInRect
"""
        errors, families = AUDIT.compact_diagnostics(output, "completed", 0, False)
        self.assertIn("CREEP_SLEEP_ART ×2", errors)
        self.assertIn("MODEL_POOL_FULL ×2 (2 resources)", errors)
        self.assertIn("JASS `EnumItemsInRect` ×2", errors)
        self.assertIn("JASS:EnumItemsInRect", families)

    def test_serially_reproduced_signal_is_reported(self):
        errors, families = AUDIT.compact_diagnostics("", "crashed", -11, True)
        self.assertEqual(errors, ["SIGSEGV (exit -11; reproduced serially)"])
        self.assertEqual(families, {"SIGSEGV"})

    def test_ordinary_nonzero_exit_is_not_called_sigsegv(self):
        errors, families = AUDIT.compact_diagnostics("", "crashed", 2, False)
        self.assertEqual(errors, ["PROCESS_EXIT (code 2)"])
        self.assertEqual(families, {"PROCESS_EXIT"})


class ReportTest(unittest.TestCase):
    def test_report_names_file_and_does_not_claim_completion(self):
        report = {
            "commit": "abc123",
            "frames": 600,
            "simulated_seconds": 60,
            "timeout_seconds": 120,
            "jobs": 4,
            "wall_seconds": 1.5,
            "maps": [{
                "edition": "RoC",
                "name": "Chapter Five — March of the Scourge",
                "filename": "Human05.w3m",
                "status": "completed",
                "compact_errors": ["JASS `SetCaptainHome` ×2"],
                "families": ["JASS:SetCaptainHome"],
            }],
        }
        markdown = AUDIT.render_markdown(report)
        self.assertIn("March of the Scourge", markdown)
        self.assertIn("`Human05.w3m`", markdown)
        self.assertIn("600 frames", markdown)
        self.assertNotIn("map works", markdown.lower())


class AuditExitTest(unittest.TestCase):
    def run_audit(self, diagnostic, exit_code, strict):
        """Drive the real CLI/process/report path with a tiny diagnostic executable."""
        with tempfile.TemporaryDirectory() as tmp:
            data = Path(tmp)
            target = data / "Maps" / "Custom.w3x"
            target.parent.mkdir()
            target.write_bytes(b"fixture")
            binary = data / "engine"
            binary.write_text("#!/bin/sh\nprintf '%s\\n' '" + diagnostic + "'\nexit " + str(exit_code) + "\n")
            binary.chmod(0o755)
            mpqtool = data / "mpqtool"
            (data / "war3map.w3i").write_bytes(w3i_fixture("Custom", "", ""))
            mpqtool.write_text(
                "#!/usr/bin/env python3\nimport sys\nfrom pathlib import Path\n"
                "if sys.argv[-1] == 'war3map.w3i':\n"
                "    sys.stdout.buffer.write(Path(__file__).with_name('war3map.w3i').read_bytes())\n")
            mpqtool.chmod(0o755)
            output = data / "report"
            argv = ["wc3_map_audit", "--data", str(data), "--binary", str(binary),
                    "--mpqtool", str(mpqtool), "--loose-map", str(target),
                    "--output-dir", str(output), "--jobs", "1", "--frames", "1"]
            if strict:
                argv.append("--fail-on-errors")
            with patch.object(sys, "argv", argv), contextlib.redirect_stdout(io.StringIO()):
                result = AUDIT.main()
            report = json.loads((output / "report.json").read_text())
            self.assertTrue((output / "report.md").is_file())
            self.assertTrue(Path(report["maps"][0]["log"]).is_file())
            return result, report["maps"][0]

    def test_exit_zero_native_error_fails_strict_audit_and_preserves_reports(self):
        diagnostic = "JASS runtime error: unknown function: DzGetGameUI"
        result, item = self.run_audit(diagnostic, 0, True)
        self.assertEqual(result, 1)
        self.assertEqual(item["status"], "completed")
        self.assertTrue(any("DzGetGameUI" in error for error in item["compact_errors"]))
        self.assertEqual(self.run_audit(diagnostic, 0, False)[0], 0)

    def test_clean_bounded_run_passes_strict_audit(self):
        result, item = self.run_audit("Server initialized.", 0, True)
        self.assertEqual(result, 0)
        self.assertEqual(item["compact_errors"], [])

    def test_process_failure_fails_strict_audit(self):
        result, item = self.run_audit("", 3, True)
        self.assertEqual(result, 1)
        self.assertEqual(item["status"], "crashed")


class LooseMapTest(unittest.TestCase):
    def test_w3x_under_data_is_tft_with_relative_map_path(self):
        with tempfile.TemporaryDirectory() as tmp:
            data = Path(tmp) / "Warcraft III"
            target = data / "Maps" / "Fake DotA.w3x"
            target.parent.mkdir(parents=True)
            target.write_bytes(b"fake")
            item = AUDIT.loose_map_spec(target, data)
            self.assertEqual(item["edition"], "TFT")
            self.assertEqual(item["filename"], "Fake DotA.w3x")
            self.assertEqual(item["path"], "Maps/Fake DotA.w3x")
            self.assertEqual(item["archive"], str(target.resolve()))
            self.assertEqual(item["loose"], "1")

    def test_w3m_under_data_is_roc(self):
        with tempfile.TemporaryDirectory() as tmp:
            data = Path(tmp) / "Warcraft III"
            target = data / "Maps" / "Custom.w3m"
            target.parent.mkdir(parents=True)
            target.write_bytes(b"fake")
            item = AUDIT.loose_map_spec(target, data)
            self.assertEqual(item["edition"], "RoC")
            self.assertEqual(item["path"], "Maps/Custom.w3m")

    def test_relative_path_resolves_from_cwd(self):
        with tempfile.TemporaryDirectory() as tmp:
            data = Path(tmp) / "Warcraft III"
            target = data / "Maps" / "Rel.w3x"
            target.parent.mkdir(parents=True)
            target.write_bytes(b"fake")
            old = os.getcwd()
            try:
                os.chdir(tmp)
                item = AUDIT.loose_map_spec(Path("Warcraft III/Maps/Rel.w3x"), data)
            finally:
                os.chdir(old)
            self.assertEqual(item["path"], "Maps/Rel.w3x")

    def test_missing_and_outside_data_are_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            data = Path(tmp) / "data"
            data.mkdir()
            with self.assertRaisesRegex(RuntimeError, "loose map not found"):
                AUDIT.loose_map_spec(data / "Maps" / "Missing.w3x", data)
            outside = Path(tmp) / "elsewhere.w3x"
            outside.write_bytes(b"fake")
            with self.assertRaisesRegex(RuntimeError, "must live under data dir"):
                AUDIT.loose_map_spec(outside, data)
            bad = data / "Maps" / "note.txt"
            bad.parent.mkdir(parents=True)
            bad.write_text("nope")
            with self.assertRaisesRegex(RuntimeError, r"must be \.w3m or \.w3x"):
                AUDIT.loose_map_spec(bad, data)

    def test_parse_args_accepts_loose_map(self):
        args = AUDIT.parse_args([
            "--loose-map", "data/Warcraft III/Maps/Fake.w3x",
            "--loose-map", "data/Warcraft III/Maps/Other.w3m",
            "--frames", "10", "--jobs", "1", "--timeout", "60",
        ])
        self.assertEqual(args.loose_map, [
            "data/Warcraft III/Maps/Fake.w3x",
            "data/Warcraft III/Maps/Other.w3m",
        ])
        self.assertEqual(args.frames, 10)

    def test_filter_maps_matches_loose_path(self):
        maps = [{
            "filename": "DotA v6.83dAI PMV 1.42 EN.w3x",
            "path": "Maps/DotA v6.83dAI PMV 1.42 EN.w3x",
            "edition": "TFT",
        }]
        self.assertEqual(len(AUDIT.filter_maps(maps, "*DotA*")), 1)
        self.assertEqual(len(AUDIT.filter_maps(maps, "Maps/*")), 1)
        self.assertEqual(AUDIT.filter_maps(maps, "Human05.w3m"), [])


@unittest.skipUnless(DOTA_LOOSE.is_file(), "DotA map not installed under data/Warcraft III/Maps")
class DotALooseMapOptionalTest(unittest.TestCase):
    def test_installed_dota_resolves_under_data(self):
        data = ROOT / "data/Warcraft III"
        item = AUDIT.loose_map_spec(DOTA_LOOSE, data)
        self.assertEqual(item["edition"], "TFT")
        self.assertEqual(item["path"], "Maps/DotA v6.83dAI PMV 1.42 EN.w3x")
        self.assertEqual(item["loose"], "1")


if __name__ == "__main__":
    unittest.main()
