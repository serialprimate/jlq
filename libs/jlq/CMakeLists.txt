add_library(jlq_lib)
add_library(jlq::lib ALIAS jlq_lib)

target_sources(
  jlq_lib
  PRIVATE src/cli.cpp
          src/MappedFile.cpp)

target_include_directories(jlq_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_features(jlq_lib PUBLIC cxx_std_23)

jlq_apply_strict_warnings(jlq_lib)
