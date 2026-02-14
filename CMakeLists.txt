cmake_minimum_required(VERSION 3.30)

project(elf-research LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(LIEF REQUIRED)
find_package(elfio REQUIRED)
find_package(CLI11 REQUIRED)

add_library(utils INTERFACE)
target_compile_features(utils INTERFACE cxx_std_23)
target_sources(utils INTERFACE
    FILE_SET HEADERS
    BASE_DIRS include
    FILES include/utils/fd_wrapper.hpp
)

add_library(common INTERFACE)
target_compile_features(common INTERFACE cxx_std_23)
target_sources(common INTERFACE
    FILE_SET HEADERS
    BASE_DIRS include
    FILES include/common/libraries.hpp
)

add_library(libelf++ INTERFACE)
target_compile_features(libelf++ INTERFACE cxx_std_23)
target_sources(utils INTERFACE
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
        include/libelf++/data.hpp
        include/libelf++/elf.hpp
        include/libelf++/header.hpp
)
target_link_libraries(libelf++
INTERFACE
    elf
    utils
)

add_library(generator
    src/generator/elfio.cpp
    src/generator/libelf.cpp
)
target_sources(generator INTERFACE
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
        include/generator/elfio.hpp
        include/generator/generator.hpp
        include/generator/libelf.hpp
        include/generator/lief.hpp
)
target_link_libraries(generator
PRIVATE
    libelf++
    elfio::elfio
    LIEF::LIEF
)

add_executable(generator_exe src/generator/main.cpp)
set_target_properties(generator_exe PROPERTIES OUTPUT_NAME generator)
target_link_libraries(generator_exe
PRIVATE
    generator
    common
    CLI11::CLI11
)

add_library(benchmark
    src/benchmark/elfio.cpp
    src/benchmark/lief.cpp
    src/benchmark/libelf.cpp
)
target_include_directories(benchmark PUBLIC include)
target_sources(benchmark INTERFACE
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
        include/benchmark/elfio.hpp
        include/benchmark/benchmark.hpp
        include/benchmark/libelf.hpp
        include/benchmark/lief.hpp
)
target_link_libraries(benchmark
PRIVATE
    elfio::elfio
    LIEF::LIEF
    elf
    utils
)

add_executable(benchmark_exe src/benchmark/main.cpp)
target_link_libraries(benchmark_exe
PRIVATE
    benchmark
    CLI11::CLI11
)
set_target_properties(benchmark_exe PROPERTIES OUTPUT_NAME benchmark)

foreach(target generator benchmark)
    target_compile_features(${target} PUBLIC cxx_std_23)
    target_compile_options(${target}
    PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>:/Wall>
        $<$<CXX_COMPILER_ID:GNU,Clang>: -Wall -Wextra -pedantic>
    )
endforeach()
