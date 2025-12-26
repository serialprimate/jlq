#!/usr/bin/env bash
set -euo pipefail

readonly ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

printf "Starting CI pipeline...\n"

# 1. Clean
"$ROOT_DIR/scripts/clean.sh"

# 2. Build (Debug)
"$ROOT_DIR/scripts/build.sh" debug

# 3. Test (Debug)
"$ROOT_DIR/scripts/test.sh" debug

# 4. Build (Release)
"$ROOT_DIR/scripts/build.sh" release

# 5. Test (Release)
# Note: We don't have a release-test preset in CMakePresets.json yet,
# but we can run the integration tests against the release binary.
printf "Running integration tests against Release build...\n"
source "$ROOT_DIR/.venv/bin/activate"
pytest "$ROOT_DIR/test/integration_tests.py"

printf "CI pipeline complete.\n"
