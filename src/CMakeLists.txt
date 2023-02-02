list(APPEND SRC
  main.cpp
  AccountRecord.cpp
  AccountsColl.cpp
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
install (TARGETS ncpwsafe RUNTIME)
target_link_libraries(ncpwsafe 
  ${PWSAFE_LIBRARIES}
  ${NCURSES_LIBRARIES} 
)
