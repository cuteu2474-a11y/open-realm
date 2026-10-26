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

    def test_zero_divisor_records_numeric_globals_without_claiming_gameplay(self):
        report = json.loads(self.inspect(
            "2000 ms [DIVIDE_SOURCE   ] numerator=360 denominator=__mul(vP,EK)\n"
            "2000 ms [ARITHMETIC_ERROR] left=360 right=0\n"
            "2000 ms [DIVISOR_GLOBAL  ] name=vP scope=global type=integer value=0\n"
            "2000 ms [DIVISOR_GLOBAL  ] name=EK scope=global type=integer value=8\n",
            "", "", ""))
        self.assertEqual(report["recent_divisor_global_states"], [
            "name=vP scope=global type=integer value=0",
            "name=EK scope=global type=integer value=8",
        ])
        self.assertFalse(report["gameplay_verified"])

    def test_simulated_network_choice_needs_both_server_traces(self):
        report = json.loads(self.inspect(
            "5500 ms [CHOICE_SELECT_COMMAND] unit=2148 raw=0x75303035 player=0\n"
            "5500 ms [CHOICE_USER_SELECTION] player=0 unit=2148 type=7 rawcode=0x75303035 queued=1\n",
            "game_ms=5500 frame=55 stage=SERVER_FRAME_COMPLETE\n",
            "wall_ms=630000 stage=SIM_CHOICE_SELECT_QUEUED unit=2148 raw=u005 step=1\n", ""))
        self.assertTrue(report["simulator_choice_probe_queued"])
        self.assertTrue(report["choice_select_command_seen"])
        self.assertTrue(report["choice_user_selection_event_seen"])
        self.assertTrue(report["simulator_choice_network_roundtrip_verified"])
        self.assertFalse(report["physical_touch_verified"])
        self.assertFalse(report["gameplay_verified"])

    def test_probe_does_not_pass_without_server_selection_event(self):
        report = json.loads(self.inspect(
            "5500 ms [CHOICE_SELECT_COMMAND] unit=2148\n", "",
            "wall_ms=630000 stage=SIM_CHOICE_SELECT_QUEUED unit=2148 raw=u005 step=1\n", ""))
        self.assertTrue(report["simulator_choice_probe_queued"])
        self.assertFalse(report["choice_user_selection_event_seen"])
        self.assertFalse(report["simulator_choice_network_roundtrip_verified"])

    def test_no_probe_does_not_pass_with_unrelated_human_selection(self):
        report = json.loads(self.inspect(
            "5500 ms [CHOICE_SELECT_COMMAND] unit=2143\n"
            "5500 ms [CHOICE_USER_SELECTION] unit=2143\n", "", "", ""))
        self.assertFalse(report["simulator_choice_network_roundtrip_verified"])

    def test_wrong_selected_unit_never_counts_as_probe_success(self):
        report = json.loads(self.inspect(
            "5500 ms [CHOICE_SELECT_COMMAND] unit=2143\n"
            "5500 ms [CHOICE_USER_SELECTION] unit=2143 queued=1\n",
            "", "wall_ms=630000 stage=SIM_CHOICE_SELECT_QUEUED unit=2148 raw=u005 step=1\n", ""))
        self.assertTrue(report["simulator_choice_probe_queued"])
        self.assertFalse(report["simulator_choice_network_roundtrip_verified"])
        self.assertEqual(report["simulator_choice_verified"], [])

    def test_staged_nightmare_isekai_probe_requires_both_confirmations(self):
        report = json.loads(self.inspect(
            "10000 ms [CHOICE_SELECT_COMMAND] unit=2148\n"
            "10000 ms [CHOICE_USER_SELECTION] player=0 unit=2148 queued=1\n"
            "12000 ms [CHOICE_SELECT_COMMAND] unit=2150\n",
            "", "wall_ms=540000 stage=SIM_CHOICE_SELECT_QUEUED unit=2148 raw=u005 step=1\n"
                "wall_ms=630000 stage=SIM_CHOICE_SELECT_QUEUED unit=2150 raw=u00C step=2\n", ""))
        self.assertEqual(report["simulator_choice_requested"], ["u005", "u00C"])
        self.assertEqual(report["simulator_choice_verified"], ["u005"])
        self.assertFalse(report["simulator_choice_network_roundtrip_verified"])

    def test_divisor_local_values_are_distinct_from_missing_globals(self):
        report = json.loads(self.inspect(
            "2000 ms [DIVISOR_GLOBAL ] name=vP scope=global state=missing\n"
            "2000 ms [DIVISOR_VALUE  ] name=vP scope=local type=integer value=0\n"
            "2000 ms [DIVISOR_VALUE  ] name=EK scope=local type=integer value=8\n",
            "", "", ""))
        self.assertEqual(report["recent_divisor_values"], [
            "name=vP scope=local type=integer value=0",
            "name=EK scope=local type=integer value=8",
        ])
        self.assertFalse(report["gameplay_verified"])

    def test_three_simulated_finger_taps_require_matching_server_events(self):
        report = json.loads(self.inspect(
            "5100 ms [CHOICE_SELECT_COMMAND] unit=2148 raw=0x75303035\n"
            "5100 ms [CHOICE_USER_SELECTION] player=0 unit=2148 queued=1\n"
            "6100 ms [CHOICE_SELECT_COMMAND] unit=2150 raw=0x75303043\n"
            "6100 ms [CHOICE_USER_SELECTION] player=0 unit=2150 queued=1\n"
            "7100 ms [CHOICE_SELECT_COMMAND] unit=2151 raw=0x75303036\n"
            "7100 ms [CHOICE_USER_SELECTION] player=0 unit=2151 queued=1\n"
            "7100 ms [FLOW_ENTER] function=CUK via=synchronous hit=1\n"
            "7100 ms [FLOW_ENTER] function=CUL via=synchronous hit=1\n"
            "7100 ms [FLOW_ENTER] function=CUM via=synchronous hit=1\n",
            "",
            "wall_ms=620000 stage=SIM_CHOICE_SELECT_QUEUED unit=2148 raw=u005 step=1 input=finger\n"
            "wall_ms=710000 stage=SIM_CHOICE_SELECT_QUEUED unit=2150 raw=u00C step=2 input=finger\n"
            "wall_ms=810000 stage=SIM_CHOICE_SELECT_QUEUED unit=2151 raw=u006 step=3 input=finger\n",
            ""))
        self.assertTrue(report["simulator_choice_full_three_tap_roundtrip_verified"])
        self.assertTrue(report["simulator_choice_network_roundtrip_verified"])
        self.assertEqual(report["simulator_choice_requested"], ["u005", "u00C", "u006"])
        self.assertEqual(report["simulator_choice_verified"], ["u005", "u00C", "u006"])
        self.assertEqual(report["simulator_choice_input_type"], "synthetic_sdl_finger")
        self.assertTrue(report["game_start_jass_called"])
        self.assertFalse(report["physical_touch_verified"])
        self.assertFalse(report["gameplay_verified"])

    def test_three_tap_check_fails_if_final_start_is_not_acknowledged(self):
        report = json.loads(self.inspect(
            "5100 ms [CHOICE_SELECT_COMMAND] unit=2148\n"
            "5100 ms [CHOICE_USER_SELECTION] unit=2148 queued=1\n"
            "6100 ms [CHOICE_SELECT_COMMAND] unit=2150\n"
            "6100 ms [CHOICE_USER_SELECTION] unit=2150 queued=1\n",
            "",
            "stage=SIM_CHOICE_SELECT_QUEUED unit=2148 raw=u005 step=1 input=finger\n"
            "stage=SIM_CHOICE_SELECT_QUEUED unit=2150 raw=u00C step=2 input=finger\n"
            "stage=SIM_CHOICE_SELECT_QUEUED unit=2151 raw=u006 step=3 input=finger\n",
            ""))
        self.assertFalse(report["simulator_choice_full_three_tap_roundtrip_verified"])
        self.assertFalse(report["game_start_jass_called"])
        self.assertEqual(report["simulator_choice_verified"], ["u005", "u00C"])

    def test_missing_original_background_is_reported_without_fake_art(self):
        report = json.loads(self.inspect(
            "5000 ms [CHOICE_BACKGROUND_AUDIT] B100_instances=0 visible=0 model_indices=0\n",
            "", "stage=SIM_CHOICE_TAP_BLOCKED raw=u005 step=1\n", ""))
        self.assertEqual(report["choice_background_instance_count"], 0)
        self.assertEqual(report["choice_background_visible_count"], 0)
        self.assertEqual(report["choice_background_model_count"], 0)
        self.assertEqual(report["simulator_choice_tap_blocked"], ["u005"])
        self.assertFalse(report["simulator_choice_network_roundtrip_verified"])

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
