list(APPEND SRC
    AccountDb.cpp
    AccountRecord.cpp
    AccountDetailsDlg.cpp
    AccountsWin.cpp
    ChangeDbPasswordCommand.cpp
    ChangeDbPasswordDlg.cpp
    ChangePasswordDlg.cpp
    CommandBarWin.cpp
    Dialog.cpp
    ExportDbCommand.cpp
    FileUtils.cpp
    GeneratePasswordDlg.cpp
    GeneratePasswordCommand.cpp
    GenerateTestDbCommand.cpp
    MessageBox.cpp
    Policy.cpp
    Prefs.cpp
    PWSafeApp.cpp
    SafeCombinationPromptDlg.cpp
    SearchBarWin.cpp
    Utils.cpp
)

add_library(libncpwsafe OBJECT ${SRC})
target_link_libraries(libncpwsafe 
    pwsafeStatic
    cpptoml
    ${NCURSES_LIBRARIES} 
    ${ICU_LIBRARIES}
    ${GLOG_LIBRARIES}
)
target_include_directories(libncpwsafe PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_executable(ncpwsafe main.cpp)
set_target_properties(ncpwsafe 
    PROPERTIES 
    VERSION ${NCPWSAFE_VERSION}
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
target_compile_definitions(ncpwsafe
    PUBLIC CPPTOML_VALUE_ASSTRING
    PUBLIC CPPTOML_NO_RTTI
)
target_link_libraries(ncpwsafe 
    libncpwsafe
)
install (TARGETS ncpwsafe RUNTIME)

add_compile_definitions(
    "$<$<CONFIG:RELEASE>:NDEBUG>"
)
