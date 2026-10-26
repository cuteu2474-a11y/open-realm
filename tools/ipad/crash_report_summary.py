#!/usr/bin/env python3
"""Collect bounded crash classifications without publishing .ips or device logs."""
import argparse
import json
import pathlib
import re


def safe_token(value, pattern):
    return value if isinstance(value, str) and re.fullmatch(pattern, value) else None


def summarize(roots, bundle_id, launched_at):
    counts = {"signature_counts": {}, "termination_namespaces": {}, "bug_types": {}}
    seen = set()
    matches = 0
    for root in roots:
        if not root.exists():
            continue
        paths = root.glob("*.ips") if root.name != "Devices" else root.glob(
            "*/data/Library/Logs/CrashReporter/*.ips")
        for path in paths:
            if path in seen or path.stat().st_mtime < launched_at:
                continue
            seen.add(path)
            if "warcraft" not in path.name.lower():
                continue
            try:
                contents = path.read_text(encoding="utf-8", errors="replace")
            except OSError:
                continue
            if bundle_id not in contents and "Warcraft3" not in contents:
                continue
            matches += 1
            header, _, body = contents.partition("\n")
            try:
                metadata = json.loads(header)
            except (ValueError, TypeError):
                metadata = {}
            try:
                detail = json.loads(body)
            except (ValueError, TypeError):
                detail = {}
            if not isinstance(metadata, dict):
                metadata = {}
            if not isinstance(detail, dict):
                detail = {}
            exception = detail.get("exception", {})
            termination = detail.get("termination", {})
            if not isinstance(exception, dict):
                exception = {}
            if not isinstance(termination, dict):
                termination = {}
            fields = (
                ("signature_counts", safe_token(exception.get("type"), r"EXC_[A-Z0-9_]{1,40}")),
                ("signature_counts", safe_token(exception.get("signal"), r"SIG[A-Z0-9_]{1,40}")),
                ("termination_namespaces", safe_token(termination.get("namespace"), r"[A-Z0-9_]{1,40}")),
                ("bug_types", safe_token(str(metadata.get("bug_type", "")), r"\d{1,4}")),
            )
            for key, value in fields:
                if value:
                    counts[key][value] = counts[key].get(value, 0) + 1
    return {"matching_report_count": matches, **counts, "reports_exported": False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", action="append", type=pathlib.Path)
    parser.add_argument("--bundle-id", required=True)
    parser.add_argument("--launched-at", type=int, required=True, help="Unix epoch seconds")
    args = parser.parse_args()
    home = pathlib.Path.home()
    roots = args.root or [home / "Library/Logs/DiagnosticReports",
                          home / "Library/Developer/CoreSimulator/Devices"]
    print(json.dumps(summarize(roots, args.bundle_id, args.launched_at), indent=2))


if __name__ == "__main__":
    main()
