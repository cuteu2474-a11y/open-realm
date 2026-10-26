#!/usr/bin/env python3
"""Summarize the last known engine stage; never infer playable gameplay from CI."""
import json
import pathlib
import re
import sys


def inspect(report):
    def read(name):
        path = report / name
        return path.read_text(encoding="utf-8", errors="replace") if path.exists() else ""

    jass, server = read("ordr-jass.log"), read("ordr-server.log")
    client, renderer = read("ordr-client.log"), read("ordr-renderer.log")
    frames = [int(n) for n in re.findall(r"frame=(\d+) stage=SERVER_FRAME_COMPLETE", server)]
    draw_start = None
    draw_max = None
    for line in client.splitlines():
        stamp = re.search(r"wall_ms=(\d+)", line)
        if stamp and "stage=CL_FRAME_BEFORE_DRAW" in line:
            draw_start = int(stamp.group(1))
        elif stamp and "stage=CL_FRAME_AFTER_DRAW" in line and draw_start is not None:
            elapsed = int(stamp.group(1)) - draw_start
            if elapsed >= 0:
                draw_max = max(draw_max or 0, elapsed)
            draw_start = None
    map_times = {}
    for stage in ("R_REGISTER_MAP_ENTER", "R_REGISTER_MAP_COMPLETE"):
        match = re.search(r"wall_since_start_ms=(\d+) stage=" + stage + r"(?:\s|$)", renderer, re.M)
        if match:
            map_times[stage] = int(match.group(1))
    map_elapsed = None
    if len(map_times) == 2:
        map_elapsed = map_times["R_REGISTER_MAP_COMPLETE"] - map_times["R_REGISTER_MAP_ENTER"]
    entered = re.search(r"\[EVENT_BEFORE_RESUME\].*ordinal=\d+.*function=BB4", jass)
    finished = re.search(r"\[EVENT_AFTER_RESUME\].*ordinal=\d+.*function=BB4", jass)
    condition_entered = re.findall(r"\[CONDITION_ENTER\s*\]\s+index=\d+\s+function=(\S+)", jass)
    condition_completed = re.findall(r"\[CONDITION_RESULT\s*\]\s+index=\d+\s+function=(\S+)", jass)
    runtime_errors = re.findall(r"\[RUNTIME_ERROR\s*\]\s+([^\r\n]+)", jass)
    division_sources = re.findall(r"\[DIVIDE_SOURCE\s*\]\s+([^\r\n]+)", jass)
    divisor_globals = re.findall(r"\[DIVISOR_GLOBAL\s*\]\s+([^\r\n]+)", jass)
    # The first few server-frame checkpoints are a diagnostic sample, not proof
    # that the server stopped running after that frame.
    jass_times = [int(n) for n in re.findall(r"^(\d+) ms\s+\[", jass, re.M)]
    choice_creates = re.findall(r"\[CHOICE_CREATE\s*\][^\r\n]*\bcreated=1\b", jass)
    choice_selectables = re.findall(r"\[CHOICE_MODEL_AUDIT\s*\][^\r\n]*\bselectable=1\b", jass)
    choice_render = re.findall(r"\[CHOICE_RENDER_AUDIT\s*\][^\r\n]*", jass)
    choice_camera = "CHOICE_CAMERA_APPLIED" in jass
    if entered and not finished and "CUX_CONDITION_AFTER" in jass:
        status = "bb4_resume_unfinished"
    elif entered and finished:
        # A VM error marks the coroutine done, but its JASS condition/action never completed.
        between = jass[entered.end():finished.start()]
        status = "bb4_resume_aborted_runtime_error" if "[RUNTIME_ERROR" in between else "bb4_resume_completed"
    else:
        status = "insufficient_evidence"
    return {
        "jass_status": status,
        "last_condition_entered": condition_entered[-1] if condition_entered else None,
        "last_condition_completed": condition_completed[-1] if condition_completed else None,
        "last_jass_runtime_error": runtime_errors[-1][:128] if runtime_errors else None,
        "last_division_source": division_sources[-1][:360] if division_sources else None,
        "recent_divisor_global_states": [entry[:160] for entry in divisor_globals[-4:]],
        "last_completed_game_frame": max(frames) if frames else None,
        "last_observed_jass_ms": max(jass_times) if jass_times else None,
        "choice_units_created": len(choice_creates),
        "choice_selectable_models": len(choice_selectables),
        "choice_camera_applied": choice_camera,
        "choice_render_audit_present": bool(choice_render),
        "choice_scene_setup_logged": bool(choice_creates and choice_camera and choice_render),
        "offline_map_flag_confirmed": "ORDR_OFFLINE_CONFIRMED" in jass,
        "simulator_choice_probe_queued": "SIM_CHOICE_SELECT_QUEUED" in client,
        "choice_select_command_seen": "CHOICE_SELECT_COMMAND" in jass,
        "choice_user_selection_event_seen": "CHOICE_USER_SELECTION" in jass,
        "simulator_choice_network_roundtrip_verified":
            "SIM_CHOICE_SELECT_QUEUED" in client and
            "CHOICE_SELECT_COMMAND" in jass and
            "CHOICE_USER_SELECTION" in jass,
        "physical_touch_verified": False,
        "client_activated": "CLIENT_ACTIVATED_LOADING_CLEARED" in client,
        "renderer_registered": "R_REGISTER_MAP_COMPLETE" in renderer,
        "map_registration_ms": map_elapsed,
        "max_observed_client_draw_ms": draw_max,
        "gameplay_verified": False,
    }


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: check_simulator_runtime.py <simulator-report-directory>")
    print(json.dumps(inspect(pathlib.Path(sys.argv[1])), indent=2, ensure_ascii=False))


if __name__ == "__main__":
    main()
