# Phase 2 Plan: Structural Logic (simdjson on-demand)

This document consolidates the Phase 2 implementation plan from the project PRD into a single, executable checklist.

Scope (Phase 2) per [PRD.md](../PRD.md):
- Implement the simdjson on-demand parser loop over JSONL lines.
- Add dot-notation path traversal (object keys only; no arrays).
- Add type-aware exact matching: `string`, `number`, `bool`, `null`.
- Enforce line handling rules: CRLF tolerance, ignore empty lines, whitespace allowed.
- Enforce the max line length policy (64 MiB) with `--strict` behavior.
- Implement safe parsing from an `mmap`ed file using a scratch buffer + `SIMDJSON_PADDING`.

Non-goals (still out of scope):
- Expression language / jq features, regex, ranges.
- Array indexing / JSONPath beyond dot-notation object keys.
- Stable output ordering for multi-threading (Phase 3).

---

## 1) Repository review (pre-work)

1. Confirm Phase 1 behavior and existing APIs:
   - CLI entry and validation behavior.
   - Standard exit codes.
   - `MappedFile` interface (span/string_view of mapped bytes).
2. Identify current CMake target names and test harness conventions.

Deliverable:
- A short list of existing constraints (target names, error handling style, CLI parser style).

---

## 2) CLI contract upgrade (Phase 2)

Update the CLI to match PRD §2.3 (still strict about rejecting unknown args):

Command:
- `jlq <file> --path <path> --value <value> [--type <type>] [--threads <n>] [--strict]`

Arguments:
- `<file>` required: path to a JSONL file.

Options:
- `--path <path>` required: dot-notation key path (object keys only).
- `--value <value>` required except `--type null` (value ignored for null).
- `--type <type>` optional: `string` (default), `number`, `bool`, `null`.
- `--threads <n>` optional: validate `n >= 1` (may be stored but single-threaded in Phase 2).
- `--strict` optional: malformed/oversized line => terminate with exit code 3.

Validation rules:
- Unknown options are usage errors.
- For `--type bool`, `--value` must be exactly `true` or `false`.
- For `--type number`, `--value` must match the PRD numeric grammar; invalid => usage error.

Exit codes (PRD §2.3):
- `0` success.
- `1` usage error.
- `2` file/OS error.
- `3` parse error (only in `--strict`).

Deliverables:
- CLI parser produces a `QueryConfig` (file path, path segments, value type + parsed value, strict flag, threads).
- Help/usage text updated accordingly.

---

## 3) Dependency + build wiring (`simdjson`)

Add `simdjson` as a dependency (prefer project’s existing dependency strategy):
- If `vcpkg.json` exists, add `simdjson` there and wire via CMake.
- Otherwise, use a consistent CMake approach already in the repo.

Deliverables:
- `simdjson` is linked to the core library target.
- Build passes with Werror enabled.

---

## 4) Phase 2 architecture (keep CLI thin)

Implement Phase 2 logic in the library, not inside CLI parsing.

Recommended minimal modules:

- `QueryConfig` (new)
  - `ValueType { string, number, bool, null }`
  - Parsed representation of `--value` appropriate to type
  - `strict` and `threads` fields

- Path parsing (new)
  - Parse dot-path into segments.
  - Enforce: no empty segments, no leading/trailing dots.

- Line scanning (new)
  - Iterate over the mapped bytes and split on `\n`.
  - Preserve whether a `\n` delimiter existed (for correct output newline preservation).
  - CRLF tolerance: if line ends with a single `\r`, trim it before parsing.
  - Ignore empty lines.
  - Enforce max line length of 64 MiB (excluding trailing `\n`).

- Query engine (new)
  - For each candidate line: copy into scratch buffer + `SIMDJSON_PADDING`, zero-pad.
  - Parse using `simdjson::ondemand::parser`.
  - Traverse by path segments.
  - Type-check and exact match.
  - If match: write original bytes exactly (including original newline only if present in input).

Deliverables:
- A library entry point such as `run_query(mapped, config, ostream)` used by CLI.

---

## 5) Line parsing + semantics (PRD alignment)

Implement the PRD’s semantics for each line:

- Whitespace around JSON is allowed.
- Ignore empty lines (no output).
- CRLF: trim a single trailing `\r` before parsing.
- Output:
  - Print matching lines exactly as they appear in input.
  - If input line had trailing `\n`, include it.
  - If the last line has no trailing `\n`, print it without adding one.

Malformed lines:
- Default: skip malformed JSON lines.
- `--strict`: first malformed line => terminate with exit code `3`.

Oversized lines (> 64 MiB excluding trailing `\n`):
- Default: treat as malformed and skip.
- `--strict`: terminate with exit code `3`.

---

## 6) Matching semantics

Dot-path traversal:
- Object keys only.
- If a segment is missing, not a match.
- If encountering a non-object before the final segment, not a match.

Exact match by type (PRD §2.4):
- `string`: byte-for-byte equality.
- `number`: numeric equality. (Implementation note: if comparing as `double`, document IEEE-754 caveats.)
- `bool`: `true/false` equality.
- `null`: matches only if JSON value is `null`.

---

## 7) Tests (Phase 2)

Add unit + integration tests using the project harness.

Unit tests (library-level):
- Dot-path parsing and validation.
- Line scanning:
  - `\n` splitting.
  - last line without `\n`.
  - CRLF trimming.
  - empty line skipping.
  - max line length handling (skip vs strict error).
- Type-specific matching:
  - string equality.
  - bool equality + CLI value validation.
  - null matching.
  - number parsing validation (CLI usage error) + numeric equality.

Integration tests (CLI-level):
- Given a temp JSONL file, verify:
  - stdout contains exactly expected lines (including newline behavior).
  - exit codes match PRD for usage errors, file errors, strict parse failures.

Deliverables:
- `ctest --preset debug-test --output-on-failure` passes.

---

## 8) Documentation updates (Phase 2)

README changes (required):
- Update usage examples to include `--path/--value/--type/--strict`.
- Mention benchmarking workflow and link to `docs/benchmarking.md`.

Repo instructions updates (required):
- Update `.github/copilot-instructions.md`:
  - CLI contract is no longer “file + --help only”.
  - Mention scratch-buffer padding policy for simdjson.
  - Point to the Phase 2 modules once added.

---

## 9) Definition of Done

Phase 2 is done when:
- `jlq` implements PRD §2.3–2.6 behaviors in single-threaded mode.
- Correct handling of CRLF, empty lines, and final-line newline preservation.
- Max line length policy enforced.
- Safe `simdjson` parsing via scratch buffer + `SIMDJSON_PADDING`.
- Default mode skips malformed lines; `--strict` fails fast with exit code 3.
- Tests are present and passing.
- README and repo instructions reflect new CLI + architecture.
