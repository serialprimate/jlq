#!/usr/bin/env bash
set -euo pipefail

readonly ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

printf "Starting CI pipeline...\n"

# 1. Clean
"$ROOT_DIR/scripts/clean.sh"

# 2. Run Debug Workflow (Configure, Build, Test)
printf "Running Debug workflow...\n"
cmake --workflow --preset debug-workflow

# 3. Run CI Workflow (Configure, Build, Test, Package - Release)
printf "Running CI workflow (Release)...\n"
cmake --workflow --preset ci-workflow

# 4. Run Python integration tests
printf "Running Python integration tests...\n"
source "$ROOT_DIR/.venv/bin/activate"
pytest "$ROOT_DIR/test/integration_tests.py"

printf "CI pipeline complete.\n"
