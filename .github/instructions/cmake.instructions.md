---
name: CMakeLists.txt Instructions
description: Modern CMakeLists.txt standards and best practices
applyTo: "**/*.cmake,**/CMakeLists.txt"
---

# CMakeLists.txt Instructions

- Use the most contemporary CMake version **CMake 3.28+** for all projects to leverage modern CMake features
- Use **targets** over global properties. Define libraries/executables with `add_library()`/`add_executable()` and set properties on targets.
  - DO NOT use global commands like `add_definitions()`, `include_directories()`, or `link_libraries()` without target scope.
  - DO NOT use hardcoded paths or paths relative to `${CMAKE_SOURCE_DIR}`; use target-based usage.
- Use `target_include_directories()`, `target_link_libraries()`, and `target_compile_features()` to specify dependencies and properties for targets.
- DO NOT use `include_directories()`, `link_libraries()`, and `add_definitions()` globally.
- Use **CMake Presets** for consistent build configurations. Define presets in `CMakePresets.json` and `CMakeUserPresets.json`.
  - Use `cmake --preset <preset-name>` to configure and `cmake --build --preset <preset-name>-build` to build.
- Organize CMake code into modular files. Use `add_subdirectory()` to include subprojects or components.
- Use `find_package()` to locate external dependencies. Prefer modern CMake packages that provide targets.
- Use `target_compile_features()` to specify required C++ standards (e.g., `cxx_std_23`).
- Enable **strict warnings** for all targets. Create a function/macro to apply warning flags consistently.
- Use `option()` to define configurable build options (e.g., `BUILD_TESTING`, `ENABLE_ASAN`).
