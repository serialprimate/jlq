add_executable(mapped_file_tests
  mapped_file_tests.cpp
  main.cpp)

# Get the source directory of jlq_lib to avoid using PROJECT_SOURCE_DIR or relative paths
get_target_property(JLQ_LIB_SRC_DIR jlq_lib SOURCE_DIR)

target_include_directories(mapped_file_tests
  PRIVATE
    ${JLQ_LIB_SRC_DIR}/src
)

target_link_libraries(mapped_file_tests
  PRIVATE
    jlq::lib
    jlq::test_utils
)

jlq_apply_strict_warnings(mapped_file_tests)

add_test(NAME mapped_file_tests COMMAND mapped_file_tests)
