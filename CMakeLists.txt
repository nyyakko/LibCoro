cmake_minimum_required(VERSION 3.25)

project(LibCoro LANGUAGES CXX)

include(cmake/get_cpm.cmake)

include(cmake/static_analyzers.cmake)
include(GNUInstallDirs)

# set(LibCoro_CompilerOptions ${LibCoro_CompilerOptions})
# set(LibCoro_LinkerOptions ${LibCoro_LinkerOptions})

# set(LibCoro_ExternalLibraries package1 package2 ...)

add_subdirectory(LibCoro)

