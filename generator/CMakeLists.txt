find_package(nlohmann_json REQUIRED)
find_package(nlohmann_json_schema_validator REQUIRED)

add_library(config STATIC src/config.cpp)
target_link_libraries(config
PRIVATE
    options
    nlohmann_json::nlohmann_json
    nlohmann_json_schema_validator
)
target_sources(config PUBLIC
FILE_SET HEADERS
BASE_DIRS include
FILES
    include/generator/config.hpp
)

add_library(generator STATIC
    src/elfio.cpp
    src/libelf.cpp
)
target_sources(generator PUBLIC
FILE_SET HEADERS
BASE_DIRS include
FILES
    include/generator/elfio.hpp
    include/generator/generator.hpp
    include/generator/libelf.hpp
)
target_link_libraries(generator
PUBLIC
    config
    libelf++
PRIVATE
    elfio::elfio
    options
)

add_executable(generator_exe src/main.cpp)
set_target_properties(generator_exe PROPERTIES OUTPUT_NAME generator)
target_link_libraries(generator_exe
PRIVATE
    generator
    common
    CLI11::CLI11
    options
    benchmark::benchmark
)

elf_install(common generator generator_exe)
