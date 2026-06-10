cmake_minimum_required (VERSION 3.23)

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED true)


if (NOT DEFINED PACKAGE_VERSION)
    message ("-- [CMake Project] WARNING: Package version is not defined, set as 0.0.0")
    set (PACKAGE_VERSION "0.0.0")
endif ()

message ("-- [CMake Project] Package version: ${PACKAGE_VERSION}")

project ("CMake Project"
    VERSION ${PACKAGE_VERSION}
    DESCRIPTION "C/C++ 项目的 CMake 模板。"
)

# write version file
file (WRITE "${PROJECT_BINARY_DIR}/.version" "${PACKAGE_VERSION}")


# configure unit test
option (WITH_GTEST "Enable unit tests by GoogleTest" OFF)

if (NOT with_gtest)
    unset (WITH_GTEST CACHE)

endif ()

if (WITH_GTEST)
    message ("-- [CMake Project] Unit test by GoogleTest is enabled")

else ()
    message ("-- [CMake Project] Unit test by GoogleTest is disabled")

endif ()

if (WITH_GTEST)
    find_package (GTest REQUIRED)

endif ()


# configue included headers
set (DIR_SRC_ROOT "${PROJECT_SOURCE_DIR}/src")
include_directories (${DIR_SRC_ROOT})

# configure compiler cross platform
if (CMAKE_HOST_WIN32)
    include (config-win32.cmake)

elseif (CMAKE_HOST_UNIX)
    include (config-unix.cmake)

else ()
    message (FATAL_ERROR "Unknown platform")

endif ()


# configure output dir of static and shared library
include (GNUInstallDirs)

set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")

# configure targets
aux_source_directory (${DIR_SRC_ROOT} PROJECT_SOURCE_FILES)
add_library (cmake-project ${PROJECT_SOURCE_FILES})

unset (CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
unset (CMAKE_LIBRARY_OUTPUT_DIRECTORY)
unset (CMAKE_RUNTIME_OUTPUT_DIRECTORY)


add_subdirectory ("snippet")


if (WITH_GTEST)
    add_subdirectory (test)

endif ()


# configure install targets
option (INSTALL_IN_PLACE "Set CMAKE_INSTALL_PREFIX inside PROJECT_BINARY_DIR" ON)

if (INSTALL_IN_PLACE)
    set (CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/install")

endif ()

message ("-- [CMake Project] Install destination would be: ${CMAKE_INSTALL_PREFIX}")


include (public-headers.cmake)
set_target_properties (cmake-project PROPERTIES PUBLIC_HEADER ${CMAKE_PROJECT_PUBLIC_HEADERS})

install (TARGETS cmake-project
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install (FILES ${PROJECT_BINARY_DIR}/.version DESTINATION .)
