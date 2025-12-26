---
name: CMakeLists.txt Instructions
description: Modern CMakeLists.txt standards and best practices
applyTo: "**/*.cmake,**/CMakeLists.txt"
---

# CMakeLists.txt Instructions

## Core Principles

- Require modern CMake: put `cmake_minimum_required(VERSION 3.28)` at the top of the root `CMakeLists.txt`, and `project(...)` immediately after.
- Use **out-of-source** builds only (generate into a `build/` directory). Never write build artifacts into the source tree.
- Model everything as **targets** + **usage requirements** (include paths, compile features, definitions, link deps).

## Presets And Workflow (Preferred)

- Use `CMakePresets.json` for repo-wide presets; use `CMakeUserPresets.json` for machine-local overrides (do not commit it).
- Prefer driving builds via presets:
  - Configure: `cmake --preset <configurePreset>`
  - Build: `cmake --build --preset <buildPreset>`
  - Test: `ctest --preset <testPreset> --output-on-failure`
  - Package: `cmake --build --preset <buildPreset> --target package` or `cpack --preset <packagePreset>`
  - Workflow: `cmake --workflow --preset <workflowPreset>`
- Keep “developer convenience” knobs (sanitizers, extra warnings, local paths) in presets or cache variables, not hardcoded in project files.

## Target-Based Buildsystem Rules

- Prefer these “backbone” commands:
  - `add_executable()`, `add_library()`, `target_sources()`, `target_link_libraries()`
  - `target_compile_features()`, `target_compile_definitions()`
- Understand and use target scopes consistently:
  - `PRIVATE`: required to build the target (implementation-only)
  - `INTERFACE`: required only by consumers
  - `PUBLIC`: required by both the target and consumers
- As a strong default: implementation `.c/.cc/.cpp/.cxx` sources are `PRIVATE`.

## Headers, Include Paths, And File Sets

- Prefer `target_sources(... FILE_SET HEADERS ...)` to describe public headers.
- Only include a `FILE_SET ... FILES ...` list when the headers are intended to be **installed**.
- Avoid “spraying” include paths globally. If you must use include dirs, do it per-target via `target_include_directories()` with correct scopes.

## Language Standard Policy

- Do **not** force `CMAKE_CXX_STANDARD` in project CMakeLists; treat it as a packager/user decision.
- Express **minimum** requirements on targets via `target_compile_features(<tgt> PUBLIC cxx_std_23)` (or `PRIVATE` if only implementation needs it).

## Compile/Link Options (Use Sparingly)

- Prefer semantic requirements (`target_compile_features`, `target_compile_definitions`) over raw flags.
- Use `target_compile_options()` / `target_link_options()` only when necessary, and gate toolchain-specific flags with proper compiler checks.
- Avoid enabling warnings-as-errors by default for broadly-consumed libraries; prefer an option (e.g. `JLQ_ENABLE_WERROR`) or enabling it in CI/dev presets.

## Options, Cache Variables, And Naming

- Use `option()` for user-togglable features; prefer cache variables over ad-hoc globals.
- Prefix project-defined cache variables with a project namespace (e.g. `JLQ_...`).
- Avoid setting or shadowing `CMAKE_...` variables in project code unless you have a very strong reason.

## Dependencies

- Prefer `find_package(<Pkg> CONFIG REQUIRED)` and link imported targets (e.g. `Pkg::Pkg`).
- Use `CMAKE_PREFIX_PATH` (commonly set via presets) to point CMake at a dependency install tree.
- Prefer package managers (vcpkg/Conan/system packages) over vendoring or manual `find_path()`/`find_library()`.
- Only fall back to `find_path()`, `find_library()`, `find_file()`, `find_program()` when no package config exists.

## Testing

- Follow CTest conventions:
  - Gate tests behind `BUILD_TESTING` (default ON in many setups).
  - Call `enable_testing()` in the root when tests are enabled.
  - Register tests with `add_test(...)` (often via helper functions to avoid repetition).

## Install, Export, And Package Configs

- Prefer `install(TARGETS ...)` (target-based install) over manually copying files.
- Use `include(GNUInstallDirs)` for portable install destinations.
- If the project is meant to be consumed via `find_package()`, also:
  - `install(EXPORT ...)` with a namespace (e.g. `Jlq::`)
  - Install a `<Project>Config.cmake` that `include(${CMAKE_CURRENT_LIST_DIR}/<Export>.cmake)`
  - Install a version file generated via `CMakePackageConfigHelpers` / `write_basic_package_version_file()`

## Avoid These Patterns

- Global mutation: `include_directories()`, `link_libraries()`, `add_definitions()`, global `CMAKE_CXX_FLAGS`, etc.
- Hardcoding paths (especially `${CMAKE_SOURCE_DIR}`-relative paths); prefer per-directory variables like `${CMAKE_CURRENT_SOURCE_DIR}` and target properties.
- Directly setting low-level target properties (`set_target_properties`, `set_property`) when a dedicated target command exists.
- Using `target_link_directories()` / `target_include_directories()` as a first resort; prefer linking targets (including imported targets).
