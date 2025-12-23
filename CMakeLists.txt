cmake_minimum_required(VERSION 3.28.3)

project(
  jlq
  VERSION 0.1.0
  DESCRIPTION "C++ High-Performance CLI for JSONL querying"
  LANGUAGES CXX)

include(CTest)

# Require out-of-source builds (mirrors example/cxxapp).
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
  message(FATAL_ERROR "In-source builds are not allowed. Please build in a separate directory.")
endif()

# Default build type if none is specified.
set(default_build_type "RelWithDebInfo")
if(NOT (CMAKE_BUILD_TYPE OR CMAKE_CONFIGURATION_TYPES))
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
endif()

# Find policy: avoid environment/package registry surprises.
set(CMAKE_FIND_USE_CMAKE_ENVIRONMENT_PATH FALSE)
set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)
set(CMAKE_FIND_USE_PACKAGE_REGISTRY FALSE)
set(CMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY FALSE)
set(CMAKE_FIND_USE_CMAKE_SYSTEM_PATH FALSE)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(JLQ_ENABLE_WERROR "Treat warnings as errors" ON)

function(jlq_apply_strict_warnings target_name)
  if(NOT TARGET "${target_name}")
    message(FATAL_ERROR "jlq_apply_strict_warnings: target '${target_name}' does not exist")
  endif()

  target_compile_options("${target_name}" PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
  )

  if(JLQ_ENABLE_WERROR)
    target_compile_options("${target_name}" PRIVATE
      $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Werror>
      $<$<CXX_COMPILER_ID:MSVC>:/WX>
    )
  endif()
endfunction()

# Keep build outputs tidy.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

# Sanitizers (similar to example/cxxapp; only for non-Release types).
if(NOT CMAKE_BUILD_TYPE MATCHES "^(Release|MinSizeRel)$")
  option(ENABLE_ASAN "Enable AddressSanitizer globally" OFF)
  if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
  endif()

  option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer globally" OFF)
  if(ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=undefined)
  endif()

  option(ENABLE_TSAN "Enable ThreadSanitizer globally" OFF)
  if(ENABLE_TSAN)
    add_compile_options(-fsanitize=thread -fPIE)
    add_link_options(-fsanitize=thread -pie)
  endif()
endif()

add_subdirectory(libs/jlq)
add_subdirectory(apps/jlq)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
