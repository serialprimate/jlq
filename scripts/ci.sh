#!/usr/bin/env bash
set -euo pipefail

readonly ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

printf "Starting CI pipeline...\n"

# 1. Clean
"$ROOT_DIR/scripts/clean.sh"

# 2. Bootstrap vcpkg
printf "Bootstrapping vcpkg...\n"
"$ROOT_DIR/scripts/bootstrap_vcpkg.sh"

# 3. Setup Python virtual environment
# shellcheck disable=SC1091
source "$ROOT_DIR/scripts/python_venv.sh"
jlq_ensure_venv "$ROOT_DIR"

# 4. Run Debug Workflow (Configure, Build, Test)
printf "Running Debug workflow...\n"
cmake --workflow --preset debug-workflow

# 5. Run CI Workflow (Configure, Build, Test, Package - Release)
printf "Running CI workflow (Release)...\n"
cmake --workflow --preset ci-workflow

# 6. Run Python integration tests
printf "Running Python integration tests (Release)...\n"
jlq_ensure_venv "$ROOT_DIR"
pytest "$ROOT_DIR/test/integration_tests.py" --jlq "$ROOT_DIR/build/release/bin/jlq"

printf "CI pipeline complete.\n"
