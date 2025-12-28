#!/usr/bin/env bash
set -euo pipefail

readonly ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
preset="${1:-debug}"

printf "Running tests for preset: %s\n" "$preset"

# 1. Run C++ tests via CTest
printf "Running C++ unit tests...\n"
ctest --preset "$preset-test" --output-on-failure

# 2. Run Python integration tests
printf "Running Python integration tests...\n"

# shellcheck disable=SC1091
source "$ROOT_DIR/scripts/python_venv.sh"
jlq_ensure_venv "$ROOT_DIR"

# Explicitly pass the binary path for integration tests
pytest "$ROOT_DIR/test/integration_tests.py" --jlq "$ROOT_DIR/build/$preset/bin/jlq"

printf "All tests passed.\n"
