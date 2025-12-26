#!/usr/bin/env bash
set -euo pipefail

readonly ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
preset="${1:-debug}"

printf "Building project with preset: %s\n" "$preset"

# 1. Bootstrap vcpkg if not already done
if [[ ! -f "$ROOT_DIR/vcpkg/vcpkg" ]]; then
    printf "Bootstrapping vcpkg...\n"
    "$ROOT_DIR/scripts/bootstrap_vcpkg.sh"
fi

# 2. Setup Python virtual environment if not already done
if [[ ! -d "$ROOT_DIR/.venv" ]]; then
    printf "Setting up Python virtual environment...\n"
    python3 -m venv "$ROOT_DIR/.venv"
    source "$ROOT_DIR/.venv/bin/activate"
    pip install pytest
else
    source "$ROOT_DIR/.venv/bin/activate"
fi

# 3. Configure CMake
printf "Configuring CMake...\n"
cmake --preset "$preset"

# 4. Build
printf "Building...\n"
cmake --build --preset "$preset-build"

printf "Build complete.\n"
