#!/usr/bin/env bash
set -euo pipefail

# Shared helpers for creating/activating the project's Python virtual environment.
# Intended to be sourced by other scripts.

jlq_ensure_venv() {
  local root_dir="$1"

  if [[ ! -d "$root_dir/.venv" ]]; then
    printf "Setting up Python virtual environment...\n"
    python3 -m venv "$root_dir/.venv"
  fi

  # shellcheck disable=SC1091
  source "$root_dir/.venv/bin/activate"

  # Ensure pytest is available for integration tests.
  python3 -m pip install --quiet --upgrade pip
  python3 -m pip install --quiet pytest
}
