add_library(jlq_lib)
add_library(jlq::lib ALIAS jlq_lib)

find_package(simdjson CONFIG REQUIRED)

target_sources(
  jlq_lib
  PRIVATE src/cli.cpp
          src/LineScanner.cpp
          src/MappedFile.cpp
          src/path.cpp
          src/Query.cpp)

target_include_directories(jlq_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_features(jlq_lib PUBLIC cxx_std_23)

target_link_libraries(jlq_lib PRIVATE simdjson::simdjson)

jlq_apply_strict_warnings(jlq_lib)
