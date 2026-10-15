#!/usr/bin/env python3
"""Offline ORDR extension coverage audit. Does not require proprietary map assets."""
import argparse
import json
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parents[1]
REGISTRY = ROOT / "games/warcraft-3/game/api/api_module.c"
PREFIXES = ("Dz", "JN", "EX")

def registry_names(source):
    source = re.sub(r"/\\*.*?\\*/|//[^\\n]*", "", source, flags=re.S)
    return set(re.findall(r'\{\s*"([A-Za-z][A-Za-z0-9_]*)"\s*,', source))

def audit(usage, registry):
    calls = usage["native_calls"]
    required = {name: count for name, count in calls.items()
                if name.startswith(PREFIXES) and count > 0}
    missing = {name: count for name, count in required.items()
               if name not in registry}
    return {
        "map": usage.get("map"),
        "method": "static name coverage only; not a runtime compatibility guarantee",
        "required_count": len(required),
        "registered_count": len(required) - len(missing),
        "missing_count": len(missing),
        "missing_by_static_frequency": dict(sorted(missing.items(), key=lambda kv: -kv[1])),
    }

def main():
    p = argparse.ArgumentParser()
    p.add_argument("usage_json", type=pathlib.Path)
    p.add_argument("--strict", action="store_true",
                   help="Fail if referenced extension names lack registry entries")
    args = p.parse_args()
    result = audit(json.loads(args.usage_json.read_text(encoding="utf-8")),
                   registry_names(REGISTRY.read_text(encoding="utf-8")))
    print(json.dumps(result, ensure_ascii=False, indent=2))
    if args.strict and result["missing_count"]:
        raise SystemExit(1)

if __name__ == "__main__":
    main()
