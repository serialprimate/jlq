#!/usr/bin/env bash
set -euo pipefail

# Configuration
readonly LINES=1000000
readonly PATH_QUERY="user.id"
readonly VALUE="42"
readonly FILE="bench_data.jsonl"
readonly JLQ_BIN="${1:-./build/release/bin/jlq}"

if [[ ! -f "$JLQ_BIN" ]]; then
    printf "Error: jlq binary not found at %s\n" "$JLQ_BIN"
    exit 1
fi

cleanup() {
    if [[ -f "$FILE" ]]; then
        rm "$FILE"
    fi
}
trap cleanup EXIT

printf "Generating %d lines of test data...\n" "$LINES"
python3 scripts/gen_jsonl.py --lines "$LINES" --path "$PATH_QUERY" --type number --value "$VALUE" --match-rate 0.01 --missing-rate 0.1 --malformed-rate 0 --out "$FILE"

FILE_SIZE=$(du -h "$FILE" | cut -f1)
printf "File size: %s\n" "$FILE_SIZE"

printf "------------------------------------------------\n"
printf "Benchmarking jlq...\n"
time "$JLQ_BIN" "$FILE" --path "$PATH_QUERY" --value "$VALUE" --type number > /dev/null

printf "------------------------------------------------\n"
printf "Benchmarking jq (compiled query)...\n"
# jq is notoriously slow for large files because it parses the whole JSON object into a tree.
time jq -c "select(.$PATH_QUERY == $VALUE)" "$FILE" > /dev/null

printf "------------------------------------------------\n"
printf "Benchmarking grep (naive string search)...\n"
# grep is very fast but doesn't understand JSON structure.
# We search for the exact string that gen_jsonl.py produces for a match.
# For user.id=42, it might look like ..."user":{"id":42}...
readonly SEARCH_STR="\"user\":{\"id\":42}"
time grep "$SEARCH_STR" "$FILE" > /dev/null

printf "------------------------------------------------\n"
