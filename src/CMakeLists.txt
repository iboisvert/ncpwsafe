list(APPEND SRC
    AccountDb.cpp
    AccountRecord.cpp
    AccountRecords.cpp
    AccountDetailsDlg.cpp
    AccountsWin.cpp
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
    MessageBox.cpp
    Policy.cpp
    Prefs.cpp
    ProgArgs.cpp
    PWSafeApp.cpp
    SafeCombinationPromptDlg.cpp
    SearchBarWin.cpp
    Utils.cpp
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
target_include_directories(libncpwsafe PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_directories(libncpwsafe PUBLIC ${NETTLE_LIBRARY_DIRS})

add_executable(ncpwsafe main.cpp)
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
