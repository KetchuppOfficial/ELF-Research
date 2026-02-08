cmake_minimum_required(VERSION 3.30)

project(lief-experiments LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(LIEF REQUIRED)
find_package(elfio REQUIRED)
find_package(CLI11 REQUIRED)

add_executable(generator src/generator/main.cpp)
target_link_libraries(generator
PRIVATE
    CLI11::CLI11
    elfio::elfio
)

add_library(benchmark
    src/benchmark/elfio.cpp
    src/benchmark/lief.cpp
)
target_include_directories(benchmark PUBLIC include)
target_link_libraries(benchmark
PRIVATE
    elfio::elfio
    LIEF::LIEF
)

add_executable(benchmark_exe src/benchmark/main.cpp)
target_link_libraries(benchmark_exe PRIVATE benchmark)
set_target_properties(benchmark_exe PROPERTIES OUTPUT_NAME benchmark)

foreach(target generator benchmark)
    target_compile_features(${target} PUBLIC cxx_std_23)
    target_compile_options(${target}
    PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>:/Wall>
        $<$<CXX_COMPILER_ID:GNU,Clang>: -Wall -Wextra -pedantic>
    )
endforeach()
