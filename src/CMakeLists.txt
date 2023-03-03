list(APPEND SRC
  main.cpp
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
  GeneratePasswordDlg.cpp
  GeneratePasswordCommand.cpp
  GenerateTestDbCommand.cpp
  MessageBox.cpp
  Policy.cpp
  PWSafeApp.cpp
  SafeCombinationPromptDlg.cpp
  SearchBarWin.cpp
  Utils.cpp
)

add_executable(ncpwsafe ${SRC})
target_link_libraries(ncpwsafe 
    ${NCURSES_LIBRARIES} 
    ICU::uc
    pwsafe
)
install (TARGETS ncpwsafe RUNTIME)
