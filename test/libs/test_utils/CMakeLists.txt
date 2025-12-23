add_library(test_utils INTERFACE)
add_library(jlq::test_utils ALIAS test_utils)

target_include_directories(test_utils INTERFACE include)
