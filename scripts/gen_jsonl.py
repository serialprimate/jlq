#!/usr/bin/env python3
"""Deterministic JSONL generator for jlq.

Generates a stream of JSONL records with controlled rates of:
- matches (path exists and equals value)
- missing path
- non-matching values
- malformed JSON lines
- empty lines
- CRLF line endings
- optional final line without a newline
- optional oversized lines (> 64MiB) for policy testing

This script is intended for local benchmarking and manual testing. It is not
part of the unit test suite.
"""

from __future__ import annotations

import argparse
import json
import random
import sys
from pathlib import Path
from typing import Any, TextIO


MAX_LINE_BYTES_DEFAULT = 64 * 1024 * 1024


def parse_dot_path(path: str) -> list[str]:
    segments = path.split(".")
    if not segments or any(seg == "" for seg in segments):
        raise ValueError(f"Invalid dot path: {path!r}")
    return segments


def set_path(obj: dict[str, Any], segments: list[str], value: Any) -> None:
    cur: dict[str, Any] = obj
    for seg in segments[:-1]:
        nxt = cur.get(seg)
        if not isinstance(nxt, dict):
            nxt = {}
            cur[seg] = nxt
        cur = nxt
    cur[segments[-1]] = value


def parse_typed_value(value_type: str, value_text: str) -> Any:
    match value_type:
        case "string":
            return value_text
        case "number":
            if any(c in value_text for c in ".eE"):
                return float(value_text)
            return int(value_text)
        case "bool":
            if value_text == "true":
                return True
            if value_text == "false":
                return False
            raise ValueError("For --type bool, --value must be 'true' or 'false'")
        case "null":
            return None
        case _:
            raise ValueError(f"Unknown type: {value_type!r}")


def write_line(out: TextIO, line: str, newline: str) -> None:
    out.write(line)
    out.write(newline)


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate JSONL files for jlq benchmarking.")
    ap.add_argument("--out", default="-", help="Output path, or '-' for stdout")
    ap.add_argument("--lines", type=int, required=True, help="Number of lines to generate")
    ap.add_argument("--seed", type=int, default=1, help="RNG seed")

    ap.add_argument("--path", required=True, help="Dot path to set (e.g. a.b.c)")
    ap.add_argument("--type", dest="value_type", choices=["string", "number", "bool", "null"], default="string")
    ap.add_argument("--value", default="", help="Value text (ignored for --type null)")

    ap.add_argument("--match-rate", type=float, default=0.01, help="Fraction of lines that match")
    ap.add_argument("--missing-rate", type=float, default=0.10, help="Fraction of lines missing the path")
    ap.add_argument("--malformed-rate", type=float, default=0.001, help="Fraction of lines malformed JSON")
    ap.add_argument("--empty-rate", type=float, default=0.0, help="Fraction of empty lines")
    ap.add_argument("--crlf-rate", type=float, default=0.0, help="Fraction of lines with CRLF")

    ap.add_argument("--noise-fields", type=int, default=2, help="Extra fields per object")
    ap.add_argument("--pad-bytes", type=int, default=0, help="Pad each valid JSON line to at least this many bytes")

    ap.add_argument("--max-line-bytes", type=int, default=MAX_LINE_BYTES_DEFAULT, help="Reference max line length")
    ap.add_argument("--oversize-rate", type=float, default=0.0, help="Fraction of lines exceeding max line bytes")

    ap.add_argument(
        "--final-newline",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Whether the last line ends with a newline",
    )

    ns = ap.parse_args()

    if ns.lines <= 0:
        ap.error("--lines must be > 0")

    for opt in ("match_rate", "missing_rate", "malformed_rate", "empty_rate", "crlf_rate", "oversize_rate"):
        v = getattr(ns, opt)
        if not (0.0 <= v <= 1.0):
            ap.error(f"--{opt.replace('_', '-')} must be in [0,1]")

    segments = parse_dot_path(ns.path)
    target_value = parse_typed_value(ns.value_type, ns.value) if ns.value_type != "null" else None

    rng = random.Random(ns.seed)

    out: TextIO
    if ns.out == "-":
        out = sys.stdout
    else:
        out_path = Path(ns.out)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out = out_path.open("w", encoding="utf-8", newline="")

    try:
        for i in range(ns.lines):
            newline = "\r\n" if rng.random() < ns.crlf_rate else "\n"
            is_last = i == (ns.lines - 1)

            if rng.random() < ns.empty_rate:
                if not is_last or ns.final_newline:
                    write_line(out, "", newline)
                continue

            if rng.random() < ns.oversize_rate:
                oversized = "{" + ("x" * (ns.max_line_bytes + 1))
                if not is_last or ns.final_newline:
                    write_line(out, oversized, newline)
                else:
                    out.write(oversized)
                continue

            if rng.random() < ns.malformed_rate:
                bad = '{"a": 1'
                if not is_last or ns.final_newline:
                    write_line(out, bad, newline)
                else:
                    out.write(bad)
                continue

            obj: dict[str, Any] = {"id": i}
            for n in range(ns.noise_fields):
                obj[f"noise_{n}"] = rng.choice(
                    [
                        rng.randint(0, 10_000),
                        rng.random(),
                        rng.choice(["a", "b", "c"]),
                        rng.choice([True, False]),
                    ]
                )

            r = rng.random()
            if r < ns.missing_rate:
                pass
            elif r < ns.missing_rate + ns.match_rate:
                set_path(obj, segments, target_value)
            else:
                match ns.value_type:
                    case "string":
                        set_path(obj, segments, f"{ns.value}_other")
                    case "number":
                        set_path(obj, segments, (target_value or 0) + 1)
                    case "bool":
                        set_path(obj, segments, not bool(target_value))
                    case "null":
                        set_path(obj, segments, "not-null")

            line = json.dumps(obj, separators=(",", ":"), ensure_ascii=False)

            if ns.pad_bytes > 0 and len(line.encode("utf-8")) < ns.pad_bytes:
                pad_len = ns.pad_bytes - len(line.encode("utf-8"))
                obj["pad"] = "p" * max(0, pad_len - 32)
                line = json.dumps(obj, separators=(",", ":"), ensure_ascii=False)

            if not is_last or ns.final_newline:
                write_line(out, line, newline)
            else:
                out.write(line)

        return 0
    finally:
        if out is not sys.stdout:
            out.close()


if __name__ == "__main__":
    raise SystemExit(main())
