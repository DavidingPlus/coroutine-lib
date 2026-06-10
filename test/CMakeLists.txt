find_package (GTest REQUIRED)

aux_source_directory (${CMAKE_CURRENT_SOURCE_DIR} SRC_GTEST)
add_executable (gtest-coroutine-lib EXCLUDE_FROM_ALL ${SRC_GTEST})
target_link_libraries (gtest-coroutine-lib gtest::gtest coroutine-lib)

add_custom_target (tests)
add_dependencies (tests gtest-coroutine-lib)


message ("-- [Coroutine Lib] Done configuring gtest")
