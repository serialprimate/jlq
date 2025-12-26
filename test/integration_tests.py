import subprocess
import pytest
import time
from pathlib import Path
from typing import List

import os

def run_jlq(args: List[str], binary: str) -> subprocess.CompletedProcess[str]:
    """Runs the jlq binary with the given arguments and returns the result."""
    binary_path = Path(binary)
    if not binary_path.exists():
        raise FileNotFoundError(
            f"jlq binary not found at {binary_path.absolute()}. "
            "Please build the project or provide the correct path via --jlq."
        )

    result = subprocess.run(
        [str(binary_path)] + args,
        capture_output=True,
        text=True
    )
    return result

def test_basic_match(tmp_path: Path, jlq_bin: str | None) -> None:
    """Tests basic matching functionality with a generated JSONL file."""
    binary = jlq_bin or "./build/debug/bin/jlq"
    jsonl_file = tmp_path / "test.jsonl"
    subprocess.run([
        "python3", "scripts/gen_jsonl.py",
        "--lines", "100",
        "--path", "user.id",
        "--type", "number",
        "--value", "42",
        "--match-rate", "1.0",
        "--missing-rate", "0.0",
        "--out", str(jsonl_file)
    ], check=True)

    result = run_jlq([str(jsonl_file), "--path", "user.id", "--value", "42", "--type", "number"], binary=binary)
    assert result.returncode == 0
    assert len(result.stdout.splitlines()) == 100

def test_strict_mode_fails_on_malformed(tmp_path: Path, jlq_bin: str | None) -> None:
    """Tests that --strict mode correctly fails on malformed JSON lines."""
    binary = jlq_bin or "./build/debug/bin/jlq"
    jsonl_file = tmp_path / "malformed.jsonl"
    subprocess.run([
        "python3", "scripts/gen_jsonl.py",
        "--lines", "10",
        "--path", "a",
        "--malformed-rate", "0.5",
        "--out", str(jsonl_file)
    ], check=True)

    # Without strict, should succeed (skipping bad lines)
    result = run_jlq([str(jsonl_file), "--path", "a", "--value", "b"], binary=binary)
    assert result.returncode == 0

    # With strict, should fail with exit code 3
    result_strict = run_jlq([str(jsonl_file), "--path", "a", "--value", "b", "--strict"], binary=binary)
    assert result_strict.returncode == 3

def test_oversized_lines_skipped(tmp_path: Path, jlq_bin: str | None) -> None:
    """Tests that oversized lines are skipped in default mode."""
    binary = jlq_bin or "./build/debug/bin/jlq"
    jsonl_file = tmp_path / "oversized.jsonl"
    subprocess.run([
        "python3", "scripts/gen_jsonl.py",
        "--lines", "10",
        "--path", "a",
        "--oversize-rate", "0.2",
        "--out", str(jsonl_file)
    ], check=True)

    result = run_jlq([str(jsonl_file), "--path", "a", "--value", "b"], binary=binary)
    assert result.returncode == 0

def test_crlf_handling(tmp_path: Path, jlq_bin: str | None) -> None:
    """Tests that CRLF line endings are handled correctly."""
    binary = jlq_bin or "./build/debug/bin/jlq"
    jsonl_file = tmp_path / "crlf.jsonl"
    subprocess.run([
        "python3", "scripts/gen_jsonl.py",
        "--lines", "50",
        "--crlf-rate", "1.0",
        "--path", "key",
        "--value", "val",
        "--match-rate", "1.0",
        "--missing-rate", "0.0",
        "--out", str(jsonl_file)
    ], check=True)

    result = run_jlq([str(jsonl_file), "--path", "key", "--value", "val"], binary=binary)
    assert result.returncode == 0
    assert len(result.stdout.splitlines()) == 50

def test_performance_smoke(tmp_path: Path, jlq_bin: str | None) -> None:
    """A smoke test for performance to ensure no major regressions."""
    # Use release binary if it exists, otherwise debug
    binary = jlq_bin
    if binary is None:
        binary = "./build/release/bin/jlq"
        if not Path(binary).exists():
            binary = "./build/debug/bin/jlq"

    jsonl_file = tmp_path / "perf.jsonl"
    lines = 100_000
    subprocess.run([
        "python3", "scripts/gen_jsonl.py",
        "--lines", str(lines),
        "--path", "a.b",
        "--type", "string",
        "--value", "v",
        "--match-rate", "0.5",
        "--missing-rate", "0.0",
        "--malformed-rate", "0.0",
        "--out", str(jsonl_file)
    ], check=True)

    start = time.perf_counter()
    result = run_jlq([str(jsonl_file), "--path", "a.b", "--value", "v"], binary=binary)
    end = time.perf_counter()

    assert result.returncode == 0
    # Match rate is probabilistic, so we check it's in a reasonable range
    assert 45_000 <= len(result.stdout.splitlines()) <= 55_000
    print(f"Processed {lines} lines in {end - start:.4f}s")

    result = run_jlq([str(jsonl_file), "--path", "a.b", "--value", "v"], binary=binary)
    end = time.perf_counter()

    duration = end - start
    assert result.returncode == 0

    # Smoke test: 100k lines should be processed in under 1 second even in debug
    # In release it's ~0.01s
    assert duration < 1.0, f"Performance regression: {lines} lines took {duration:.4f}s"
