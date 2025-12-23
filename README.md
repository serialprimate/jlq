# jlq

C++23 command-line tool for querying JSONL files.

## Requirements

- Linux (the included presets are Linux-focused)
- CMake >= 3.28
- A C++23 compiler (the presets default to `g++`)

## Configure

This repo uses CMake presets.

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

## Test

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
