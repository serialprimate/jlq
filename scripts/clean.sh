#!/usr/bin/env bash
set -euo pipefail

# Get the root directory of the project
readonly ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

printf "Cleaning project...\n"

# Remove build directories
if [[ -d "$ROOT_DIR/build" ]]; then
    printf "Removing build/...\n"
    rm -rf "$ROOT_DIR/build"
fi

# Remove Python virtual environment
if [[ -d "$ROOT_DIR/.venv" ]]; then
    printf "Removing .venv/...\n"
    rm -rf "$ROOT_DIR/.venv"
fi

# Remove pytest cache
if [[ -d "$ROOT_DIR/.pytest_cache" ]]; then
    printf "Removing .pytest_cache/...\n"
    rm -rf "$ROOT_DIR/.pytest_cache"
fi

# Remove temporary test files
printf "Removing temporary .jsonl files...\n"
find "$ROOT_DIR" -maxdepth 1 -name "*.jsonl" -delete

printf "Clean complete.\n"
