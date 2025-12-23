# Copilot Instructions for jlq

## Project Overview
- **jlq** is a high-performance C++23 command-line tool for querying large JSONL files using memory mapping and SIMD-accelerated parsing.
- The codebase is modular, with clear separation between CLI logic, file mapping, and (future) JSON querying.
- The MVP supports only basic file mapping and CLI argument validation; future phases will add JSON path traversal and filtering.

## Key Components
- `libs/jlq/`: Core library code
  - `src/cli.cpp`, `include/jlq/cli.hpp`: CLI entry point, argument parsing, usage contract
  - `src/mapped_file.cpp`, `include/jlq/mapped_file.hpp`: Memory-mapped file abstraction
  - `include/jlq/exit_codes.hpp`: Standardized exit codes for CLI
- `apps/jlq/`: CLI executable (`main.cpp`)
- `tests/`: Minimal custom test harness, test cases for CLI and file mapping

## Build & Test Workflow
- **Presets:** All builds use CMake presets. Configure with `cmake --preset debug`, build with `cmake --build --preset debug-build`.
- **Testing:** Run tests with `ctest --preset debug-test --output-on-failure` or directly via `./build/debug/bin/jlq_tests`.
- **Sanitizers:** Enable with `-DENABLE_ASAN=ON` (AddressSanitizer), `-DENABLE_UBSAN=ON`, or `-DENABLE_TSAN=ON` for debug builds only. Do not use Valgrind and ASAN together.
- **Strict Warnings:** Warnings are treated as errors by default. Disable with `-DJLQ_ENABLE_WERROR=OFF`.
- **No in-source builds:** Building in the source directory is forbidden by CMakeLists.txt.

## Project-Specific Patterns
- **CLI contract:** Only a single positional file argument and `--help` are accepted in MVP. All other arguments are rejected.
- **Error handling:** Uses RAII and exceptions for resource management and error propagation. Exit codes are standardized.
- **Testing:** Uses a custom test harness (`test_harness.hpp`) with `JLQ_TEST_CASE` and `JLQ_CHECK_EQ` macros. Test output is printed to stdout.
- **Temporary files:** Tests use `TempFile` utility for safe, auto-cleaned temp files.

## Extending Functionality
- Add new CLI options and logic in `libs/jlq/src/cli.cpp` and update `run()` in `cli.hpp`.
- Implement JSON path traversal and filtering in future phases, following the roadmap in `PRD.md`.
- All new features should be covered by tests in `tests/` using the custom harness.
- For each change, enable strict warnings, use relevant sanitizers and ensure no warnings/errors.

## Integration & Dependencies
- Uses `simdjson` (planned) for fast JSON parsing and `mmap` for file access.
- Dependency management is via CMake and (optionally) vcpkg.

## References
- See `README.md` for build/test instructions and sanitizer usage.
- See `PRD.md` for project goals, CLI contract, and technical constraints.
- See `CMakePresets.json` for available build/test presets.

---

If you are unsure about a workflow or pattern, check the referenced files or ask for clarification. Update this file as the project evolves, especially when new CLI features or major refactors are introduced.
