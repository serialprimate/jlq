add_executable(
  jlq_tests
  test_main.cpp
  mapped_file_tests.cpp
  cli_tests.cpp)

target_include_directories(jlq_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                                            ${PROJECT_SOURCE_DIR}/libs/jlq/include)

target_link_libraries(jlq_tests PRIVATE jlq::lib)

jlq_apply_strict_warnings(jlq_tests)

add_test(NAME jlq_tests COMMAND jlq_tests)
