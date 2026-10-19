#!/usr/bin/env python3
"""Validate ORDR map-startup milestones in an on-device JASS diagnostic log.

This does not claim gameplay success: it requires evidence of both a fully
returned G_LoadMap and at least one completed game simulation JASS frame.
"""
import argparse
import pathlib
import re
import sys

EVENT = re.compile(r"^\\s*\\d+\\s+ms\\s+\\[\\s*([A-Z][A-Z0-9_ ]*)\\s*\\]\\s*(.*)$")
MILESTONES = (
    "FLOW_RETURN_MAIN",
    "MAP_LOAD_AFTER_JASS",
    "MAP_LOAD_AFTER_LOADING_FRAME",
    "MAP_LOAD_READY",
    "FRAME_AFTER_JASS",
)
ERROR_EVENTS = {
    "JASS_ERROR", "JASS_RUNTIME_ERROR", "NATIVE_ERROR",
    "UNIMPLEMENTED_NATIVE", "START_BLOCKED", "MAP_LOAD_FAILED",
}

def inspect(text):
    observed = []
    issues = []
    last_event = "(no tagged events)"
    for line in text.splitlines():
        match = EVENT.match(line)
        if not match:
            continue
        event, detail = match.groups()
        event = event.strip()
        last_event = event
        if event in ERROR_EVENTS:
            issues.append(event + ": " + detail[:160])
        if event == "FLOW_RETURN" and re.search(r"\\bfunction=main(?:\\s|$)", detail):
            observed.append("FLOW_RETURN_MAIN")
        elif event in MILESTONES:
            observed.append(event)

    seen = []
    for item in observed:
        if item not in seen:
            seen.append(item)
    cursor = 0
    out_of_order = []
    for stage in seen:
        target = MILESTONES.index(stage)
        if target < cursor:
            out_of_order.append(stage)
        else:
            cursor = target + 1
    missing = [s for s in MILESTONES if s not in seen]
    return seen, missing, issues, out_of_order, last_event

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=pathlib.Path)
    args = parser.parse_args()
    seen, missing, issues, disorder, last = inspect(
        args.log.read_text(encoding="utf-8", errors="replace")
    )
    print("ORDR startup milestones:", ", ".join(seen) or "(none)")
    print("Last recorded event:", last)
    if missing:
        print("Missing:", ", ".join(missing))
    if issues:
        print("Runtime errors:", "; ".join(issues))
    if disorder:
        print("Out of order:", ", ".join(disorder))
    if missing or issues or disorder:
        print("RESULT: INCOMPLETE / NOT VERIFIED")
        return 2
    print("RESULT: STARTUP AND FIRST JASS FRAME VERIFIED (not a gameplay test)")
    return 0

if __name__ == "__main__":
    sys.exit(main())
