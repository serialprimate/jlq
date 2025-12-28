# Phase 3: Array Indexing Support

## Goal
Add list (array) indexing to `jlq` paths by allowing numeric dot-path segments such as `a.b.0.c`.

## Non-Goals
- No wildcards (e.g. `[*]`), slices, ranges, or predicates.
- No JSONPath syntax beyond dot-separated segments.
- No support for distinguishing numeric object keys vs. indices (a segment consisting only of digits is treated as an index).

## Path Parsing

### Overview
Update the path parser to produce a sequence of typed segments:
- **Key segment**: object key (string)
- **Index segment**: array index (non-negative integer)

### Implementation Tasks
- Update `libs/jlq/src/path.hpp` to introduce a segment representation that distinguishes keys from indices.
- Update `libs/jlq/src/path.cpp`:
  - Keep existing validation rules (no leading/trailing `.`, no empty segments).
  - When a segment is **all digits**, parse it as a `size_t` array index.
  - If the index does not fit in `size_t`, treat it as a CLI usage error.

### Path Syntax Rules
- `a.b.0.c` is valid.
- `a..b`, `.a`, and `a.` are invalid.
- A segment is an **index** if and only if it is composed solely of ASCII digits (`0-9`).

## Query Engine

### Overview
Extend traversal so each path segment advances the current value by:
- Key segment: `value` must be an object → lookup field
- Index segment: `value` must be an array → access element at index

### simdjson On-Demand Details
- Use `simdjson::ondemand::value::get_object()` / `get_array()` for type-checked traversal.
- Use `simdjson::ondemand::array::at(index)` to retrieve the indexed element.

### Error Handling Semantics
Treat these conditions as **non-matches** (not malformed JSON):
- Missing field (`NO_SUCH_FIELD`)
- Incorrect type during traversal (`INCORRECT_TYPE`)
- Index out of bounds (`INDEX_OUT_OF_BOUNDS`)

Treat other simdjson errors as **malformed JSON**:
- In default mode: skip the line
- In `--strict` mode: fail fast with exit code 3

### Performance Notes
On-demand array indexing is not random access; reaching index $N$ may require scanning up to $N$ elements. Array indexing is therefore $O(N)$ in the index.

## CLI & Validation

### CLI Contract Updates
- `--path <path>` supports numeric segments for array indices.
- Help text and usage examples should include at least one index example (e.g. `a.b.0.c`).

### Validation
- Keep strict option parsing (unknown flags remain a usage error).
- Ensure invalid paths (empty segments, leading/trailing dot, out-of-range index) return exit code 1.

## Testing

### Unit Tests (C++)
Add/extend tests in `test/apps/cli_tests/unit_tests.cpp`:
- Path parsing:
  - `a.b.0.c` produces key/key/index/key segments.
  - Invalid forms still reject (`""`, `.a`, `a.`, `a..b`).
- Query traversal:
  - Array index path matches correct element.
  - Out-of-bounds index yields no match.
  - Type mismatch (expect array but see object) yields no match.

### Integration Tests (Python)
Add tests in `test/integration_tests.py` using a small hand-written JSONL file:
- Query with `--path a.b.1.c --value y` matches exactly the intended line.
- Out-of-bounds index returns zero matches.

## Documentation

### README
Update:
- Usage option description for `--path` to mention indices.
- Limitations section to remove the “no array indexing” limitation.
- Add/adjust an example that includes an index segment.
