#!/usr/bin/env python3
"""Distinguish a loaded renderer/client from a completed ORDR startup."""
import json
import pathlib
import subprocess
import sys
import tempfile
import unittest

TOOL = pathlib.Path(__file__).resolve().parents[1] / "tools/ordr/check_simulator_runtime.py"


class SimulatorRuntimeTests(unittest.TestCase):
    def inspect(self, jass, server, client, renderer):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            for name, contents in (("ordr-jass.log", jass), ("ordr-server.log", server),
                                   ("ordr-client.log", client), ("ordr-renderer.log", renderer)):
                (root / name).write_text(contents, encoding="utf-8")
            completed = subprocess.run([sys.executable, str(TOOL), str(root)],
                                       capture_output=True, text=True, check=False)
            self.assertEqual(completed.returncode, 0, completed.stderr)
            return completed.stdout

    def test_last_condition_is_not_completed_frame(self):
        output = self.inspect(
            "2000 ms [EVENT_BEFORE_RESUME] ordinal=2 function=BB4\n"
            "2000 ms [CUX_CONDITION_AFTER] authored trigger condition returned\n",
            "game_ms=800 frame=8 stage=SERVER_FRAME_COMPLETE\n",
            "wall_ms=262899 stage=CLIENT_ACTIVATED_LOADING_CLEARED\n"
            "wall_ms=283825 stage=CL_FRAME_BEFORE_DRAW\n"
            "wall_ms=304692 stage=CL_FRAME_AFTER_DRAW\n",
            "wall_since_start_ms=2 stage=R_REGISTER_MAP_ENTER\n"
            "wall_since_start_ms=27083 stage=R_REGISTER_MAP_COMPLETE\n")
        self.assertIn('"jass_status": "bb4_resume_unfinished"', output)
        self.assertIn('"client_activated": true', output)
        self.assertIn('"renderer_registered": true', output)
        self.assertIn('"last_completed_game_frame": 8', output)
        self.assertIn('"max_observed_client_draw_ms": 20867', output)
        self.assertIn('"map_registration_ms": 27081', output)

    def test_completed_bb4_wont_report_blocked(self):
        output = self.inspect(
            "2000 ms [EVENT_BEFORE_RESUME] ordinal=2 function=BB4\n"
            "2000 ms [CUX_CONDITION_AFTER] authored trigger condition returned\n"
            "2000 ms [EVENT_AFTER_RESUME] ordinal=2 function=BB4\n"
            "2100 ms [RUNTIME_ERROR] unrelated later callback\n",
            "game_ms=2100 frame=21 stage=SERVER_FRAME_COMPLETE\n", "", "")
        self.assertIn('"jass_status": "bb4_resume_completed"', output)

    def test_incomplete_map_does_not_claim_jass_deadlock(self):
        output = self.inspect("2000 ms [CUX_CONDITION_AFTER] returned\n", "", "", "")
        self.assertIn('"jass_status": "insufficient_evidence"', output)
        self.assertIn('"last_completed_game_frame": null', output)

    def test_late_choice_scene_is_distinct_from_early_server_frame_sample(self):
        output = self.inspect(
            "2000 ms [EVENT_BEFORE_RESUME] ordinal=2 function=BB4\n"
            "2000 ms [RUNTIME_ERROR] integer division by zero\n"
            "2000 ms [EVENT_AFTER_RESUME] ordinal=2 function=BB4\n"
            "2200 ms [ORDR_OFFLINE_CONFIRMED] map offline flag set\n"
            "4000 ms [CHOICE_CREATE] type=u000 created=1\n"
            "4000 ms [CHOICE_CREATE] type=u001 created=1\n"
            "4100 ms [CHOICE_CAMERA_APPLIED] camera_player=0\n"
            "5000 ms [CHOICE_MODEL_AUDIT] unit=u000 visible=1 selectable=1\n"
            "5000 ms [CHOICE_RENDER_AUDIT] buttons=2 modeled=2 visible=2\n"
            "6800 ms [EVENT_DRAIN_EXIT] visited=1\n",
            "game_ms=800 frame=8 stage=SERVER_FRAME_COMPLETE\n", "", "")
        report = json.loads(output)
        self.assertEqual(report["last_completed_game_frame"], 8)
        self.assertEqual(report["last_observed_jass_ms"], 6800)
        self.assertEqual(report["choice_units_created"], 2)
        self.assertEqual(report["choice_selectable_models"], 1)
        self.assertTrue(report["choice_scene_setup_logged"])
        self.assertTrue(report["offline_map_flag_confirmed"])
        self.assertEqual(report["jass_status"], "bb4_resume_aborted_runtime_error")
        self.assertFalse(report["gameplay_verified"])

    def test_choice_creation_without_camera_and_render_is_not_full_setup(self):
        report = json.loads(self.inspect(
            "4000 ms [CHOICE_CREATE] type=u000 created=1\n",
            "game_ms=800 frame=8 stage=SERVER_FRAME_COMPLETE\n", "", ""))
        self.assertEqual(report["choice_units_created"], 1)
        self.assertFalse(report["choice_scene_setup_logged"])
        self.assertFalse(report["offline_map_flag_confirmed"])

    def test_cv5_condition_and_arithmetic_error_are_reported_separately(self):
        output = self.inspect(
            "2000 ms [EVENT_BEFORE_RESUME] ordinal=2 function=BB4\n"
            "2000 ms [CONDITION_ENTER ] index=2 function=CUl\n"
            "2000 ms [CONDITION_RESULT] index=2 function=CUl returns=0\n"
            "2000 ms [CONDITION_ENTER ] index=3 function=CV5\n"
            "2000 ms [DIVIDE_SOURCE ] numerator=360 denominator=GetPlayerState(Player(0),11)\n"
            "2000 ms [ARITHMETIC_ERROR] left=18 right=0\n"
            "2000 ms [RUNTIME_ERROR] integer division by zero\n"
            "2000 ms [EVENT_AFTER_RESUME] ordinal=2 function=BB4\n",
            "game_ms=800 frame=8 stage=SERVER_FRAME_COMPLETE\n", "", "")
        report = json.loads(output)
        self.assertEqual(report["jass_status"], "bb4_resume_aborted_runtime_error")
        self.assertEqual(report["last_condition_entered"], "CV5")
        self.assertEqual(report["last_condition_completed"], "CUl")
        self.assertEqual(report["last_jass_runtime_error"], "integer division by zero")
        self.assertEqual(report["last_division_source"], "numerator=360 denominator=GetPlayerState(Player(0),11)")
        self.assertFalse(report["gameplay_verified"])


if __name__ == "__main__":
    unittest.main()
