#!/usr/bin/env python3
"""Benchmark runner for jlq.

Runs jlq repeatedly, redirects stdout to /dev/null, and reports median wall time
and computed throughput (GiB/s) based on input file size.

This is intended for local benchmarking and manual performance regression checks.
"""

from __future__ import annotations

import argparse
import json
import os
import statistics
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class RunResult:
    seconds: float
    exit_code: int


def run_once(cmd: list[str]) -> RunResult:
    t0 = time.perf_counter()
    p = subprocess.run(
        cmd,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        text=True,
        check=False,
    )
    t1 = time.perf_counter()
    return RunResult(seconds=(t1 - t0), exit_code=p.returncode)


def main() -> int:
    ap = argparse.ArgumentParser(description="Benchmark jlq throughput on a JSONL file.")
    ap.add_argument("--jlq", default="./build/debug/bin/jlq", help="Path to jlq executable")
    ap.add_argument("--file", required=True, help="Path to JSONL file")
    ap.add_argument("--path", required=True, help="Dot path, e.g. a.b.c")
    ap.add_argument("--type", choices=["string", "number", "bool", "null"], default="string")
    ap.add_argument("--value", default="", help="Value text (ignored for --type null)")
    ap.add_argument("--strict", action="store_true", help="Pass --strict")
    ap.add_argument("--threads", type=int, default=1, help="Pass --threads")

    ap.add_argument("--warmups", type=int, default=1, help="Warmup runs")
    ap.add_argument("--runs", type=int, default=5, help="Measured runs")
    ap.add_argument("--json-out", default="", help="Optional JSON summary output path")

    ns = ap.parse_args()

    if ns.warmups < 0 or ns.runs <= 0:
        ap.error("--warmups must be >= 0 and --runs must be > 0")
    if ns.threads <= 0:
        ap.error("--threads must be >= 1")

    file_size = os.path.getsize(ns.file)

    cmd = [
        ns.jlq,
        ns.file,
        "--path",
        ns.path,
        "--type",
        ns.type,
        "--value",
        ns.value,
        "--threads",
        str(ns.threads),
    ]
    if ns.strict:
        cmd.append("--strict")

    for _ in range(ns.warmups):
        r = run_once(cmd)
        if r.exit_code != 0:
            sys.stderr.write(f"Warmup failed (exit {r.exit_code}).\n")
            sys.stderr.write(f"Command: {' '.join(cmd)}\n")
            return r.exit_code

    results: list[RunResult] = []
    for _ in range(ns.runs):
        r = run_once(cmd)
        if r.exit_code != 0:
            sys.stderr.write(f"Run failed (exit {r.exit_code}).\n")
            sys.stderr.write(f"Command: {' '.join(cmd)}\n")
            return r.exit_code
        results.append(r)

    times = [r.seconds for r in results]
    median_s = statistics.median(times)
    mean_s = statistics.mean(times)
    stdev_s = statistics.pstdev(times) if len(times) > 1 else 0.0

    gib = file_size / (1024**3)
    gib_s_median = gib / median_s if median_s > 0 else float("inf")

    print(f"file: {ns.file}")
    print(f"size: {file_size} bytes ({gib:.3f} GiB)")
    print(f"command: {' '.join(cmd)}")
    print(f"runs: {ns.runs} (warmups: {ns.warmups})")
    print(f"time (median): {median_s:.6f} s")
    print(f"time (mean):   {mean_s:.6f} s")
    print(f"time (stdev):  {stdev_s:.6f} s")
    print(f"throughput (median): {gib_s_median:.3f} GiB/s")

    if ns.json_out:
        out_path = Path(ns.json_out)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        payload = {
            "file": ns.file,
            "file_size_bytes": file_size,
            "cmd": cmd,
            "runs": ns.runs,
            "warmups": ns.warmups,
            "times_s": times,
            "median_s": median_s,
            "mean_s": mean_s,
            "stdev_s": stdev_s,
            "throughput_gib_s_median": gib_s_median,
        }
        out_path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
