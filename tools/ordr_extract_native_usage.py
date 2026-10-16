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
NATIVE = re.compile(
    r'(?m)^[ \t]*(?:constant[ \t]+)?native[ \t]+((?:Dz|JN|EX)[A-Za-z0-9_]+)'
    r'[ \t]+takes[ \t]+([^\r\n]+?)[ \t]+returns[ \t]+([A-Za-z_][A-Za-z0-9_]*)[ \t]*\r?$')


def clean_source(script):
    # Keep newlines so native diagnostics refer to the original private file.
    return TOKEN.sub(lambda match: re.sub(r'[^\r\n]', ' ', match.group()), script)


def extract_declarations(script):
    """Export extension ABI metadata without including script bodies or strings."""
    clean = clean_source(script)
    declarations = {}
    for match in NATIVE.finditer(clean):
        name, takes, returns = match.groups()
        parameters = []
        if takes.strip() != 'nothing':
            for parameter in takes.split(','):
                kind, variable = parameter.split()
                parameters.append({'type': kind, 'name': variable})
        declarations[name] = {'parameters': parameters, 'returns': returns,
                              'line': clean.count('\n', 0, match.start()) + 1}
    return declarations

def extract_calls(script):
    clean = clean_source(script)
    return dict(sorted(Counter(
        match.group().split("(")[0].strip() for match in NAME.finditer(clean)
    ).items(), key=lambda item: (-item[1], item[0])))

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("script", type=Path, help="locally extracted war3map.j")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    script = args.script.read_text(encoding="utf-8", errors="replace")
    usage = {
        "map": args.script.name,
        "method": "approximate lexical call counts, comments and strings excluded",
        "native_calls": extract_calls(script),
        "native_declarations": extract_declarations(script),
    }
    args.output.write_text(json.dumps(usage, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"Analyzed {len(usage['native_calls'])} extension names -> {args.output}")

if __name__ == "__main__":
    main()
