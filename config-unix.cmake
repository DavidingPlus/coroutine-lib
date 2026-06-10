message ("-- [CMake Project] Building on Linux ...")

# configure compiler gcc with CCache
set (CMAKE_C_COMPILER "gcc")
set (CMAKE_CXX_COMPILER "g++")

message ("-- [CMake Project] Using C Compiler: ${CMAKE_C_COMPILER} (${CMAKE_C_COMPILER_VERSION})")
message ("-- [CMake Project] Using C++ Compiler: ${CMAKE_CXX_COMPILER} (${CMAKE_CXX_COMPILER_VERSION})")


option (BUILD_SHARED_LIBS "Build using shared libraries" ON)
