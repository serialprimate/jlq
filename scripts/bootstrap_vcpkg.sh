#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
vcpkg_dir="$root_dir/vcpkg"
vcpkg_commit_file="$root_dir/vcpkg-commit.txt"

log_info() { printf '[INFO] %s\n' "$*"; }
log_error() { printf '[ERROR] %s\n' "$*" >&2; }

require_cmd() {
  local cmd="$1"
  if ! command -v "$cmd" >/dev/null 2>&1; then
    log_error "Missing required command: $cmd"
    exit 1
  fi
}

read_vcpkg_commit() {
  if [[ -f "$vcpkg_commit_file" ]]; then
    # Trim whitespace
    tr -d ' \t\n\r' < "$vcpkg_commit_file"
  fi
}

checkout_commit_if_needed() {
  local desired_commit="$1"
  if [[ -z "$desired_commit" ]]; then
    return 0
  fi

  local current_commit
  current_commit="$(git -C "$vcpkg_dir" rev-parse HEAD)"
  if [[ "$current_commit" == "$desired_commit" ]]; then
    log_info "vcpkg already at pinned commit: $desired_commit"
    return 0
  fi

  log_info "Checking out pinned vcpkg commit: $desired_commit"
  # Fetch exactly the desired commit (works even with shallow clones).
  git -C "$vcpkg_dir" fetch --depth 1 origin "$desired_commit" >/dev/null
  git -C "$vcpkg_dir" checkout --detach "$desired_commit" >/dev/null
}

require_cmd git

if [[ -d "$vcpkg_dir" ]]; then
  log_info "vcpkg already exists: $vcpkg_dir"
else
  log_info "Cloning vcpkg into: $vcpkg_dir"
  git clone --depth 1 https://github.com/microsoft/vcpkg "$vcpkg_dir" >/dev/null
fi

desired_commit="$(read_vcpkg_commit || true)"
checkout_commit_if_needed "$desired_commit"

log_info "Bootstrapping vcpkg"
"$vcpkg_dir/bootstrap-vcpkg.sh" -disableMetrics >/dev/null

log_info "Done. Configure with: cmake --preset debug"
