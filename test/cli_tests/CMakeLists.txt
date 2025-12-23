add_executable(cli_tests
  cli_tests.cpp
  main.cpp)

target_include_directories(cli_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..
                                            ${PROJECT_SOURCE_DIR}/libs/jlq/include
                                            ${PROJECT_SOURCE_DIR}/libs/jlq/src)

target_link_libraries(cli_tests PRIVATE jlq::lib)

jlq_apply_strict_warnings(cli_tests)

add_test(NAME cli_tests COMMAND cli_tests)
