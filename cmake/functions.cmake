include(GNUInstallDirs)

function(elf_install)
    install(TARGETS ${ARGV}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                COMPONENT ELF_Runtime
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                COMPONENT ELF_Runtime
                NAMELINK_COMPONENT ELF_Development
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                COMPONENT ELF_Development
        FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
                        COMPONENT ELF_Development
    )
endfunction()
