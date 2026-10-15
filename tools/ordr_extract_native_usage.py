#!/usr/bin/env python3
"""Extract extension call frequencies from a locally extracted war3map.j script.

This tool does not decrypt, extract, or distribute a .w3x map.
"""
import argparse
from collections import Counter
import json
from pathlib import Path
import re

NAME = re.compile(r"\b(?:Dz|JN|EX)[A-Za-z0-9_]+\s*\(")
# Remove strings and // comments without mistaking escaped quotes for terminators.
TOKEN = re.compile(r'"(?:\\.|[^"\\])*"|//[^\n]*|/\*[\s\S]*?\*/')

def extract_calls(script):
    clean = TOKEN.sub(lambda match: " " * len(match.group()), script)
    return dict(sorted(Counter(
        match.group().split("(")[0].strip() for match in NAME.finditer(clean)
    ).items(), key=lambda item: (-item[1], item[0])))

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("script", type=Path, help="locally extracted war3map.j")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    usage = {
        "map": args.script.name,
        "method": "approximate lexical call counts, comments and strings excluded",
        "native_calls": extract_calls(args.script.read_text(encoding="utf-8", errors="replace")),
    }
    args.output.write_text(json.dumps(usage, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"Analyzed {len(usage['native_calls'])} extension names -> {args.output}")

if __name__ == "__main__":
    main()
