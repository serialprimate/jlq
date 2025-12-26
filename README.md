# jlq

C++23 command-line tool for querying JSONL files.

## Requirements

- Linux (the included presets are Linux-focused)
- CMake >= 3.28
- A C++23 compiler (the presets default to `g++`)

## Quick Start (Scripts)

For convenience, several scripts are provided to automate the workflow:

- **Clean**: `./scripts/clean.sh` (removes `build/`, `.venv/`, and test artifacts)
- **Build**: `./scripts/build.sh [preset]` (defaults to `debug`)
- **Test**: `./scripts/test.sh [preset]` (runs C++ unit tests and Python integration tests)
- **CI**: `./scripts/ci.sh` (runs clean, build, and test for both debug and release)

## Configure

This repo uses CMake presets.

This project uses **vcpkg in manifest mode**. In a fresh dev container, bootstrap vcpkg first:

```bash
./scripts/bootstrap_vcpkg.sh
```

List available presets:

```bash
cmake --list-presets
```

Configure (choose one of: `debug`, `release`, `relwithdebinfo`):

```bash
cmake --preset debug
```

This generates build output under:

- `build/debug/`

## Build

```bash
cmake --build --preset debug-build
```

Artifacts are placed under:

- `build/debug/bin/` (executables)
- `build/debug/lib/` (libraries)

Run the CLI:

```bash
./build/debug/bin/jlq --help
```

Query example:

```bash
./build/debug/bin/jlq data.jsonl --path network.http.status --type number --value 500
```

Strict mode (first malformed/oversized line => exit code 3):

```bash
./build/debug/bin/jlq data.jsonl --strict --path network.http.status --type number --value 500
```

## Test

### C++ Unit Tests
Run all tests with CTest:

```bash
ctest --preset debug-test --output-on-failure
```

Or run individual test executables directly:

```bash
# CLI tests
./build/debug/bin/cli_tests

# Mapped file tests
./build/debug/bin/mapped_file_tests
```

### Python Integration Tests
These tests use `pytest` and `gen_jsonl.py` to verify `jlq` against various edge cases (malformed JSON, oversized lines, CRLF, etc.).

```bash
source .venv/bin/activate
pytest test/integration_tests.py
```

## Clean

To remove all build outputs for a preset, delete its build directory:

```bash
rm -rf build/debug
```

## Strict warnings

By default, warnings are treated as errors.

To disable `-Werror`/`/WX`:

```bash
cmake --preset debug -DJLQ_ENABLE_WERROR=OFF
cmake --build --preset debug-build
```

## Sanitizers

Sanitizers are available for non-Release build types.

Example (ASAN):

```bash
cmake --preset debug -DENABLE_ASAN=ON
cmake --build --preset debug-build
ctest --preset debug-test --output-on-failure
```

## Benchmarking

Generate a deterministic dataset:

```bash
./scripts/gen_jsonl.py --out /mnt/nvme/jlq_10g.jsonl --lines 20000000 --seed 1 \
	--path network.http.status --type number --value 500 --match-rate 0.01
```

Build (recommended: release):

```bash
cmake --preset release
cmake --build --preset release-build
```

Run the benchmark runner:

```bash
./scripts/bench.py --jlq ./build/release/bin/jlq --file /mnt/nvme/jlq_10g.jsonl \
	--path network.http.status --type number --value 500 --runs 7 --warmups 1
```

### Comparison with other tools
To compare `jlq` performance against `jq` and `grep`:

```bash
./scripts/compare_bench.sh
```

See [docs/benchmarking.md](docs/benchmarking.md) for methodology and reporting guidance.
