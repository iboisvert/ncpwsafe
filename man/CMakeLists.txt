# The ncpwsafe(1) man page, written in mdoc(7). configure_file() fills in the version.
# Preview it with: man -l build/man/ncpwsafe.1
configure_file(ncpwsafe.1.in "${CMAKE_CURRENT_BINARY_DIR}/ncpwsafe.1" @ONLY)

if(NOT WIN32)
    include(GNUInstallDirs)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ncpwsafe.1" DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
endif()
