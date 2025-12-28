# Copilot Instructions for jlq

## Project Overview
- **jlq** is a high-performance C++23 command-line tool for querying large JSONL files using memory mapping and SIMD-accelerated parsing.
- The codebase is modular, with clear separation between CLI logic, file mapping, and (future) JSON querying.
- Phase 3 implements single-threaded querying using `simdjson` on-demand parsing, dot-notation path traversal (object keys + numeric array indices), and type-aware exact matching.

## Key Components
- `libs/jlq/`: Core library code
  - `src/cli.cpp`, `include/jlq/cli.hpp`: CLI entry point, argument parsing, usage contract
  - `src/MappedFile.cpp`, `src/MappedFile.hpp`: Memory-mapped file abstraction
  - `src/ExitCode.hpp`: Standardized exit codes for CLI
  - `src/path.cpp`, `src/path.hpp`: Dot-path parsing into segments
  - `src/LineScanner.cpp`, `src/LineScanner.hpp`: JSONL line splitting (CRLF tolerant, empty-line skipping, max-line enforcement)
  - `src/Query.cpp`, `src/Query.hpp`: Query engine (scratch-buffer + `simdjson::SIMDJSON_PADDING`, on-demand parsing)
  - `src/QueryConfig.hpp`: `QueryConfig` / `QueryValue` / parsed value representation
- `apps/jlq/`: CLI executable (`main.cpp`)
- `test/`: Test suite
  - `apps/`: Test executables (e.g., `cli_tests`, `mapped_file_tests`)
  - `libs/test_utils/`: Shared test utilities (`test_harness.hpp`, `TempFile.hpp` in `include/`)
  - `integration_tests.py`: Python-based integration tests using `pytest`

## Build & Test Workflow
- **Scripts:** Preferred workflow uses `./scripts/build.sh`, `./scripts/test.sh`, `./scripts/package.sh`, and `./scripts/clean.sh`.
- **CI:** `./scripts/ci.sh` runs the full pipeline (Clean, Debug Workflow, CI Workflow, Integration Tests).
- **Presets:** All builds use CMake presets.
  - Configure: `cmake --preset debug`
  - Build: `cmake --build --preset debug-build`
  - Test: `ctest --preset debug-test`
  - Package: `cmake --build --preset release-build --target package`
  - Workflow: `cmake --workflow --preset debug-workflow`
- **vcpkg bootstrap (dev containers):** Run `./scripts/bootstrap_vcpkg.sh` once in a fresh container before configuring.
- **Testing:**
  - **Unit Tests:** Run with `ctest --preset debug-test --output-on-failure` or directly via `./build/debug/bin/cli_tests`.
  - **Integration Tests:** Run with `pytest test/integration_tests.py` (requires `.venv`).
- **Sanitizers:** Enable with `-DENABLE_ASAN=ON` (AddressSanitizer), `-DENABLE_UBSAN=ON`, or `-DENABLE_TSAN=ON` for debug builds only. Do not use Valgrind and ASAN together.
- **Strict Warnings:** Warnings are treated as errors by default. Disable with `-DJLQ_ENABLE_WERROR=OFF`.
- **No in-source builds:** Building in the source directory is forbidden by CMakeLists.txt.

## Project-Specific Patterns
- **CLI contract (Phase 3):** `jlq <file> --path <path> --value <value> [--type <type>] [--threads <n>] [--strict]` and `--help`.
- **Path traversal:** Dot-path segments that are all digits (e.g., `0`, `12`) are treated as array indices (e.g., `a.items.0.id`). All other segments are object keys.
- **Array indexing performance:** On-demand array indexing is $O(N)$ in the index (reaching index $N$ may scan up to $N$ elements).
- **Parsing safety:** Never parse directly from the `mmap` span; always copy each line to a scratch buffer sized `line_length + simdjson::SIMDJSON_PADDING` and zero-pad.
- **Strict mode:** Default skips malformed/oversized lines; `--strict` fails fast with exit code 3.
- **Error handling:** Uses RAII and exceptions for resource management and error propagation. Exit codes are standardized.
- **Testing:**
  - Uses a custom test harness (`test_harness.hpp`) with `JLQ_TEST_CASE` and `JLQ_CHECK_EQ` macros.
  - Integration tests use `scripts/gen_jsonl.py` to generate deterministic test data.
- **Temporary files:** Tests use `TempFile` utility for safe, auto-cleaned temp files.
- **Test Utilities:** Shared test code is located in `test/libs/test_utils` and exposed via the `jlq::test_utils` target.
- **Dev Container**: You are working within a dev container. Everything you do must be dev container friendly.

## Extending Functionality
- Add new CLI options and logic in `libs/jlq/src/cli.cpp` and update `run()` in `cli.hpp`.
- Implement JSON path traversal and filtering in future phases, following the roadmap in `docs/PRD.md`.
- All new features should be covered by tests in `test/` using the custom harness.
- For each change, enable strict warnings, use sanitizers and ensure no warnings/errors.

## Integration & Dependencies
- Uses `simdjson` for fast JSON parsing and `mmap` for file access.
- Dependency management is via CMake and (optionally) vcpkg.

## References
- See `README.md` for build/test instructions and sanitizer usage.
- See `docs/PRD.md` for project goals, CLI contract, and technical constraints.
- See `CMakePresets.json` for available build/test presets.

---

If you are unsure about a workflow or pattern, check the referenced files or ask for clarification. Update this file as the project evolves, especially when new CLI features or major refactors are introduced.
