#!/usr/bin/env bash
set -euo pipefail

readonly ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
preset="${1:-release}"

printf "Packaging project with preset: %s\n" "$preset"

# 1. Ensure build exists
if [[ ! -d "$ROOT_DIR/build/$preset" ]]; then
    printf "Build directory not found. Running build first...\n"
    "$ROOT_DIR/scripts/build.sh" "$preset"
fi

# 2. Package
printf "Packaging...\n"
cmake --build --preset "$preset-build" --target package

printf "Packaging complete.\n"
