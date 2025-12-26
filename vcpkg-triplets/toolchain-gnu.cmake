# vcpkg chainload toolchain file for a GNU (gcc / g++) toolchain.
#
# Typical usage:
#   cmake -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake \
#         -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=/path/to/toolchain-gnu.cmake \
#         ...

if(NOT DEFINED vcpkgGnuToolchainLoaded)
  set(vcpkgGnuToolchainLoaded TRUE CACHE INTERNAL "vcpkg GNU toolchain loaded")
endif()

if(NOT DEFINED CMAKE_SYSTEM_NAME)
  set(CMAKE_SYSTEM_NAME "Linux" CACHE STRING "Target system name for the toolchain")
endif()

if(NOT DEFINED CMAKE_C_COMPILER)
  find_program(defaultGcc NAMES gcc HINTS /usr/bin /usr/local/bin)
  if(defaultGcc)
    set(CMAKE_C_COMPILER ${defaultGcc} CACHE FILEPATH "Path to C compiler")
  else()
    message(FATAL_ERROR "GNU C compiler (gcc) not found and CMAKE_C_COMPILER not provided")
  endif()
endif()

if(NOT DEFINED CMAKE_CXX_COMPILER)
  find_program(defaultGpp NAMES g++ HINTS /usr/bin /usr/local/bin)
  if(defaultGpp)
    set(CMAKE_CXX_COMPILER ${defaultGpp} CACHE FILEPATH "Path to C++ compiler")
  else()
    message(FATAL_ERROR "GNU C++ compiler (g++) not found and CMAKE_CXX_COMPILER not provided")
  endif()
endif()

message(STATUS "Using C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "Using C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "CMake system name: ${CMAKE_SYSTEM_NAME}")

if(NOT DEFINED gnuSysroot)
  if(DEFINED ENV{SYSROOT})
    set(gnuSysroot $ENV{SYSROOT})
  elseif(DEFINED ENV{GNU_SYSROOT})
    set(gnuSysroot $ENV{GNU_SYSROOT})
  endif()
endif()

if(DEFINED gnuSysroot AND gnuSysroot)
  if(NOT DEFINED CMAKE_SYSROOT)
    set(CMAKE_SYSROOT ${gnuSysroot} CACHE PATH "Sysroot for the target toolchain")
  endif()

  if(NOT DEFINED CMAKE_FIND_ROOT_PATH)
    set(CMAKE_FIND_ROOT_PATH ${gnuSysroot} CACHE PATH "Find root path (sysroot) for cross-compilation")
  endif()

  message(STATUS "Configured sysroot: ${CMAKE_SYSROOT}")
endif()

if(NOT DEFINED VCPKG_TARGET_TRIPLET)
  if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET})
    set(VCPKG_TARGET_TRIPLET $ENV{VCPKG_DEFAULT_TRIPLET} CACHE STRING "vcpkg target triplet")
    message(STATUS "Set VCPKG_TARGET_TRIPLET from environment: ${VCPKG_TARGET_TRIPLET}")
  endif()
endif()

message(STATUS "GNU toolchain chainload configuration complete.")
