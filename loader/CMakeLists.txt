add_library(loader
    src/elfio.cpp
    src/libelf.cpp
    src/lief.cpp
    src/llvm.cpp
)

target_sources(loader PUBLIC
FILE_SET HEADERS
BASE_DIRS include
FILES
    include/loader/elfio.hpp
    include/loader/libelf.hpp
    include/loader/lief.hpp
    include/loader/llvm.hpp
    include/loader/loader.hpp
)

target_link_libraries(loader
PRIVATE
    elfio::elfio
    LIEF::LIEF
    LLVM
    libelf++
    utils
    options
    benchmark::benchmark
)

add_executable(loader_exe src/main.cpp)
target_link_libraries(loader_exe
PRIVATE
    loader
    CLI11::CLI11
    options
    common
)
set_target_properties(loader_exe PROPERTIES OUTPUT_NAME loader)

elf_install(common loader loader_exe)