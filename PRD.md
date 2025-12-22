# PRD: JLQ (C++ High-Performance CLI for JSONL Querying)

**Project Status:** Initial Sprint (Weekend)
**Primary Stack:** Modern C++ (C++23), CMake, vcpkg, `simdjson`, `mmap`

---

## 1. Executive Summary

### 1.1 Problem Statement
Developers and Data Engineers often work with massive (10GB+) JSONL files.
Existing tools like `grep` lack structural awareness, leading to false
positives, while high-level tools like `jq` are often CPU-bound and slow for
multi-gigabyte streams.

### 1.2 Solution
A lightweight C++ CLI utility that leverages Memory Mapping and
SIMD-accelerated parsing to search and filter nested JSONL records at the
speed of the underlying hardware.

---

## 2. Technical Scope & Requirements

### 2.1 Core Functional Requirements

| ID | Requirement | Description |
| :--- | :--- | :--- |
| FR-01 | Memory Mapping | Use `mmap` (POSIX) to map files into the process address space. |
| FR-02 | Path Traversal | Support nested key lookups using dot-notation (e.g., `a.b.c`). |
| FR-03 | Value Matching | Filter lines where the value at the path matches the input. |
| FR-04 | SIMD Parsing | Use `simdjson` for high-speed structural validation. |
| FR-05 | Stream Output | Print matching lines to `stdout` directly from the buffer. |

### 2.2 Technical Constraints
* **Language:** C++23 or later.
* **Memory Management:** Use RAII; avoid raw pointers for resource ownership.
* **Performance:** Minimise data copying; use `std::string_view` for lookups.
* **Build System:** CMake with `vcpkg` for dependency management.

### 2.3 CLI Contract (MVP)

#### Command
`jlq <file> --path <path> --value <value> [--type <type>] [--threads <n>] [--strict]`

#### Arguments
- `<file>`: Path to a JSONL file.

#### Options
- `--path <path>`: Lookup path using dot-notation (e.g., `a.b.c`).
  - Path segments are object keys only.
  - **MVP limitation:** no array indexing support.
- `--value <value>`: The value to compare against.
- `--type <type>`: How to interpret `--value`.
  - Allowed: `string` (default), `number`, `bool`, `null`.
  - For `bool`, accepted inputs: `true` or `false`.
  - For `null`, `--value` is ignored.
- `--threads <n>`: Number of worker threads.
  - Default: `1` in Phase 1–2, `std::thread::hardware_concurrency()` in Phase 3.
- `--strict`: If set, any malformed JSON line causes a non-zero exit.
  - Default behavior: skip malformed lines.

#### Output
- Print matching lines to `stdout` exactly as they appear in the input (including their trailing `\n` if present).
- If the last line of the file does not end with `\n`, print it as-is (no extra newline).

#### Exit Codes
- `0`: Completed successfully (even if zero matches).
- `1`: CLI usage error (missing/invalid args).
- `2`: File or OS error (open/stat/mmap failures).
- `3`: Parse error in `--strict` mode.

### 2.4 Matching Semantics (Exact Match)

For each line:
1. Parse the line as a single JSON document.
2. Traverse the document via `--path`.
3. If the path is missing or the type does not match `--type`, it is not a match.
4. Compare using exact equality:
   - `string`: byte-for-byte equality.
   - `number`: numeric equality (see **2.4.1**).
   - `bool`: equality on `true`/`false`.
   - `null`: matches only if the JSON value is `null`.

#### 2.4.1 Numeric parsing (`--type number`)
- Accepted `--value` grammar matches JSON numbers: `-?(0|[1-9][0-9]*)(\.[0-9]+)?([eE][+-]?[0-9]+)?`.
  - Reject `+1`, `NaN`, `Inf`, and hex forms as invalid.
- If `--value` cannot be parsed as a JSON number, treat it as a CLI usage error (exit code `1`).
- Matching is numeric equality between the parsed `--value` and the JSON number at `--path`.
  - Implementation note: if comparing via IEEE-754 `double`, document that some decimal values are not exactly representable; prefer the same parsing strategy for both sides to minimize surprises.

### 2.5 Input Handling & Edge Cases
- Treat input as UTF-8 bytes; do not validate Unicode.
- Handle files with `\n` line endings; tolerate `\r\n` by trimming a single trailing `\r` before parsing a line.
- Ignore empty lines (no output).
- Whitespace around JSON is allowed.

#### 2.5.1 Max line length policy (bounded memory)
- To keep memory bounded (scratch buffer + padding), impose a maximum line length of **64 MiB** (excluding the trailing `\n`).
- If a line exceeds this limit:
  - Default mode: treat as malformed and skip (no output).
  - `--strict`: terminate with exit code `3`.

### 2.6 Implementation Notes (Important)

#### `simdjson` + `mmap` padding
`simdjson` requires access to up to `SIMDJSON_PADDING` bytes beyond the end of the parsed buffer. An `mmap`-ed file mapping cannot safely read beyond EOF.

**MVP approach (bounded memory):** copy each candidate line into a thread-local scratch buffer sized `line_length + SIMDJSON_PADDING`, append zero padding, then parse from that buffer. This preserves constant memory usage with respect to file size (bounded by maximum line length encountered).

#### Concurrency & output
In Phase 3, output ordering is **not guaranteed** (lines may be printed by threads as they are found). If stable ordering is later required, it must be specified and will require buffering/coordination.

---

## 3. Implementation Roadmap

### Phase 1: Infrastructure (Hours 1-4)
* **Goal:** A CLI that accepts a filename and maps it to memory.
* **Validation:** Verify the file is accessible as a single `std::span` or
  `std::string_view` without reading the whole file into a `std::vector`.

### Phase 2: Structural Logic (Hours 5-10)
* **Goal:** Implement the `simdjson` on-demand parser loop.
* **Logic:**
  1. Scan the buffer for `\n`.
  2. For each line, use `simdjson::ondemand::parser`.
  3. Extract the value using `--path` traversal (dot-notation segments).
  4. Apply the exact-match rules in **2.4**.

### Phase 3: Parallelisation (Hours 10-16)
* **Goal:** Use `std::jthread` to split the search across CPU cores.
* **Strategy:** Divide the `mmap` region into chunks. Each thread finds the
  start of the next full line and processes until the end of its segment.

---

## 4. Non-Goals (MVP)
- No full `jq` expression language.
- No regex, substring, glob, or numeric ranges.
- No JSONPath features beyond simple dot-notation keys.
- No array indexing.

## 5. Quality & Validation

### 7.1 Correctness checks
- Unit tests for:
  - Dot-path traversal on nested objects.
  - Type-specific equality (`string`, `number`, `bool`, `null`).
  - CRLF handling (`\r\n`).
- Integration tests on small JSONL fixtures (valid lines, invalid lines, empty lines).

### 7.2 Performance sanity checks
- Benchmark on a synthetic JSONL dataset (documented generation method) with:
  - Single-threaded baseline.
  - Multi-threaded run using `--threads`.

---

## 6. User Stories

### US-001: Precision Search
**As a** DevOps Engineer,
**I want to** search for `status: 500` inside a nested `network.http` object,
**So that** I don't get false positives from other fields containing "500".

### US-002: High Throughput
**As a** Data Scientist,
**I want to** filter a 20GB dataset in under 10 seconds,
**So that** I can iterate on data cleaning tasks quickly.

---

## 7. Success Metrics
* **Speed:** Successfully process a 10GB JSONL file in < 5 seconds on NVMe.
* **Accuracy:** Zero false positives when searching nested keys.
* **Efficiency:** Memory usage stays constant regardless of file size.
