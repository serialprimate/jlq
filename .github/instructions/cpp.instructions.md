--
name: C++ Development Instructions
description: Modern C++23 standards and best practices
applyTo: "**/*.{cpp,cc,cxx,hpp,h,hxx}"
---

# C++ Development Instructions

## Language Standards & Features
- Use **C++23** as the default standard (or C++20 as minimum)
- Prefer modern C++ idioms over legacy C-style approaches
- Leverage standard library features (STL) before third-party libraries
- Use concepts for template constraints when applicable
- Adopt ranges library for collection operations

## Memory Management & Ownership
- Follow **RAII** (Resource Acquisition Is Initialization) principles strictly
- Avoid raw pointers for ownership; use smart pointers:
  - `std::unique_ptr` for exclusive ownership
  - `std::shared_ptr` only when shared ownership is necessary
  - `std::weak_ptr` to break circular references
- Prefer stack allocation over heap allocation when possible
- Use `std::make_unique` and `std::make_shared` for smart pointer creation
- Avoid manual `new`/`delete` - let destructors handle cleanup

## Type Safety & Correctness
- Enable and maintain **const correctness** throughout
- Mark member functions `const` when they don't modify object state
- Use `constexpr` for compile-time computations
- Prefer `enum class` over plain `enum` for type-safe enumerations
- Use `std::optional` for potentially absent values instead of pointers or magic values
- Use `std::variant` for type-safe unions
- Use `std::expected` (C++23) for error handling with values
- Use `std::mdspan` (C++23) for multi-dimensional array views
- Consider `if consteval` (C++23) for compile-time vs runtime branching

## Strings & Text
- Use `std::string` for owned string data
- Use `std::string_view` for non-owning, read-only string parameters
- Prefer `std::format` (C++20) or `fmt` library for string formatting
- Use `std::filesystem::path` for file path manipulation

## Code Organization
- Use `#pragma once` for include guards
- Organize code in namespaces to avoid name collisions
- Keep header files minimal - include only what's necessary
- Prefer forward declarations in headers when possible
- Use `inline` or header-only implementations judiciously

## Error Handling
- Use exceptions for exceptional conditions, not normal control flow
- Create domain-specific exception hierarchies derived from `std::exception`
- Use RAII to ensure exception safety
- Consider `noexcept` specification for functions that guarantee no exceptions
- Document exception specifications in function contracts

## Modern Idioms & Best Practices
- Use `auto` for complex types and obvious initializations
- Use structured bindings for tuple/pair decomposition
- Prefer range-based for loops over index-based iteration
- Use lambda expressions for short, local operations
- Move semantics: implement move constructors/assignment for resource-owning types
- Delete copy operations explicitly if copying doesn't make sense
- Use `[[nodiscard]]` attribute for functions where ignoring the return value is an error

## Concurrency
- Use `std::thread`, `std::async`, or C++20 coroutines for multithreading
- Prefer `std::mutex`, `std::lock_guard`, and `std::unique_lock` for synchronization
- Use `std::atomic` for lock-free operations on simple types
- Consider thread-safe data structures or message passing over shared mutable state

## Testing & Quality
- Write unit tests using modern frameworks (Google Test, Catch2)
- Use sanitizers (AddressSanitizer, UndefinedBehaviorSanitizer) during development
- Enable high warning levels (`-Wall -Wextra -Wpedantic`) and treat warnings as errors
- Use static analysis tools (clang-tidy, cppcheck)

## Documentation
- Use Doxygen or similar for API documentation
- Document class invariants and function preconditions/postconditions
- Explain non-obvious algorithmic choices
- Document thread safety and ownership semantics
