list(APPEND SRC
    AccountDb.cpp
    AccountRecord.cpp
    AccountRecords.cpp
    AccountDetailsDlg.cpp
    AccountList.cpp
    AccountsWin.cpp
    AutoSignOff.cpp
    ChangeDbPasswordCommand.cpp
    ChangeDbPasswordDlg.cpp
    ChangePasswordDlg.cpp
    CommandBarWin.cpp
    Dialog.cpp
    ExportDbCommand.cpp
    Filesystem.cpp
    GeneratePasswordDlg.cpp
    GeneratePasswordCommand.cpp
    GenerateTestDbCommand.cpp
    HelpWin.cpp
    MessageBox.cpp
    Policy.cpp
    Prefs.cpp
    ProgArgs.cpp
    PWSafeApp.cpp
    SafeCombinationPromptDlg.cpp
    SearchBarWin.cpp
    TextArea.cpp
    Theme.cpp
    Utils.cpp
    Win32.cpp
)

add_library(libncpwsafe OBJECT ${SRC})
if(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
target_link_libraries(libncpwsafe 
    pwsafeStatic
    confini
    ${CURSES_LIBRARIES} 
    form
    panel
    menu
    ${ICU_LINK_LIBRARIES}
    ${GLOG_LINK_LIBRARIES}
)
else()
target_link_libraries(libncpwsafe 
    pwsafeStatic
    confini
    ${NCURSES_LINK_LIBRARIES} 
    ${ICU_LINK_LIBRARIES}
    ${GLOG_LINK_LIBRARIES}
)
endif()
target_include_directories(libncpwsafe PUBLIC 
    ${CMAKE_CURRENT_LIST_DIR}
    ${NCURSES_INCLUDE_DIRS}
    ${ICU_INCLUDE_DIRS}
)
target_link_directories(libncpwsafe PUBLIC 
    ${NETTLE_LIBRARY_DIRS}
    ${NCURSES_LIBRARY_DIRS}
    ${ICU_LIBRARY_DIRS}
)
if(WIN32)
    target_link_libraries(libncpwsafe
        ${GETOPT_LINK_LIBRARIES}
        # Clipboard
        user32
        # SHGetKnownFolderPath(), CoTaskMemFree(), FOLDERID_*
        shell32
        # Process DACL
        advapi32
        ole32
        uuid
    )
endif()

add_executable(ncpwsafe main.cpp)
if(WIN32)
    # Application manifest: UTF-8 active code page, Windows 10+
    target_sources(ncpwsafe PRIVATE ncpwsafe.manifest)
endif()
set_target_properties(ncpwsafe 
    PROPERTIES 
    VERSION ${NCPWSAFE_VERSION}
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
target_link_libraries(ncpwsafe 
    libncpwsafe
)
install (TARGETS ncpwsafe RUNTIME)

add_compile_definitions(
    "$<$<CONFIG:RELEASE>:NDEBUG>"
)

set(CPACK_PACKAGE_VENDOR "Ian Boisvert" PARENT_SCOPE)
set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-only" PARENT_SCOPE)
set(CPACK_RPM_PACKAGE_REQUIRES "libicu >= 72" PARENT_SCOPE)
