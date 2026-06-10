find_package (GTest REQUIRED)

aux_source_directory (${CMAKE_CURRENT_SOURCE_DIR} SRC_GTEST)
add_executable (gtest-cmake-project EXCLUDE_FROM_ALL ${SRC_GTEST})
target_link_libraries (gtest-cmake-project gtest::gtest cmake-project)

add_custom_target (tests)
add_dependencies (tests gtest-cmake-project)


message ("-- [CMake Project] Done configuring gtest")
