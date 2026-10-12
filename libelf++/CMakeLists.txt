add_library(libelf++ INTERFACE)
target_link_libraries(libelf++ INTERFACE options)
target_sources(libelf++ INTERFACE
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
        include/libelf++/data.hpp
        include/libelf++/elf.hpp
        include/libelf++/error.hpp
        include/libelf++/header.hpp
        include/libelf++/section.hpp
        include/libelf++/segment.hpp
)
target_link_libraries(libelf++
INTERFACE
    elf # libelf itself
    utils
)

elf_install(libelf++)
