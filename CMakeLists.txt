cmake_minimum_required(VERSION 3.30)

project(elf-research LANGUAGES CXX)

if (${PROJECT_SOURCE_DIR} STREQUAL ${PROJECT_BINARY_DIR})
    message(FATAL_ERROR "In-source build is forbidden")
endif()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Default build type" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/functions.cmake)

find_package(LIEF REQUIRED)
find_package(elfio REQUIRED)
find_package(CLI11 REQUIRED)
find_package(LLVM REQUIRED)

find_package(benchmark REQUIRED)

add_library(options INTERFACE)
target_compile_features(options INTERFACE cxx_std_26)
target_compile_options(options
INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:/Wall>
    $<$<CXX_COMPILER_ID:GNU,Clang>: -Wall -Wextra -pedantic>
    $<$<CXX_COMPILER_ID:GNU>: -freflection>
)

add_library(common INTERFACE)
target_link_libraries(common INTERFACE options)
target_sources(common INTERFACE
    FILE_SET HEADERS
    BASE_DIRS include
    FILES include/common/libraries.hpp
)

add_library(utils INTERFACE)
target_link_libraries(utils INTERFACE options)
target_sources(utils INTERFACE
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
        include/utils/enum_support.hpp
        include/utils/fd_wrapper.hpp
        include/utils/mmap_wrapper.hpp
        include/utils/range_map.hpp
)

elf_install(common utils)

add_subdirectory(libelf++)
add_subdirectory(loader)
add_subdirectory(generator)
