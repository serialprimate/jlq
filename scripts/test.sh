#!/usr/bin/env bash
set -euo pipefail

readonly ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
preset="${1:-debug}"

printf "Running tests for preset: %s\n" "$preset"

# 1. Run C++ tests via CTest
printf "Running C++ unit tests...\n"
ctest --preset "$preset-test" --output-on-failure

# 2. Run Python integration tests
if [[ -d "$ROOT_DIR/.venv" ]]; then
    printf "Running Python integration tests...\n"
    source "$ROOT_DIR/.venv/bin/activate"
    # Explicitly pass the binary path for integration tests
    pytest "$ROOT_DIR/test/integration_tests.py" --jlq "$ROOT_DIR/build/$preset/bin/jlq"
else
    printf "Warning: .venv not found. Skipping integration tests.\n" >&2
    printf "Run ./scripts/build.sh first.\n" >&2
fi

printf "All tests passed.\n"
