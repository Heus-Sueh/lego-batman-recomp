add_executable(runtime_foundation_tests foundation.cpp abi_c.c)
target_link_libraries(runtime_foundation_tests PRIVATE pcrecomp::win32_time)
add_test(NAME runtime_foundation COMMAND runtime_foundation_tests)
add_executable(legacy_icall_tests legacy_icall.cpp)
target_compile_definitions(legacy_icall_tests PRIVATE RECOMP_RUNTIME_DIAGNOSTICS)
target_link_libraries(legacy_icall_tests PRIVATE pcrecomp::bridge)
add_test(NAME legacy_icall_failure COMMAND legacy_icall_tests)
add_executable(runtime_file_tests file_foundation.cpp)
target_link_libraries(runtime_file_tests PRIVATE pcrecomp::win32_fs)
add_test(NAME runtime_file COMMAND runtime_file_tests)

add_test(NAME runtime_dependency_rules
    COMMAND ${CMAKE_COMMAND} -DROOT=${PROJECT_SOURCE_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/dependency_rules.cmake)
