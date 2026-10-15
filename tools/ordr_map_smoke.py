#!/usr/bin/env python3
"""Smoke-check a PRIVATE Warcraft III map with the standalone JASS interpreter.

Only confirms MPQ extraction and JASS syntax parsing. Standalone jass has no
game host natives, so InitHashtable is expected to fail at runtime.
Never uploads or copies the map outside a temporary local directory.
"""
import argparse
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def classify(stderr: str, exit_code: int) -> str:
    if "Parser Error" in stderr or "error parsing function" in stderr:
        return "PARSE_FAILED"
    if "unknown function: InitHashtable" in stderr:
        return "PARSE_PASSED_NO_GAME_HOST"
    if "JASS runtime error:" in stderr:
        return "PARSE_PASSED_RUNTIME_ERROR"
    if exit_code == 0:
        return "PARSE_PASSED_INTERPRETER_EXITED"
    return "INCONCLUSIVE"


def smoke(map_file: Path, mpqtool: Path, jass: Path, timeout: int) -> str:
    if not map_file.is_file():
        raise FileNotFoundError(map_file)
    for tool in (mpqtool, jass):
        if not tool.is_file():
            raise FileNotFoundError(f"Build the local tool first: {tool}")
    with tempfile.TemporaryDirectory(prefix="ordr-private-smoke-") as tmp:
        script = Path(tmp) / "war3map.j"
        with script.open("wb") as out:
            extraction = subprocess.run(
                [str(mpqtool), "-mpq", str(map_file.resolve()), "cat", "war3map.j"],
                stdout=out, stderr=subprocess.PIPE, timeout=timeout
            )
        if extraction.returncode or not script.stat().st_size:
            raise RuntimeError("MPQ extraction failed: " +
                               extraction.stderr.decode("utf-8", errors="replace")[:1000])
        result = subprocess.run(
            [str(jass), str(script)], stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, timeout=timeout
        )
        diagnostic = result.stderr.decode("utf-8", errors="replace")
        status = classify(diagnostic, result.returncode)
        print("map_script_bytes:", script.stat().st_size)
        print("status:", status)
        print("interpreter_stderr:", diagnostic[:2000].strip())
        return status


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("map_file", type=Path, help="Private local .w3x; not uploaded")
    parser.add_argument("--mpqtool", type=Path, default=ROOT / "build/bin/mpqtool")
    parser.add_argument("--jass", type=Path, default=ROOT / "build/bin/jass")
    parser.add_argument("--timeout", type=int, default=120)
    args = parser.parse_args()
    try:
        status = smoke(args.map_file, args.mpqtool, args.jass, args.timeout)
    except (OSError, subprocess.TimeoutExpired, RuntimeError) as err:
        print("smoke error:", err, file=sys.stderr)
        return 2
    return 0 if status.startswith("PARSE_PASSED") else 1


if __name__ == "__main__":
    sys.exit(main())
