add_executable(cli_tests
  cli_tests.cpp
  unit_tests.cpp
  main.cpp)

target_link_libraries(cli_tests
  PRIVATE
    jlq::lib
    jlq::test_utils
)

jlq_apply_strict_warnings(cli_tests)

add_test(NAME cli_tests COMMAND cli_tests)
