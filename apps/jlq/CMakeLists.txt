add_executable(jlq src/main.cpp)

target_link_libraries(jlq PRIVATE jlq::lib)

jlq_apply_strict_warnings(jlq)

include(GNUInstallDirs)
install(TARGETS jlq RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
