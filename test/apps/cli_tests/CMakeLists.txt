add_executable(cli_tests
  cli_tests.cpp
  unit_tests.cpp
  main.cpp)

# Get the source directory of jlq_lib to access internal headers
get_target_property(JLQ_LIB_SRC_DIR jlq_lib SOURCE_DIR)

target_include_directories(cli_tests
  PRIVATE
    ${JLQ_LIB_SRC_DIR}/src
)

target_link_libraries(cli_tests
  PRIVATE
    jlq::lib
    jlq::test_utils
)

jlq_apply_strict_warnings(cli_tests)

add_test(NAME cli_tests COMMAND cli_tests)
