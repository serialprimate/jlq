add_executable(mapped_file_tests
  mapped_file_tests.cpp
  main.cpp)

target_include_directories(mapped_file_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..
                                                ${PROJECT_SOURCE_DIR}/libs/jlq/include
                                                ${PROJECT_SOURCE_DIR}/libs/jlq/src)

target_link_libraries(mapped_file_tests PRIVATE jlq::lib)

jlq_apply_strict_warnings(mapped_file_tests)

add_test(NAME mapped_file_tests COMMAND mapped_file_tests)
